#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>

#include <algorithm>
#include <cstdint>
#include <cstring>

#include "core/frame_buffer.hpp"
#include "core/pipeline.hpp"

// vc_init/vc_detect/vc_dispose dideklarasi di bindings.cpp (bukan header, karena
// itu bukan bagian yang dipublish sebagai kontrak internal C++ -- cuma dipanggil
// dari JS via embind). Declare manual di sini biar linker nemuin definisinya.
// (Gak perlu extern "C" -- embind sendiri manggil pointer fungsi C++ biasa, jadi
// name mangling standar udah cukup asal signature-nya persis sama.)
uintptr_t vc_init(int maxWidth, int maxHeight);
int vc_detect(uintptr_t inputPtr, int w, int h, uintptr_t outputPtr, float threshold);
void vc_dispose();

// Test ini nyimulasiin persis apa yang bakal dilakuin browser: dapetin RGBA dari
// <canvas> (di sini disimulasikan dari BGR->RGBA konversi gambar test), tulis ke
// input buffer, panggil vc_detect, baca hasil dari output buffer. Hasilnya HARUS
// sama persis kayak manggil Pipeline::run() langsung -- karena RGBA->BGR di dalam
// vc_detect itu cuma kebalikan dari BGR->RGBA yang kita lakuin di sini buat simulasi.
TEST(BindingsBridgeTest, MatchesDirectPipelineCall) {
    cv::Mat original = cv::imread("tests/fixtures/bus.jpg");
    ASSERT_FALSE(original.empty());

    // MAX_WIDTH/MAX_HEIGHT (640x480) itu kontrak buat resolusi capture webcam
    // (§10.1), bukan sembarang foto -- bus.jpg (810x1080) lebih gede dari itu,
    // jadi di-resize dulu biar simulasinya realistis sesuai kontrak buffer beneran.
    cv::Mat bgrImage;
    double scale = std::min(
        static_cast<double>(vision_core::MAX_WIDTH) / original.cols,
        static_cast<double>(vision_core::MAX_HEIGHT) / original.rows
    );
    cv::resize(original, bgrImage, cv::Size(), scale, scale);

    int w = bgrImage.cols;
    int h = bgrImage.rows;
    ASSERT_LE(w, vision_core::MAX_WIDTH);
    ASSERT_LE(h, vision_core::MAX_HEIGHT);

    // --- hasil "ground truth": panggil Pipeline langsung, persis kayak Fase 1 ---
    vision_core::Pipeline directPipeline;
    directPipeline.init("models/yolov8n.onnx");
    auto expected = directPipeline.run(bgrImage, 0.5f);

    // --- hasil lewat "jembatan" vc_init/vc_detect, simulasiin flow wasm ---
    cv::Mat rgbaImage;
    cv::cvtColor(bgrImage, rgbaImage, cv::COLOR_BGR2RGBA);
    ASSERT_TRUE(rgbaImage.isContinuous());

    uintptr_t inputPtr = vc_init(vision_core::MAX_WIDTH, vision_core::MAX_HEIGHT);
    ASSERT_NE(inputPtr, 0u);

    // simulasiin "JS nulis ke Module.HEAPU8 di inputPtr"
    std::memcpy(reinterpret_cast<void*>(inputPtr), rgbaImage.data,
                static_cast<size_t>(w) * h * 4);

    std::vector<float> outputBuffer(vision_core::MAX_DETECTIONS * vision_core::DETECTION_STRIDE, 0.0f);
    uintptr_t outputPtr = reinterpret_cast<uintptr_t>(outputBuffer.data());

    int n = vc_detect(inputPtr, w, h, outputPtr, 0.5f);

    ASSERT_EQ(static_cast<size_t>(n), expected.size());
    for (int i = 0; i < n; ++i) {
        const float* row = outputBuffer.data() + i * vision_core::DETECTION_STRIDE;
        EXPECT_FLOAT_EQ(row[0], expected[i].x);
        EXPECT_FLOAT_EQ(row[1], expected[i].y);
        EXPECT_FLOAT_EQ(row[2], expected[i].w);
        EXPECT_FLOAT_EQ(row[3], expected[i].h);
        EXPECT_FLOAT_EQ(row[4], expected[i].score);
        EXPECT_FLOAT_EQ(row[5], expected[i].classId);
    }

    vc_dispose();
}
