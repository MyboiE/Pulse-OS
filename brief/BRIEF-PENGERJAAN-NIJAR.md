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

- [x] Kernel berhasil boot di QEMU `-kernel` dengan serial headless. Logo, keyboard GUI, dan keyboard/serial interaktif belum diverifikasi.
- [ ] CLI menerima command valid, menolak command/argumen tidak valid, dan menangani batas input.
- [x] Hak akses VFS dua role diuji melalui QEMU: Visitor Administration CRUD `visitor-*`; operasi visitor pada file emergency ditolak; Emergency Reception CRUD emergency berhasil.
- [x] `protecttest` menjalankan pengujian VFS langsung: visitor create/read/write/delete visitor PASS; visitor create/read/write/delete emergency ditolak PASS; Emergency Reception read/write/delete emergency PASS.
- [ ] VFS edge cases (duplikat, file tidak ditemukan, kapasitas penuh, nama/data terlalu panjang) belum diuji.
- [x] `memtest` QEMU PASS: accounting alokasi, free memulihkan kapasitas, OOM menghasilkan NULL, dan heap tetap konsisten setelah OOM.
- [x] `meminfo` menampilkan statistik aktual melalui serial; hasil awal: heap 1024 KB, terpakai 0 KB, tersedia 1023 KB.
- [ ] Uji allocator alignment, zero-size, oversized, invalid/double-free, reuse/coalescing belum lengkap.
- [ ] `demo-igd` belum tersedia/teruji end-to-end. `task2thread` belum diverifikasi tuntas karena input serial berbenturan dengan prompt interaktif.
- [ ] Simpan log uji QEMU dan langkah reproduksi untuk laporan.
- [ ] Laporan mencantumkan desain, hasil aktual, keterbatasan, pembagian tugas, dan penggunaan AI sesuai ketentuan tugas.

## Status pengerjaan saat ini

- [x] Hak VFS ditegakkan berdasarkan role dan namespace pada operasi CLI serta API VFS.
- [x] Output `meminfo` dan daftar file tersedia melalui serial.
- [ ] Pergantian role hanya simulasi, tanpa autentikasi; `role emergency` ditolak setelah turun ke visitor dan pemulihan perlu reboot.
- [ ] Boot headless via serial sudah diuji; keyboard GUI dan input interaktif lengkap belum diuji.
- [ ] Demo IGD, scheduler hybrid, dan sinkronisasi menunggu integrasi dengan subsistem Kiran/Qina dan Fathir.

## Dependensi tim

Minta Kiran/Qina menetapkan API untuk membuat/menjalankan skenario task serta melaporkan event/skor scheduler. Minta Fathir menetapkan API lock dan bentuk uji shared counter/deadlock. Jangan mengarang pemanggilan API sebelum header final tersedia.
