

#include "ns430-atoi.h"
#include "ns430-uart.h"


void uart_putchar(uint8_t c)
{
    while ((UART0_SR & (UART_TDRE | UART_TXEMPTY)) == 0) {
        // wait for register to clear
    }
    UART0_TDR = (c & 0xff);
}

uint8_t uart_getchar(void)
{
    uint8_t c;

    while ((UART0_SR & UART_RDRF) == 0) {
        // wait for char
    }
    c = (UART0_RDR & 0x00ff);
    return c;
} 


void uart_puts(uint8_t *s)
{
    uint16_t i = 0;
    uint8_t c = 1;

    while (c != 0) {
        c = s[i++];
        uart_putchar(c);
    }
    uart_putchar('\r');
    uart_putchar('\n');
}

