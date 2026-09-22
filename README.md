# EdgeVision

Real-time object detection that runs **entirely in the browser** — no backend, no GPU server, no video frame ever leaves the client. The detection engine is written in C++/OpenCV and compiled to WebAssembly; Next.js handles orchestration and UI only.

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

All pixel extraction and inference happen inside a Web Worker so the main thread stays free to render UI. Full write-up of this design — including the backpressure strategy, memory-management details, and the profiling process used to confirm the architecture actually works as intended — is in [`docs/architecture.md`](./docs/architecture.md).

## Tech stack

| Layer | Technology |
|---|---|
| AI / Vision core | C++17, OpenCV 4.10.0+ (`core`, `imgproc`, `dnn`) |
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
| 4 — Hardening & performance | Move detection off main thread via Web Worker + OffscreenCanvas | ✅ Done |
| 5 — Portfolio polish | Demo GIF/video, public live deploy | ⬜ Not started — deprioritized for now |

The engineering work this project is meant to demonstrate — the C++/Wasm pipeline, the memory contract across the JS boundary, and the Web Worker performance investigation — is complete as of Phase 4. Phase 5 (demo media, optional public deploy) is left for later, revisited before active job applications rather than blocking the project on it now.

**Benchmark (dev machine: Windows + WSL2, Chrome):**

| Metric | Phase 3 (main thread) | Phase 4 (worker) |
|---|---|---|
| Average FPS | 4-6 | 3-5 |
| Main-thread responsiveness while running | Noticeably janky | Smooth |
| Heap growth over 5 minutes | Not tested | Stable (~15.0 MB constant) |

FPS did not improve between Phase 3 and 4 — this was investigated and confirmed to be expected, not a bug. Profiling showed the worker genuinely runs in parallel with the main thread (main-thread responsiveness improved measurably), but the actual bottleneck is the DNN forward pass in C++ itself, which `OffscreenCanvas` cannot speed up. Full investigation in `docs/architecture.md`.

## Getting started

### Prerequisites (one-time)
```bash
sudo apt update && sudo apt install -y git cmake python3 build-essential
git clone https://github.com/emscripten-core/emsdk.git ~/emsdk
cd ~/emsdk && ./emsdk install latest && ./emsdk activate latest
echo 'source ~/emsdk/emsdk_env.sh' >> ~/.bashrc && source ~/.bashrc
```

OpenCV must be built from source at **4.10.0+** — older versions (including the Ubuntu 24.04 apt default) crash on YOLOv8's DFL head due to a shape-inference bug. See `scripts/setup-opencv-native.sh` and `scripts/setup-opencv-wasm.sh`.

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
- **FPS is currently inference-bound** (3-5 FPS), not thread-contention-bound. Improving it further would require model quantization (INT8) or a smaller input resolution — not attempted.
- **No demo media or public deploy yet.** The project is feature/architecture-complete as of Phase 4; a recorded walkthrough and/or hosted demo may be added later if useful for a specific application.
- **Development-mode artifact:** in `next dev`, React Strict Mode double-invokes effects, which briefly spawns a second wasm instance that is immediately disposed. This does not occur in production builds (`next build && next start`).
- **Turbopack dev-mode tooling quirk:** Chrome DevTools' Sources → Threads panel nests the detection worker under Turbopack's internal HMR worker context instead of listing it separately — cosmetic only, does not affect the app.