/* Ripes executes only the section named .text and loads every section at
 * its VMA, so code goes in one .text at 0 and data at Ripes' data segment.
 */
ENTRY(_start)
SECTIONS
{
    . = 0x00000000;
    .text : { *(.text.start) *(.text .text.*) }
    . = 0x10000000;
    .rodata : { *(.rodata .rodata.* .srodata .srodata.*) }
    .data : { *(.data .data.* .sdata .sdata.*) }
    .bss : { *(.bss .bss.* .sbss .sbss.* COMMON) }
    /DISCARD/ : { *(.comment .riscv.attributes) }
}
