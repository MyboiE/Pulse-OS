# Pulse-OS

Bukan Distribusi Linux, Bukan juga Sistem operasi yang proper. Pulse-OS adalah sistem operasi yang menggunakan basis kernel **x86 32-bit bare-metal** yang dibuat menggunakan **C** dan **Assembly (NASM)** dengan **modal iman dan taqwa**. OS ini Bertujuan untuk menjadi salah satu projek tugas besar yang kami lakukan untuk memahami cara kerja dan implementasi Sistem Operasi terhadap kasus tertentu yang pada kali ini yakni sistem manajemen gawat darurat pada IGD.

Kernel di-boot lewat standar **Multiboot**, sehingga bisa dijalankan langsung di **QEMU** maupun lewat **GRUB** (ISO).

Versi saat ini: **v1.0.0**


## Struktur Proyek

```
Pulse-OS/
├── boot/
│   ├── boot.asm        # Entry point + Multiboot header
│   └── linker.ld       # Linker script (kernel dimuat di 1 MB)
├── drivers/
│   ├── vga.c
│   ├── serial.c
│   └── keyboard.c
├── kernel/
│   ├── include/        # Header (kernel.h, cli.h, task.h, vfs.h, ...)
│   └── src/            # main.c, cli.c, task.c, sync.c, memory.c, vfs.c, protect.c
├── scripts/
│   └── grub.cfg        # Konfigurasi GRUB untuk build ISO
├── Makefile
└── README.md
```


## Kebutuhan (Prasyarat)

| Alat | Fungsi | 
|------|--------|
| `gcc` (dengan dukungan `-m32`) | Kompilasi kode C |
| `nasm` | Merakit `boot.asm` |
| `ld` (GNU binutils) | Linking kernel |
| `make` | Menjalankan build |
| `qemu-system-x86_64` | Menjalankan kernel |

> **Catatan:** Makefile menghasilkan objek format **ELF 32-bit** (`-m32` + `ld -m elf_i386`). Karena itu proses build harus dilakukan di lingkungan **Linux** (native atau WSL2 di Windows).

---


## Instalasi & Menjalankan di Linux

### 1. Pasang dependensi

**Debian / Ubuntu / Linux Mint**

```bash
sudo apt update
sudo apt install -y build-essential gcc-multilib nasm make qemu-system-x86 git
# Opsional, untuk membuat ISO:
sudo apt install -y grub-pc-bin grub-common xorriso mtools
```

**Fedora**

```bash
sudo dnf install -y gcc glibc-devel.i686 nasm make qemu-system-x86 git
# Opsional, untuk membuat ISO:
sudo dnf install -y grub2-tools-extra xorriso mtools
```

**Arch Linux / Manjaro**

```bash
sudo pacman -S --needed base-devel lib32-glibc nasm qemu-system-x86 git
# Opsional, untuk membuat ISO:
sudo pacman -S --needed grub xorriso mtools
```

### 2. Ambil source code

```bash
git clone https://github.com/MyboiE/mini-os.git Pulse-OS
cd Pulse-OS
```

Atau, jika sudah punya file `Pulse-OS.zip`:

```bash
unzip Pulse-OS.zip
cd Pulse-OS
```

### 3. Build

```bash
make clean   # hapus file .o dan binary lama (disarankan)
make         # menghasilkan mini-os.bin
```

### 4. Jalankan

```bash
make run
```

Jendela QEMU akan terbuka, lalu shell `Pulse-OS>` muncul. Input juga bisa diketik lewat **terminal** tempat `make run` dijalankan (karena memakai `-serial stdio`).

### 5. (Opsional) Jalankan lewat ISO + GRUB

```bash
make run-iso     # membuat mini-os.iso lalu menjalankannya di QEMU
```

File `mini-os.iso` juga bisa dipakai di VirtualBox atau VMware (buat VM baru, pilih tipe *Other / Other 32-bit*, lalu pasang ISO sebagai CD-ROM).

---



## Instalasi & Menjalankan di Windows

Cara paling mudah dan andal di Windows adalah memakai **WSL2** (Windows Subsystem for Linux), karena proses build membutuhkan toolchain ELF/Linux.

### 1. Aktifkan WSL2 dan pasang Ubuntu

Buka **PowerShell sebagai Administrator**, lalu:

```powershell
wsl --install -d Ubuntu
```

Restart komputer jika diminta, lalu buka aplikasi **Ubuntu** dari Start Menu dan buat username serta password.

### 2. Pasang dependensi di dalam Ubuntu (WSL)

```bash
sudo apt update
sudo apt install -y build-essential gcc-multilib nasm make qemu-system-x86 git
# Opsional, untuk membuat ISO:
sudo apt install -y grub-pc-bin grub-common xorriso mtools
```

