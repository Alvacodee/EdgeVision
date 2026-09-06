'use client';

import { useEffect } from 'react';
import { VisionEngine } from '@/lib/wasm/vision-engine';

async function testEngine() {
  const engine = new VisionEngine();
  await engine.init(640, 480);

  const img = await createImageBitmap(await (await fetch('/test-bus.jpg')).blob());
  const scale = Math.min(640 / img.width, 480 / img.height);
  const w = Math.round(img.width * scale);
  const h = Math.round(img.height * scale);

  const canvas = new OffscreenCanvas(w, h);
  const ctx = canvas.getContext('2d')!;
  ctx.drawImage(img, 0, 0, w, h);
  const { data } = ctx.getImageData(0, 0, w, h);

  const detections = engine.detect(data, w, h, 0.5);
  console.log(`Ketemu ${detections.length} deteksi (frame ${w}x${h}):`);
  detections.forEach(d =>
    console.log(`  class=${d.classId} score=${d.score.toFixed(2)} bbox=[${d.x.toFixed(0)},${d.y.toFixed(0)},${d.w.toFixed(0)},${d.h.toFixed(0)}]`)
  );

  engine.dispose();
}

export default function Page() {
  useEffect(() => {
    testEngine();
  }, []);

  return <div>Cek console untuk hasil smoke test.</div>;
}