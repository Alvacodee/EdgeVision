// components/vision/Controls.tsx
// UI kontrol: slider threshold, tombol start/stop kamera, FPS counter.
// Murni presentational — semua state dikelola parent (page.tsx), file ini
// nggak nyimpen state sendiri.

import { THRESHOLD_MIN, THRESHOLD_MAX } from '@/features/detection/types';
import type { CameraStatus, EngineStatus } from '@/features/detection/types';

interface ControlsProps {
  cameraStatus: CameraStatus;
  engineStatus: EngineStatus;
  threshold: number;
  fps: number;
  onThresholdChange: (value: number) => void;
  onStart: () => void;
  onStop: () => void;
}

export function Controls({
  cameraStatus,
  engineStatus,
  threshold,
  fps,
  onThresholdChange,
  onStart,
  onStop,
}: ControlsProps) {
  const isActive = cameraStatus === 'active';
  const canStart = engineStatus === 'ready' && !isActive;

  return (
    <div className="w-full max-w-2xl flex flex-col gap-3 mt-4">
      <div className="flex items-center gap-4">
        <button
          onClick={isActive ? onStop : onStart}
          disabled={!isActive && !canStart}
          className="px-4 py-2 rounded-md bg-cyan-600 text-white text-sm font-medium disabled:opacity-40 disabled:cursor-not-allowed hover:bg-cyan-500 transition-colors"
        >
          {isActive ? 'Stop kamera' : 'Mulai kamera'}
        </button>

        <span className="text-sm text-neutral-500">
          {engineStatus === 'loading' && 'Memuat model...'}
          {engineStatus === 'error' && 'Gagal load model'}
          {cameraStatus === 'denied' && 'Izin kamera ditolak'}
        </span>

        <span className="ml-auto text-sm font-mono tabular-nums">
          {isActive ? `${fps} FPS` : '-- FPS'}
        </span>
      </div>

      <label className="flex items-center gap-3 text-sm">
        <span className="w-32 shrink-0">Threshold: {threshold.toFixed(2)}</span>
        <input
          type="range"
          min={THRESHOLD_MIN}
          max={THRESHOLD_MAX}
          step={0.05}
          value={threshold}
          onChange={(e) => onThresholdChange(Number(e.target.value))}
          className="flex-1"
        />
      </label>
    </div>
  );
}
