SECTIONS
{
    . = 0x80000;
    .text : { 
      *(.text.boot)
      *(.text) 
    }
    .rodata : { 
      *(.rodata) 
    }
    .data : { 
      *(.data) 
    }
}


