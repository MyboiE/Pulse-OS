# Brief Ide dan Demo Pulse-OS: Simulasi IGD

## Tujuan

Pulse-OS adalah kernel x86 32-bit bare-metal untuk mendemonstrasikan konsep OS melalui simulasi alur triase IGD. Ini simulasi edukasi, bukan alat klinis dan bukan rekomendasi medis. Prioritas pasien ditentukan oleh skenario demo yang sudah ditetapkan, bukan diagnosis nyata.

## Ide demo terpadu

Sediakan satu perintah CLI, misalnya `demo-igd`, yang menjalankan skenario deterministik berisi tiga pasien simulasi:

- Pasien A: kritis, perlu tindakan cepat.
- Pasien B: serius, sumber daya sedang.
- Pasien C: stabil, tindakan lebih ringan.

Perintah menghasilkan event log yang menunjukkan: pasien masuk (VFS), data pasien dibaca/diubah (CRUD), triase memberi prioritas, thread layanan dibuat, scheduler memilih thread, pekerjaan memakai shared resource yang dilindungi lock, lalu status thread dan penggunaan memori ditampilkan. Hindari timer/alat medis sungguhan; event, beban kerja dan hasil harus simulasi.

Alur yang diharapkan:

1. CLI validasi perintah dan input skenario.
2. Setiap pasien disimpan sebagai record teks di RAMDisk, misalnya `patient-A.txt`.
3. Triase menetapkan kelas `priority` sesuai kategori skenario; data estimasi sisa waktu kerja dan kebutuhan resource terukur juga disiapkan untuk tiap task.
4. Kernel membuat satu thread layanan per pasien.
5. Priority queue mengurutkan kelas triase terlebih dahulu. Dalam kelas sama, gunakan skor hybrid waktu tersisa + beban resource, lalu FIFO jika masih seri.
6. Thread melakukan beberapa unit kerja terhingga; setiap yield/tick scheduler mencatat pilihan thread, status, sisa unit kerja, komponen skor, dan alasan dispatch.
7. Lock melindungi counter ruang/tindakan bersama; log menunjukkan akses terserialisasi. Demo deadlock terpisah dan terkendali.
8. Tampilkan urutan event, hasil akhir, status thread, VFS, dan ringkasan memori.

## Kebijakan priority queue hybrid

Gunakan kebijakan dua tingkat agar urgensi triase tidak tertimpa skor workload:

1. **Kelas triase sebagai kunci utama**: `0` kritis, `1` serius, `2` stabil; angka lebih kecil menang. Kelas ini tidak boleh dikalahkan oleh estimasi durasi atau kebutuhan resource.
2. **Skor hybrid di dalam kelas yang sama**: gabungkan estimasi sisa waktu eksekusi dan beban resource terukur. Contoh bentuk skor: `hybrid_score = time_weight * normalized_remaining_time + resource_weight * normalized_resource_load`. Tetapkan dan dokumentasikan normalisasi, rentang nilai, bobot, dan arah urutan sebelum uji. Jika skor lebih kecil dipilih dulu, jelaskan konsekuensinya.
3. **FIFO sebagai tie-break terakhir** bila kelas dan skor hybrid sama.

Estimasi waktu harus berupa sisa workload yang diperkirakan, bukan waktu CPU yang sudah terpakai. Resource harus angka yang dapat diamati dalam simulasi, misalnya jumlah slot simulasi yang dibutuhkan; jangan gunakan penilaian subjektif. Simpan seluruh komponen skor di event log agar urutan dispatch dapat dijelaskan.

Jangan biarkan skor hybrid mengurutkan task dari kelas triase berbeda. Ini mempertahankan prioritas pasien kritis di atas stabil. Kebijakan ini mengukur prioritas untuk demo edukasi; bukan triase klinis.

