# Brief Pengerjaan: Boot, CLI, Hak Akses, VFS RAMDisk, Alokasi Memori

## Tanggung jawab

Membangun antarmuka boot dan shell Pulse-OS, lalu memastikan VFS, allocator, dan kontrol hak akses mendukung simulasi IGD serta dapat diuji melalui CLI.

## Tahapan pengerjaan

1. **Boot dan CLI**
   - Pastikan kernel boot melalui jalur Multiboot yang dipakai tim, logo tampil, input keyboard/serial bekerja.
   - Jaga perintah CLI memakai pencocokan token utuh; validasi argumen dan batas panjang input.
   - Tambahkan satu perintah integrasi `demo-igd` setelah kontrak subsistem disepakati. CLI hanya mengatur skenario dan menampilkan event; jangan menduplikasi logika scheduler/lock.
   - Siapkan record pasien simulasi dengan kelas triase, estimasi sisa unit kerja, dan kebutuhan resource terukur. Nilai ini input workload untuk scheduler, bukan penilaian klinis.

2. **VFS RAMDisk**
   - Tetapkan struktur record pasien sebagai teks sederhana dengan nama file deterministik.
   - Verifikasi create, read, update, delete, list; tangani nama duplikat, file tidak ditemukan, kapasitas penuh, nama/data terlalu panjang.
   - Sediakan data demo yang bisa dibuat ulang dan dibersihkan tanpa merusak data lain.

3. **Allocator dinamis**
   - Tulis kontrak alokasi/free, alignment, ukuran nol, permintaan terlalu besar, OOM, invalid/double free, dan asumsi concurrency.
   - Uji alokasi, free, penggunaan ulang/coalescing jika didukung, dan pemulihan kapasitas.
   - Tampilkan angka meminfo aktual; jangan menyamakan kapasitas arena mentah dengan payload tersedia.

4. **Hak akses dua role untuk simulasi IGD**
   - `Emergency Reception`: role berwenang tertinggi; boleh mengatur operasi ruang emergency, data emergency, administrasi visitor, dan menjalankan demo sistem.
   - `Visitor Administration`: hanya boleh membuat, membaca, dan memperbarui data administrasi visitor. Dilarang mengakses/mengubah data emergency dan menjalankan operasi ruang emergency.
   - Lindungi operasi VFS di jalur sebenarnya. Gunakan namespace `visitor-` untuk record visitor; file lain dianggap resource emergency. Terapkan pemeriksaan izin pada `list`, `create`, `read`, `write`, dan `delete`, termasuk penolakan akses langsung melalui API VFS.
   - Sediakan `role`, `role visitor`, `role emergency` untuk demonstrasi. Tolak perubahan ke `Emergency Reception` dari role visitor. Karena login/identitas belum tersedia, jelaskan bahwa pergantian role ini hanya simulasi; bukan autentikasi aman.
   - `protecttest` harus menguji allow visitor pada resource visitor, deny visitor pada resource emergency, allow Emergency Reception pada kedua resource, serta penolakan role/nama role invalid.
   - Tegaskan ini kontrol akses software, bukan isolasi hardware ring-3.

5. **Integrasi demo dan bukti**
   - Hubungkan `demo-igd` ke API task, VFS, allocator, dan sinkronisasi setelah kontrak tim tersedia.
   - Tampilkan kelas triase, estimasi sisa waktu, beban resource, skor hybrid, dan alasan dispatch yang dilaporkan scheduler. CLI tidak menghitung ulang kebijakan scheduler.
   - Uji CLI di QEMU melalui keyboard dan serial. Simpan command, hasil aktual, serta log/screenshot untuk laporan.
   - Koordinasikan perubahan header/API bersama. Jangan menghapus atau mengubah artefak build tracked tanpa koordinasi.

## Checklist penyelesaian Nijar

Tandai `[x]` hanya setelah benar-benar dijalankan dan hasilnya dicatat. Build sukses saja bukan bukti fitur berfungsi.

- [ ] Kernel boot QEMU pernah dilaporkan berhasil via serial, tetapi log reproduksi belum diarsipkan. Logo, keyboard GUI, dan input interaktif belum diverifikasi.
- [ ] CLI command valid/invalid dan batas input belum acceptance-tested; perubahan terakhir belum build-verified.
- [ ] RBAC dua role/CRUD dan `protecttest` pernah dilaporkan lulus, tetapi log serial aktual belum tersimpan untuk audit ulang.
- [ ] `vfstest` dan `memtest` pernah dilaporkan lulus; tanpa log QEMU tersimpan status belum acceptance.
- [ ] `meminfo` pernah dilaporkan menampilkan statistik; angka historis belum memiliki log yang dapat diperiksa.
- [ ] `demo-igd` dan `task2thread` belum diverifikasi end-to-end.
- [ ] Simpan log QEMU dan instruksi reproduksi bagi setiap hasil sebelum menandai PASS.
- [ ] Laporan mencantumkan desain, hasil aktual, keterbatasan, pembagian tugas, dan penggunaan AI sesuai ketentuan tugas.

