

#include "ns430.h"

#include "ns430-atoi.h"
#include "ns430-uart.h"
#include "ns430-spi.h"

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
  __asm__ __volatile__("mov #0xffb0,r1"::);
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

    register int16_t data asm("r5");
    register int16_t *addr asm("r7");

    dint();


    PAOUT = 0x0001;
    PAOEN = 0x0001;
    PAPER = 0x000e;


    // check pin state
    if ((PADSR & (1 << 7)) == 0) {
        // msp4th interp
    } else {
        // load flash image into RAM

        // setup SPI mode 3, 8b transfers
        SPI0_CR = (SPI_CPHA | SPI_CPOL | SPI_EN);

        // /CS, pin[0] low
        PAOUT &= ~(1 << 0);

        // release flash from deep sleep
        SPI0_TDR = 0xab00;
        while ((SPI0_SR & TDRE) == 0) { /* wait */ }

        // change to 16-bit transfers
        SPI0_CR = (SPI_DL | SPI_CPHA | SPI_CPOL | SPI_EN);

        // /CS high
        PAOUT |= (1 << 0);

        // /CS low
        PAOUT &= ~(1 << 0);
        // send flash command
        // 0x03 - read data bytes
        // 0x00 - address MSB
        SPI0_TDR = 0x0300;
        SPI0_TDR = RAMStart;
        while ((SPI0_SR & TDRE) == 0) { /* wait */ }

        SPI0_TDR = 0x0000;
        while ((SPI0_SR & TDRE) == 0) { /* wait */ }

        SPI0_TDR = 0x0000;
        while ((SPI0_SR & TDRE) == 0) { /* wait */ }

        // clear status
        SPI0_SR = 0;

        for (addr=RAMStart; addr <= RAMStart+RAMSize-2; addr++) {
            while ((SPI0_SR & TDRE) == 0) { /* wait */ }

            // store data
            data = SPI0_RDR;
            // keep receiving (by sending zeros)
            SPI0_TDR = 0x0000;

            addr = data;
        }
    }



    // chip setup for UART0 use
    PAPER = 0x0030;
    PAOUT = 0x0000;
    PAOEN = 0x0010;  // set data direction registers

    UART0_BCR = UART_BCR(DEVBOARD_CLOCK, BAUDRATE);
    UART0_CR = UARTEn;

    // a read clears the register -- ready for TX/RX
    tmp = UART0_SR;


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


