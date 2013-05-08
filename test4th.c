
#include <stdio.h>
#include <stdint.h>

#include "msp4th.h"



#define MATH_STACK_SIZE 32
#define ADDR_STACK_SIZE 64
#define CMD_LIST_SIZE 128
#define PROG_SPACE 256
#define USR_OPCODE_SIZE 32

int16_t mathStackArray[MATH_STACK_SIZE];
int16_t addrStackArray[ADDR_STACK_SIZE];
int16_t progArray[USR_OPCODE_SIZE];
int16_t progOpcodesArray[USR_OPCODE_SIZE];
uint8_t cmdListArray[CMD_LIST_SIZE];

volatile int16_t *mathStackStartAddress;
volatile int16_t *addrStackStartAddress;
volatile int16_t *progStartAddress;
volatile int16_t *progOpcodesStartAddress;
volatile uint8_t *cmdListStartAddress;

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

    mathStackStartAddress = &mathStackArray[MATH_STACK_SIZE - 1];
    addrStackStartAddress = &addrStackArray[ADDR_STACK_SIZE - 1];
    progStartAddress = &progArray[0];
    progOpcodesStartAddress = &progOpcodesArray[0];
    cmdListStartAddress = &cmdListArray[0];

    for (i=0; i < MATH_STACK_SIZE; i++) {
        mathStackArray[i] = 0;
    }

    for (i=0; i < ADDR_STACK_SIZE; i++) {
        addrStackArray[i] = 0;
    }

    msp4th_putchar = &my_putchar;
    msp4th_getchar = &my_getchar;
}



int main(void)
{
    config_msp4th();

    init_msp4th();
    processLoop();

    return 0;
}
