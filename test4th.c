
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

int16_t *msp4th_mathStackStartAddress;
int16_t *msp4th_addrStackStartAddress;
int16_t *msp4th_prog;
int16_t *msp4th_progOpcodes;
uint8_t *msp4th_cmdList;
uint8_t *msp4th_lineBuffer;
int16_t msp4th_lineBufferLength;
uint8_t *msp4th_wordBuffer;
int16_t msp4th_wordBufferLength;

void (*msp4th_putchar)(uint8_t);
uint8_t (*msp4th_getchar)(void);



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

    msp4th_mathStackStartAddress = &mathStackArray[MATH_STACK_SIZE - 1];
    msp4th_addrStackStartAddress = &addrStackArray[ADDR_STACK_SIZE - 1];

    msp4th_prog = &progArray[0];

    msp4th_progOpcodes = &progOpcodesArray[0];

    msp4th_cmdList = &cmdListArray[0];

    msp4th_lineBuffer = &lineBufferArray[0];
    msp4th_lineBufferLength = LINE_BUFFER_SIZE;

    msp4th_wordBuffer = &wordBufferArray[0];
    msp4th_wordBufferLength = WORD_BUFFER_SIZE;


    for (i=0; i < MATH_STACK_SIZE; i++) {
        mathStackArray[i] = 0;
    }

    for (i=0; i < ADDR_STACK_SIZE; i++) {
        addrStackArray[i] = 0;
    }

    lineBufferArray[0] = 0;
    wordBufferArray[0] = 0;

    msp4th_putchar = &my_putchar;
    msp4th_getchar = &my_getchar;
}



int main(void)
{

    while (1) {
        config_msp4th();
        init_msp4th();
        processLoop();
    }

    return 0;
}
