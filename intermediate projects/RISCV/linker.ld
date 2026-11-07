/* Minimal linker script for QEMU's "virt" machine.
 * QEMU loads the ELF straight into RAM at 0x80000000, so there is no
 * flash/RAM split like on an STM32: code, data and bss all live in RAM.
 */
ENTRY(_start)

MEMORY
{
    RAM (rwx) : ORIGIN = 0x80000000, LENGTH = 128M
}

SECTIONS
{
    /* _start must come first: QEMU begins execution at 0x80000000 */
    .text : {
        *(.text.init)
        *(.text*)
    } > RAM

    .rodata : {
        *(.rodata*)
    } > RAM

    .data : {
        *(.data*)
    } > RAM

    .bss : {
        . = ALIGN(8);
        __bss_start = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(8);
        __bss_end = .;
    } > RAM

    /* Stack grows down from the top of RAM */
    _stack_top = ORIGIN(RAM) + LENGTH(RAM);
}
