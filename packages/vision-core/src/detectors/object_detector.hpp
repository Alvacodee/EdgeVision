#pragma once

#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <string>
#include <vector>

#include "core/frame_buffer.hpp"

namespace vision_core {

// Info hasil letterbox, dipakai lagi buat unletterbox koordinat balik ke frame asli.
struct LetterboxInfo {
    float scale;   // faktor resize yang dipakai (sama untuk x dan y, aspect ratio terjaga)
    int padX;      // padding kiri-kanan (px) di kanvas hasil letterbox
    int padY;      // padding atas-bawah (px) di kanvas hasil letterbox
};

class ObjectDetector {
public:
    // Fixed sesuai §9: input model 320x320, NMS IoU 0.45 (gak diekspos ke UI di MVP).
    static constexpr int INPUT_SIZE = 320;
    static constexpr float NMS_IOU_THRESHOLD = 0.45f;

    // Load model ONNX. Return false kalau gagal (caller yang mutusin mau throw atau enggak,
    // sesuai §15: boleh throw sekali di init(), tapi detector sendiri gak throw).
    bool loadModel(const std::string& modelPath);

    // Jalanin full pipeline: letterbox -> blob -> forward -> decode -> NMS -> unletterbox.
    // Hasil sudah dalam koordinat pixel relatif ke frame asli (bukan letterboxed 320x320).
    std::vector<Detection> detect(const cv::Mat& frame, float scoreThreshold) const;

    // --- Static helper, sengaja public biar bisa di-gtest tanpa perlu load model (§13.1) ---

    // Resize gambar ke kanvas persegi targetSize x targetSize dengan padding abu-abu
    // (aspect ratio terjaga, bukan stretch). Ini "letterbox" yang dimaksud §9.1.
    static cv::Mat letterbox(const cv::Mat& src, int targetSize, LetterboxInfo& outInfo);

    // Decode raw output tensor YOLOv8 [1, 84, 2100] jadi list Detection dalam koordinat
    // frame asli, sudah lolos confidence filter + NMS. Murni logic, gak nyentuh cv::dnn::Net,
    // jadi bisa ditest pakai tensor buatan sendiri tanpa model beneran.
    static std::vector<Detection> decodeAndNms(
        const cv::Mat& rawOutput,
        const LetterboxInfo& letterboxInfo,
        int origWidth,
        int origHeight,
        float scoreThreshold,
        float iouThreshold
    );

private:
    // mutable: forward() OpenCV secara internal nulis ke buffer cache si Net,
    // tapi dari sudut pandang caller detect() tetap logically const (input sama -> output sama).
    mutable cv::dnn::Net net_;
};

} // namespace vision_core
