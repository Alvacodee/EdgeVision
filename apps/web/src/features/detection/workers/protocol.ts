// features/detection/workers/protocol.ts
import type { Detection } from '@/lib/wasm/vision-engine';

export type MainToWorkerMessage =
  | { type: 'init' }
  | { type: 'frame'; bitmap: ImageBitmap; threshold: number }
  | { type: 'dispose' };

export type WorkerToMainMessage =
  | { type: 'ready' }
  | { type: 'error'; message: string }
  | { type: 'detections'; detections: Detection[]; frameWidth: number; frameHeight: number };
