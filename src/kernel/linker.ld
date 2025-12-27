ENTRY(_start)

/* Total of 512 MB of RAM, 256 for kernel, 256 for user*/
MEMORY {
  KRAM (rwx) : ORIGIN = 0x000000, LENGTH = 0x800000  /* 128 MiB for kernel */
  URAM (rwx) : ORIGIN = 0x800000, LENGTH = 0x100000  /* 128 MiB for user */
}

PHDRS {
  text PT_LOAD FLAGS(0x5);  /* R | X */
  rodata PT_LOAD FLAGS(0x4); /* R */
  data PT_LOAD FLAGS(0x6);  /* R | W */
  bss  PT_LOAD FLAGS(0x6);  /* R | W */
}

SECTIONS {
  . = 0x8000;

  __kernel_start = .;
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

  .font ALIGN(0x10) : {
    __font_start = .;
    bin/font.o(*)
    __font_end = .;
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

  . = ALIGN(0x10);
  .kstack_top (NOLOAD) : {
    __kstack_top = .;
    . = . + 0x2000;
    __kstack_bottom = .;
  }

  __kernel_end = .;

  . = ALIGN(0x10);
  .page_frame_array (NOLOAD) : {
    __page_frame_array_start = .;
    . = . + 0x100000;
    __page_frame_array_end = .;
  } > KRAM : data

  .uram.text : {
    *(.uram.text)
  } > URAM : text
}
