# Brief Pengerjaan: Sinkronisasi dan Deadlock

## Tanggung jawab

Fathir mengembangkan primitive sinkronisasi kernel dan skenario uji shared data serta deadlock untuk simulasi IGD. Mekanisme harus terhubung dengan lifecycle scheduler: thread yang menunggu lock tidak boleh busy-wait tanpa batas atau tetap dianggap berjalan.

## Tahapan

1. **Kontrak API**
   - Tetapkan primitive yang dipakai (mutex atau semaphore), API init/lock/unlock, aturan ownership, dan perilaku ketika lock sudah dimiliki.
   - Sepakati dengan Kiran/Qina cara mem-block dan membangunkan thread. Sinkronisasi harus memakai API scheduler yang disepakati, bukan mengakses struktur internal secara liar.
   - Dokumentasikan apakah primitive aman dipakai dari interrupt; jika tidak, larang penggunaan dari interrupt handler.

2. **Shared counter**
   - Buat dua thread yang menambah shared counter dengan jumlah operasi tetap.
   - Jalankan kontrol tanpa lock dan skenario dengan lock. Operasi baca-ubah-tulis harus dapat terinterleave agar race condition dapat diamati; jangan mengklaim race hanya dari output yang kebetulan sama.
   - Dengan lock, hasil akhir harus sama dengan total increment yang diharapkan. Catat hasil aktual dan urutan event.

3. **Skenario IGD**
   - Gunakan lock untuk melindungi resource bersama simulasi, misalnya satu slot tindakan/prosedur atau counter ruang tersedia.
   - Event log menunjukkan thread meminta lock, masuk critical section, memperbarui state, melepas lock, dan thread lain melanjutkan.
   - Critical section harus kecil; jangan panggil operasi yang dapat yield atau mencetak panjang di dalam lock kecuali desain dan implementasi menjamin aman.

4. **Deadlock demonstrasi**
   - Buat dua lock dan dua thread uji yang mengambil lock dalam urutan berlawanan hanya pada command uji khusus.
   - Deadlock dapat membuat kernel macet. Jalankan di QEMU terisolasi dengan batas waktu/restart; jangan masukkan jalur deadlock ke `demo-igd` utama.
   - Tunjukkan kondisi circular wait dan solusi berupa urutan pengambilan lock global yang konsisten. Uji ulang solusi tanpa deadlock.

5. **Kegagalan dan invariants**
   - Cegah unlock oleh pemilik salah, double init yang merusak state, kehilangan waiter, dan wake-up yang salah.
   - Hindari mencetak dari critical section jika output memakai shared state yang sama.
   - Dokumentasikan batasan implementasi yang belum ditangani.

## Kriteria selesai

- [x] API mutex/semaphore, ownership, wait queue, shared-room IGD, dan deadlock mitigation tersedia di header.
- [x] Uji counter dengan/tanpa lock berjalan dan hasil aktual tersimpan (expected locked total: 100): log `/tmp/pulse-subsystem-tests.log`, tanpa lock `50`, dengan mutex `100`.
- [ ] Scheduler mem-block dan membangunkan waiter dengan transisi status benar; handoff mutex/semaphore dan no-lost-wakeup teruji.
- [x] Deadlock hanya memengaruhi uji terisolasi; `deadlocksafe` selesai pada run QEMU. `deadlocktest` tetap tidak dijalankan karena berpotensi macet.
- [x] Hasil QEMU/log dan penjelasan laporan tersedia: `/tmp/pulse-subsystem-tests.log`.

Acceptance tersisa: handoff/block-wake/no-lost-wakeup dan `deadlocktest` terisolasi dengan timeout.
## Status audit kode saat ini

- Commit origin `862f765` menambahkan mutex/semaphore dengan FIFO wait queue, ownership check dan panggilan `thread_block()`/`thread_wake()`, plus API shared room IGD.
- `synctest`, `deadlocktest`, `deadlocksafe` ditambahkan ke CLI. Implementasi test ada, tetapi hasil aktual belum disimpan sebagai log QEMU.
- Audit source menunjukkan risiko yang perlu diuji: `mutex_init()` membaca `initialized` sebelum objek dijamin zero-initialized; caller saat ini menghindarinya dengan mengatur field manual. `sem_init()` serupa. `sem_wait()` setelah wake perlu memastikan permit/handoff tidak hilang.
- Test counter memaksa yield antara load/store; hasil race bergantung perilaku scheduler dan wajib dicatat, bukan diasumsikan.
- Deadlock test dapat macet permanen; hanya jalankan pada QEMU terisolasi dengan timeout.
- Build penuh setelah pull gagal link karena object `kernel/src/sync.o` stale / tidak dibangun ulang; compile `sync.c` langsung dan link manual berhasil, namun bukan verifikasi clean build/QEMU.
- `thread_block()`/`thread_wake()` dan jalur deadlock perlu diverifikasi terhadap lifecycle scheduler. Jangan anggap aman hanya karena API dipanggil.

## Langkah berikutnya

1. Audit dan uji race pada init mutex/semaphore yang membaca `initialized` dari objek belum tentu terinisialisasi; desain init harus aman untuk objek baru.
2. Uji handoff mutex, semaphore permit setelah wake, FIFO waiter, self-lock, non-owner unlock, queue penuh, dan block/wake terhadap scheduler.
3. Pastikan `synctest` benar-benar interleave; pastikan counter dengan lock berakhir tepat 100. Catat nilai aktual dan exit status.
4. Jalankan `deadlocktest` hanya pada QEMU terisolasi dengan timeout; jalankan `deadlocksafe` dan buktikan dua thread selesai.
5. Perbaiki `make` dependency/source selection agar link selalu memakai `kernel/src/sync.c` aktual; jalankan build valid dan simpan log QEMU.
6. Setelah uji lulus, serahkan kontrak API/event format kepada Nijar untuk integrasi demo IGD.

## Koordinasi

Sepakati bersama Kiran/Qina kontrak block/wake sebelum implementasi integrasi. Sepakati dengan pemilik CLI nama command uji, format event, dan reset skenario. Jangan mengubah API/header tim lain tanpa koordinasi.

Jangan tandai kriteria selesai hanya berdasarkan stub, pesan simulasi, atau build sukses; arsipkan output uji aktual.

## Batas klaim

Sinkronisasi saat ini belum menjamin progress saat contention dan belum aman untuk menjelaskan sebagai mutex scheduler-aware. Deadlock demo tidak boleh dimasukkan ke `demo-igd` utama sebelum isolasi dan pemulihan terbukti.

