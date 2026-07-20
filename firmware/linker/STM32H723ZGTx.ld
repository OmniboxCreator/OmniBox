/* STM32H723ZGTx - 1 MB Flash, DTCM 128K, AXI SRAM (D1) 320K, D2/D3 SRAM */
ENTRY(Reset_Handler)

_estack = ORIGIN(DTCMRAM) + LENGTH(DTCMRAM);
_Min_Heap_Size = 0x800;
_Min_Stack_Size = 0x1000;

MEMORY
{
 ITCMRAM (xrw) : ORIGIN = 0x00000000, LENGTH = 64K
 /* FLASH is limited to 896K. Sector 7 (128K @ 0x080E0000) is reserved for
  * configuration storage. Do not place code there. */
 FLASH  (rx) : ORIGIN = 0x08000000, LENGTH = 896K
 DTCMRAM (xrw) : ORIGIN = 0x20000000, LENGTH = 128K
 RAM_D1 (xrw) : ORIGIN = 0x24000000, LENGTH = 320K
 RAM_D2 (xrw) : ORIGIN = 0x30000000, LENGTH = 32K
 RAM_D3 (xrw) : ORIGIN = 0x38000000, LENGTH = 16K
}

SECTIONS
{
 .isr_vector : { . = ALIGN(4); KEEP(*(.isr_vector)) . = ALIGN(4); } >FLASH

 .text : {
  . = ALIGN(4);
  *(.text) *(.text*) *(.glue_7) *(.glue_7t) *(.eh_frame)
  KEEP (*(.init)) KEEP (*(.fini))
  . = ALIGN(4); _etext = .;
 } >FLASH

 .rodata : { . = ALIGN(4); *(.rodata) *(.rodata*) . = ALIGN(4); } >FLASH
 .ARM.extab : { *(.ARM.extab* .gnu.linkonce.armextab.*) } >FLASH
 .ARM : { __exidx_start = .; *(.ARM.exidx*) __exidx_end = .; } >FLASH

 .preinit_array : { PROVIDE_HIDDEN(__preinit_array_start = .); KEEP(*(.preinit_array*)) PROVIDE_HIDDEN(__preinit_array_end = .); } >FLASH
 .init_array  : { PROVIDE_HIDDEN(__init_array_start = .); KEEP(*(SORT(.init_array.*))) KEEP(*(.init_array*)) PROVIDE_HIDDEN(__init_array_end = .); } >FLASH
 .fini_array  : { PROVIDE_HIDDEN(__fini_array_start = .); KEEP(*(SORT(.fini_array.*))) KEEP(*(.fini_array*)) PROVIDE_HIDDEN(__fini_array_end = .); } >FLASH

 _sidata = LOADADDR(.data);
 .data : {
  . = ALIGN(4); _sdata = .;
  *(.data) *(.data*) *(.RamFunc) *(.RamFunc*)
  . = ALIGN(4); _edata = .;
 } >RAM_D1 AT> FLASH

 .bss : {
  . = ALIGN(4); _sbss = .; __bss_start__ = _sbss;
  *(.bss) *(.bss*) *(COMMON)
  . = ALIGN(4); _ebss = .; __bss_end__ = _ebss;
 } >RAM_D1

 /* Not initialized and not zeroed on reset. It survives software reset while
   SRAM is retained, but not a power loss. Used by volatile USB mode state. */
 .noinit (NOLOAD) : {
  . = ALIGN(4);
  *(.noinit) *(.noinit*)
  . = ALIGN(4);
 } >RAM_D1

 ._user_heap_stack : {
  . = ALIGN(8); PROVIDE ( end = . ); PROVIDE ( _end = . );
  . = . + _Min_Heap_Size;
  . = . + _Min_Stack_Size;
  . = ALIGN(8);
 } >DTCMRAM

 /* Keep libc/libm/libgcc sections required by newlib-nano and gcc runtime. */
 .ARM.attributes 0 : { *(.ARM.attributes) }
}
