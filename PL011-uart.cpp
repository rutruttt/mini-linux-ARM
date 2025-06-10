#include "uart.h"
#include <cstdint>

extern "C" {
    extern uint8_t UART_BASE[];  /* Provided by linker */
}
//UART_BASE is a pointer. for some reason when i initialize like a pointer
//extern uint8_t* UART_BASE, or like a regular number in the size of a pointer extern uint32_t UART_BASE
//it is still 0 EVEN DURING RUNTIME. so an array solves it for some reason...
//also as a pointer (not a number) it DOES have to be a 1-byte pointer, otherwise the arithmetic "UART_BASE + 0x18" wont work

/* Register offsets for PL011 */
volatile uint8_t* UARTDR;
volatile uint32_t* UARTFR;

#define TXF_FULL  (1 << 5)
#define RXF_EMPTY (1 << 4)

void init_uart() {

    // Now calculate pointers
    UARTDR = (volatile uint8_t*)(UART_BASE + 0x00);
    UARTFR = (volatile uint32_t*)(UART_BASE + 0x18);
}


void write_char_uart(char c) {
    while (*UARTFR & TXF_FULL);
    *UARTDR = c;
}

char read_char_uart() {
    while (*UARTFR & RXF_EMPTY);
    return *UARTDR;
}