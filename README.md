# EdgeVision

Real-time object detection that runs **entirely in the browser** — no backend, no GPU server, no video frame ever leaves the client. The detection engine is written in C++/OpenCV and compiled to WebAssembly; Next.js handles orchestration and UI only.

> Status: active development. See [Project status](#project-status) below for what's actually verified vs. still in progress — nothing here is overstated.

## Why this exists

Server-based computer vision has two structural problems: GPU inference costs money at scale, and sending a user's camera feed to a third-party server is a privacy liability. EdgeVision is a proof of concept that both can be eliminated with an efficient C++ → WebAssembly pipeline, without giving up real-time performance.

This project is built as a portfolio piece demonstrating low-level systems work (C++, manual memory management across a JS/Wasm boundary) alongside a modern web stack (Next.js, TypeScript, Web Workers).

## How it works

```
Main thread                                Web Worker (module worker)
────────────                               ──────────────────────────
<video> (camera)
   │
   ▼
createImageBitmap(video, {resize...})
   │  postMessage({type:'frame', bitmap}, [bitmap])   (Transferable, zero-copy)
   └──────────────────────────────────────────►  OffscreenCanvas.drawImage(bitmap)
                                                       │
                                                       ▼
                                                 getImageData()
                                                       │
                                                       ▼
                                                 Module.HEAPU8.set(...)
                                                       │
                                                       ▼
                                                 Module._vc_detect(...)
                                                   (C++: preprocess → cv::dnn
                                                    forward → NMS → postprocess)
                                                       │
   ◄──────────────────────────────────────────  postMessage({type:'detections'})
   ▼
render bounding boxes on overlay canvas
```

All pixel extraction and inference happen inside a Web Worker so the main thread stays free to render UI. Full write-up of this design (including the backpressure strategy and memory-management details) is in [`docs/architecture.md`](./docs/architecture.md).

## Tech stack

| Layer | Technology |
|---|---|
| AI / Vision core | C++17, OpenCV (`core`, `imgproc`, `dnn`) |
| Compilation | Emscripten, CMake |
| Model | YOLOv8n, ONNX format, 320×320 input |
| Frontend | Next.js (App Router), React 19, TypeScript |
| Styling | Tailwind CSS |
| Concurrency | Web Worker + OffscreenCanvas |
| Testing (native) | GoogleTest |

## Project status

| Phase | Focus | Status |
|---|---|---|
| 0 — Tooling validation | emsdk + CMake + wasm "hello world" reachable from Next.js | ✅ Done |
| 1 — Native engine | OpenCV DNN module, YOLOv8n inference via native CLI, gtest coverage | ✅ Done |
| 2 — Wasm port | `vc_init` / `vc_detect` / `vc_dispose` compiled to wasm, verified against native output | ✅ Done |
| 3 — Frontend integration | Camera, canvas overlay, threshold/FPS controls, main-thread detection loop | ✅ Done |
| 4 — Hardening & performance | Move detection off main thread via Web Worker + OffscreenCanvas | 🔶 Implemented, FPS/memory verification in progress |
| 5 — Portfolio polish | Final benchmark numbers, demo GIF/video, architecture write-up for reviewers | ⬜ Not started |

**Baseline (Phase 3, main thread):** ~4–6 FPS on a mid-range laptop CPU. Phase 4 moves the expensive work off the main thread; whether that translates into a higher FPS number depends on whether the DNN forward pass itself (not `getImageData`) is the actual bottleneck — this is what's currently being measured.

## Getting started

### Prerequisites (one-time)
```bash
sudo apt update && sudo apt install -y git cmake python3 build-essential
git clone https://github.com/emscripten-core/emsdk.git ~/emsdk
cd ~/emsdk && ./emsdk install latest && ./emsdk activate latest
echo 'source ~/emsdk/emsdk_env.sh' >> ~/.bashrc && source ~/.bashrc
```

OpenCV must be built from source at **4.10.0** — the Ubuntu 24.04 default (4.6.0) has a shape-inference bug that crashes on YOLOv8's DFL head. See `scripts/setup-opencv-native.sh` and `scripts/setup-opencv-wasm.sh`.

### Build the wasm module
```bash
./scripts/build-wasm.sh
```
This produces `vision-core.js`, `vision-core.wasm`, and `vision-core.data` under `apps/web/public/wasm/` (gitignored — must be built locally, not committed).

### Run the app
```bash
cd apps/web
npm install
npm run dev
```
Open `http://localhost:3000`, allow camera access, and detection starts automatically once the wasm module reports ready.

### Run native tests (fast iteration, no wasm required)
```bash
cd packages/vision-core
cmake -B build && cmake --build build
ctest --test-dir build
```

## Project structure

```
edge-vision/
├── apps/web/                      # Next.js app
│   └── src/
│       ├── app/                   # routes, page composition
│       ├── components/vision/     # camera view, overlay, controls
│       ├── features/detection/
│       │   ├── hooks/             # useCamera, useDetectionWorker, useFrameCapture
│       │   └── workers/           # detection.worker.ts, message protocol
│       └── lib/wasm/              # thin TS wrapper around the wasm module
├── packages/vision-core/          # C++ detection engine (native + wasm bindings)
├── scripts/                       # toolchain setup, wasm build, model download
├── docs/architecture.md           # Phase 4 architecture deep-dive
└── AGENT.md                       # living spec / single source of truth for API contracts
```

## Known limitations

- **Chrome/Edge is the primary target.** Firefox is secondary; Safari's WASM SIMD support has not been evaluated. OffscreenCanvas + module workers require a reasonably modern browser.
- **No custom-trained model.** Uses off-the-shelf YOLOv8n on COCO classes — not fine-tuned for any specific use case.
- **Development-mode artifact:** in `next dev`, React Strict Mode double-invokes effects, which briefly spawns a second wasm instance that is immediately disposed. This does not occur in production builds (`next build && next start`).