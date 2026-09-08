# EdgeVision — Arsitektur Fase 4: Web Worker + OffscreenCanvas

## Kenapa dipindah dari main thread

Di Fase 3, satu frame diproses seluruhnya di main thread:

```
main thread: drawImage(video) → getImageData() → HEAPU8.set() → vc_detect() → render
```

`getImageData()` dan `vc_detect()` (forward pass DNN) adalah dua operasi paling
mahal di pipeline ini, dan keduanya bersaing rebutan waktu CPU dengan hal-hal
yang React/browser juga perlu kerjakan di main thread yang sama: re-render
komponen, layout, paint, dan respons terhadap input (klik slider, dsb).
Hasilnya: FPS rendah (~4-6 di pengujian Fase 3) dan UI terasa berat meski
kamera "cuma" jalan di background.

Fase 4 memindahkan **semua kerja berat** (ekstraksi pixel + inference) ke
Web Worker, supaya main thread cuma jadi "kurir": ambil frame ringkas, kirim,
render hasil balik.

## Arsitektur baru

```
Main thread                                Web Worker (module worker)
────────────                               ──────────────────────────
<video> (kamera)
   │
   ▼
createImageBitmap(video, {resizeWidth, resizeHeight})   ← native, ringan,
   │                                                       sering hardware-
   │  postMessage({type:'frame', bitmap}, [bitmap])        accelerated
   │  (Transferable — zero-copy, bukan clone)
   └──────────────────────────────────────────►  onmessage
                                                      │
                                                      ▼
                                              OffscreenCanvas.drawImage(bitmap)
                                                      │
                                                      ▼
                                              getImageData()  ← sekarang di
                                                      │          sini, bukan
                                                      ▼          main thread
                                              Module.HEAPU8.set(...)
                                                      │
                                                      ▼
                                              Module._vc_detect(...)
                                                      │  (C++: preprocess,
                                                      │   dnn forward,
                                                      │   NMS, postprocess)
                                                      ▼
                                              baca Module.HEAPF32
                                                      │
   ◄──────────────────────────────────────────  postMessage({type:'detections', ...})
   │
   ▼
setDetections() → render bounding box di canvas overlay
```

`VisionEngine` (dari `lib/wasm/vision-engine.ts`) dipakai **tanpa modifikasi**
di dalam worker — kelas ini dari awal didesain cuma bergantung ke `Module`
(heap view + fungsi embind), bukan ke API spesifik main thread/DOM, jadi
portable ke context worker begitu saja. Ini validasi langsung dari prinsip
desain di AGENT.md §6.2.

## Backpressure: kenapa perlu, dan bagaimana caranya

Kalau main thread ngirim frame secepat `requestAnimationFrame` (berpotensi
~60x/detik) tapi worker cuma sanggup proses ~5-10 frame/detik (dibatasi
kecepatan inference DNN), pesan bakal numpuk di message queue worker.
Akibatnya: detection yang dirender jadi **stale** — bounding box ketinggalan
beberapa ratus milidetik dari posisi objek aktual, makin lama makin telat.

Solusinya: `useDetectionWorker` melacak status `busy` (true begitu frame
dikirim, false lagi begitu balasan `detections`/`error` diterima).
`useFrameCapture` cek status ini **sebelum** memanggil `createImageBitmap` —
kalau worker masih sibuk, frame saat ini di-skip sepenuhnya, bukan
di-queue. Efeknya: FPS yang dilaporkan UI adalah *throughput asli* (berapa
frame yang benar-benar diproses worker), bukan cuma seberapa sering rAF
menyala.

## Manajemen memori — `ImageBitmap.close()`

`ImageBitmap` memegang resource GPU/memory yang **tidak** dikumpulkan
otomatis oleh garbage collector JS seperti objek biasa — harus ditutup
manual lewat `.close()`. ini dipanggil di tiga tempat:
- Di worker, di blok `finally` setelah `drawImage()` (baik sukses maupun
  gagal) — kalau cuma dipanggil di jalur sukses, frame yang gagal diproses
  (misal exception di `engine.detect()`) akan numpuk bitmap yang nggak
  pernah dilepas.
- Di `useDetectionWorker.sendFrame()`, kalau ternyata worker belum ready
  atau masih busy — bitmap yang gagal terkirim juga harus ditutup di sisi
  pengirim, bukan cuma di penerima.

## Trade-off yang disadari

| Aspek | Main thread (Fase 3) | Worker + OffscreenCanvas (Fase 4) |
|---|---|---|
| UI responsiveness | Bisa nge-jank kalau inference lambat | Main thread nyaris selalu responsif |
| Latensi per-frame | Lebih rendah (nggak ada overhead postMessage) | Sedikit lebih tinggi (serialize/transfer pesan) |
| Kompleksitas kode | Lebih sederhana, satu alur linear | Perlu protokol pesan eksplisit, 2 hook + 1 worker file |
| Kompatibilitas browser | Lebih luas (nggak perlu module worker) | Perlu dukungan `OffscreenCanvas` + module worker (aman di Chrome/Edge, sesuai target §4 AGENT.md) |

`postMessage` dengan `Transferable` (bukan structured clone biasa) dipilih
justru supaya biaya transfer bitmap antar-thread mendekati nol — inilah
yang membuat trade-off latensi di atas kecil dibanding potensi speedup dari
tidak nge-block main thread.

## Hasil benchmark

> Isi tabel ini setelah menjalankan checklist manual di `FASE4-NOTES.md` di
> device kamu sendiri — saya nggak punya akses kamera/hardware untuk
> ngukur ini dari sandbox.

| Metrik | Fase 3 (main thread) | Fase 4 (worker) |
|---|---|---|
| FPS rata-rata | 4-6 (hasil pengujian awal) | *(isi di sini)* |
| Heap growth setelah 5 menit | *(belum diuji)* | *(isi di sini)* |
| Device/browser yang diuji | *(isi di sini)* | *(isi di sini)* |

## Known limitation (bukan blocker Fase 4)

- Kuantisasi model (INT8) dan pengurangan resolusi input belum disentuh —
  itu opsi lanjutan kalau FPS di Fase 4 masih di bawah target G1 (≥15 FPS)
  meski sudah lepas dari main thread. Root cause paling mungkin: waktu
  inference DNN itu sendiri, yang OffscreenCanvas nggak bisa percepat
  (itu hanya menghilangkan biaya `getImageData` dari main thread, bukan
  mempercepat `cv::dnn::forward`).
