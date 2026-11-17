# Rencana Tim Pulse-OS: Simulasi Penanganan Emergency IGD

## Tujuan dan batasan

Pulse-OS adalah kernel x86 32-bit bare-metal untuk pembelajaran konsep sistem operasi. Demo IGD adalah simulasi edukasi berbasis event dan workload buatan, bukan sistem medis, bukan alat diagnosis/triase, dan tidak berinteraksi dengan perangkat klinis sungguhan.

## Konsep demo terpadu

Buat satu perintah, misalnya `demo-igd`, yang menjalankan skenario deterministik tiga pasien simulasi. Kategori skenario: kritis, serius, stabil. Jangan memakai kriteria klinis nyata; kategori hanya label uji yang disepakati tim.

Alur demo:

1. CLI memvalidasi command dan menyiapkan skenario.
2. VFS RAMDisk menyimpan record teks pasien.
3. Triase demo memberi kelas prioritas statis.
4. Kernel membuat satu thread layanan per pasien, dengan unit kerja terbatas.
5. Priority queue memilih thread READY.
6. Thread menjalankan unit kerja dan memakai lock untuk memperbarui resource simulasi bersama.
7. Log mencatat pilihan scheduler, perubahan status, progres workload, VFS, hasil lock, dan ringkasan memori.
8. Demo menampilkan hasil akhir dan membiarkan peserta memeriksa record melalui command VFS.

## Basis priority queue hybrid

Gunakan urutan dua tingkat:

1. Kelas triase utama: `0` kritis, `1` serius, `2` stabil; angka lebih kecil berarti lebih tinggi dan tidak boleh dikalahkan skor workload.
2. Dalam kelas sama, gunakan `hybrid_score = time_weight * normalized_remaining_time + resource_weight * normalized_resource_load`. Komponen wajib terukur, normalisasi dan bobot wajib ditetapkan sebelum pengujian.
3. Gunakan FIFO bila kelas dan skor hybrid sama.

Estimasi waktu berarti sisa workload, bukan CPU time yang sudah dipakai. Beban resource menjadi faktor pengurutan hanya dalam kelas triase yang sama. Catat seluruh komponen skor pada event log. Uji baseline kelas triase + FIFO lebih dulu, lalu bandingkan kebijakan hybrid dengan workload sama. Aging menjadi pengembangan lanjutan setelah starvation diuji.

## Anggota dan hasil kerja

- **Pemilik boot + CLI, hak akses, VFS RAMDisk, allocator:** brief terpisah di `BRIEF-PENGERJAAN-AKU.md`.
- **Kiran dan Qina — thread, lifecycle, preemptive priority queue:** `BRIEF-PENGERJAAN-KIRAN-QINA.md`.
- **Fathir — sinkronisasi dan deadlock:** `BRIEF-PENGERJAAN-FATHIR.md`.
- **Ide demo dan kebijakan scheduler:** juga dirinci pada `BRIEF-IDE-DEMO-IGD.md`.

## Kontrak integrasi yang harus disepakati

Sebelum menggabungkan perubahan, tulis kontrak singkat untuk:

- API membuat thread, menjalankan/yield, keluar, block/wake, status, dan pengaturan prioritas.
- API lock, aturan ownership, dan interaksi dengan scheduler.
- Batas ukuran nama/data VFS dan perilaku error.
- API allocator, alignment, free, dan OOM.
- Format event log bersama, agar output demo konsisten dan tidak bergantung pada print dari critical section.
- Batasan concurrency dan area yang tidak aman dipanggil dari interrupt.

Jangan menebak API subsistem lain; gunakan deklarasi header yang benar-benar sudah disepakati. Integrasikan command demo setelah subsistem terkait memiliki pengujian dasar.

## Urutan kerja tim

1. Bekukan skenario, definisi prioritas, contoh input/output, dan kontrak API.
2. Anggota menguji subsystem masing-masing secara terpisah dan mengirim perubahan kecil.
3. Integrasikan task dengan lock melalui block/wake yang benar.
4. Hubungkan VFS dan allocator ke skenario, lalu tambah satu command demo.
5. Uji tiap kriteria brief tugas besar secara terpisah, kemudian jalankan demo end-to-end di QEMU.
6. Rekam log/screenshot aktual, catat keterbatasan, siapkan laporan dan pembagian tugas.

Jaga perubahan lokal tiap anggota. Repo saat ini melacak sejumlah `.o` dan `mini-os.bin`; hindari `make clean` pada working tree bersama tanpa memastikan efeknya dan koordinasi dengan tim.

## Kriteria penerimaan end-to-end

- Kernel boot di QEMU dan CLI menerima input yang didukung.
- Setidaknya tiga command buatan tim berjalan; command demo terpadu dapat diulang.
- Dua atau lebih kernel thread dengan tugas berbeda benar-benar dieksekusi; status hanya dilaporkan jika tercapai.
- Preemption/context switch nyata, bukan simulasi teks.
- Priority queue memilih prioritas tertinggi dan FIFO untuk nilai sama.
- Shared counter menunjukkan risiko race tanpa sinkronisasi pada kondisi yang dapat interleave, dan hasil konsisten dengan lock.
- Deadlock dua lock dapat dijelaskan/didemonstrasikan pada run terisolasi; lock ordering menghilangkan circular wait.
- Allocator membuktikan alokasi, pelepasan, statistik dan OOM aman.
- VFS membuktikan CRUD record teks.
- Hak akses menunjukkan satu operasi yang diizinkan berhasil dan yang tidak diizinkan ditolak; jangan klaim hardware isolation bila hanya role software.
- Log/screenshot QEMU aktual dan laporan memenuhi format tugas.

## Bahan laporan

Dokumentasikan arsitektur dan pembagian kerja, desain thread/scheduler/priority queue, sinkronisasi/race, deadlock dan mitigasi, allocator, VFS, kontrol akses, skenario serta hasil uji aktual, kendala, batasan, instruksi build/run, dan penggunaan AI/kode pihak ketiga. Jangan melaporkan hasil uji yang belum benar-benar dijalankan.
