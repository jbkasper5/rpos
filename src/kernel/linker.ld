ENTRY(_start)

PHDRS {
  boot  PT_LOAD FLAGS(0x5);  /* R | X */
  boot_data  PT_LOAD FLAGS(0x6);  /* R | X */
  boot_rodata  PT_LOAD FLAGS(0x4);  /* R | X */

  text PT_LOAD FLAGS(0x5);  /* R | X */
  rodata PT_LOAD FLAGS(0x4); /* R */
  data PT_LOAD FLAGS(0x6);  /* R | W */

  test_text PT_LOAD FLAGS(0x5);  /* R | X */
  test_rodata PT_LOAD FLAGS(0x4); /* R */
  test_data PT_LOAD FLAGS(0x6);  /* R | W */
}

__STATIC_PAGES = 100;

DEVICE_TREE_BASE = 0x10000;
PHYS_BASE = 0x80000;
VIRT_BASE = 0xFFFF800000000000;

TEST_BASE = 0x8000000;

SECTIONS {
  . = DEVICE_TREE_BASE;
  .device_tree_reserved (NOLOAD) : {
    . += 0x20000; 
    . = ALIGN(4096); /* Buffer space before BSS starts */
  }

  . = PHYS_BASE;
  __kernel_start = .;

  __kernel_high_start = . + VIRT_BASE;

  /* ---- R-X : boot code ---- */
  .text.boot : {
    __boot_text_start = .;
    */boot_s.o(.text.boot)
    */gic.o(.text)
    *(.text.boot)
    . = ALIGN(0x1000);
    __boot_text_end = .;
  } : boot

  . = ALIGN(0x1000);                 /* R-X | R-W permission boundary */

  /* ---- R-W : boot data ---- */
  .boot.data : {
    __boot_data_start = .;
    *(.boot.data)
    */gic.o(.data)
    . = ALIGN(0x1000);
    __boot_data_end = .;
  } : boot_data

  . = ALIGN(0x1000);                 /* R-W | R permission boundary */

  /* ---- R-- : boot rodata ---- */
  .boot.rodata : {
    __boot_rodata_start = .;
    *(.boot.rodata)
    */gic.o(.rodata)
    . = ALIGN(0x1000);
    __boot_rodata_end = .;
  } : boot_rodata

  . = ALIGN(0x1000);                 /* R | R-W permission boundary */

  /* ---- R-W : boot bss (NOLOAD) ---- */
  .boot.bss (NOLOAD) : {
      __boot_bss_start = .;
      *(.boot.bss)
      */gic.o(.bss)
      __boot_bss_end = .;
  } : boot_data

  . = . + VIRT_BASE;

  /* ---- R-X : kernel code ---- */
  .text : AT(ADDR(.text) - VIRT_BASE) {
    __text_start = .;
    *(EXCLUDE_FILE (**/usrtmp/user.o **/usrtmp/user_s.o **/usrtmp/printf.o) .text*)
    . = ALIGN(0x1000);
    __text_end = .;
  } : text

  . = ALIGN(0x1000);                 /* R-X | R permission boundary */

  /* ---- R-- : kernel read-only data (incl. embedded font) ---- */
  .rodata : AT(ADDR(.rodata) - VIRT_BASE) {
    __rodata_start = .;
    *(EXCLUDE_FILE (**/usrtmp/user.o **/usrtmp/user_s.o **/usrtmp/printf.o) .rodata*)
  } : rodata

  . = ALIGN(0x10);
  .font : AT(ADDR(.font) - VIRT_BASE) {
    __font_start = .;
    bin/font.o(*)
    __font_end = .;
    . = ALIGN(0x1000);
    __rodata_end = .;
  } : rodata

  . = ALIGN(0x1000);                 /* R | R-W permission boundary */

  /* ---- R-W : kernel data + bss (the loadable/zeroed RW group) ---- */
  .data : AT(ADDR(.data) - VIRT_BASE) {
    __data_start = .;
    *(EXCLUDE_FILE (**/usrtmp/user.o **/usrtmp/user_s.o **/usrtmp/printf.o) .data*)
  } : data

  . = ALIGN(0x8);
  .bss : AT(ADDR(.bss) - VIRT_BASE) {
    __bss_start = .;
    *(.bss*)
    *(COMMON)
    __bss_end = .;
    . = ALIGN(0x1000);
    __data_end = .;                  /* end of the loadable R-W group */
  } : data


  __kernel_end = .;


  /* TODO: Stopgap - eventually need to remove this */
  __kernel_high_end = .;

  /* Place kernel stack after the kernel segment */
  . = ALIGN(0x1000);  
  .kstack_top (NOLOAD) : AT(ADDR(.kstack_top) - VIRT_BASE) {
    __kstack_bottom = . - VIRT_BASE;
    __kstack_bottom_hi = .;
    . += 0x2000;
    __kstack_top = . - VIRT_BASE;
    __kstack_top_hi = .;
  }

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


  __test_phys_loc = __static_page_region_end - VIRT_BASE;
  __test_phys_loc = ALIGN(__test_phys_loc, 0x1000);

  /* 2. Map Test Text - Force VMA to 0x8000000 */
  .text.test TEST_BASE : AT(__test_phys_loc) {
    __test_text_start = .;
    **/usrtmp/printf.o(.text*)
    **/usrtmp/user.o(.text*)
    **/usrtmp/user_s.o(.text*)
    . = ALIGN(0x1000);
    __test_text_end = .;
  } : test_text

  /* 3. Map Test Data - Force VMA to stay in the Low-Half (0x800xxxx) */
  /* This 'ADDR(...) + SIZEOF(...)' is the magic that fixes the truncation */
  .test.data ADDR(.text.test) + SIZEOF(.text.test) : AT(LOADADDR(.text.test) + SIZEOF(.text.test)) {
    **/usrtmp/printf.o(.rodata*)
    **/usrtmp/printf.o(.data*)
    **/usrtmp/user.o(.rodata*)
    **/usrtmp/user.o(.data*)
    **/usrtmp/user_s.o(.rodata*)
    **/usrtmp/user_s.o(.data*)
    . = ALIGN(0x1000);
    __test_data_end = .;
  } : test_data

  __test_size = LOADADDR(.test.data) + SIZEOF(.test.data) - __test_phys_loc;
}
