/* Reference constants defined in Constants.h */
TEXT_BASE = 0x00000000;
RODATA_BASE = 0x00400000;
DATA_BASE = 0x00800000;
BSS_BASE = 0x00C00000;
PROVIDE(FILESTORAGE_BASE = 0x01000000);
PROVIDE(FILESTORAGE_TOP = 0x07000000);
PROVIDE(STACK_TOP = 0x07FFFFFC);

PROVIDE(UART_BASE = 0x101F1000);  /* VersatilePB UART */

ENTRY(_start)

SECTIONS
{
    /* text section */
    . = TEXT_BASE;
    .text : {
        *(.text*)
    	*(.gnu.linkonce.t.*)
    	*(.glue_7) *(.glue_7t)
    	*(.vfp11_veneer)
    	*(.v4_bx)
    }

    /* read-only data */
    . = RODATA_BASE;
    .rodata : {
        *(.rodata*)
        *(.gnu.linkonce.r.*)
    }

    /* initialized data */
    . = DATA_BASE;
    .data : {
        *(.data*)
    }

    /* uninitialized data..... plus zeroed variable unfortunately. the compiler forces it, so i must zero the bss myself lol */
    .bss : {
        _sbss = .;          /* Start of .bss section */
        *(.bss*)
        *(COMMON)
        _ebss = .;          /* End of .bss section */
    }
}