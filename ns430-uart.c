

#include <iomacros.h>

#include "ns430-atoi.h"
#include "ns430-uart.h"


int putchar(int c)
{
    int16_t i;
    /*while ((UART0_SR & TDRE) == 0) {*/
    while ((UART0_SR & (TDRE | TXEMPTY)) == 0) {
        // wait for register to clear
        i++;
    }
    UART0_TDR = c;
    return 0;
}

int getchar(void){
  uint8_t c;

  while ((UART0_SR & RDRF) == 0) {
      // wait for char
  }
  c = UART0_RDR & 0x00ff;
  return c;
} 

