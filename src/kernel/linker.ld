ENTRY(_start)

PHDRS {
  boot  PT_LOAD FLAGS(0x5);  /* R | X */
  boot_data  PT_LOAD FLAGS(0x6);  /* R | X */
  boot_rodata  PT_LOAD FLAGS(0x4);  /* R | X */
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

  __kernel_high_start = . + VIRT_BASE;

  .text.boot : { 
    */boot_s.o(.text.boot)
    *(.text.boot) 
  } : boot

  .boot.data : {
    *(.boot.data)
  } : boot_data

  .boot.rodata : {
    *(.boot.rodata)
  } : boot_rodata

  .boot.bss (NOLOAD) : {
      *(.boot.bss)
  } : boot_data

  . = . + VIRT_BASE;

  .text : AT(ADDR(.text) - VIRT_BASE) { 
    *(.text) 
  } : text

  /* String literals end up going here */
  .rodata : AT(ADDR(.rodata) - VIRT_BASE) { 
    *(.rodata) 
  } : rodata


  . = ALIGN(0x10);
  .font : AT(ADDR(.font) - VIRT_BASE) {
    __font_start = .;
    bin/font.o(*)
    __font_end = .;
  } : rodata

  .data : AT(ADDR(.data) - VIRT_BASE) { 
    *(.data) 
  } : data

  . = ALIGN(0x8);
  .bss : AT(ADDR(.bss) - VIRT_BASE) {
    bss_begin = .;
    *(.bss*)
    *(COMMON)
    bss_end = .;
  } : data

  . = ALIGN(0x10);
  .kstack_top (NOLOAD) : AT(ADDR(.kstack_top) - VIRT_BASE) {
    __kstack_bottom = . - VIRT_BASE;
    __kstack_bottom_hi = .;
    . += 0x2000;
    __kstack_top = . - VIRT_BASE;
    __kstack_top_hi = .;
  }

  .test.text : AT(ADDR(.test.text) - VIRT_BASE) {
    *(.test.text)
  } : test_text


  __kernel_end = .;


  /* TODO: Stopgap - eventually need to remove this */
  __kernel_high_end = .;

  /*
    If uisng 1 GiB RAM: PFE = 32 bytes, 2^12 page size, 2^30 RAM bytes => 2^18 pages * 2^5 bytes = 2^23
    If using 4 GiB RAM: PFE = 32 bytes, 2^12 page size, 2^32 RAM bytes => 2^20 pages * 2^5 bytes = 2^25
  */
  . = ALIGN(0x1000);
  .page_frame_array (NOLOAD) : AT(ADDR(.page_frame_array) - VIRT_BASE) {
    __page_frame_array_start = .;
    . += 0x800000; /* 2^23 bytes, only maps 1GiB */
    __page_frame_array_end = .;
  } : data


  . = ALIGN(0x1000);
  .static_page_region (NOLOAD) : AT(ADDR(.static_page_region) - VIRT_BASE) {
    __static_page_region_start_virt = .;
    __static_page_region_start = .;
    __static_page_region_start_phys = LOADADDR(.static_page_region);
    . += (0x1000 * __STATIC_PAGES);
    __static_page_region_end = .;
  } : data


  .uram.text : AT(ADDR(.uram.text) - VIRT_BASE) {
    *(.uram.text)
  } : text
}
