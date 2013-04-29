#if !defined(NS430_SPI)
#define NS430_SPI

#include "ns430.h"

/* SPIx_CR bits */
#define SPI_TDRE_IE     (1 << 0)
#define SPI_TXEMPTY_IE  (1 << 1)
#define SPI_RDRF_IE     (1 << 2)
#define SPI_OVER_IE     (1 << 3)
#define SPI_EN          (1 << 4)
#define SPI_CPOL        (1 << 5)
#define SPI_CPHA        (1 << 6)
#define SPI_DL          (1 << 7)

/* SPI_SCBR
 * 8-bit number
 * Baud = CLK / (2 * (1 + SBCR))
 * or
 * BCR = (CLK / (2 * Baud)) - 1
 */

#define SPI_SCBR(c, b) (((c / (2 * b)) - 1) << 8)

/* SPIx_SR bits */
#define TDRE        (1 << 0)
#define TXEMPTY     (1 << 1)
#define RDRF        (1 << 2)
#define OVER        (1 << 3)


#endif
