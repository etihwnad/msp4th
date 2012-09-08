

#include <stdio.h>
#include <signal.h>
#include <iomacros.h>

#include "ns430-atoi.h"
#include "ns430-uart.h"

#include "msp4th.h"


#define DEVBOARD_CLOCK 25000000L
#define BAUDRATE 4800L


NAKED(_reset_vector__){
  __asm__ __volatile__("br #main"::);
}


static void __inline__ delay(register unsigned int n){
  __asm__ __volatile__ (
      "1: \n"
      " dec     %[n] \n"
      " jne     1b \n"
      : [n] "+r"(n));
}


int main(void){

    uint16_t tmp;

    dint();

    PAPER = 0x0030;
    PAOUT = 0x0000;
    PAOEN = 0x0010;  // set data direction registers

    UART0_BCR = BCR(DEVBOARD_CLOCK, BAUDRATE);
    UART0_CR = UARTEn;

    tmp = UART0_SR;


    putchar('!');   

    char c;
    while (1) {
        c = getchar();
        if (c == '`')
            break;
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

    init_msp4th();
    processLoop();

    return 0;
}


