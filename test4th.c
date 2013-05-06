

#include "ns430.h"
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

volatile uint16_t mathStackStartAddress;
volatile uint16_t addrStackStartAddress;
volatile uint16_t progStartAddress;
volatile uint16_t progOpcodesStartAddress;
volatile uint16_t cmdListStartAddress;


void config_msp4th(void)
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



int main(void)
{
    config_msp4th();

    init_msp4th();
    processLoop();

    return 0;
}
