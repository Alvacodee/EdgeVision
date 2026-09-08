cmake_minimum_required(VERSION 3.16)
project(vision_core CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(EMSCRIPTEN)
    # --- Build wasm (Fase 2) ---
    # OpenCV di sini HARUS di-build lewat emcmake (lihat scripts/setup-opencv-wasm.sh),
    # BUKAN build_js.py -- build_js.py generate opencv.js monolitik dengan binding JS-nya
    # sendiri, bukan static lib yang bisa dilink ke embind kustom kita. Configure project
    # ini dengan: emcmake cmake -S . -B build-wasm -DCMAKE_PREFIX_PATH=/opt/opencv-wasm-4.10.0
    #
    # Cuma butuh core/imgproc/dnn -- imgcodecs (baca/tulis file gambar) gak perlu di sisi
    # wasm karena frame masuk langsung sebagai raw RGBA buffer dari <canvas>, bukan file.
    set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)
    find_package(OpenCV REQUIRED COMPONENTS core imgproc dnn)

    if(OpenCV_VERSION VERSION_LESS "4.8.0")
        message(FATAL_ERROR
            "OpenCV wasm ${OpenCV_VERSION} ketemu, tapi versi ini punya bug shape-inference "
            "buat head YOLOv8 (sama kayak kasus native, lihat AGENT.md §16). Pastikan "
            "scripts/setup-opencv-wasm.sh build dari tag 4.10.0."
        )
    endif()

    add_executable(vision_core
        src/core/pipeline.cpp
        src/detectors/object_detector.cpp
        src/bindings/bindings.cpp
    )
    target_include_directories(vision_core PRIVATE src ${OpenCV_INCLUDE_DIRS})
    target_link_libraries(vision_core PRIVATE ${OpenCV_LIBS})

    # Model di-bundle langsung ke package wasm (jadi vision-core.data) biar vc_init
    # gak perlu API tambahan buat load model dari luar -- tetap 3 fungsi persis §10.2.
    set(MODEL_FILE "${CMAKE_CURRENT_SOURCE_DIR}/models/yolov8n.onnx")

    # Flag wajib sesuai AGENT.md §12.4
    target_link_options(vision_core PRIVATE
        "-sMODULARIZE=1"
        "-sEXPORT_ES6=1"
        "-sALLOW_MEMORY_GROWTH=1"
        "-sENVIRONMENT=web,worker"
        "-sEXPORTED_RUNTIME_METHODS=['HEAPU8','HEAPF32']"
        "-sEXPORTED_FUNCTIONS=['_malloc','_free']" 
        "--bind"
        "-O3"
        "--preload-file" "${MODEL_FILE}@models/yolov8n.onnx"
    )
    set_target_properties(vision_core PROPERTIES
        OUTPUT_NAME "vision-core"
        SUFFIX ".js"
    )
else()
    # --- Build native (Fase 1: engine deteksi + gtest. Fase 2: tambah bindings_test) ---
    # OpenCV kita di-link ke OpenEXR (dependency imgcodecs di sistem build),
    # jadi perlu di-resolve juga di sini biar target OpenEXR::OpenEXR ketemu.
    find_package(OpenEXR QUIET)
    find_package(OpenCV REQUIRED COMPONENTS core imgproc dnn imgcodecs)

    # OpenCV < 4.8 punya bug shape-inference di cv::dnn buat node Slice/Reshape
    # dinamis yang dipakai head DFL YOLOv8 -- forward() bakal crash saat runtime,
    # bukan gagal compile, jadi kita cek versi di sini biar gagalnya cepat & jelas.
    # Kalau kena ini, jalankan scripts/setup-opencv-native.sh lalu configure ulang
    # dengan -DCMAKE_PREFIX_PATH=/opt/opencv-4.10.0
    if(OpenCV_VERSION VERSION_LESS "4.8.0")
        message(FATAL_ERROR
            "OpenCV ${OpenCV_VERSION} ketemu, tapi versi ini punya bug shape-inference "
            "buat head YOLOv8 (lihat AGENT.md §16). Jalankan scripts/setup-opencv-native.sh "
            "lalu configure ulang dengan -DCMAKE_PREFIX_PATH=/opt/opencv-4.10.0"
        )
    endif()

    add_library(vision_core_lib
        src/core/pipeline.cpp
        src/detectors/object_detector.cpp
    )
    target_include_directories(vision_core_lib PUBLIC src ${OpenCV_INCLUDE_DIRS})
    target_link_libraries(vision_core_lib PUBLIC ${OpenCV_LIBS})

    add_executable(vision_core_native src/main.cpp)
    target_link_libraries(vision_core_native PRIVATE vision_core_lib)

    # --- Unit test (gtest, §13.1) ---
    enable_testing()
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.15.2
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    add_executable(vision_core_tests
        tests/letterbox_test.cpp
        tests/decode_nms_test.cpp
        tests/bindings_test.cpp
        src/bindings/bindings.cpp
    )
    target_link_libraries(vision_core_tests PRIVATE vision_core_lib gtest_main)
    target_include_directories(vision_core_tests PRIVATE src)

    include(GoogleTest)
    gtest_discover_tests(vision_core_tests)
endif()
