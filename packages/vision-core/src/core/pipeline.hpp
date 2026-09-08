#pragma once

#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

#include "core/frame_buffer.hpp"
#include "detectors/object_detector.hpp"

namespace vision_core {

// Pipeline = satu-satunya pintu masuk ke logic deteksi. bindings.cpp (Fase 2) manggil
// class ini, BUKAN ObjectDetector langsung — biar kalau nanti ada preprocessing
// tambahan sebelum/sesudah detector (misal tracking antar frame), gak perlu bongkar bindings.
class Pipeline {
public:
    // Boleh throw sekali di sini kalau model gagal dimuat (§15) — caller (main.cpp
    // atau nanti vc_init) yang nentuin gimana nanganinnya.
    void init(const std::string& modelPath);

    std::vector<Detection> run(const cv::Mat& frame, float scoreThreshold) const;

    bool isInitialized() const { return initialized_; }

private:
    ObjectDetector detector_;
    bool initialized_ = false;
};

} // namespace vision_core