Preemption terjadi saat timer tick atau thread yield/block/exit memicu scheduler. Bila thread READY memiliki kelas triase lebih tinggi dari thread yang berjalan, scheduler harus memilihnya dan melakukan context switch. Untuk thread satu kelas, gunakan skor hybrid yang telah dihitung. Jangan mengklaim preemptive scheduling otomatis bila timer interrupt belum memicu reschedule.

Implementasikan bertahap: buktikan dahulu kelas triase + FIFO, lalu aktifkan skor hybrid intra-kelas dan bandingkan urutan dengan workload yang sama. Uji starvation; pertimbangkan aging hanya setelah hasil baseline dan hybrid tercatat.

## Kriteria demo lulus

- Satu perintah menjalankan keseluruhan skenario tanpa input tersembunyi.
- Log memperlihatkan pasien masuk, prioritas, thread, dispatch scheduler, lock, penyimpanan VFS, dan ringkasan memori.
- Thread prioritas lebih tinggi dipilih sebelum thread prioritas lebih rendah ketika keduanya READY.
- Thread prioritas sama dipilih FIFO.
- Tunjukkan dua kernel thread aktif dengan tugas berbeda dan perubahan status yang memang didukung implementasi. Jangan mengklaim status yang tidak benar-benar dicapai.
- Hasil VFS dapat diverifikasi dengan `ls`, `cat`, dan pengubahan/penghapusan record uji.
- Uji sinkronisasi membandingkan shared counter tanpa lock dan dengan lock; catat hasil aktual. Race tanpa lock harus diuji dengan operasi yang benar-benar dapat interleave, bukan hanya dua pemanggilan berurutan.
- Deadlock ditampilkan sebagai uji terisolasi yang bisa dipulihkan/reset; jangan membuat kernel demo utama macet permanen.
- `meminfo` menunjukkan penggunaan sebelum, selama, sesudah, termasuk pembebasan. Uji OOM harus gagal aman.
- Jalankan di QEMU, simpan log/screenshot, dan dokumentasikan batasan.

## Urutan integrasi

1. Semua anggota sepakati antarmuka scheduler, task, lock, VFS, allocator, dan event log sebelum integrasi.
2. Masing-masing buat/test subsistem terpisah dengan perubahan kecil dan kontrak header stabil.
3. Integrasikan `demo-igd` setelah subsistem inti lulus uji sendiri.
4. Jalankan build bersih hanya pada salinan/branch aman; repo saat ini melacak artefak build, jadi hindari `make clean` di working tree bersama tanpa koordinasi.
5. Jalankan skenario berulang dan pastikan urutan deterministik; catat hasil sebenarnya.

## Status audit dan rencana aktual

Command `demo-igd` tersedia dan dijalankan di QEMU sebelum heap scheduler diganti. Log lama `/tmp/pulse-demo-igd.log` bukan verifikasi versi heap baru. Demo saat ini baseline edukasi; belum membuktikan timer preemption atau concurrency paralel.

Yang sudah tersedia sebagai fondasi kode:

