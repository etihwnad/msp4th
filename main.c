

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




/*
 * Default msp4th settings
 */
#define MATH_STACK_SIZE 32
#define ADDR_STACK_SIZE 64

//total length of all user programs in opcodes
#define USER_PROG_SIZE 256

//max number of user-defined words
#define USER_OPCODE_MAPPING_SIZE 32

//total string length of all word names (+ 1x<space> each)
#define USER_CMD_LIST_SIZE 128

/*
 * The ".lastram" section should be placed so these vectors are the last part
 * of allocated RAM.  All space beyond, up until 0xff00, is empty or unused.
 */
int16_t __attribute__ ((section(".noinit"))) mathStackArray[MATH_STACK_SIZE];
int16_t __attribute__ ((section(".noinit"))) addrStackArray[ADDR_STACK_SIZE];
int16_t __attribute__ ((section(".noinit"))) progArray[USER_PROG_SIZE];
int16_t __attribute__ ((section(".noinit"))) progOpcodesArray[USER_OPCODE_MAPPING_SIZE];
uint8_t __attribute__ ((section(".noinit"))) cmdListArray[USER_CMD_LIST_SIZE];


void config_default_msp4th(void)
{
    int16_t i;

    mathStackStartAddress = (uint16_t)&mathStackArray[MATH_STACK_SIZE - 1];
    addrStackStartAddress = (uint16_t)&addrStackArray[ADDR_STACK_SIZE - 1];
    progStartAddress = (uint16_t)&progArray[0];
    progOpcodesStartAddress = (uint16_t)&progOpcodesArray[0];
    cmdListStartAddress = (uint16_t)&cmdListArray[0];


    for (i=0; i < MATH_STACK_SIZE; i++) {
        mathStackArray[i] = 0;
    }

    for (i=0; i < ADDR_STACK_SIZE; i++) {
        addrStackArray[i] = 0;
    }
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
    config_default_msp4th();

    while (1) {
        init_msp4th();
        processLoop();
    }

    return 0;
}


