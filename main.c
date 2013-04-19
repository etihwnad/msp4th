

#include "ns430.h"

#include "ns430-atoi.h"
#include "ns430-uart.h"

#include "msp4th.h"


#define DEVBOARD_CLOCK 12000000L
#define BAUDRATE 19200L


/*
 * Re-define the startup/reset behavior to this.
 *
 * By doing this, we take all initialization into our own hands.
 *
 *      YE BE WARNED
 */
void __attribute__ ((naked)) _reset_vector__(void) {
  __asm__ __volatile__("br #main"::);
}


/*
 * Burn power for no reason..
 *
 * TODO: document # of MCLK's per loop + overhead
 */
static void __inline__ delay(register unsigned int n){
  __asm__ __volatile__ (
      "1: \n"
      " dec     %[n] \n"
      " jne     1b \n"
      : [n] "+r"(n));
}


/*
 * Dis/Enable interrupts from C
 */
static void __inline__ dint(void) {
  __asm__ __volatile__ ( "dint" :: );
}

static void __inline__ eint(void){
  __asm__ __volatile__ ( "eint" :: );
}






int main(void){

    int16_t tmp;


    dint();


    // chip setup for UART0 use
    PAPER = 0x0030;
    PAOUT = 0x0000;
    PAOEN = 0x0010;  // set data direction registers

    UART0_BCR = BCR(DEVBOARD_CLOCK, BAUDRATE);
    UART0_CR = UARTEn;

    // a read clears the register -- ready for TX/RX
    tmp = UART0_SR;


    /*
     * Startup and run msp4th interp
     *
     * word "exit" makes processLoop() return
     */
    init_msp4th();
    processLoop();

    return 0;
}


