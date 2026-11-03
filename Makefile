# Toolchain
CC = gcc
ASM = nasm
LD = ld

# Compiler & Linker Flags (Target x86 32-bit bare-metal)
CFLAGS = -m32 -ffreestanding -O2 -Wall -Wextra -Ikernel/include
ASMFLAGS = -f elf32
LDFLAGS = -m elf_i386 -T boot/linker.ld

# Daftar File Object (.o) dari seluruh source code di struktur folder
OBJS = boot/boot.o \
       drivers/vga.o \
       drivers/serial.o \
       drivers/keyboard.o \
       kernel/src/main.o \
       kernel/src/cli.o \
       kernel/src/task.o \
       kernel/src/sync.o \
       kernel/src/memory.o \
       kernel/src/vfs.o \
       kernel/src/protect.o

# Target Default (Build Binary Kernel)
all: mini-os.bin

# Rule untuk mengompilasi file C (.c -> .o)
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Rule untuk mengompilasi Bootloader Assembly (.asm -> .o)
boot/boot.o: boot/boot.asm
	$(ASM) $(ASMFLAGS) $< -o $@

# Rule Linking seluruh object menjadi 1 Binary Kernel
mini-os.bin: $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o $@

# Rule untuk membuat file ISO (opsional jika menggunakan GRUB)
mini-os.iso: mini-os.bin
	mkdir -p isodir/boot/grub
	cp mini-os.bin isodir/boot/mini-os.bin
	cp scripts/grub.cfg isodir/boot/grub/grub.cfg
	grub-mkrescue -o mini-os.iso isodir
	rm -rf isodir

# QEMU Flags (Skala tampilan mengikuti ukuran jendela)
QEMU_FLAGS ?= -serial stdio -display gtk,zoom-to-fit=on

# Perintah untuk Kompilasi + Jalankan Langsung Binary di QEMU
run: mini-os.bin
	qemu-system-x86_64 -kernel mini-os.bin $(QEMU_FLAGS)

# Perintah untuk Jalankan ISO di QEMU
run-iso: mini-os.iso
	qemu-system-x86_64 -cdrom mini-os.iso $(QEMU_FLAGS)

# Perintah Membersihkan File Build
clean:
	rm -f $(OBJS) mini-os.bin mini-os.iso
	rm -rf isodir