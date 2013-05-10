
#include <stdio.h>
#include <stdint.h>

#include "msp4th.h"



#define MATH_STACK_SIZE 32
#define ADDR_STACK_SIZE 64
#define CMD_LIST_SIZE 128
#define PROG_SPACE 256
#define USR_OPCODE_SIZE 32
#define LINE_BUFFER_SIZE 128
#define WORD_BUFFER_SIZE 32


/*
 * Create storage for user-provided arrays, variables, and I/O function
 * pointers.
 */
int16_t mathStackArray[MATH_STACK_SIZE];
int16_t addrStackArray[ADDR_STACK_SIZE];
int16_t progArray[USR_OPCODE_SIZE];
int16_t progOpcodesArray[USR_OPCODE_SIZE];
uint8_t cmdListArray[CMD_LIST_SIZE];
uint8_t lineBufferArray[CMD_LIST_SIZE];
uint8_t wordBufferArray[CMD_LIST_SIZE];

struct msp4th_config default_config;

/*
int16_t *msp4th_mathStackStartAddress;
int16_t *msp4th_addrStackStartAddress;
int16_t *msp4th_prog;
int16_t *msp4th_progOpcodes;
uint8_t *msp4th_cmdList;
uint8_t *msp4th_lineBuffer;
int16_t msp4th_lineBufferLength;
uint8_t *msp4th_wordBuffer;
int16_t msp4th_wordBufferLength;
*/

//void (*msp4th_putchar)(uint8_t);
//uint8_t (*msp4th_getchar)(void);



void my_putchar(uint8_t c)
{
    putchar((char)c);
}


uint8_t my_getchar(void)
{
    return (uint8_t)getchar();
}




void config_msp4th(void)
{
    int16_t i;

    default_config.mathStackStartAddress = &mathStackArray[MATH_STACK_SIZE - 1];
    default_config.addrStackStartAddress = &addrStackArray[ADDR_STACK_SIZE - 1];
    default_config.prog = &progArray[0];
    default_config.progOpcodes = &progOpcodesArray[0];
    default_config.cmdList = &cmdListArray[0];
    default_config.lineBuffer = &lineBufferArray[0];
    default_config.lineBufferLength = LINE_BUFFER_SIZE;
    default_config.wordBuffer = &wordBufferArray[0];
    default_config.wordBufferLength = WORD_BUFFER_SIZE;
    default_config.putchar = &my_putchar;
    default_config.getchar = &my_getchar;

    // terminate the strings
    lineBufferArray[0] = 0;
    wordBufferArray[0] = 0;
    cmdListArray[0] = 0;


    // howto execute a line of words on init
    /*
    uint8_t *str = (uint8_t *)"1 2 3 4 5 s.\r";
    for (i=0; i < 14; i++) {
        lineBufferArray[i] = str[i];
        lineBufferArray[i+1] = 0;
    }
    */
}



int main(void)
{
    config_msp4th();

    msp4th_init(&default_config);
    msp4th_processLoop();

    return 0;
}
