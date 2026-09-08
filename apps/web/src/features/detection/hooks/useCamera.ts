// features/detection/hooks/useCamera.ts
// Tanggung jawab tunggal: akses kamera device via getUserMedia, kelola
// stream + status. Tidak tahu apa-apa soal deteksi/canvas/wasm.

import { useCallback, useEffect, useRef, useState } from 'react';
import { MAX_WIDTH, MAX_HEIGHT, type CameraStatus } from '../types';

interface UseCameraResult {
  videoRef: React.RefObject<HTMLVideoElement | null>;
  status: CameraStatus;
  error: string | null;
  start: () => Promise<void>;
  stop: () => void;
}

export function useCamera(): UseCameraResult {
  const videoRef = useRef<HTMLVideoElement | null>(null);
  const streamRef = useRef<MediaStream | null>(null);
  const [status, setStatus] = useState<CameraStatus>('idle');
  const [error, setError] = useState<string | null>(null);

  const stop = useCallback(() => {
    streamRef.current?.getTracks().forEach((track) => track.stop());
    streamRef.current = null;
    if (videoRef.current) videoRef.current.srcObject = null;
    setStatus('idle');
  }, []);

  const start = useCallback(async () => {
    setStatus('requesting');
    setError(null);
    try {
      // Minta resolusi ideal sesuai kapasitas buffer wasm (§10.1) — kamera
      // boleh kasih lebih kecil/besar, useDetectionLoop tetap jaga-jaga
      // dengan scale-down manual di bawah MAX_WIDTH/MAX_HEIGHT.
      const stream = await navigator.mediaDevices.getUserMedia({
        video: {
          facingMode: 'user',
          width: { ideal: MAX_WIDTH },
          height: { ideal: MAX_HEIGHT },
        },
        audio: false,
      });
      streamRef.current = stream;
      if (videoRef.current) {
        videoRef.current.srcObject = stream;
        await videoRef.current.play();
      }
      setStatus('active');
    } catch (err) {
      // NotAllowedError paling umum (user tolak permission kamera).
      const isDenied = err instanceof DOMException && err.name === 'NotAllowedError';
      setStatus(isDenied ? 'denied' : 'error');
      setError(err instanceof Error ? err.message : String(err));
    }
  }, []);

  // Cleanup kalau komponen unmount saat kamera masih nyala.
  useEffect(() => {
    return () => {
      streamRef.current?.getTracks().forEach((track) => track.stop());
    };
  }, []);

  return { videoRef, status, error, start, stop };
}
