#include "detectors/object_detector.hpp"

#include <algorithm>
#include <cmath>

namespace vision_core {

bool ObjectDetector::loadModel(const std::string& modelPath) {
    net_ = cv::dnn::readNetFromONNX(modelPath);
    return !net_.empty();
}

cv::Mat ObjectDetector::letterbox(const cv::Mat& src, int targetSize, LetterboxInfo& outInfo) {
    int srcW = src.cols;
    int srcH = src.rows;

    // scale dipilih dari sisi yang lebih "ketat" biar gak ada bagian gambar kepotong,
    // sisanya diisi padding (bukan stretch, biar aspect ratio objek gak berubah).
    float scale = std::min(static_cast<float>(targetSize) / srcW,
                            static_cast<float>(targetSize) / srcH);
    int newW = static_cast<int>(std::round(srcW * scale));
    int newH = static_cast<int>(std::round(srcH * scale));

    cv::Mat resized;
    cv::resize(src, resized, cv::Size(newW, newH));

    int padX = (targetSize - newW) / 2;
    int padY = (targetSize - newH) / 2;

    // 114 abu-abu adalah konvensi padding standar YOLO (bukan hitam/putih).
    cv::Mat canvas(targetSize, targetSize, src.type(), cv::Scalar(114, 114, 114));
    resized.copyTo(canvas(cv::Rect(padX, padY, newW, newH)));

    outInfo.scale = scale;
    outInfo.padX = padX;
    outInfo.padY = padY;
    return canvas;
}

std::vector<Detection> ObjectDetector::decodeAndNms(
    const cv::Mat& rawOutput,
    const LetterboxInfo& info,
    int origWidth,
    int origHeight,
    float scoreThreshold,
    float iouThreshold
) {
    // rawOutput shape [1, 84, 2100] (§9.2). Reshape drop batch dim -> 84 x 2100,
    // lalu transpose -> 2100 x 84 biar tiap row = satu anchor [cx,cy,w,h,class0..79].
    cv::Mat out = rawOutput.reshape(1, rawOutput.size[1]);
    cv::Mat outT;
    cv::transpose(out, outT);

    const int numAnchors = outT.rows;
    const int numClasses = outT.cols - 4;

    std::vector<cv::Rect> boxes;
    std::vector<float> scores;
    std::vector<int> classIds;

    for (int i = 0; i < numAnchors; ++i) {
        const float* row = outT.ptr<float>(i);
        float cx = row[0], cy = row[1], w = row[2], h = row[3];

        int bestClass = -1;
        float bestScore = 0.0f;
        for (int c = 0; c < numClasses; ++c) {
            float s = row[4 + c];
            if (s > bestScore) {
                bestScore = s;
                bestClass = c;
            }
        }
        if (bestScore < scoreThreshold) continue;

        // masih di koordinat letterboxed 320x320, cx/cy/w/h -> x1,y1,w,h buat cv::Rect
        int x1 = static_cast<int>(std::round(cx - w / 2.0f));
        int y1 = static_cast<int>(std::round(cy - h / 2.0f));
        int bw = static_cast<int>(std::round(w));
        int bh = static_cast<int>(std::round(h));

        boxes.emplace_back(x1, y1, bw, bh);
        scores.push_back(bestScore);
        classIds.push_back(bestClass);
    }

    std::vector<int> keepIndices;
    cv::dnn::NMSBoxes(boxes, scores, scoreThreshold, iouThreshold, keepIndices);

    std::vector<Detection> results;
    results.reserve(keepIndices.size());

    for (int idx : keepIndices) {
        const cv::Rect& box = boxes[idx];

        // unletterbox: balik dari kanvas 320x320 ke koordinat frame asli
        float x = (box.x - info.padX) / info.scale;
        float y = (box.y - info.padY) / info.scale;
        float w = box.width / info.scale;
        float h = box.height / info.scale;

        // clip biar gak keluar batas frame asli (bisa kejadian karena pembulatan)
        x = std::max(0.0f, x);
        y = std::max(0.0f, y);
        w = std::min(w, origWidth - x);
        h = std::min(h, origHeight - y);

        Detection d;
        d.x = x;
        d.y = y;
        d.w = w;
        d.h = h;
        d.score = scores[idx];
        d.classId = static_cast<float>(classIds[idx]);
        results.push_back(d);
    }

    // NMSBoxes udah ngurutin by score desc secara internal, tapi kita sort ulang
    // biar eksplisit gak gantung ke behavior internal OpenCV yang bisa berubah versi.
    std::sort(results.begin(), results.end(),
              [](const Detection& a, const Detection& b) { return a.score > b.score; });

    if (static_cast<int>(results.size()) > MAX_DETECTIONS) {
        results.resize(MAX_DETECTIONS);
    }

    return results;
}

std::vector<Detection> ObjectDetector::detect(const cv::Mat& frame, float scoreThreshold) const {
    LetterboxInfo info;
    cv::Mat letterboxed = letterbox(frame, INPUT_SIZE, info);

    // scale 1/255, swapRB=true (BGR punya OpenCV -> RGB yang diharapkan model, §9.1),
    // tanpa mean subtraction (konvensi training YOLO standar).
    cv::Mat blob = cv::dnn::blobFromImage(
        letterboxed, 1.0 / 255.0, cv::Size(INPUT_SIZE, INPUT_SIZE),
        cv::Scalar(0, 0, 0), /*swapRB=*/true, /*crop=*/false
    );

    net_.setInput(blob);
    cv::Mat output = net_.forward();

    return decodeAndNms(output, info, frame.cols, frame.rows, scoreThreshold, NMS_IOU_THRESHOLD);
}

} // namespace vision_core
