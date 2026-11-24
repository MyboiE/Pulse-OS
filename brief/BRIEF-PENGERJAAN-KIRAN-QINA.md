# Brief Pengerjaan: Thread dan Priority Queue Scheduler

## Tanggung jawab

Kiran dan Qina mengembangkan pengelolaan thread kernel dan preemptive scheduler berbasis priority queue untuk mengatur unit kerja layanan pasien pada simulasi IGD. Keduanya perlu berbagi kontrak API dan pengujian; pembagian internal tugas ditetapkan bersama.

## Kebijakan scheduler hybrid

Gunakan urutan dua tingkat:

1. Kelas triase utama: 0 kritis, 1 serius, 2 stabil; angka lebih kecil berarti lebih tinggi dan tidak boleh dikalahkan oleh skor workload.
2. Di dalam kelas yang sama, urutkan dengan skor hybrid dari estimasi sisa waktu eksekusi dan beban resource terukur, misalnya `time_weight * normalized_remaining_time + resource_weight * normalized_resource_load`. Tim harus menetapkan normalisasi, bobot, rentang dan arah urutan sebelum uji; jangan menebak nilai.
3. FIFO saat kelas dan skor hybrid sama.

Estimasi waktu mengacu pada sisa workload, bukan CPU time yang sudah dipakai. Resource harus terukur dalam skenario simulasi. Catat komponen skor pada event log. Baseline kelas triase + FIFO harus diuji dulu, lalu bandingkan hybrid dengan workload yang sama. Aging hanya setelah baseline/hybrid terbukti dan starvation diuji.

## Tahapan dan status implementasi (audit 2026-11-19)

- [x] Selector memakai binary min-heap eksplisit (`heap_push()`/`heap_pop()`) dengan comparator triase → hybrid score → FIFO.
- [x] `make -B` berhasil tanpa warning; belum membuktikan perilaku runtime.
- [ ] Persistent ready queue: heap saat ini dibangun ulang dengan scan TCB READY tiap seleksi.
- [ ] Uji deterministik dispatch triase/hybrid/FIFO dan lifecycle QEMU setelah perubahan heap.
- [ ] Perbaiki serta uji yield/preemption: jadwalkan ulang thread RUNNING yang dipreempt; yield saat ini dapat memilih kembali thread yang sama.
- [ ] Validasi range `triage_class`, `remaining_work`, `resource_load` pada `thread_create_prio()`.
- [ ] Block/wake dan lock handoff diuji bersama scheduler.
- [ ] Timer preemption: `scheduler_tick()` belum ditemukan dipanggil IRQ/PIT; kernel `idt_init()` belum diimplementasikan.
- Log demo lama `/tmp/pulse-demo-igd.log` berasal sebelum perubahan heap; bukan bukti scheduler versi ini.

---

### Tahapan sebelumnya

1. **Kontrak task**: sepakati struktur TCB, status yang didukung, API create/yield/exit/block/wake, kepemilikan stack, ukuran batas, dan mekanisme event/log dengan pemilik CLI.
2. **Lifecycle thread**: pastikan transisi status valid dan dapat diamati. Minimal siapkan dua thread dengan tugas berbeda. Jangan menampilkan status palsu hanya demi memenuhi checklist.
3. **Priority queue**: implementasikan enqueue/dequeue/update sesuai prioritas; pertahankan FIFO pada nilai sama. Uji urutan prioritas campuran dan tie-break.
4. **Preemption/context switch**: tunjukkan pergantian eksekusi nyata antara thread, bukan sekadar mencetak nama task. Dokumentasikan sumber tick/preemption dan keterbatasan implementasi.
5. **Workload demo**: sediakan task simulasi pasien dengan jumlah unit kerja kecil dan deterministik. Tiap langkah mencatat thread terpilih, prioritas, status, dan sisa pekerjaan.
6. **Integrasi sinkronisasi**: koordinasikan dengan Fathir agar task dapat block saat menunggu lock dan wake setelah lock tersedia. Jangan memegang lock saat mencetak log panjang atau yield kecuali desainnya memang aman.
7. **Verifikasi**: jalankan dua atau lebih thread dengan prioritas berbeda dan sama; catat urutan aktual, status, serta bahwa thread rendah akhirnya berjalan jika aging/fairness dijanjikan.

## Uji wajib

- [x] Tiga thread demo berbeda dieksekusi di QEMU; log `/tmp/pulse-demo-igd-latest.log`.
- [x] Thread READY kelas triase lebih tinggi dipilih lebih dahulu dalam demo aktual (KRITIS → SERIOUS → STABIL).
- [ ] Hybrid score membedakan workload dalam kelas sama.
- [ ] FIFO berlaku pada triase dan skor setara.
- [x] Dispatch melakukan context switch; status READY → RUNNING → TERMINATED tercatat untuk ketiga thread.
- [ ] Preemption timer hanya dicentang bila IRQ timer memanggil scheduler dan hasil diuji.
- [ ] Block/wake dengan lock tidak kehilangan thread.
- [ ] Demo deterministik, dapat diulang, tidak merusak state scheduler.