- VFS RAMDisk, operasi file, namespace `visitor-*`, pemeriksaan role, dan test harness `vfstest`.
- Allocator dan test harness `memtest`; `meminfo` menampilkan statistik.
- Selector scheduler kini memakai binary min-heap eksplisit dengan triase → skor hybrid → FIFO; heap dibangun ulang dari `thread_pool` setiap pemilihan, belum persistent ready queue. Timer IRQ belum terbukti memanggil `scheduler_tick()`.
- Origin `862f765` menambahkan mutex/semaphore wait queue, ownership check, `thread_block()`/`thread_wake()`, API shared room IGD, serta command `synctest`, `deadlocktest`, `deadlocksafe`. `demo-igd` memakai mutex shared room; run QEMU selesai, tetapi handoff/block-wake dan fail-safe alokasi thread belum diuji.
- Build `make -B` kini berhasil tanpa warning; target mencakup `kernel/src/sync.c`, lalu link ke `mini-os.bin`.
- Run QEMU terarsip `/tmp/pulse-subsystem-tests.log`: `protecttest`, `vfstest`, `memtest`, `synctest`, `deadlocksafe`, `shutdown`; 29 `[PASS]`, 0 `[FAIL]`. Counter synctest tanpa lock `50` (expected `100`), dengan mutex `100`; ini demonstrasi cooperative interleaving, bukan race CPU paralel.
- `protecttest` membuktikan visitor ditolak untuk operasi emergency di test harness; perintah CRUD CLI umum tetap perlu pemeriksaan guard agar tidak menganggap penolakan harness sebagai perlindungan semua jalur.
- Overflow input CLI diperbaiki untuk menolak command; belum diuji dengan baris melebihi kapasitas lewat serial.
- `demo-igd` rerun dengan heap scheduler terbaru; log `/tmp/pulse-demo-igd-latest.log` membuktikan pembacaan VFS record, dispatch aktual `KRITIS > SERIOUS > STABIL`, tiga pemrosesan shared room, transisi READY → RUNNING → TERMINATED, dan cleanup fixture. Tidak menguji tie-break intra-kelas.
- CRUD umum CLI (`create/rm/cat/write`) belum semuanya dibatasi role emergency; verifikasi jalur RBAC masih wajib sebelum menganggap akses demo aman.
- Log demo belum menguji visitor denial dalam command demo, fail-safe jika thread allocation gagal, atau counter lock final yang eksplisit.

Checklist penerimaan demo:

- [x] Command `demo-igd` tersedia; run QEMU lama mencakup fixture, shared room, cleanup.
- [x] Rerun demo QEMU dengan binary scheduler heap terbaru; log aktual `/tmp/pulse-demo-igd-latest.log`.
- [x] Dispatch aktual tiga thread mengurut KRITIS → SERIOUS → STABIL; log menunjukkan triage, hybrid, FIFO, READY → RUNNING → TERMINATED serta shared-room lock. Ini membuktikan kasus tiga kelas triase tersebut, bukan tie-break FIFO/hybrid intra-kelas.
- [ ] Uji deterministik hybrid intra-kelas dan FIFO tie-break.
- [ ] QEMU shutdown command tidak menghentikan emulator; run dihentikan oleh timeout setelah hasil demo tervalidasi.

- [ ] Lifecycle penuh, block/wake dan preemption masih belum dibuktikan.
- [ ] Verifikasi visitor denial end-to-end pada jalur CLI CRUD.
- [ ] Verifikasi lock waiter/handoff/no-lost-wakeup dan counter aktual.
- [ ] Verifikasi input overflow, fail-safe alokasi thread, dan ulang demo bersih.
- [ ] Timer preemption hanya jika IRQ/PIT menghubungkan `scheduler_tick()`; saat ini belum terbukti.

### Tahapan berikutnya untuk demo layak presentasi

1. Uji akses Visitor terhadap emergency lewat CLI dan pastikan command CRUD tidak memberi akses silang.
2. Uji `thread_block()`/`thread_wake()`, ownership mutex, semaphore permit, serta kegagalan alokasi task.
3. Jalankan `deadlocktest` di QEMU terisolasi dengan timeout.
4. Uji command overflow lewat serial dan pastikan baris panjang ditolak utuh.
5. Ulangi `demo-igd` beberapa kali, bandingkan urutan dan pastikan fixture bersih tiap run.
6. Bila demo menyatakan preemption, integrasikan scheduler ke timer IRQ dan buktikan di log; jika tidak, jelaskan batas scheduler kooperatif.

### Klaim tertunda

Demo terpadu kini berjalan dan log tersedia, tetapi belum lengkap untuk acceptance final. Belum terbukti timer preemption, semaphore handoff, deadlock scenario terisolasi, CLI visitor denial end-to-end, overflow input, dan fail-safe alokasi. Shared counter synctest menunjukkan interleaving kooperatif, bukan race CPU paralel.
Yang sudah tersedia sebagai fondasi kode:

