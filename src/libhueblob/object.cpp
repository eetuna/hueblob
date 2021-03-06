#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/video/tracking.hpp>
#include "libhueblob/object.hh"
#include <algorithm>
#include <iostream>
#include "highgui.h"
// Histogram parameters initialization.
static const int hist_size[] = {Object::h_bins, Object::s_bins};
//  0 (~0°red) to 180 (~360°red again)
static const float hue_range[] = { 0, 250 };
//  0 (black-gray-white) to 255 (pure spectrum color)
static const float sat_range[] = { 0, 250 };
//  combine the two previous information
static const float* ranges[] = { hue_range, sat_range };


Object::Object()
  :
     anchor_x_(),
     anchor_y_(),
     anchor_z_(),
     modelHistogram_(),
     searchWindow_(-1, -1, -1, -1)
{}

cv::Mat
Object::computeMask(const cv::Mat& model)
{
  cv::Mat gmodel(model.size(), CV_8UC1);
  cv::Mat mask(model.size(), CV_8UC1);

  cv::cvtColor(model, gmodel, CV_BGR2GRAY);

  cv::threshold(gmodel, mask, 5, 255, CV_THRESH_BINARY);
  return mask;
}
void
Object::addView(const cv::Mat& model)
{
  // Compute the mask.
  cv::Mat mask = computeMask(model);
  // cv::namedWindow("test", CV_WINDOW_AUTOSIZE);
  // cv::imshow("test", model );
  // cv::waitKey();

  // Compute the histogram.
  //  only use channels 0 and 1 (hue and saturation).
  int channels[] = {0, 1};
  cv::Mat hsv;
  cv::MatND hist;
  cv::cvtColor(model, hsv, CV_BGR2HSV);
  // cv::imshow("test", hsv );
  // cv::waitKey();
  using namespace std;
  calcHist(&hsv, 1, channels, mask,
	   hist, 2, hist_size, ranges,
	   true, false);

  // Normalize.

  double max = 0.;
  cv::minMaxLoc(hist, 0, &max, 0, 0);

  //  convert MatND into Mat, no copy is done, two types will be
  //  merged soon enough.
  cv::Mat hist_(hist);
  cv::convertScaleAbs(hist_, hist_, max ? 255. / max : 0., 0);
  this->modelHistogram_.push_back(hist);

  // compute histogram and thresholds for naive method
}


namespace
{
  /// \brief Reset search zone if it is incorrect.
  void resetSearchZone(cv::Rect& rect, const cv::Mat& backProject)
  {
    if (rect.x < 0 || rect.y < 0 || rect.x > backProject.cols - 1
        || rect.y > backProject.rows - 1
        || rect.width <= 0
        || rect.height <= 0)
      {
	rect.x = 0;
	rect.y = 0;
	rect.width = backProject.cols - 1;
	rect.height = backProject.rows - 1;
      }

    if (rect.x + rect.width > backProject.cols - 1)
      rect.width = backProject.cols - 1 - rect.x;
    if (rect.y + rect.height > backProject.rows - 1)
      rect.height = backProject.rows - 1 - rect.y;
  }
} // end of anonymous namespace.

boost::optional<cv::RotatedRect>
Object::track(const cv::Mat& image)
{
  boost::optional<cv::RotatedRect> result;

  int nViews = modelHistogram_.size();
  if (!nViews)
    return result;

  // Convert to HSV.
  cv::cvtColor(image, imgHSV_, CV_BGR2HSV);

  // Compute back projection.
  //  only use channels 0 and 1 (hue and saturation).
  int channels[] = {0, 1};
  cv::Mat backProject;
  cv::calcBackProject(&imgHSV_, 1, channels, modelHistogram_[0],
                      backProject,
		      ranges);

  for(int nmodel = 1; nmodel < nViews; ++nmodel)
    {
      cv::Mat backProjectTmp;
      cv::calcBackProject(&imgHSV_, 1, channels, modelHistogram_[nmodel],
			  backProjectTmp, ranges);

      // Merge back projections while taking care of overflows.
      for (int i = 0 ; i < backProject.rows; ++i)
	for (int j = 0 ; j < backProject.cols; ++j)
	  {
	    int v =
	      backProject.at<unsigned char>(i, j)
	      + backProjectTmp.at<unsigned char>(i, j);
	    if (v <= 0)
	      v = 0;
	    else if (v >= 255)
	      v = 255;
	    backProject.at<unsigned char>(i, j) = v;
	  }
    }

  cv::threshold(backProject, backProject, 32, 0, CV_THRESH_TOZERO);
  cv::medianBlur(backProject, backProject, 3);

  resetSearchZone(searchWindow_, backProject);

  cv::TermCriteria criteria =
    cv::TermCriteria(CV_TERMCRIT_ITER|CV_TERMCRIT_EPS, 50, 1);
  result = cv::CamShift(backProject, searchWindow_, criteria);

  if (std::abs(result->size.width) > 1e6)
    {
      searchWindow_.x = searchWindow_.y = -1;
      return boost::optional<cv::RotatedRect>();
    }

  searchWindow_ = result->boundingRect();

  if (searchWindow_.height <= 0 || searchWindow_.width <= 0)
    searchWindow_.x = searchWindow_.y = -1;
  return result;
}

void
Object::clearViews()
{
  modelHistogram_.clear();
}

void
Object::setSearchWindow(const cv::Rect window)
{
  searchWindow_ = window;
  return;
}
