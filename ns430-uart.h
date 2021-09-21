#if !defined(NS430_UART)
#define NS430_UART

#include "ns430.h"

/* UARTx_CR bits */
#define UART_TDRE_IE     (1 << 0)
#define UART_TXEMPTY_IE  (1 << 1)
#define UART_RDRF_IE     (1 << 2)
#define UART_OVER_IE     (1 << 3)
#define UART_PE_IE       (1 << 4)
#define UART_FE_IE       (1 << 5)
#define UART_Parity      (1 << 6)
#define UARTEn           (1 << 7)
#define UART_LLEN        (1 << 9)
#define UART_PAR_En      (1 << 10)

/* UARTx_BCR
 * 11-bit number
 * Baud = CLK / (16 * (1 + BCR))
 * or
 * BCR = (CLK / (16 * Baud)) - 1
 */

#define UART_BCR(c, b) ((c / (16 * b)) - 1)

/* UARTx_SR bits */
#define UART_TDRE        (1 << 0)
#define UART_TXEMPTY     (1 << 1)
#define UART_RDRF        (1 << 2)
#define UART_OVER        (1 << 3)
#define UART_PE          (1 << 4)
#define UART_FE          (1 << 5)

void uart_putchar(uint8_t c);
uint8_t uart_getchar(void);
void uart_puts(uint8_t *s);


#endif
