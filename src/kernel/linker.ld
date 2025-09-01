ENTRY(_start)

MEMORY {
  RAM (rwx) : ORIGIN = 0x0, LENGTH = 0x40000000 /* Example 1GB */
}

SECTIONS
{
  . = 0x10000;
  . = ALIGN(0x1000);
  .page_table : {
    KEEP(*(.page_table))
  } > RAM

  . = 0x80000;

  .text.boot : { *(.text.boot) }
  .text : { *(.text) }
  .rodata : { *(.rodata) }
  .data : { *(.data) }

  . = ALIGN(0x8);
  bss_begin = .;
  .bss : { *(.bss*) }
  bss_end = .;
}