- VFS RAMDisk, operasi file, namespace `visitor-*`, pemeriksaan role, dan test harness `vfstest`.
- Allocator dan test harness `memtest`; `meminfo` menampilkan statistik.
- Selector scheduler kini memakai binary min-heap eksplisit dengan triase → skor hybrid → FIFO; heap dibangun ulang dari `thread_pool` setiap pemilihan, belum persistent ready queue. Timer IRQ belum terbukti memanggil `scheduler_tick()`.
- Origin `862f765` menambahkan mutex/semaphore wait queue, ownership check, `thread_block()`/`thread_wake()`, API shared room IGD, serta command `synctest`, `deadlocktest`, `deadlocksafe`. `demo-igd` memakai mutex shared room; run QEMU selesai, tetapi handoff/block-wake dan fail-safe alokasi thread belum diuji.
- Build `make -B` kini berhasil tanpa warning; target mencakup `kernel/src/sync.c`, lalu link ke `mini-os.bin`.
- Run QEMU terarsip `/tmp/pulse-subsystem-tests.log`: `protecttest`, `vfstest`, `memtest`, `synctest`, `deadlocksafe`, `shutdown`; 29 `[PASS]`, 0 `[FAIL]`. Counter synctest tanpa lock `50` (expected `100`), dengan mutex `100`; ini demonstrasi cooperative interleaving, bukan race CPU paralel.
- `protecttest` membuktikan visitor ditolak untuk operasi emergency di test harness; perintah CRUD CLI umum tetap perlu pemeriksaan guard agar tidak menganggap penolakan harness sebagai perlindungan semua jalur.
- Overflow input CLI diperbaiki untuk menolak command; belum diuji dengan baris melebihi kapasitas lewat serial.
- `demo-igd` rerun dengan heap scheduler terbaru; log `/tmp/pulse-demo-igd-latest.log` membuktikan pembacaan VFS record, dispatch aktual `KRITIS > SERIOUS > STABIL`, tiga pemrosesan shared room, transisi READY → RUNNING → TERMINATED, dan cleanup fixture. Tidak menguji tie-break intra-kelas.
- CRUD umum CLI (`create/rm/cat/write`) belum semuanya dibatasi role emergency; verifikasi jalur RBAC masih wajib sebelum menganggap akses demo aman.
- Log demo belum menguji visitor denial dalam command demo, fail-safe jika thread allocation gagal, atau counter lock final yang eksplisit.

### Tahapan berikutnya untuk demo layak presentasi

1. Uji akses Visitor terhadap emergency lewat CLI dan pastikan command CRUD tidak memberi akses silang.
2. Uji `thread_block()`/`thread_wake()`, ownership mutex, semaphore permit, serta kegagalan alokasi task.
3. Jalankan `deadlocktest` di QEMU terisolasi dengan timeout.
4. Uji command overflow lewat serial dan pastikan baris panjang ditolak utuh.
5. Ulangi `demo-igd` beberapa kali, bandingkan urutan dan pastikan fixture bersih tiap run.
6. Bila demo menyatakan preemption, integrasikan scheduler ke timer IRQ dan buktikan di log; jika tidak, jelaskan batas scheduler kooperatif.

### Klaim tertunda

Demo terpadu kini berjalan dan log tersedia, tetapi belum lengkap untuk acceptance final. Belum terbukti timer preemption, semaphore handoff, deadlock scenario terisolasi, CLI visitor denial end-to-end, overflow input, dan fail-safe alokasi. Shared counter synctest menunjukkan interleaving kooperatif, bukan race CPU paralel.

## Batasan dan pelaporan

Sebut hasil sebagai simulasi kernel, bukan hardware IGD atau sistem medis. Jelaskan bahwa role-based access control bukan isolasi ring-3/memori hardware. Lampirkan skenario, log aktual, hasil uji, kegagalan yang diketahui, serta keterbatasan.

Jangan klaim preemptive timer, priority queue, race/deadlock, atau mutex scheduler-aware sebelum implementasi dan bukti uji mendukungnya.

