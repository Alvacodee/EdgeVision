// features/detection/types.ts
// Tipe & konstanta bersama antar hook/komponen deteksi.
// Detection di re-export dari lib/wasm biar shape datanya satu sumber
// kebenaran (ikut kontrak §10.1 AGENT.md), bukan didefinisikan ulang.

export type { Detection } from '@/lib/wasm/vision-engine';

// Harus sama persis dengan buffer wasm (§10.1: MAX_WIDTH=640, MAX_HEIGHT=480).
export const MAX_WIDTH = 640;
export const MAX_HEIGHT = 480;

// Confidence threshold (§9.2: default 0.5, range 0.1-0.9).
export const DEFAULT_THRESHOLD = 0.5;
export const THRESHOLD_MIN = 0.1;
export const THRESHOLD_MAX = 0.9;

// Cap FPS loop deteksi di main thread. Belum ada di AGENT.md karena Web
// Worker baru masuk Fase 4 — ini cuma jaga-jaga biar UI nggak nge-jank
// kalau device kenceng banget dan detect() kepanggil kelewat sering.
export const TARGET_FPS = 30;

export type EngineStatus = 'idle' | 'loading' | 'ready' | 'error';
export type CameraStatus = 'idle' | 'requesting' | 'active' | 'denied' | 'error';
