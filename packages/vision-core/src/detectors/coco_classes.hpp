#pragma once

#include <array>
#include <string>

// Urutan class ID HARUS sama persis kayak urutan training COCO yang dipakai YOLOv8.
// Kalau nanti ganti model (misal fallback ke MobileNet-SSD di §9), cek ulang urutan ini
// karena beberapa format punya urutan/jumlah class yang beda.
namespace vision_core {

constexpr int NUM_COCO_CLASSES = 80;

inline const std::array<std::string, NUM_COCO_CLASSES>& cocoClassNames() {
    static const std::array<std::string, NUM_COCO_CLASSES> names = {
        "person", "bicycle", "car", "motorcycle", "airplane", "bus", "train", "truck",
        "boat", "traffic light", "fire hydrant", "stop sign", "parking meter", "bench",
        "bird", "cat", "dog", "horse", "sheep", "cow", "elephant", "bear", "zebra",
        "giraffe", "backpack", "umbrella", "handbag", "tie", "suitcase", "frisbee",
        "skis", "snowboard", "sports ball", "kite", "baseball bat", "baseball glove",
        "skateboard", "surfboard", "tennis racket", "bottle", "wine glass", "cup",
        "fork", "knife", "spoon", "bowl", "banana", "apple", "sandwich", "orange",
        "broccoli", "carrot", "hot dog", "pizza", "donut", "cake", "chair", "couch",
        "potted plant", "bed", "dining table", "toilet", "tv", "laptop", "mouse",
        "remote", "keyboard", "cell phone", "microwave", "oven", "toaster", "sink",
        "refrigerator", "book", "clock", "vase", "scissors", "teddy bear", "hair drier",
        "toothbrush"
    };
    return names;
}

} // namespace vision_core
