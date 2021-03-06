#include <boost/format.hpp>
#include <boost/optional.hpp>
#include <gtest/gtest.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "libhueblob/object.hh"
#include <vector>

void trackObject(std::vector<std::string> viewFilenames,
		 const std::string& frameFilename)
{
  Object object;

  for (  std::vector<std::string>::iterator iter =
           viewFilenames.begin();
         iter != viewFilenames.end(); iter++ )
    {
      std::string viewFilename = *iter;
      cv::Mat view = cv::imread(viewFilename + ".png");
      object.addView(view);
    }

  EXPECT_EQ(object.modelHistogram.size(), viewFilenames.size());

  cv::Mat image = cv::imread(frameFilename + ".png");
  boost::optional<cv::RotatedRect> rrect = object.track(image);

  // Check that tracking succeed.
  EXPECT_TRUE(rrect);

  cv::Rect rect = rrect->boundingRect();

  boost::format fmt("Blob location: %dx%d+%dx%d");
  fmt % rect.x % rect.y % rect.width % rect.height;

  std::cout << fmt << std::endl;

  cv::Point top_left(rect.x, rect.y);
  cv::Point bottom_right(rect.x + rect.width, rect.y + rect.height);
  cv::rectangle(image, top_left, bottom_right,
		cv::Scalar(0, 0, 255, 255), CV_FILLED);

  static int i = 0;
  boost::format filename("tracking_result_%d.png");
  filename % i++;

  if(!cv::imwrite(filename.str(), image))
    std::cerr << "Failed to save image.";
}

void viewHistogram(const cv::MatND& hist, const std::string& filename)
{
  int hbins = Object::h_bins, sbins = Object::s_bins;
  int scale = 10;

  double maxVal = 0.;
  minMaxLoc(hist, 0, &maxVal, 0, 0);

  cv::Mat histImg = cv::Mat::zeros(sbins * scale, hbins * scale, CV_8UC3);

  for(int h = 0; h < hbins; ++h)
    for(int s = 0; s < sbins; ++s)
      {
	float binVal = hist.at<float>(h, s);
	int intensity = cvRound(binVal*255/maxVal);
	cv::rectangle(histImg, cv::Point(h * scale, s * scale),
		      cv::Point((h + 1) * scale - 1, (s + 1) * scale - 1),
		      cv::Scalar::all(intensity),
		      CV_FILLED);
      }

  if(!cv::imwrite(filename, histImg))
    std::cerr << "Failed to save image.";
}

void addView(const std::string& viewFilename)
{
  Object object;
  cv::Mat img = cv::imread(viewFilename + ".png");
  object.addView(img);

  EXPECT_EQ(object.modelHistogram.size(), 1u);

  static int i = 0;
  boost::format filename("hist_%d.png");
  filename % i++;
  viewHistogram(object.modelHistogram[0], filename.str());
}

void computeMask(const std::string& filename)
{
  Object object;
  cv::Mat img = cv::imread(filename + ".png");
  cv::Mat mask = object.computeMask(img);

  static int i = 0;
  boost::format outputFilename("mask_%d.png");
  outputFilename % i++;
  if(!cv::imwrite(outputFilename.str(), mask))
    std::cerr << "Failed to save image.";
}


// Misc. tests.
TEST(TestSuite, simple)
{
  Object object;

  EXPECT_EQ(object.anchor_x, 0.);
  EXPECT_EQ(object.anchor_y, 0.);
  EXPECT_EQ(object.anchor_z, 0.);

  EXPECT_TRUE(object.modelHistogram.empty());
  EXPECT_EQ(object.modelHistogram.size(), 0u);
}


// Mask tests.
TEST(TestSuite, compute_mask_ball_rose)
{
  computeMask("./data/models/ball-rose");
}

TEST(TestSuite, compute_mask_ball_orange)
{
  computeMask("./data/models/ball-orange");
}


TEST(TestSuite, compute_mask_door)
{
  computeMask("./data/models/door");
}

// View tests.
TEST(TestSuite, add_view_door)
{
  addView("./data/models/door");
}

TEST(TestSuite, add_view_ball_rose)
{
  addView("./data/models/ball-rose");
}

TEST(TestSuite, add_view_ball_orange)
{
  addView("./data/models/ball-orange");
}

// Tracking tests.
TEST(TestSuite, track_door)
{

  std::vector<std::string> door_models;
  door_models.push_back("./data/models/door");
  trackObject(door_models, "./data/frames/door-frame");
}


TEST(TestSuite, track_ball_rose1)
{
  std::vector<std::string> ball_rose_models;
  ball_rose_models.push_back("./data/models/ball-rose");
  trackObject(ball_rose_models,"./data/frames/ball-rose-frame");
}

TEST(TestSuite, track_ball_rose)
{
  std::vector<std::string> ball_rose_models;
  ball_rose_models.push_back("./data/models/ball-rose");
  ball_rose_models.push_back("./data/models/ball-rose-2");
  ball_rose_models.push_back("./data/models/ball-rose-3");
  trackObject(ball_rose_models,"./data/frames/ball-rose-frame");
}

TEST(TestSuite, track_ball_rose2)
{
  std::vector<std::string> ball_rose_models;
  ball_rose_models.push_back("./data/models/ball-rose");
  ball_rose_models.push_back("./data/models/ball-rose-2");
  ball_rose_models.push_back("./data/models/ball-rose-3");
  trackObject(ball_rose_models,"./data/frames/ball-orange-frame");
}

TEST(TestSuite, track_ball_orange)
{
  std::vector<std::string> ball_orange_models;
  ball_orange_models.push_back("./data/models/ball-orange");
  ball_orange_models.push_back("./data/models/ball-orange-2");
  ball_orange_models.push_back("./data/models/ball-orange-3");
  trackObject(ball_orange_models,"./data/frames/ball-orange-frame");
}

TEST(TestSuite, track_ball_orange2)
{
  std::vector<std::string> ball_orange_models;
  ball_orange_models.push_back("./data/models/ball-orange");
  ball_orange_models.push_back("./data/models/ball-orange-2");
  ball_orange_models.push_back("./data/models/ball-orange-3");
  trackObject(ball_orange_models,"./data/frames/ball-orange-frame");
}

int main(int argc, char **argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
