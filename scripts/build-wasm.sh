#!/usr/bin/env bash
# Compile packages/vision-core ke wasm, hasilnya diarahkan ke apps/web/public/wasm.
# Butuh emsdk sudah di-source (scripts/setup-emsdk.sh) dan OpenCV wasm sudah
# dibuild (scripts/setup-opencv-wasm.sh) sebelum jalanin ini.
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CORE_DIR="${ROOT_DIR}/packages/vision-core"
BUILD_DIR="${CORE_DIR}/build-wasm"
OUT_DIR="${ROOT_DIR}/apps/web/public/wasm"
OPENCV_WASM_PREFIX="/opt/opencv-wasm-4.10.0"

if ! command -v emcmake &> /dev/null; then
    echo "emcmake tidak ketemu. Pastikan sudah 'source ~/emsdk/emsdk_env.sh'." >&2
    exit 1
fi

if [ ! -d "$OPENCV_WASM_PREFIX" ]; then
    echo "OpenCV wasm belum ada di ${OPENCV_WASM_PREFIX}. Jalankan scripts/setup-opencv-wasm.sh dulu." >&2
    exit 1
fi

if [ ! -f "${CORE_DIR}/models/yolov8n.onnx" ]; then
    echo "Model ${CORE_DIR}/models/yolov8n.onnx gak ketemu (gitignored, harus ada lokal dari Fase 1)." >&2
    exit 1
fi

mkdir -p "$OUT_DIR"

emcmake cmake -S "$CORE_DIR" -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH="$OPENCV_WASM_PREFIX"
cmake --build "$BUILD_DIR"

# --preload-file (di CMakeLists.txt) generate .data tambahan buat model yang di-bundle,
# jadi ada 3 file yang harus ikut disalin, bukan cuma .js + .wasm.
cp "$BUILD_DIR"/vision-core.js "$OUT_DIR"/
cp "$BUILD_DIR"/vision-core.wasm "$OUT_DIR"/
cp "$BUILD_DIR"/vision-core.data "$OUT_DIR"/

echo "Build wasm selesai -> ${OUT_DIR}/vision-core.{js,wasm,data}"
