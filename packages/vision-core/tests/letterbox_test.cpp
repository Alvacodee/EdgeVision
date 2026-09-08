#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>

#include "detectors/object_detector.hpp"

using vision_core::LetterboxInfo;
using vision_core::ObjectDetector;

TEST(LetterboxTest, LandscapeImageGetsCorrectPadding) {
    cv::Mat src = cv::Mat::zeros(720, 1280, CV_8UC3);  // height=720, width=1280
    LetterboxInfo info;
    cv::Mat result = ObjectDetector::letterbox(src, 320, info);

    EXPECT_EQ(result.cols, 320);
    EXPECT_EQ(result.rows, 320);
    EXPECT_FLOAT_EQ(info.scale, 0.25f);
    EXPECT_EQ(info.padX, 0);
    EXPECT_EQ(info.padY, 70);  // sisa vertikal yang dipadding
}

TEST(LetterboxTest, PortraitImageGetsCorrectPadding) {
    cv::Mat src = cv::Mat::zeros(1080, 810, CV_8UC3);  // dimensi mirip bus.jpg
    LetterboxInfo info;
    cv::Mat result = ObjectDetector::letterbox(src, 320, info);

    EXPECT_EQ(result.cols, 320);
    EXPECT_EQ(result.rows, 320);
    EXPECT_NEAR(info.scale, 0.296296f, 1e-4f);
    EXPECT_EQ(info.padX, 40);  // sisa horizontal yang dipadding
    EXPECT_EQ(info.padY, 0);
}

TEST(LetterboxTest, SquareImageNeedsNoPadding) {
    cv::Mat src = cv::Mat::zeros(400, 400, CV_8UC3);
    LetterboxInfo info;
    cv::Mat result = ObjectDetector::letterbox(src, 320, info);

    EXPECT_EQ(result.cols, 320);
    EXPECT_EQ(result.rows, 320);
    EXPECT_FLOAT_EQ(info.scale, 0.8f);
    EXPECT_EQ(info.padX, 0);
    EXPECT_EQ(info.padY, 0);
}

TEST(LetterboxTest, PaddingColorIsGray114NotBlack) {
    cv::Mat src = cv::Mat::zeros(720, 1280, CV_8UC3);
    LetterboxInfo info;
    cv::Mat result = ObjectDetector::letterbox(src, 320, info);

    // pojok kiri-atas pasti area padding (padY=70), harus abu-abu 114, bukan hitam
    cv::Vec3b pixel = result.at<cv::Vec3b>(0, 0);
    EXPECT_EQ(pixel[0], 114);
    EXPECT_EQ(pixel[1], 114);
    EXPECT_EQ(pixel[2], 114);
}
