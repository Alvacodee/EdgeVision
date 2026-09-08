// features/detection/hooks/useDetectionWorker.ts
// Bungkus lifecycle Web Worker (spawn, init, terminate) + kirim frame ke
// worker. Satu concern: komunikasi main thread <-> worker, nggak tau
// apa-apa soal kamera atau capture loop (itu tanggung jawab
// useFrameCapture).

import { useCallback, useEffect, useRef, useState } from 'react';
import type { Detection, EngineStatus } from '../types';
import type { MainToWorkerMessage, WorkerToMainMessage } from '../workers/protocol';

interface UseDetectionWorkerResult {
  status: EngineStatus;
  error: string | null;
  detections: Detection[];
  frameSize: { width: number; height: number } | null;
  // Return false kalau frame ditolak (worker belum ready / masih sibuk
  // ngerjain frame sebelumnya) — pemanggil (useFrameCapture) pakai ini
  // buat backpressure, bukan numpuk frame di mailbox worker.
  sendFrame: (bitmap: ImageBitmap, threshold: number) => boolean;
  isBusy: () => boolean;
}

export function useDetectionWorker(): UseDetectionWorkerResult {
  const workerRef = useRef<Worker | null>(null);
  // Ref, bukan state — dibaca sinkron di rAF loop lewat isBusy(), nggak
  // perlu trigger re-render tiap kali berubah.
  const busyRef = useRef(false);

  const [status, setStatus] = useState<EngineStatus>('loading');
  const [error, setError] = useState<string | null>(null);
  const [detections, setDetections] = useState<Detection[]>([]);
  const [frameSize, setFrameSize] = useState<{ width: number; height: number } | null>(null);

  useEffect(() => {
    // Strict Mode (dev) manggil effect ini 2x — pola yang sama seperti
    // useVisionEngine di Fase 3: tiap invocation punya worker sendiri,
    // `cancelled` nandain worker "hantu" dari mount pertama biar nggak
    // nyetak state React lagi setelah instance-nya sudah di-terminate.
    let cancelled = false;
    const worker = new Worker(new URL('../workers/detection.worker.ts', import.meta.url), {
      type: 'module',
    });

    worker.onmessage = (event: MessageEvent<WorkerToMainMessage>) => {
      if (cancelled) return;
      const msg = event.data;

      switch (msg.type) {
        case 'ready':
          workerRef.current = worker;
          setStatus('ready');
          break;
        case 'error':
          busyRef.current = false;
          setStatus('error');
          setError(msg.message);
          break;
        case 'detections':
          busyRef.current = false;
          setDetections(msg.detections);
          setFrameSize({ width: msg.frameWidth, height: msg.frameHeight });
          break;
      }
    };

    worker.postMessage({ type: 'init' } satisfies MainToWorkerMessage);

    return () => {
      cancelled = true;
      if (workerRef.current === worker) workerRef.current = null;
      // dispose() di sisi worker sempat-sempatnya jalan kalau belum
      // ke-terminate duluan; terminate() tetap dipanggil sebagai jaring
      // pengaman kalau worker macet/nggak merespons dispose.
      worker.postMessage({ type: 'dispose' } satisfies MainToWorkerMessage);
      worker.terminate();
    };
  }, []);

  const sendFrame = useCallback((bitmap: ImageBitmap, threshold: number): boolean => {
    if (!workerRef.current || busyRef.current) {
      // Worker belum siap / masih sibuk — bitmap ini nggak akan dipakai,
      // harus ditutup di sini juga biar nggak leak (G3).
      bitmap.close();
      return false;
    }
    busyRef.current = true;
    workerRef.current.postMessage({ type: 'frame', bitmap, threshold } satisfies MainToWorkerMessage, [
      bitmap,
    ]);
    return true;
  }, []);

  const isBusy = useCallback(() => busyRef.current, []);

  return { status, error, detections, frameSize, sendFrame, isBusy };
}
