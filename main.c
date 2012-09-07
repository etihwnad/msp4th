

#include <stdio.h>
#include <signal.h>
#include <iomacros.h>

#include "ns430-atoi.h"
#include "ns430-uart.h"

#include "msp4th.h"


int main(void){

  PAPER = 0x0030;
  PAOUT = 0x0000;
  PAOEN = 0x0010;  // set data direction registers

  init_msp4th();

  /*TMR0_CNT = 0x0000;*/
  /*TMR0_SR = 0;*/
  /*TMR0_RC = 1059;*/
  /*TMR0_CR = 0x003C;*/

  /* 8e6 / (16*19200) - 1 = 25.0416 */
  /* 8e6 / (16*2400) - 1 = 207.33 */
  /* 25e6 / (16*2400) - 1 = 207.33 */
  UART0_BCR = BCR(25000000L, 2400L);
  UART0_CR = UARTEn;

  dint();
  putchar('!');   

  while (1) {
      uint8_t c;
      c = getchar();
      if (c == '`') break;
      putchar(c);
  }

  putchar('t');
  putchar('e');
  putchar('s');
  putchar('t');
  putchar('i');
  putchar('n');
  putchar('g');
  puts("This is a test of the UART serial printing\r\nit really does work ...\r\n");

  processLoop();

  return 0;
}
