import { loadVisionCoreFactory, type VisionCoreModule } from './loader';

const MAX_DETECTIONS = 20;
const DETECTION_FLOATS = 6; // x, y, w, h, score, classId

export interface Detection {
  x: number; y: number; w: number; h: number;
  score: number; classId: number;
}

export class VisionEngine {
  private mod: VisionCoreModule | null = null;
  private inputPtr = 0;
  private outputPtr = 0;
  private maxWidth = 0;
  private maxHeight = 0;

  async init(maxWidth: number, maxHeight: number): Promise<void> {
    const createModule = await loadVisionCoreFactory();
    this.mod = await createModule({
        locateFile: (path: string) => `/wasm/${path}`,
    });
    this.maxWidth = maxWidth;
    this.maxHeight = maxHeight;
    this.inputPtr = this.mod.vc_init(maxWidth, maxHeight);
    this.outputPtr = this.mod._malloc(MAX_DETECTIONS * DETECTION_FLOATS * 4);
  }

  detect(frame: Uint8ClampedArray, w: number, h: number, threshold: number): Detection[] {
    if (!this.mod) throw new Error('VisionEngine belum di-init()');
    if (w > this.maxWidth || h > this.maxHeight) {
      throw new Error(`Frame ${w}x${h} melebihi kapasitas buffer ${this.maxWidth}x${this.maxHeight}`);
    }

    this.mod.HEAPU8.set(frame, this.inputPtr);
    const n = this.mod.vc_detect(this.inputPtr, w, h, this.outputPtr, threshold);

    // slice() penting: heap bisa tumbuh (ALLOW_MEMORY_GROWTH) di frame
    // berikutnya, buffer lama jadi detached — copy dulu sebelum itu terjadi.
    const raw = new Float32Array(this.mod.HEAPF32.buffer, this.outputPtr, n * DETECTION_FLOATS).slice();

    const detections: Detection[] = [];
    for (let i = 0; i < n; i++) {
      const off = i * DETECTION_FLOATS;
      detections.push({
        x: raw[off], y: raw[off + 1], w: raw[off + 2], h: raw[off + 3],
        score: raw[off + 4], classId: raw[off + 5],
      });
    }
    return detections;
  }

  dispose(): void {
    if (!this.mod) return;
    this.mod._free(this.outputPtr);
    this.mod.vc_dispose();
    this.mod = null;
  }
}