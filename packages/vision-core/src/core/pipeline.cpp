#include "core/pipeline.hpp"

#include <stdexcept>

namespace vision_core {

void Pipeline::init(const std::string& modelPath) {
    if (!detector_.loadModel(modelPath)) {
        throw std::runtime_error("Gagal load model ONNX dari: " + modelPath);
    }
    initialized_ = true;
}

std::vector<Detection> Pipeline::run(const cv::Mat& frame, float scoreThreshold) const {
    if (!initialized_) {
        throw std::runtime_error("Pipeline belum di-init, panggil init() dulu");
    }
    return detector_.detect(frame, scoreThreshold);
}

} // namespace vision_core
