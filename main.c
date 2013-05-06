

#include "ns430.h"

#include "ns430-atoi.h"
#include "ns430-uart.h"
#include "ns430-spi.h"

#include "msp4th.h"


#define DEVBOARD_CLOCK 12000000L
#define BAUDRATE 19200L


/*
 * Re-define the startup/reset behavior to this.  GCC normally uses this
 * opportunity to initialize all variables (bss) to zero.
 *
 * By doing this, we take all initialization into our own hands.
 *
 *      YE BE WARNED
 */
void __attribute__ ((naked)) _reset_vector__(void) {
  __asm__ __volatile__("mov #0xff00,r1"::);
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




void init_uart(void)
{
    int16_t tmp;
    // chip setup for UART0 use
    PAPER = 0x0030;
    PAOUT = 0x0000;
    PAOEN = 0x0010;  // set data direction registers

    UART0_BCR = UART_BCR(DEVBOARD_CLOCK, BAUDRATE);
    UART0_CR = UARTEn;

    // a read clears the register -- ready for TX/RX
    tmp = UART0_SR;
}





int main(void){
    /*
     * SPECIAL BOOTLOADER CALLING
     *
     * We *do not* want to 'call' the initial bootloader.  Such a call instruction
     * attempts to use the stack, which is in RAM -- whose health is unknown.  The
     * functionality must use ROM code and registers only.  * Declaring as an
     * inline function and .include'ing the ASM code is a hop and skip to ensure
     * this happens.
     *
     * As a consequence, the ASM file must use local labels only -- of the form
     *  N:
     *      mov foo, bar
     *
     *  where N is an integer, referenced as
     *
     *      jmp Nb
     *
     *  where the label is searched for forwards or backwards according to the
     *  suffix Nf or Nb.
     */
#ifdef BOOTROM
    asm(".set BOOTROM, 1");
#endif
    asm(".include \"flashboot.s\"");


    dint();
    init_uart();

    /*
     * Startup and run msp4th interp
     *
     * word "exit" makes processLoop() return
     */
    while (1) {
        init_msp4th();
        processLoop();
    }

    return 0;
}


