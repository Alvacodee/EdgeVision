// components/vision/CameraView.tsx
// Kontainer visual: <video> kamera + <canvas> overlay ditumpuk pakai
// absolute positioning. Nggak ada logic deteksi di sini, cuma plumbing UI.

import type { RefObject } from 'react';
import type { Detection } from '@/features/detection/types';
import { DetectionOverlay } from './DetectionOverlay';

interface CameraViewProps {
  videoRef: RefObject<HTMLVideoElement | null>;
  detections: Detection[];
  frameSize: { width: number; height: number } | null;
}

export function CameraView({ videoRef, detections, frameSize }: CameraViewProps) {
  return (
    <div className="relative w-full max-w-2xl overflow-hidden rounded-lg bg-black">
      <video ref={videoRef} className="w-full h-auto block" playsInline muted />
      {frameSize && (
        <DetectionOverlay
          detections={detections}
          width={frameSize.width}
          height={frameSize.height}
        />
      )}
    </div>
  );
}
