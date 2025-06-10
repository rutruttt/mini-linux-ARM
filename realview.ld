/* Reference constants defined in Constants.h */
TEXT_BASE = 0x00010000;
RODATA_BASE = 0x00400000;
DATA_BASE = 0x00800000;
BSS_BASE = 0x00C00000;
PROVIDE(STACK_TOP = 0x00FFFFFC);
PROVIDE(FILESTORAGE_BASE = 0x01000000);
PROVIDE(FILESTORAGE_TOP = 0x07000000);

PROVIDE(UART_BASE = 0x10009000);  

ENTRY(_start)

SECTIONS
{
    /* Text section */
    . = TEXT_BASE;
    .text : {
    	*(.text*)
    	*(.gnu.linkonce.t.*)
    	*(.glue_7) *(.glue_7t)
    	*(.vfp11_veneer)
    	*(.v4_bx)
    }

    /* Read-only data */
    . = RODATA_BASE;
    .rodata : {
        *(.rodata*)
        *(.gnu.linkonce.r.*)
    }

    /* Initialized data */
    . = DATA_BASE;
    .data : {
        *(.data*)
    }

    /* Uninitialized data */
    . = BSS_BASE;
    .bss : {
        *(.bss*)
        *(COMMON)
    }
}
