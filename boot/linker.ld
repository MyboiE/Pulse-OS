/* ==============================================================================
   linker.ld - Linker Script untuk Mini-OS
   ============================================================================== */

/* Menentukan entry point utama kernel (didefinisikan di boot.asm) */
ENTRY(_start)

SECTIONS
{
    /* Kernel akan dimuat mulai dari alamat memori 1MB (0x100000) */
    . = 1M;

    /* Section .text: Tempat untuk Multiboot Header dan instruksi kode program */
    .text BLOCK(4K) : ALIGN(4K)
    {
        *(.multiboot)
        *(.text)
    }

    /* Section .rodata: Tempat variabel/data read-only (seperti string constant) */
    .rodata BLOCK(4K) : ALIGN(4K)
    {
        *(.rodata)
    }

    /* Section .data: Tempat variabel global/statis yang terinisialisasi */
    .data BLOCK(4K) : ALIGN(4K)
    {
        *(.data)
    }

    /* Section .bss: Tempat variabel yang belum terinisialisasi & stack kernel */
    .bss BLOCK(4K) : ALIGN(4K)
    {
        *(COMMON)
        *(.bss)
    }
}