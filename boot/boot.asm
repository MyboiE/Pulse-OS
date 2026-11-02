; ==============================================================================
; boot.asm - Entry point & Multiboot Header untuk Mini-OS
; ==============================================================================

; Multiboot 1 Constants
MBALIGN     equ  1 << 0                   ; Align loaded modules on page boundaries
MEMINFO     equ  1 << 1                   ; Provide memory map
FLAGS       equ  MBALIGN | MEMINFO        ; Combined Flags
MAGIC       equ  0x1BADB002               ; Magic number multiboot
CHECKSUM    equ -(MAGIC + FLAGS)          ; Checksum untuk verifikasi header

; ------------------------------------------------------------------------------
; 1. Multiboot Header (Wajib agar dapat di-boot oleh QEMU/GRUB)
; ------------------------------------------------------------------------------
section .multiboot
align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM

; ------------------------------------------------------------------------------
; 2. Kernel Stack Allocation (16 KB)
; ------------------------------------------------------------------------------
section .bss
align 16
stack_bottom:
    resb 16384 ; Reserve 16 KB untuk stack kernel
stack_top:

; ------------------------------------------------------------------------------
; 3. Kernel Entry Point
; ------------------------------------------------------------------------------
section .text
global _start
extern kmain   ; Mengambil pustaka fungsi kmain() dari kernel/src/main.c

_start:
    ; Set pointer stack (ESP) ke batas atas memori stack
    mov esp, stack_top

    ; Panggil fungsi utama kernel C
    call kmain

    ; Jika kmain() return / selesai, hentikan CPU dalam infinite loop
    cli
.hang:
    hlt
    jmp .hang