### 3. Ambil source code

Simpan proyek di filesystem Linux (misalnya `~/`), **bukan** di `/mnt/c/...`, supaya build lebih cepat dan tidak bermasalah dengan permission.

```bash
cd ~
git clone https://github.com/MyboiE/mini-os.git Pulse-OS
cd Pulse-OS
```

Jika memakai `Pulse-OS.zip` dari Windows, salin dulu ke WSL, misalnya:

```bash
cp /mnt/c/Users/<NamaUser>/Downloads/Pulse-OS.zip ~/
unzip ~/Pulse-OS.zip -d ~/
cd ~/Pulse-OS
```

### 4. Build

```bash
make clean
make
```

### 5. Jalankan

Pilih salah satu cara di bawah ini.

**Opsi A: Windows 11 (atau Windows 10 terbaru dengan WSLg)**

Jendela QEMU langsung muncul dari WSL:

```bash
make run
```

**Opsi B: QEMU versi Windows (jika Opsi A tidak menampilkan jendela)**

1. Pasang QEMU untuk Windows dari <https://www.qemu.org/download/#windows>.
2. Salin hasil build dari WSL ke Windows:

   ```bash
   cp mini-os.bin /mnt/c/Users/<NamaUser>/Desktop/
   ```

3. Di **PowerShell**, jalankan (sesuaikan lokasi instalasi QEMU):

   ```powershell
   & "C:\Program Files\qemu\qemu-system-x86_64.exe" -kernel "$env:USERPROFILE\Desktop\mini-os.bin" -serial stdio
   ```

**Opsi C: Tanpa jendela grafis (mode terminal saja)**

```bash
make run QEMU_FLAGS="-nographic"
```

Untuk keluar dari QEMU pada mode ini, tekan `Ctrl+A` lalu `X`.

---

## Perintah Make

| Perintah | Keterangan |
|----------|------------|
| `make` | Build kernel menjadi `mini-os.bin` |
| `make run` | Build lalu jalankan `mini-os.bin` di QEMU |
| `make mini-os.iso` | Membuat ISO bootable dengan GRUB |
| `make run-iso` | Membuat ISO lalu jalankan di QEMU |
| `make clean` | Hapus semua hasil build (`.o`, `.bin`, `.iso`) |

Flag QEMU bisa diubah lewat variabel `QEMU_FLAGS`, contoh:

```bash
make run QEMU_FLAGS="-serial stdio -display sdl"
make run QEMU_FLAGS="-serial stdio -m 128M"
```

---

## Perintah di Shell Pulse-OS

Setelah booting, ketik perintah berikut pada prompt `Pulse-OS>`:

| Perintah | Fungsi |
|----------|--------|
| `help` | Menampilkan daftar perintah |
| `version` | Menampilkan versi Pulse-OS |
| `meminfo` | Status alokasi memori dinamis |
| `ls` | Menampilkan file di VFS RAMDisk |
| `tasks` | Status thread dan scheduler |
| `clear` | Membersihkan layar |
| `logs` / `history` | Riwayat perintah yang sudah dijalankan |
| `restart` | Merestart Pulse-OS |
| `shutdown` / `poweroff` | Mematikan sistem |

---

## Troubleshooting

**`make: nasm: No such file or directory`**
NASM belum terpasang. Jalankan `sudo apt install nasm` (atau padanannya di distro lain).

**`gcc: error: unrecognized command-line option '-m32'` atau error library 32-bit**
Pasang dukungan 32-bit: `sudo apt install gcc-multilib` (Debian/Ubuntu).

**`qemu-system-x86_64: command not found`**
Pasang QEMU: `sudo apt install qemu-system-x86`.

**`gtk initialization failed` / jendela QEMU tidak muncul**
Tidak ada display grafis yang tersedia. Gunakan salah satu alternatif:

```bash
make run QEMU_FLAGS="-serial stdio -display sdl"
make run QEMU_FLAGS="-nographic"
```

**`grub-mkrescue: command not found` atau gagal membuat ISO**
Pasang paket tambahan: `sudo apt install grub-pc-bin grub-common xorriso mtools`.

**Hasil build lama terpakai / perilaku aneh**
Repo ini menyertakan file `.o` dan `mini-os.bin` hasil build sebelumnya. Jalankan `make clean && make` untuk build ulang dari awal.

**Error `Permission denied` atau build sangat lambat di WSL**
Pastikan proyek berada di dalam filesystem Linux (`~/Pulse-OS`), bukan di `/mnt/c/...`.

---

## Lisensi

Belum ditentukan. Tambahkan file `LICENSE` jika proyek ini akan dipublikasikan.
