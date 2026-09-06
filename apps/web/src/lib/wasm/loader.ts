// Tanggung jawab tunggal: load module factory dari hasil build wasm.
// Dipisah dari vision-engine.ts biar gampang diganti kalau strategi
// loading berubah, tanpa nyentuh logic buffer/deteksi.

export interface VisionCoreModule {
  vc_init(maxWidth: number, maxHeight: number): number;
  vc_detect(inputPtr: number, w: number, h: number, outputPtr: number, threshold: number): number;
  vc_dispose(): void;
  _malloc(size: number): number;
  _free(ptr: number): void;
  HEAPU8: Uint8Array;
  HEAPF32: Float32Array;
}

type VisionCoreFactory = (opts?: Record<string, unknown>) => Promise<VisionCoreModule>;

let factoryPromise: Promise<VisionCoreFactory> | null = null;

// webpackIgnore: file ini disajikan statis dari public/wasm, bukan lewat
// resolver Next.js. Biar browser yang fetch langsung saat runtime.
export function loadVisionCoreFactory(): Promise<VisionCoreFactory> {
  if (!factoryPromise) {
    factoryPromise = import(/* webpackIgnore: true */ '/wasm/vision-core.js').then(
      (m) => m.default as VisionCoreFactory
    );
  }
  return factoryPromise;
}