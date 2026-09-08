// components/vision/DetectionOverlay.tsx
// Render bounding box + label + confidence score di canvas yang ditumpuk
// pas di atas <video>. Ukuran drawing buffer = ukuran frame yang diproses
// C++ (bukan ukuran tampilan di layar) — browser yang nge-stretch canvas
// ini via CSS supaya sejajar sama video, jadi koordinat bbox dari wasm
// (§10.1: pixel relatif ke frame asli) langsung valid tanpa perlu konversi.

import { useEffect, useRef } from 'react';
import { COCO_CLASSES } from '@/features/detection/coco-classes';
import type { Detection } from '@/features/detection/types';

interface DetectionOverlayProps {
  detections: Detection[];
  width: number;
  height: number;
}

const BOX_COLOR = '#22d3ee';
const LABEL_TEXT_COLOR = '#0a0a0a';

export function DetectionOverlay({ detections, width, height }: DetectionOverlayProps) {
  const canvasRef = useRef<HTMLCanvasElement>(null);

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    if (!ctx) return;

    ctx.clearRect(0, 0, width, height);
    ctx.lineWidth = 2;
    ctx.strokeStyle = BOX_COLOR;
    ctx.font = '14px sans-serif';
    ctx.textBaseline = 'top';

    for (const d of detections) {
      ctx.strokeRect(d.x, d.y, d.w, d.h);

      const label = `${COCO_CLASSES[d.classId] ?? d.classId} ${(d.score * 100).toFixed(0)}%`;
      const textWidth = ctx.measureText(label).width;
      const labelY = Math.max(0, d.y - 18);

      ctx.fillStyle = BOX_COLOR;
      ctx.fillRect(d.x, labelY, textWidth + 8, 18);

      ctx.fillStyle = LABEL_TEXT_COLOR;
      ctx.fillText(label, d.x + 4, labelY + 2);
    }
  }, [detections, width, height]);

  return (
    <canvas
      ref={canvasRef}
      width={width}
      height={height}
      className="absolute inset-0 w-full h-full pointer-events-none"
    />
  );
}
