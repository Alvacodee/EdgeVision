#!/usr/bin/env bash
# Build OpenCV ke wasm pakai emcmake (subset core/imgproc/dnn) buat Fase 2.
#
# KENAPA BUKAN build_js.py?
# build_js.py (cara "resmi" yang disebut dokumentasi OpenCV) generate satu file
# opencv.js monolitik lengkap sama binding JS bawaannya sendiri (cv.imread, cv.Mat,
# dst) -- itu didesain buat dipakai LANGSUNG dari JS, bukan buat dilink sebagai
# static library ke project embind kustom kita (bindings.cpp). Yang kita butuh:
# static lib wasm (.a) hasil emcmake, biar bisa di-link ke vision_core.js yang isi
# bindingnya vc_init/vc_detect/vc_dispose kita sendiri (§10.2). Makanya pakai
# emcmake cmake langsung, persis pola setup-opencv-native.sh tapi toolchain wasm.
#
# Butuh emsdk udah di-source duluan (jalankan scripts/setup-emsdk.sh dulu kalau belum).
set -euo pipefail

OPENCV_VERSION="4.10.0"
INSTALL_PREFIX="/opt/opencv-wasm-${OPENCV_VERSION}"
SRC_DIR="${HOME}/opencv-src-wasm-tmp"
BUILD_DIR="${HOME}/opencv-build-wasm-tmp"

if ! command -v emcmake &> /dev/null; then
    echo "emcmake tidak ketemu. Pastikan sudah 'source ~/emsdk/emsdk_env.sh'." >&2
    exit 1
fi

if [ -d "$INSTALL_PREFIX" ]; then
    echo "OpenCV wasm ${OPENCV_VERSION} udah keinstall di ${INSTALL_PREFIX}, skip."
    exit 0
fi

git clone --depth 1 --branch "${OPENCV_VERSION}" https://github.com/opencv/opencv.git "$SRC_DIR"

# Gak perlu imgcodecs/jpeg/png/zlib -- wasm gak baca file gambar, cuma nerima
# raw RGBA buffer langsung dari <canvas> (lihat bindings.cpp).
emcmake cmake -S "$SRC_DIR" -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX" \
    -DBUILD_LIST=core,imgproc,dnn \
    -DBUILD_SHARED_LIBS=OFF \
    -DBUILD_TESTS=OFF \
    -DBUILD_PERF_TESTS=OFF \
    -DBUILD_EXAMPLES=OFF \
    -DBUILD_DOCS=OFF \
    -DBUILD_opencv_apps=OFF \
    -DWITH_IPP=OFF -DWITH_ADE=OFF -DWITH_ITT=OFF \
    -DWITH_PROTOBUF=BUILTIN -DBUILD_PROTOBUF=ON \
    -DWITH_JPEG=OFF -DWITH_PNG=OFF -DWITH_TIFF=OFF -DWITH_WEBP=OFF -DWITH_OPENEXR=OFF

cmake --build "$BUILD_DIR" -j"$(nproc)"
cmake --install "$BUILD_DIR"

rm -rf "$SRC_DIR" "$BUILD_DIR"

echo "OpenCV wasm ${OPENCV_VERSION} terinstall di ${INSTALL_PREFIX}"
echo "Configure vision-core (wasm) dengan:"
echo "  emcmake cmake -S . -B build-wasm -DCMAKE_PREFIX_PATH=${INSTALL_PREFIX}"
