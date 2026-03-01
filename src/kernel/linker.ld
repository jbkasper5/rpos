ENTRY(_start)

PHDRS {
  boot  PT_LOAD FLAGS(0x5);  /* R | X */
  boot_data  PT_LOAD FLAGS(0x6);  /* R | X */
  text PT_LOAD FLAGS(0x5);  /* R | X */
  test_text PT_LOAD FLAGS(0x5);  /* R | X */
  rodata PT_LOAD FLAGS(0x4); /* R */
  data PT_LOAD FLAGS(0x6);  /* R | W */
}

__STATIC_PAGES = 100;

PHYS_BASE = 0x80000;
VIRT_BASE = 0xFFFF800000000000;

SECTIONS {
  . = PHYS_BASE;
  __kernel_start = .;

  /* TODO: Stopgap - eventually need to remove this */
  __kernel_high_start = . + VIRT_BASE;
  /* . = . - VIRT_BASE; */

  .text.boot : { 
    */boot_s.o(.text.boot)
    *(.text.boot) 
    bin/src/io/mini_uart.o(.text)
    bin/src/io/gpio.o(.text)
  } : boot

  .boot.data : {
    *(.boot.data)
  } : boot_data

  .boot.bss (NOLOAD) : {
      *(.boot.bss)
  } : boot_data

  /* . = . + VIRT_BASE; */

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
  /* .kstack_top (NOLOAD) : AT(ADDR(.kstack_top) - VIRT_BASE) { */
  .kstack_top (NOLOAD) : {
    __kstack_bottom = .;
    __kastck_bottom_hi = . + VIRT_BASE;
    . += 0x2000;
    __kstack_top = .;
    __kstack_top_hi = . + VIRT_BASE;
  }

  . = VIRT_BASE + .;
  .test.text : AT(ADDR(.test.text) - VIRT_BASE) {
    *(.test.text)
  } : test_text
  . = . - VIRT_BASE;


  __kernel_end = .;


  /* TODO: Stopgap - eventually need to remove this */
  /* . = VIRT_BASE + .; */
  __kernel_high_end = .;
  /* . = . - VIRT_BASE; */

  /*
    If uisng 1 GiB RAM: PFE = 32 bytes, 2^12 page size, 2^30 RAM bytes => 2^18 pages * 2^5 bytes = 2^23
    If using 4 GiB RAM: PFE = 32 bytes, 2^12 page size, 2^32 RAM bytes => 2^20 pages * 2^5 bytes = 2^25
  */
  . = ALIGN(0x1000);
  /* .page_frame_array (NOLOAD) : AT(ADDR(.page_frame_array) - VIRT_BASE) { */
  .page_frame_array (NOLOAD) : {
    __page_frame_array_start = .;
    . += 0x800000; /* 2^23 bytes, only maps 1GiB */
    __page_frame_array_end = .;
  } : data


  . = ALIGN(0x1000);
  /* .static_page_region (NOLOAD) : AT(ADDR(.static_page_region) - VIRT_BASE) { */
  .static_page_region (NOLOAD) : {
    __static_page_region_start_virt = .;
    __static_page_region_start = .;
    __static_page_region_start_phys = LOADADDR(.static_page_region);
    . += (0x1000 * __STATIC_PAGES);
    __static_page_region_end = .;
  } : data


  /* .uram.text : AT(ADDR(.uram.text) - VIRT_BASE) { */
  .uram.text : {
    *(.uram.text)
  } : text
}
