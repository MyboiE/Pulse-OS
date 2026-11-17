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

## Batasan dan pelaporan

Sebut hasil sebagai simulasi kernel, bukan hardware IGD atau sistem medis. Jelaskan bahwa role-based access control bukan isolasi ring-3/memori hardware. Lampirkan skenario, log aktual, hasil uji, kegagalan yang diketahui, serta keterbatasan.
