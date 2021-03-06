

#include "ns430.h"

#include "ns430-atoi.h"
#include "ns430-uart.h"

#include "msp4th.h"



/*
 * Assumptions about clocking.  The affected register is the UART0_BCR.  See
 * "ns430-uart.h" or the ns430 documentation for the relationship between BCR,
 * the cpu clock rate, and the effective UART baud rate.
 *
 * A 12 MHz clock and a desired baudrate of 4800 yields a UARTx_CR value of 155
 * from the UART_BCR() macro.
 */
#define DEVBOARD_CLOCK 12000000L
#define BAUDRATE 4800L


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

//maximum input line length
#define LINE_BUFFER_SIZE 128

//maximum word character width
#define WORD_BUFFER_SIZE 32


const uint8_t chip_id[] = {
    "AMSP1 -> piranha -> cheetah\r\n"
    "{\r\n"
    " Balkir\r\n"
    " Gharzai\r\n"
    " Hoffman\r\n"
    " Schemm\r\n"
    " Schmitz\r\n"
    " White\r\n"
    "}\r\n"
    "2015 UNL\r\n"
    "cheetah\r\n"
};




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




static __inline__ void init_uart(void)
{
    // chip setup for UART0 use
    PAPER = 0x0030;
    PAOUT = 0x0000;
    PAOEN = 0x0010;  // set data direction registers

    UART0_BCR = UART_BCR(DEVBOARD_CLOCK, BAUDRATE);
    UART0_CR = UARTEn;

    // a read clears the register -- ready for TX/RX
    // NOTE: make sure the ASM actually reads this register
    //       and isn't "optimized" out.  So far, no problems...
    UART0_SR;
}




/*
 * The ".noinit" section should be placed so these vectors are the last part
 * of allocated RAM.  All space beyond, up until 0xff00, is empty or unused.
 * This keeps all the msp4th global variables in RAM in one continuous block.
 */
int16_t __attribute__ ((section(".noinit"))) mathStackArray[MATH_STACK_SIZE];
int16_t __attribute__ ((section(".noinit"))) addrStackArray[ADDR_STACK_SIZE];
int16_t __attribute__ ((section(".noinit"))) progArray[USER_PROG_SIZE];
int16_t __attribute__ ((section(".noinit"))) progOpcodesArray[USER_OPCODE_MAPPING_SIZE];
uint8_t __attribute__ ((section(".noinit"))) cmdListArray[USER_CMD_LIST_SIZE];
uint8_t __attribute__ ((section(".noinit"))) lineBufferArray[LINE_BUFFER_SIZE];
uint8_t __attribute__ ((section(".noinit"))) wordBufferArray[WORD_BUFFER_SIZE];

struct msp4th_config __attribute__ ((section(".noinit"))) default_config;


static __inline__ void setup_default_msp4th(void)
{
    default_config.mathStackStart = &mathStackArray[MATH_STACK_SIZE - 1];
    default_config.addrStackStart = &addrStackArray[ADDR_STACK_SIZE - 1];
    default_config.prog = &progArray[0];
    default_config.progOpcodes = &progOpcodesArray[0];
    default_config.cmdList = &cmdListArray[0];
    default_config.lineBuffer = &lineBufferArray[0];
    default_config.lineBufferLength = LINE_BUFFER_SIZE;
    default_config.wordBuffer = &wordBufferArray[0];
    default_config.wordBufferLength = WORD_BUFFER_SIZE;
    default_config.putchar = &uart_putchar;
    default_config.getchar = &uart_getchar;
    default_config.puts = &uart_puts;

    // stack top is zero
    mathStackArray[MATH_STACK_SIZE - 1] = 0;

    // terminate the strings
    lineBufferArray[0] = 0;
    //wordBufferArray[0] = 0;
    cmdListArray[0] =0;


    // howto execute a line of words on init
    /*
    uint8_t *str = (uint8_t *)"1 2 3 4 5 s.\r";
    for (i=0; i < 14; i++) {
        lineBufferArray[i] = str[i];
        lineBufferArray[i+1] = 0;
    }
    */
}



int main(void){
    /*
     * SPECIAL BOOTLOADER CALLING
     *
     * We *do not* want to 'call' the initial bootloader.  Such a call
     * instruction attempts to use the stack, which is in RAM -- whose health
     * is unknown.  The functionality must use ROM code and registers only.  *
     * Declaring as an inline function and .include'ing the ASM code is a hop
     * and skip to ensure this happens.
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
    // set a flag to generate the code with the proper constants
    // otherwise use the constants setup for testing
    asm(".set BOOTROM, 1");
#endif
    asm(".include \"flashboot.s\"");


    init_uart();

    /*
     * Startup and run msp4th interp.
     *
     * See config_default_msp4th() and "test4th.c" for examples of
     * re-configuring the program vector sizes and providing I/O functions.
     *
     * The following make processLoop() return:
     *  - executing the "exit" word
     *  - any EOT character in the input ('^D', control-D, 0x04)
     *  - any 0xff character in the input
     */
    int16_t x;

    while (1) {
        setup_default_msp4th();

        msp4th_init(&default_config);
        x = msp4th_processLoop();

        if (x == 42) {
            uart_puts((uint8_t *)chip_id);
        }
    }

    return 0;
}


