#!/usr/bin/env bash
# Build OpenCV dari source (subset core/imgproc/dnn/imgcodecs) buat native testing (Fase 1).
#
# KENAPA GAK PAKAI `apt install libopencv-dev` AJA?
# Versi apt di Ubuntu 24.04 (4.6.0) punya bug shape-inference di cv::dnn buat
# node Slice/Reshape dinamis yang dipakai head DFL YOLOv8 -- forward() crash
# dengan assertion error di shape_utils.hpp. Sudah divalidasi: model ONNX yang
# sama jalan normal di OpenCV 4.13. Kita pin ke 4.10.0 (LTS-ish, jelas di atas
# batas minimal yang kepake buat YOLOv8 head).
#
# Build ini SENGAJA minimal (cuma 4 modul, static, tanpa GUI/video/GPU) biar
# compile-nya gak lama-lama amat dan gak nyeret dependency yang gak perlu.
set -euo pipefail

OPENCV_VERSION="4.10.0"
INSTALL_PREFIX="/opt/opencv-${OPENCV_VERSION}"
SRC_DIR="${HOME}/opencv-src-tmp"
BUILD_DIR="${HOME}/opencv-build-tmp"

if [ -d "$INSTALL_PREFIX" ]; then
    echo "OpenCV ${OPENCV_VERSION} udah keinstall di ${INSTALL_PREFIX}, skip."
    exit 0
fi

sudo apt update
sudo apt install -y git cmake build-essential

git clone --depth 1 --branch "${OPENCV_VERSION}" https://github.com/opencv/opencv.git "$SRC_DIR"

cmake -S "$SRC_DIR" -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX" \
    -DBUILD_LIST=core,imgproc,dnn,imgcodecs \
    -DBUILD_SHARED_LIBS=OFF \
    -DBUILD_TESTS=OFF \
    -DBUILD_PERF_TESTS=OFF \
    -DBUILD_EXAMPLES=OFF \
    -DBUILD_DOCS=OFF \
    -DBUILD_opencv_apps=OFF \
    -DBUILD_opencv_python2=OFF \
    -DBUILD_opencv_python3=OFF \
    -DBUILD_opencv_java=OFF \
    -DWITH_IPP=OFF -DWITH_ADE=OFF -DWITH_ITT=OFF \
    -DWITH_CUDA=OFF -DWITH_OPENCL=OFF \
    -DWITH_1394=OFF -DWITH_GTK=OFF -DWITH_QT=OFF \
    -DWITH_FFMPEG=OFF -DWITH_GSTREAMER=OFF -DWITH_V4L=OFF \
    -DWITH_PROTOBUF=BUILTIN -DBUILD_PROTOBUF=ON \
    -DBUILD_JPEG=ON -DBUILD_PNG=ON -DBUILD_ZLIB=ON

# -j$(nproc) dipakai di WSL2 kamu (harusnya lebih dari 1 core, jauh lebih cepat
# dibanding sandbox yang saya pakai buat validasi awal).
cmake --build "$BUILD_DIR" -j"$(nproc)"
sudo cmake --install "$BUILD_DIR"

rm -rf "$SRC_DIR" "$BUILD_DIR"

echo "OpenCV ${OPENCV_VERSION} terinstall di ${INSTALL_PREFIX}"
echo "Configure vision-core dengan:"
echo "  cmake -S . -B build-native -DCMAKE_PREFIX_PATH=${INSTALL_PREFIX}"
