// features/detection/workers/detection.worker.ts
// Module worker: satu-satunya tempat wasm module + OffscreenCanvas hidup
// untuk Fase 4. Main thread cuma ngirim ImageBitmap; getImageData() dan
// inference (paling berat) terjadi di sini, di luar main thread.
//
// VisionEngine dipakai APA ADANYA dari lib/wasm — nggak ada kode yang
// diubah di sana. Ini bukti kalau §6.2 AGENT.md ("hot path lewat pointer +
// heap-view, embind cuma buat kontrol") emang portable ke konteks worker
// tanpa perlu refactor apa pun.

/// <reference lib="webworker" />
declare const self: DedicatedWorkerGlobalScope;

import { VisionEngine } from '@/lib/wasm/vision-engine';
import { MAX_WIDTH, MAX_HEIGHT } from '../types';
import type { MainToWorkerMessage, WorkerToMainMessage } from './protocol';

const engine = new VisionEngine();
// OffscreenCanvas dibuat sekali, di-resize per frame kalau dimensinya
// berubah — sama prinsipnya kayak hidden canvas di Fase 3, cuma sekarang
// hidup di worker, bukan main thread.
const canvas = new OffscreenCanvas(MAX_WIDTH, MAX_HEIGHT);
const ctx = canvas.getContext('2d', { willReadFrequently: true });

function post(msg: WorkerToMainMessage) {
  self.postMessage(msg);
}

self.onmessage = async (event: MessageEvent<MainToWorkerMessage>) => {
  const msg = event.data;
  
  switch (msg.type) {
    case 'init': {
      try {
        await engine.init(MAX_WIDTH, MAX_HEIGHT);
        post({ type: 'ready' });
      } catch (err) {
        post({ type: 'error', message: err instanceof Error ? err.message : String(err) });
      }
      break;
    }

    case 'frame': {
      const { bitmap, threshold } = msg;
      try {
        if (!ctx) throw new Error('OffscreenCanvas context gagal dibuat');

        // Bitmap sudah di-resize di main thread (createImageBitmap), jadi
        // di sini tinggal pakai dimensinya langsung — nggak ada scaling
        // ulang, biar kerjaan di worker seminimal mungkin.
        const w = bitmap.width;
        const h = bitmap.height;
        if (canvas.width !== w || canvas.height !== h) {
          canvas.width = w;
          canvas.height = h;
        }

        ctx.drawImage(bitmap, 0, 0, w, h);
        const { data } = ctx.getImageData(0, 0, w, h);

        const detections = engine.detect(data, w, h, threshold);
        post({ type: 'detections', detections, frameWidth: w, frameHeight: h });
      } catch (err) {
        post({ type: 'error', message: err instanceof Error ? err.message : String(err) });
      } finally {
        // ImageBitmap adalah resource GPU/memory yang harus ditutup
        // manual, beda dari objek JS biasa — lupa close() = leak (G3).
        bitmap.close();
      }
      break;
    }

    case 'dispose': {
      engine.dispose();
      break;
    }
  }
};
