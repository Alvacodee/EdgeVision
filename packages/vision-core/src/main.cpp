// main.cpp — CLI harness native buat Fase 1. Baca satu gambar, jalanin pipeline
// deteksi, cetak hasil ke stdout, dan simpan gambar dengan bounding box buat cek visual.
//
// Pemakaian: ./vision_core_native <path_gambar> [threshold] [path_model]
// Nama file output otomatis ngikut nama gambar input (biar run beda gambar
// gak saling timpa hasil satu sama lain).

#include <cstdio>
#include <filesystem>
#include <string>

#include <opencv2/opencv.hpp>

#include "core/pipeline.hpp"
#include "detectors/coco_classes.hpp"

int main(int argc, char** argv) {
    std::string imagePath = argc > 1 ? argv[1] : "tests/fixtures/bus.jpg";
    float threshold = argc > 2 ? std::stof(argv[2]) : 0.5f;
    std::string modelPath = argc > 3 ? argv[3] : "models/yolov8n.onnx";

    vision_core::Pipeline pipeline;
    try {
        pipeline.init(modelPath);
    } catch (const std::exception& e) {
        fprintf(stderr, "Error init pipeline: %s\n", e.what());
        return 1;
    }

    cv::Mat frame = cv::imread(imagePath);
    if (frame.empty()) {
        fprintf(stderr, "Gagal baca gambar: %s\n", imagePath.c_str());
        return 1;
    }

    auto detections = pipeline.run(frame, threshold);

    const auto& classNames = vision_core::cocoClassNames();
    printf("Ketemu %zu deteksi (threshold=%.2f):\n", detections.size(), threshold);

    for (const auto& d : detections) {
        int classId = static_cast<int>(d.classId);
        std::string label = (classId >= 0 && classId < vision_core::NUM_COCO_CLASSES)
                                 ? classNames[classId]
                                 : "unknown";
        printf("  - %-15s score=%.2f  bbox=[x=%.0f y=%.0f w=%.0f h=%.0f]\n",
               label.c_str(), d.score, d.x, d.y, d.w, d.h);

        // gambar bounding box + label di frame buat verifikasi visual
        cv::Rect box(static_cast<int>(d.x), static_cast<int>(d.y),
                     static_cast<int>(d.w), static_cast<int>(d.h));
        cv::rectangle(frame, box, cv::Scalar(0, 255, 0), 2);
        std::string text = label + " " + std::to_string(static_cast<int>(d.score * 100)) + "%";
        cv::putText(frame, text, cv::Point(box.x, box.y - 6),
                    cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 255, 0), 2);
    }

    std::string outputPath = "output/" + std::filesystem::path(imagePath).stem().string() + "_detected.jpg";
    std::filesystem::create_directories("output");  // biar gak gagal kalau folder belum ada
    cv::imwrite(outputPath, frame);
    printf("Hasil visual disimpan ke: %s\n", outputPath.c_str());

    return 0;
}
