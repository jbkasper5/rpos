ENTRY(_start)

/* Total of 512 MB of RAM, 256 for kernel, 256 for user*/
MEMORY {
  KRAM (rwx) : ORIGIN = 0x000000, LENGTH = 0xB00000  /* 128 MiB for kernel */
  URAM (rwx) : ORIGIN = 0xB00000, LENGTH = 0x100000  /* 128 MiB for user */
}

PHDRS {
  text PT_LOAD FLAGS(0x5);  /* R | X */
  rodata PT_LOAD FLAGS(0x4); /* R */
  data PT_LOAD FLAGS(0x6);  /* R | W */
  bss  PT_LOAD FLAGS(0x6);  /* R | W */
}

__STATIC_PAGES = 100;

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
    *(COMMON)
    bss_end = .;
  } > KRAM : bss

  . = ALIGN(0x10);
  .kstack_top (NOLOAD) : {
    __kstack_bottom = .;
    . += 0x2000;
    __kstack_top = .;
  }

  __kernel_end = .;
  /*
    If uisng 1 GiB RAM: PFE = 32 bytes, 2^12 page size, 2^30 RAM bytes => 2^18 pages * 2^5 bytes = 2^23
    If using 4 GiB RAM: PFE = 32 bytes, 2^12 page size, 2^32 RAM bytes => 2^20 pages * 2^5 bytes = 2^25
  */
  .page_frame_array (NOLOAD) : ALIGN(0x1000) {
    __page_frame_array_start = .;
    . += 0x800000; /* 2^23 bytes, only maps 1GiB */
    __page_frame_array_end = .;
  } > KRAM : data

  .static_page_region (NOLOAD) : ALIGN(0x1000) {
    __static_page_region_start = .;
    . += (0x1000 * __STATIC_PAGES);
    __static_page_region_end = .;
  } > KRAM : data


  .uram.text : {
    *(.uram.text)
  } > URAM : text
}
