
#include <stdio.h>
#include <stdint.h>

#include "msp4th.h"



/*
 * msp4th settings
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


/*
 * Create storage for user-provided arrays, variables, and I/O function
 * pointers.
 */
int16_t mathStackArray[MATH_STACK_SIZE];
int16_t addrStackArray[ADDR_STACK_SIZE];
int16_t progArray[USER_PROG_SIZE];
int16_t progOpcodesArray[USER_OPCODE_MAPPING_SIZE];
uint8_t cmdListArray[USER_CMD_LIST_SIZE];
uint8_t lineBufferArray[LINE_BUFFER_SIZE];
uint8_t wordBufferArray[WORD_BUFFER_SIZE];

struct msp4th_config config;




void my_putchar(uint8_t c)
{
    putchar((char)c);
}


uint8_t my_getchar(void)
{
    return (uint8_t)getchar();
}


void my_puts(uint8_t *s)
{
    puts((char *)s);
}



void config_msp4th(void)
{
    config.mathStackStart = &mathStackArray[MATH_STACK_SIZE - 1];
    config.addrStackStart = &addrStackArray[ADDR_STACK_SIZE - 1];
    config.prog = &progArray[0];
    config.progOpcodes = &progOpcodesArray[0];
    config.cmdList = &cmdListArray[0];
    config.lineBuffer = &lineBufferArray[0];
    config.lineBufferLength = LINE_BUFFER_SIZE;
    config.wordBuffer = &wordBufferArray[0];
    config.wordBufferLength = WORD_BUFFER_SIZE;
    config.putchar = &my_putchar;
    config.getchar = &my_getchar;
    config.puts = &my_puts;

    // stack top is zero
    mathStackArray[MATH_STACK_SIZE - 1] = 0;

    // terminate the strings
    lineBufferArray[0] = 0;
    //wordBufferArray[0] = 0;
    cmdListArray[0] = 0;


    // howto execute a line of words on init
    /*
    int16_t i;
    uint8_t *str = (uint8_t *)"1 2 3 4 5 s.\r";
    for (i=0; i < 14; i++) {
        lineBufferArray[i] = str[i];
        lineBufferArray[i+1] = 0;
    }
    */
}


uint8_t chip_id[] = "msp4th, PC edition\r\n";

int main(void)
{
    int16_t x;
    config_msp4th();

    msp4th_init(&config);
    x = msp4th_processLoop();

    if (x == 42) {
        my_puts(chip_id);
    }

    return 0;
}
