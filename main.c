

#include <msp430.h>
#include <stdint.h>

#include "msp4th.h"



// default on LaunchPad
#define SMCLK_FREQ 1000000L
#define USCI_CLOCK_FREQ SMCLK_FREQ
#define BAUDRATE 115200L


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
    "SHALL WE PLAY AGAIN?\r\n"
};


void led1(x)
{
    const uint8_t BIT = (1 << 0);

    if (x !=0) {
        P1OUT |= BIT;
    }
    else {
        P1OUT &= ~BIT;
    }
}


void led2(x)
{
    const uint8_t BIT = (1 << 7);

    if (x !=0) {
        P9OUT |= BIT;
    }
    else {
        P9OUT &= ~BIT;
    }
}





void board_setup(void)
{
    // Configure GPIO

    // LEDs are outputs
    P1DIR |= (0x01 << 0);  // LED1
    P9DIR |= (0x01 << 7);  // LED2

    // P1.1 is SW1 input
    P1REN |= (0x01 << 1);
    P1OUT |= (0x01 << 1);

    // P1.2 is SW2 input
    P1REN |= (0x01 << 2);
    P1OUT |= (0x01 << 2);


    // Done with all IO configuration.
    // as Captain Picard says:
    // ... "Make it so"

    // Disable the GPIO power-on default high-impedance mode to activate
    // previously configured port settings
    PM5CTL0 &= ~LOCKLPM5;


    /* Clock setup
     *
     * none, we are using the LaunchPad default setup, which is
     *    MCLK = 1 MHz
     *    SMCLK = 1 MHz
     */
}



void uart_setup(void)
{
    /*
     * MSP-EXPFR6989 LaunchPad
     * Application "backchannel" UART uses
     *  USCI_A1 connected via target pins
     *      P3.4_TXD
     *      P3.5_RXD
     */


    // Configure USCI_A1 for UART mode
    //  FUG section 30.3.1 recommended order
    // 1. Reset
    UCA1CTLW0 = UCSWRST;

    // 2. Init registers
    UCA1CTL1 |= UCSSEL__SMCLK;  // CLK = SMCLK

    // Baud rate setup
    // FUG section 30.3.10
    // must guard this section since it requires human action when changing
    // baud rate and/or module clock
#if (BAUDRATE == 115200L) && (USCI_CLOCK_FREQ == 1000000L)

    //  step 1.  1000000/115200 = 8.68
    //  step 2.  (skip step 3)
    UCA1BRW = 8;

    //  step 4.  1000000/115200 - INT(1000000/115200)=0.68
    // --> table lookup UCBRSx value = 0xD6
    UCA1MCTLW = 0xD600;
    // 2.end
#else
    #error Unknown BAUDRATE / USCI_CLOCK_FREQ combination
#endif

    // 3. Config ports
    //  MSP430FR698x datasheet slas789d
    //  Table 6-25, page 103
    // setup P3.4_TXD
    P3SEL0 &= ~BIT4;
    P3SEL1 |= BIT4;

    // setup P3.5_RXD
    P3SEL0 &= ~BIT4;
    P3SEL1 |= BIT4;

    // 4. Clear WCSWRST
    UCA1CTL1 &= ~UCSWRST;

    // 5. Enable interrupts
    UCA1IE |= UCRXIE;           // Enable USCI_A0 RX interrupt
}



// allocate space for interpreter variables
int16_t mathStackArray[MATH_STACK_SIZE];
int16_t addrStackArray[ADDR_STACK_SIZE];
int16_t progArray[USER_PROG_SIZE];
int16_t progOpcodesArray[USER_OPCODE_MAPPING_SIZE];
uint8_t cmdListArray[USER_CMD_LIST_SIZE];
uint8_t lineBufferArray[LINE_BUFFER_SIZE];
uint8_t wordBufferArray[WORD_BUFFER_SIZE];

struct msp4th_config default_config;



static void setup_default_msp4th(void)
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

    // Stop WDT
    WDTCTL = WDTPW | WDTHOLD;

    board_setup();
    uart_setup();


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
    int16_t ret;

    while (1) {
        setup_default_msp4th();

        msp4th_init(&default_config);
        ret = msp4th_processLoop();

        if (ret == 42) {
            uart_puts((uint8_t *)chip_id);
        }
    }

    return 0;
}