## Status pengerjaan saat ini

- RBAC VFS berbasis role dan namespace tersedia pada CLI dan API VFS. `protecttest`/CRUD sebelumnya dilaporkan lulus, tetapi log serial reproducible belum disimpan.
- `vfstest`, `memtest`, `meminfo`, dan daftar file tersedia lewat CLI. Laporan PASS pada checklist historis belum disertai log QEMU yang tersimpan; jalankan ulang dan simpan output sebelum menyatakan bukti final.
- CLI guard panjang command ada pada `cli_process_command()`, tetapi input loop belum benar-benar menolak overflow: karakter setelah buffer penuh diabaikan dan command terpotong dapat diproses. Perlu perbaikan + uji nyata.
- `help` sudah dirapikan agar hanya menampilkan satu entri `memtest`.
- Perubahan CLI terakhir belum diverifikasi; percobaan `help` serial tidak menangkap output help, dan pernah ada warning unused variable `overflow`. Build ulang dan bereskan warning sebelum klaim selesai.
- Pergantian role adalah simulasi tanpa autentikasi. Setelah turun ke Visitor Administration, `role emergency` ditolak; pemulihan memerlukan reboot.
- Boot headless via serial pernah diuji; keyboard GUI, logo/boot flow dan input interaktif belum diverifikasi.
- `demo-igd` belum tersedia. Integrasi scheduler dan lock belum dilakukan.

## Langkah Nijar berikutnya

1. Perbaiki overflow di `cli_run()`: buang sisa baris sampai Enter, tolak seluruh command, tampilkan error serial/VGA; jangan jalankan prefix yang terpotong.
2. Uji command valid/invalid, argumen kosong/salah, dan panjang batas lewat QEMU serial dan keyboard GUI; simpan log aktual.
3. Setelah kontrak API Qina/Fathir stabil, integrasikan VFS, allocator, role, scheduler, dan lock ke satu `demo-igd`.
4. Buat cleanup fixture demo yang hanya menghapus record milik demo; tampilkan `meminfo` sebelum/sesudah.
5. Lengkapi laporan dengan perintah reproduksi, log, screenshot, batasan RBAC, dan batasan simulasi edukasi.

## Dependensi tim

- Kiran/Qina: scheduler triase/hybrid/FIFO sudah ada; scan bukan priority queue eksplisit, timer preemption dan uji deterministik belum terbukti.
- Fathir (origin `862f765`): mutex/semaphore wait queue, ownership, block/wake, shared-room API, dan test command sudah masuk. Belum acceptance-tested; verifikasi init objek, waiter handoff, block/wake, counter, deadlock-safe.
- Nijar: CLI/VFS/allocator/RBAC dan orkestrasi `demo-igd`; perbaiki overflow/warning CLI, integrasikan setelah API subsistem lulus test.
- Build: setelah pull, `make` gagal `undefined reference to test_deadlock_resolved` akibat source/object lokal tidak sinkron. Compile/link manual berhasil tapi belum boot/QEMU. Jangan `make clean` di working tree bersama.

## Bukti dan batas klaim

- Simpan log lengkap untuk `protecttest`, `vfstest`, `memtest`, `meminfo`, CLI invalid/boundary, dan `demo-igd`.
- Checklist di atas adalah status kerja, bukan bukti arsip. Jangan tandai acceptance final sebelum log aktual disimpan.
- RBAC ini pemeriksaan software, bukan isolasi ring-3 atau autentikasi.
- IGD adalah simulasi edukasi, bukan alat medis.

## Riwayat sinkronisasi

- Branch lokal `main` diperbarui fast-forward ke `origin/main` pada commit `385e4ea`; perubahan lokal Nijar dipertahankan.
- Working tree memiliki perubahan lokal yang belum commit. Jangan melakukan reset/clean atau menghapus artefak tanpa memeriksa `git status`.
- `help` hanya punya satu baris `memtest`; build/verifikasi akhir sesudah penyuntingan CLI masih perlu dilakukan.


