ENTRY(_start)

/* Total of 512 MB of RAM, 256 for kernel, 256 for user*/
MEMORY {
  KRAM (rwx) : ORIGIN = 0x000000, LENGTH = 0x100000  /* 128 MiB for kernel */
  URAM (rwx) : ORIGIN = 0x100000, LENGTH = 0x100000  /* 128 MiB for user */
}

PHDRS {
  text PT_LOAD FLAGS(0x5);  /* R | X */
  rodata PT_LOAD FLAGS(0x4); /* R */
  data PT_LOAD FLAGS(0x6);  /* R | W */
  bss  PT_LOAD FLAGS(0x6);  /* R | W */
}

SECTIONS {
  . = 0x8000;
  _page_table_metadata = .;
  .page_table_metadata . : {
    KEEP(*(.page_table_metadata))
    . = . + 0x8000;   /* Reserve 32 KiB for metadata, for example */
  } > KRAM : data

  . = 0x10000;
  . = ALIGN(0x1000);
  _page_table_base = .;
  .page_table . : {
    KEEP(*(.page_table))
    . = . + 0x2000;   /* Ensure enough space for L1 and L2 page tables */
  } > KRAM : data

  .text.boot 0x80000 : { 
    *(.text.boot) 
  } > KRAM : text

  .text . : { 
    *(.text) 
  } > KRAM : text

  /* String literals end up going here */
  .rodata . : { 
    *(.rodata) 
  } > KRAM : rodata

  .data . : { 
    *(.data) 
  } > KRAM : data

  . = ALIGN(0x8);
  .bss : {
    bss_begin = .;
    *(.bss*)
    bss_end = .;
  } > KRAM : bss

  .uram.text : {
    *(.uram.text)
  } > URAM : text
}
