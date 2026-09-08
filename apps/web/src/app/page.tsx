'use client';

import { useState } from 'react';
import { useDetectionWorker } from '@/features/detection/hooks/useDetectionWorker';
import { useCamera } from '@/features/detection/hooks/useCamera';
import { useFrameCapture } from '@/features/detection/hooks/useFrameCapture';
import { CameraView } from '@/components/vision/CameraView';
import { Controls } from '@/components/vision/Controls';
import { DEFAULT_THRESHOLD } from '@/features/detection/types';

export default function Page() {
  const {
    status: engineStatus,
    error: engineError,
    detections,
    frameSize,
    sendFrame,
    isBusy,
  } = useDetectionWorker();
  const { videoRef, status: cameraStatus, start, stop } = useCamera();
  const [threshold, setThreshold] = useState(DEFAULT_THRESHOLD);

  const active = cameraStatus === 'active' && engineStatus === 'ready';

  const { fps } = useFrameCapture({
    videoRef,
    sendFrame,
    isBusy,
    threshold,
    active,
  });

  return (
    <div className="flex flex-col items-center gap-4 p-6">
      <h1 className="text-xl font-semibold">EdgeVision</h1>

      <CameraView
        videoRef={videoRef}
        detections={active ? detections : []}
        frameSize={active ? frameSize : null}
      />

      <Controls
        cameraStatus={cameraStatus}
        engineStatus={engineStatus}
        threshold={threshold}
        fps={fps}
        onThresholdChange={setThreshold}
        onStart={start}
        onStop={stop}
      />

      {engineError && <p className="text-sm text-red-500">{engineError}</p>}
    </div>
  );
}
