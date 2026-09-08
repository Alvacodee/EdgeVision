#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>

#include <array>
#include <vector>

#include "core/frame_buffer.hpp"
#include "detectors/object_detector.hpp"

using vision_core::Detection;
using vision_core::LetterboxInfo;
using vision_core::ObjectDetector;

namespace {

// Bikin tensor sintetis [1, 84, numAnchors] biar decodeAndNms bisa ditest
// tanpa perlu load model ONNX beneran. Layout harus sama persis kayak output
// asli YOLOv8: row 0-3 = cx,cy,w,h ; row 4..83 = score per class.
cv::Mat makeSyntheticOutput(const std::vector<std::array<float, 4>>& boxes,
                             const std::vector<int>& classIds,
                             const std::vector<float>& classScores,
                             int numAnchors) {
    int sizes[] = {1, 84, numAnchors};
    cv::Mat raw(3, sizes, CV_32F, cv::Scalar(0.0f));

    for (size_t i = 0; i < boxes.size(); ++i) {
        int idx = static_cast<int>(i);
        raw.at<float>(0, 0, idx) = boxes[i][0];  // cx
        raw.at<float>(0, 1, idx) = boxes[i][1];  // cy
        raw.at<float>(0, 2, idx) = boxes[i][2];  // w
        raw.at<float>(0, 3, idx) = boxes[i][3];  // h
        raw.at<float>(0, 4 + classIds[i], idx) = classScores[i];
    }
    return raw;
}

}  // namespace

TEST(DecodeNmsTest, SuppressesOverlappingLowerScoreBox) {
    std::vector<std::array<float, 4>> boxes = {
        {100, 100, 50, 50},  // anchor 0: box utama
        {105, 105, 50, 50},  // anchor 1: overlap tinggi sama anchor 0, score lebih rendah
        {300, 300, 40, 40},  // anchor 2: jauh dari yang lain, harus lolos
        {500, 500, 30, 30},  // anchor 3: score di bawah threshold, harus kefilter duluan
    };
    std::vector<int> classIds = {0, 0, 5, 2};
    std::vector<float> scores = {0.9f, 0.6f, 0.8f, 0.3f};

    cv::Mat raw = makeSyntheticOutput(boxes, classIds, scores, 4);

    LetterboxInfo info{1.0f, 0, 0};  // scale=1, no padding biar gampang diverifikasi manual
    auto results = ObjectDetector::decodeAndNms(raw, info, 640, 640, 0.5f, 0.45f);

    ASSERT_EQ(results.size(), 2u);

    // anchor 0 menang atas anchor 1 (score lebih tinggi, IoU tinggi -> anchor 1 disupress)
    EXPECT_FLOAT_EQ(results[0].score, 0.9f);
    EXPECT_EQ(static_cast<int>(results[0].classId), 0);
    EXPECT_FLOAT_EQ(results[0].x, 75.0f);  // cx(100) - w/2(25)
    EXPECT_FLOAT_EQ(results[0].y, 75.0f);

    // anchor 2 lolos karena gak overlap sama box manapun
    EXPECT_FLOAT_EQ(results[1].score, 0.8f);
    EXPECT_EQ(static_cast<int>(results[1].classId), 5);
    EXPECT_FLOAT_EQ(results[1].x, 280.0f);  // cx(300) - w/2(20)
    EXPECT_FLOAT_EQ(results[1].y, 280.0f);
}

TEST(DecodeNmsTest, CapsAtMaxDetections) {
    // bikin box lebih banyak dari MAX_DETECTIONS, semuanya gak overlap,
    // pastiin hasil akhir tetap dibatasin ke MAX_DETECTIONS (§9.2)
    const int numBoxes = vision_core::MAX_DETECTIONS + 5;
    std::vector<std::array<float, 4>> boxes;
    std::vector<int> classIds;
    std::vector<float> scores;

    for (int i = 0; i < numBoxes; ++i) {
        boxes.push_back({static_cast<float>(i * 30), static_cast<float>(i * 30), 20.0f, 20.0f});
        classIds.push_back(0);
        scores.push_back(0.5f + static_cast<float>(i) * 0.001f);
    }

    cv::Mat raw = makeSyntheticOutput(boxes, classIds, scores, numBoxes);
    LetterboxInfo info{1.0f, 0, 0};
    auto results = ObjectDetector::decodeAndNms(raw, info, 2000, 2000, 0.4f, 0.45f);

    EXPECT_EQ(results.size(), static_cast<size_t>(vision_core::MAX_DETECTIONS));
}

TEST(DecodeNmsTest, FiltersOutEverythingBelowThreshold) {
    std::vector<std::array<float, 4>> boxes = {{50, 50, 20, 20}};
    std::vector<int> classIds = {3};
    std::vector<float> scores = {0.2f};  // di bawah threshold

    cv::Mat raw = makeSyntheticOutput(boxes, classIds, scores, 1);
    LetterboxInfo info{1.0f, 0, 0};
    auto results = ObjectDetector::decodeAndNms(raw, info, 640, 640, 0.5f, 0.45f);

    EXPECT_TRUE(results.empty());
}
