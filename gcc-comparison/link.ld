ENTRY(_start)
SECTIONS {
 . = 0;
 .text : { *(.text.start) *(.text*) }
 . = 0x10000000;
 .data : { *(.data*) *(.sdata*) }
 .bss : { *(.bss*) *(.sbss*) *(COMMON) }
 . = ALIGN(4);
 .rodata : { *(.rodata*) *(.srodata*) }
 /DISCARD/ : { *(.comment) *(.eh_frame*) }
}
