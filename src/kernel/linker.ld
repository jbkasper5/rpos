ENTRY(_start)

PHDRS {
  boot  PT_LOAD FLAGS(0x5);  /* R | X */
  text PT_LOAD FLAGS(0x5);  /* R | X */
  rodata PT_LOAD FLAGS(0x4); /* R */
  data PT_LOAD FLAGS(0x6);  /* R | W */
}

__STATIC_PAGES = 100;

PHYS_BASE = 0x80000;
/* VIRT_BASE = 0xFFFF000000000000; */

SECTIONS {
  . = PHYS_BASE;
  __kernel_start = .;

  .text.boot : { 
    *(.text.boot) 
    bin/src/io/mini_uart.o(.text)
    bin/src/io/gpio.o(.text)
  } : boot

  /* . = VIRT_BASE + .; */

  /* .text : AT(ADDR(.text) - VIRT_BASE) {  */
  .text : {
    *(.text) 
  } : text

  /* String literals end up going here */
  /* .rodata : AT(ADDR(.rodata) - VIRT_BASE) {  */
  .rodata : {
    *(.rodata) 
  } : rodata


  . = ALIGN(0x10);
  /* .font : AT(ADDR(.font) - VIRT_BASE) { */
  .font : {
    __font_start = .;
    bin/font.o(*)
    __font_end = .;
  } : rodata

  /* .data : AT(ADDR(.data) - VIRT_BASE) {  */
  .data : {
    *(.data) 
  } : data

  . = ALIGN(0x8);
  /* .bss : AT(ADDR(.bss) - VIRT_BASE) { */
  .bss : {
    bss_begin = .;
    *(.bss*)
    *(COMMON)
    bss_end = .;
  } : data

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
  . = ALIGN(0x1000);
  /*
    TODO: DO THIS LATER
    AT(ADDR(.page_frame_array) - VIRT_BASE) 
  */
  .page_frame_array (NOLOAD) : {
    __page_frame_array_start = .;
    . += 0x800000; /* 2^23 bytes, only maps 1GiB */
    __page_frame_array_end = .;
  } : data


  . = ALIGN(0x1000);
  /*
    TODO: DO THIS LATER
    AT(ADDR(.page_frame_array) - VIRT_BASE) 
  */
  .static_page_region (NOLOAD) : {
    __static_page_region_start = .;
    . += (0x1000 * __STATIC_PAGES);
    __static_page_region_end = .;
  } : data

  /*
    TODO: DO THIS LATER
    AT(ADDR(.page_frame_array) - VIRT_BASE) 
  */
  .uram.text : {
    *(.uram.text)
  } : text
}
