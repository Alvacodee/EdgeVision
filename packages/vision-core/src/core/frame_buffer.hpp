#pragma once

#include <vector>

// Kontrak memori sesuai AGENT.md §10.1. Konstanta ini dipakai bareng-bareng
// sama native harness (Fase 1) dan nanti wasm bindings (Fase 2) — jangan diubah
// sendiri-sendiri, biar gak ada dua "sumber kebenaran" untuk ukuran buffer.

namespace vision_core {

constexpr int MAX_WIDTH = 640;
constexpr int MAX_HEIGHT = 480;
constexpr int MAX_DETECTIONS = 20;

// Tiap deteksi ditulis sebagai 6 float berurutan: x, y, w, h, score, classId.
// x,y,w,h dalam satuan pixel, relatif ke frame asli (sudah di-unletterbox).
constexpr int DETECTION_STRIDE = 6;

struct Detection {
    float x;
    float y;
    float w;
    float h;
    float score;
    float classId;
};

// Tulis vector<Detection> ke buffer float flat sesuai layout output buffer §10.1.
// outBuffer harus punya kapasitas minimal MAX_DETECTIONS * DETECTION_STRIDE float.
// Dipakai bareng sama CLI harness (Fase 1) dan vc_detect (Fase 2, lewat outputPtr).
inline int writeDetections(const std::vector<Detection>& detections, float* outBuffer) {
    int n = static_cast<int>(detections.size());
    if (n > MAX_DETECTIONS) n = MAX_DETECTIONS;

    for (int i = 0; i < n; ++i) {
        const Detection& d = detections[i];
        float* row = outBuffer + i * DETECTION_STRIDE;
        row[0] = d.x;
        row[1] = d.y;
        row[2] = d.w;
        row[3] = d.h;
        row[4] = d.score;
        row[5] = d.classId;
    }
    return n;
}

} // namespace vision_core