Semua kriteria masih belum acceptance-tested sampai log serial aktual diarsipkan.


## Status implementasi dibanding brief (audit kode, belum acceptance test)

Sudah ada pada `kernel/include/task.h` dan `kernel/src/task.c`:

- TCB menyimpan triase, remaining work, resource load, hybrid score, enqueue order.
- API `thread_create_prio`, yield, wait, exit, block/wake, `thread_update_work`, `scheduler_tick`.
- Context switch memakai stack switching assembly.
- Selector memakai binary min-heap eksplisit (`heap_push()`/`heap_pop()`) dengan urutan triase → skor hybrid → FIFO, dibangun ulang dari scan `thread_pool` setiap pemilihan.
- Label output boot scheduler `Round Robin` diganti menjadi `priority-queue scheduler`; QEMU `demo-igd` kini menguji dispatch heap.
- `make -B` berhasil pada 2026-11-19; QEMU demo heap terbaru lulus pada `/tmp/pulse-demo-igd-latest.log` untuk tiga kelas triase, context switch dan lifecycle READY → RUNNING → TERMINATED. Tie-break intra-kelas dan pengulangan belum diuji.
- Batasan terlihat: `schedule()` memilih READY sebelum requeue thread RUNNING yang dipreempt, dan yield tidak memaksa rotasi ke READY setara/rendah. Perlu perbaikan dan uji lifecycle.
- Belum ada validasi range triase/workload/resource pada `thread_create_prio()`.
- `scheduler_tick()` belum tampak dipanggil IRQ/PIT; preemption timer belum tersedia.
- Sinkronisasi memanggil `thread_block()`/`thread_wake()`, tetapi handoff dan contention scheduler belum diuji dalam run aktual.
- Log demo lama `/tmp/pulse-demo-igd.log` bukan bukti scheduler heap versi baru.

**Belum selesai:** persistent ready queue; uji deterministik triase/hybrid/FIFO, lifecycle/context switch, block/wake, scheduler-preemption; timer IRQ/PIT.

- Skor 0–100: 60% remaining work (normalisasi maksimum 10) + 40% resource (maksimum 5); skor kecil menang.
- Dispatch dan transisi status dilog ke VGA/serial.

Belum dipenuhi/dibuktikan:

- Persistent heap dan uji transisi READY belum dibuat.
- Urutan triase/hybrid/FIFO belum diuji deterministik.
- Belum ada validasi batas triase/workload/resource di `thread_create_prio()`.
- `scheduler_tick()` belum ditemukan dipanggil IRQ/PIT; jangan klaim timer preemption.
- Belum ada hasil QEMU yang membuktikan context switch, fairness yield, block/wake, atau perilaku heap.

## Langkah berikutnya

1. Tetapkan validasi input dan uji unit/in-kernel untuk batas skor serta triase.
2. Tambahkan uji deterministik yang membuktikan triase, hybrid intra-kelas, FIFO, lifecycle, dan context switch nyata.
3. Ganti heap rebuild dengan ready-queue heap persisten (enqueue saat READY, dequeue saat dispatch, reinsert saat yield/wake/preempt); perbarui key saat work berubah dan uji tiap transisi.
4. Hubungkan PIT/IRQ timer ke `scheduler_tick()` sebelum menyebut scheduler preemptive berbasis timer; jika belum, dokumentasikan preemption yang benar-benar terjadi (yield/block/wake).
5. Hapus label Round Robin dan ubah demo dasar agar memakai workload/prioritas eksplisit.
6. Sepakati API block/wake dan event dengan Fathir; uji mutex contention tanpa busy-wait tak terbatas.
7. Serahkan header, contoh pemanggilan, dan log QEMU aktual kepada Nijar untuk integrasi `demo-igd`.

## Dampak origin/main 862f765

Fathir menambahkan mutex/semaphore dengan wait queue dan memanggil `thread_block()`/`thread_wake()`. Ini belum membuktikan integrasi benar. Langkah wajib: uji block/wake, ownership handoff, waiter FIFO, no lost wakeup, lalu simpan serial log. Build penuh setelah pull gagal link lokal `test_deadlock_resolved`; compile/link manual dengan `sync.c` baru berhasil, bukan pengganti QEMU test.

## Integrasi dan hasil serah-terima

- Jangan tandai kriteria selesai hanya karena API tersedia atau build berhasil.
- Simpan command QEMU dan output serial untuk setiap test.
- Hindari `make clean` pada working tree bersama; periksa `git status` sebelum build.
- Koordinasikan perubahan header/source bersama anggota lain.
