ENTRY(_start)

/* Total of 512 MB of RAM, 256 for kernel, 256 for user*/
MEMORY {
  KRAM (rwx) : ORIGIN = 0x00000000, LENGTH = 0x10000000  /* 256 MiB for kernel */
  URAM (rwx) : ORIGIN = 0x10000000, LENGTH = 0x10000000  /* 256 MiB for user */
}


SECTIONS {
  . = 0x10000;
  . = ALIGN(0x1000);
  _page_table_base = .;
  .page_table . : {
    KEEP(*(.page_table))
  } > KRAM

  .text.boot 0x80000 : { 
    *(.text.boot) 
  } > KRAM

  .text . : { 
    *(.text) 
  } > KRAM

  .rodata . : { 
    *(.rodata) 
  } > KRAM

  .data . : { 
    *(.data) 
  } > KRAM

  . = ALIGN(0x8);
  .bss . : ALIGN(0x1000) {
    bss_begin = .;
    *(.bss*)
    bss_end = .;
  } > KRAM

  .uram.text : {
    *(.uram.text)
  } > URAM
}
