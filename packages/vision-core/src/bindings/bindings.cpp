// bindings.cpp — satu-satunya tempat yang boleh nyentuh emscripten API (§6.2).
// Implementasi vc_init/vc_detect/vc_dispose sesuai kontrak §10.2.
//
// SENGAJA didesain supaya file ini bisa dicompile NATIVE juga (tanpa emscripten) --
// bagian EMSCRIPTEN_BINDINGS dibungkus #ifdef __EMSCRIPTEN__ di bawah. Ini biar
// logic pointer-handling (bungkus raw buffer jadi cv::Mat, tulis balik ke outputPtr)
// bisa divalidasi native dulu sebelum kena kompleksitas build wasm (lihat tests/bindings_test.cpp).

#include <cstdint>
#include <cstdio>
#include <stdexcept>

#include <opencv2/opencv.hpp>

#include "core/frame_buffer.hpp"
#include "core/pipeline.hpp"

namespace {

// Model dibundle langsung ke wasm package via --preload-file (lihat CMakeLists.txt),
// jadi path virtual-nya sengaja disamain sama path relatif native, biar C++-nya
// gak perlu tau dia lagi jalan native atau wasm.
constexpr const char* MODEL_PATH = "models/yolov8n.onnx";

// State global sengaja dipakai di sini (bukan anti-pattern buat kasus ini): vc_init/
// vc_detect/vc_dispose emang didesain sebagai C-style API satu-instance, sesuai §10.2
// (gak ada "handle" yang di-pass balik ke tiap panggilan). Alokasi SEKALI di init(),
// di-reuse tiap frame, dibebasin di dispose() -- persis §6.2.
vision_core::Pipeline* g_pipeline = nullptr;
uint8_t* g_inputBuffer = nullptr;

}  // namespace

uintptr_t vc_init(int maxWidth, int maxHeight) {
    if (maxWidth > vision_core::MAX_WIDTH || maxHeight > vision_core::MAX_HEIGHT) {
        throw std::runtime_error("maxWidth/maxHeight melebihi MAX_WIDTH/MAX_HEIGHT (§10.1)");
    }

    if (g_inputBuffer == nullptr) {
        g_inputBuffer = new uint8_t[vision_core::MAX_WIDTH * vision_core::MAX_HEIGHT * 4];
    }

    if (g_pipeline == nullptr) {
        g_pipeline = new vision_core::Pipeline();
        g_pipeline->init(MODEL_PATH);
    }

    return reinterpret_cast<uintptr_t>(g_inputBuffer);
}

int vc_detect(uintptr_t inputPtr, int w, int h, uintptr_t outputPtr, float threshold) {
    if (g_pipeline == nullptr) {
        throw std::runtime_error("vc_detect dipanggil sebelum vc_init");
    }

    uint8_t* input = reinterpret_cast<uint8_t*>(inputPtr);
    float* output = reinterpret_cast<float*>(outputPtr);

    // Frame dari <canvas> ImageData selalu RGBA (drawImage/getImageData). Bungkus
    // sebagai cv::Mat TANPA copy (constructor dengan pointer eksternal), baru convert
    // ke BGR (drop alpha channel sekalian) -- ini satu-satunya copy yang gak terhindarkan
    // karena semua logic Pipeline/ObjectDetector dari Fase 1 asumsinya cv::Mat BGR biasa.
    cv::Mat rgba(h, w, CV_8UC4, input);
    cv::Mat bgr;
    cv::cvtColor(rgba, bgr, cv::COLOR_RGBA2BGR);

    auto detections = g_pipeline->run(bgr, threshold);
    return vision_core::writeDetections(detections, output);
}

void vc_dispose() {
    delete g_pipeline;
    g_pipeline = nullptr;
    delete[] g_inputBuffer;
    g_inputBuffer = nullptr;
}

#ifdef __EMSCRIPTEN__
#include <emscripten/bind.h>

EMSCRIPTEN_BINDINGS(vision_core_module) {
    emscripten::function("vc_init", &vc_init);
    emscripten::function("vc_detect", &vc_detect);
    emscripten::function("vc_dispose", &vc_dispose);
}
#endif
