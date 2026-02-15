ENTRY(_start)

PHDRS
{
  text   PT_LOAD FLAGS(5);  /* R + X */
  rodata PT_LOAD FLAGS(4);  /* R */
  data   PT_LOAD FLAGS(6);  /* R + W */
}

SECTIONS
{
  . = 0x400000;

  /* ------------------ */
  /* Text / code (RX)   */
  /* ------------------ */
  .text : ALIGN(4K)
  {
    KEEP(*(.text._start))
    *(.text*)
  } :text

  /* ------------------ */
  /* Read-only data (R) */
  /* ------------------ */
  .rodata : ALIGN(4K)
  {
    *(.rodata*)
  } :rodata

  /* ------------------ */
  /* Writable data (RW) */
  /* ------------------ */
  .data : ALIGN(4K)
  {
    *(.data*)
  } :data

  .bss : ALIGN(4K)
  {
    *(COMMON)
    *(.bss*)
  } :data

  /DISCARD/ :
  {
    *(.comment*)
    *(.note*)
    *(.eh_frame*)
  }
}
