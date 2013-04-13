

#include "ns430.h"

#include "ns430-atoi.h"
#include "ns430-uart.h"

#include "msp4th.h"


#define DEVBOARD_CLOCK 12000000L
#define BAUDRATE 19200L


void __attribute__ ((naked)) _reset_vector__(void) {
  __asm__ __volatile__("br #main"::);
}


static void __inline__ delay(register unsigned int n){
  __asm__ __volatile__ (
      "1: \n"
      " dec     %[n] \n"
      " jne     1b \n"
      : [n] "+r"(n));
}


static void __inline__ dint(void) {
  __asm__ __volatile__ ( "dint" :: );
}

static void __inline__ eint(void){
  __asm__ __volatile__ ( "eint" :: );
}






int main(void){

    uint16_t tmp;

    volatile int16_t *dirMemory;
    dirMemory = 0;

    dint();


    // chip setup for UART0 use
    PAPER = 0x0030;
    PAOUT = 0x0000;
    PAOEN = 0x0010;  // set data direction registers

    UART0_BCR = BCR(DEVBOARD_CLOCK, BAUDRATE);
    UART0_CR = UARTEn;

    // a read clears the register
    tmp = UART0_SR;




    /*
    uart_puts((uint8_t *)"UART echo mode, ` to exit:");
    char c;
    while (1) {
        c = uart_getchar();
        if (c == '`')
            break;
        uart_putchar(c);
    }
    */



    /*
     * dump contents of all RAM
     */
    volatile uint16_t *addr;

    uart_puts((uint8_t *)"Memory dump:");
    uart_putchar('\r');
    uart_putchar('\n');

    for (addr = (uint16_t *)0x4000; addr < (uint16_t *)0xfffe; addr++) {
        printHexWord((int16_t)addr);
        uart_putchar(' ');
        printHexWord((int16_t)*addr);
        uart_putchar('\r');
        uart_putchar('\n');
    }






    /*
     * memtest starting at end of program code
     */

    const int16_t patterns[] = {
        0x0000,
        0x0001,
        0x0002,
        0x0004,
        0x0008,
        0x0010,
        0x0020,
        0x0040,
        0x0080,
        0x0100,
        0x0200,
        0x0400,
        0x0800,
        0x1000,
        0x2000,
        0x4000,
        0x8000,
        0xaaaa,
        0x5555,
        0xffff,
        0x0000,
    };

    volatile int16_t idx;
    volatile int16_t readback;
    volatile int16_t pattern;

    const int16_t dend = (int16_t)0x51f0;
    const int16_t memend = (int16_t)0xff80;

    uart_puts((uint8_t *)"*** Memtest patterns ***");

    printHexWord((int16_t)dend);
    uart_puts((uint8_t *)" -- start address");

    printHexWord((int16_t)memend);
    uart_puts((uint8_t *)" -- end address");

    for (idx=0; idx < sizeof(patterns)/sizeof(int16_t); idx++) {
        pattern = patterns[idx];

        printHexWord(pattern);
        uart_puts((uint8_t *)" -- write pattern");

        // write pattern to all locations
        for (addr = (uint16_t *)dend; addr < (uint16_t *)memend; addr++) {
            *addr = (uint16_t)pattern;
        }

        printHexWord(pattern);
        uart_puts((uint8_t *)" -- readback pattern");

        // readback pattern
        for (addr = (uint16_t *)dend; addr < (uint16_t *)memend; addr++) {
            readback = (int16_t)*addr;

            if (readback != pattern) {
                printHexWord((int16_t)addr);
                uart_putchar(' ');
                printHexWord(pattern);
                uart_putchar(' ');
                printHexWord(readback);
                uart_putchar('\r');
                uart_putchar('\n');
            }
        }
    }

    uart_puts((uint8_t *)"*** Memtest done ***");

    init_msp4th();
    processLoop();

    return 0;
}


