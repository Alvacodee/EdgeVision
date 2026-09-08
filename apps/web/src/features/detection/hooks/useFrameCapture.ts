// features/detection/hooks/useFrameCapture.ts
// Loop requestAnimationFrame di main thread: ambil frame dari <video> pakai
// createImageBitmap (native, ringan) — beda dari Fase 3 yang masih pakai
// drawImage()+getImageData() langsung di main thread. Throttle ke
// TARGET_FPS, dan skip capture kalau worker masih sibuk (backpressure)
// biar frame nggak numpuk di mailbox worker pas device lagi lemot.

import { useEffect, useRef, useState } from 'react';
import { MAX_WIDTH, MAX_HEIGHT, TARGET_FPS } from '../types';

interface UseFrameCaptureArgs {
  videoRef: React.RefObject<HTMLVideoElement | null>;
  sendFrame: (bitmap: ImageBitmap, threshold: number) => boolean;
  isBusy: () => boolean;
  threshold: number;
  active: boolean; // true kalau kamera nyala + worker ready
}

interface UseFrameCaptureResult {
  fps: number;
}

const FRAME_INTERVAL_MS = 1000 / TARGET_FPS;

export function useFrameCapture({
  videoRef,
  sendFrame,
  isBusy,
  threshold,
  active,
}: UseFrameCaptureArgs): UseFrameCaptureResult {
  const [fps, setFps] = useState(0);

  // Ref biar callback rAF selalu baca value terbaru tanpa harus restart
  // loop tiap kali threshold/sendFrame/isBusy berubah identitasnya.
  const thresholdRef = useRef(threshold);
  const sendFrameRef = useRef(sendFrame);
  const isBusyRef = useRef(isBusy);

  useEffect(() => {
    thresholdRef.current = threshold;
  }, [threshold]);

  useEffect(() => {
    sendFrameRef.current = sendFrame;
  }, [sendFrame]);

  useEffect(() => {
    isBusyRef.current = isBusy;
  }, [isBusy]);

  useEffect(() => {
    if (!active) return;

    const video = videoRef.current;
    if (!video) return;

    let rafId: number;
    let lastFrameTime = 0;
    // Sliding window sederhana buat FPS: hitung berapa frame terkirim ke
    // worker dalam 1 detik terakhir — ini "effective throughput", bukan
    // cuma seberapa sering rAF menyala.
    let frameTimestamps: number[] = [];

    const loop = (now: number) => {
      rafId = requestAnimationFrame(loop);

      // Worker masih ngerjain frame sebelumnya — jangan tumpuk frame baru.
      if (isBusyRef.current()) return;
      if (now - lastFrameTime < FRAME_INTERVAL_MS) return;
      if (video.readyState < video.HAVE_CURRENT_DATA || video.videoWidth === 0) return;

      lastFrameTime = now;

      // Scale-down proporsional kalau resolusi kamera aktual melebihi
      // buffer wasm (§10.1) — sama logikanya kayak Fase 3, cuma sekarang
      // resize-nya dikerjain langsung sama createImageBitmap (native),
      // bukan drawImage manual ke canvas.
      const videoW = video.videoWidth;
      const videoH = video.videoHeight;
      const scale = Math.min(1, MAX_WIDTH / videoW, MAX_HEIGHT / videoH);
      const w = Math.round(videoW * scale);
      const h = Math.round(videoH * scale);

      createImageBitmap(video, { resizeWidth: w, resizeHeight: h }).then((bitmap) => {
        sendFrameRef.current(bitmap, thresholdRef.current);
      });

      frameTimestamps.push(now);
      frameTimestamps = frameTimestamps.filter((t) => now - t <= 1000);
      setFps(frameTimestamps.length);
    };

    rafId = requestAnimationFrame(loop);
    return () => cancelAnimationFrame(rafId);
  }, [active, videoRef]);

  return { fps: active ? fps : 0 };
}
