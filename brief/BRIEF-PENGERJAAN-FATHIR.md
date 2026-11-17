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

- API final dan cara pemanggilan diserahkan kepada pemilik demo.
- Uji counter dengan/tanpa lock berjalan dengan hasil aktual tercatat.
- Scheduler mem-block dan membangunkan waiter dengan transisi status benar.
- Deadlock hanya dapat memengaruhi uji terisolasi; mitigasi melalui lock ordering didemonstrasikan.
- Hasil QEMU/log dan penjelasan untuk laporan tersedia.

## Koordinasi

Sepakati bersama Kiran/Qina kontrak block/wake sebelum implementasi integrasi. Sepakati dengan pemilik CLI nama command uji, format event, dan reset skenario. Jangan mengubah API/header tim lain tanpa koordinasi.
