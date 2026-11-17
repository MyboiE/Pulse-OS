# Brief Pengerjaan: Thread dan Priority Queue Scheduler

## Tanggung jawab

Kiran dan Qina mengembangkan pengelolaan thread kernel dan preemptive scheduler berbasis priority queue untuk mengatur unit kerja layanan pasien pada simulasi IGD. Keduanya perlu berbagi kontrak API dan pengujian; pembagian internal tugas ditetapkan bersama.

## Kebijakan scheduler hybrid

Gunakan urutan dua tingkat:

1. Kelas triase utama: 0 kritis, 1 serius, 2 stabil; angka lebih kecil berarti lebih tinggi dan tidak boleh dikalahkan oleh skor workload.
2. Di dalam kelas yang sama, urutkan dengan skor hybrid dari estimasi sisa waktu eksekusi dan beban resource terukur, misalnya `time_weight * normalized_remaining_time + resource_weight * normalized_resource_load`. Tim harus menetapkan normalisasi, bobot, rentang dan arah urutan sebelum uji; jangan menebak nilai.
3. FIFO saat kelas dan skor hybrid sama.

Estimasi waktu mengacu pada sisa workload, bukan CPU time yang sudah dipakai. Resource harus terukur dalam skenario simulasi. Catat komponen skor pada event log. Baseline kelas triase + FIFO harus diuji dulu, lalu bandingkan hybrid dengan workload yang sama. Aging hanya setelah baseline/hybrid terbukti dan starvation diuji.

## Tahapan

1. **Kontrak task**: sepakati struktur TCB, status yang didukung, API create/yield/exit/block/wake, kepemilikan stack, ukuran batas, dan mekanisme event/log dengan pemilik CLI.
2. **Lifecycle thread**: pastikan transisi status valid dan dapat diamati. Minimal siapkan dua thread dengan tugas berbeda. Jangan menampilkan status palsu hanya demi memenuhi checklist.
3. **Priority queue**: implementasikan enqueue/dequeue/update sesuai prioritas; pertahankan FIFO pada nilai sama. Uji urutan prioritas campuran dan tie-break.
4. **Preemption/context switch**: tunjukkan pergantian eksekusi nyata antara thread, bukan sekadar mencetak nama task. Dokumentasikan sumber tick/preemption dan keterbatasan implementasi.
5. **Workload demo**: sediakan task simulasi pasien dengan jumlah unit kerja kecil dan deterministik. Tiap langkah mencatat thread terpilih, prioritas, status, dan sisa pekerjaan.
6. **Integrasi sinkronisasi**: koordinasikan dengan Fathir agar task dapat block saat menunggu lock dan wake setelah lock tersedia. Jangan memegang lock saat mencetak log panjang atau yield kecuali desainnya memang aman.
7. **Verifikasi**: jalankan dua atau lebih thread dengan prioritas berbeda dan sama; catat urutan aktual, status, serta bahwa thread rendah akhirnya berjalan jika aging/fairness dijanjikan.

## Uji wajib

- Dua thread tugas berbeda benar-benar dieksekusi.
- Thread READY dengan prioritas lebih tinggi dipilih lebih dahulu.
- FIFO berlaku saat prioritas setara.
- Context switch terjadi saat scheduler memilih thread lain.
- Transisi status sesuai implementasi, tanpa status dekoratif.
- Interaksi block/wake dengan lock tidak merusak queue atau kehilangan thread.
- Demo selesai deterministik dan tidak meninggalkan scheduler dalam keadaan rusak.

## Integrasi dan hasil serah-terima

- Header/API final dan contoh pemanggilan minimal diberikan kepada pemilik `demo-igd`.
- Dokumentasikan algoritma, alasan pemilihan prioritas, tie-break, sumber preemption, dan kasus yang belum didukung.
- Sertakan log QEMU aktual untuk demonstrasi penjadwalan dan status.
- Koordinasikan perubahan file bersama. Hindari `make clean` pada working tree bersama jika artefak tracked.
