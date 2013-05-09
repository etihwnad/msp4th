#if !defined(__MSP4TH)
#define __MSP4TH

#if defined(MSP430)
extern int16_t *msp4th_mathStackStartAddress asm("__msp4th_mathStackStartAddress");
extern int16_t *msp4th_addrStackStartAddress asm("__msp4th_addrStackStartAddress");
extern int16_t *msp4th_prog asm("__msp4th_prog");
extern int16_t *msp4th_progOpcodes asm("__msp4th_progOpcodes");
extern uint8_t *msp4th_cmdList asm("__msp4th_cmdList");
extern uint8_t *msp4th_lineBuffer asm("__msp4th_lineBuffer");
extern int16_t msp4th_lineBufferLength asm("__msp4th_lineBufferLength");
extern uint8_t *msp4th_wordBuffer asm("__msp4th_wordBuffer");
extern int16_t msp4th_wordBufferLength asm("__msp4th_wordBufferLength");
#else
extern int16_t *msp4th_mathStackStartAddress;
extern int16_t *msp4th_addrStackStartAddress;
extern int16_t *msp4th_prog;
extern int16_t *msp4th_progOpcodes;
extern uint8_t *msp4th_cmdList;
extern uint8_t *msp4th_lineBuffer;
extern int16_t msp4th_lineBufferLength;
extern uint8_t *msp4th_wordBuffer;
extern int16_t msp4th_wordBufferLength;
#endif

/*
 * Provide definitions for these functions in user code and assign the function
 * pointer before calling init_msp4th()
 */
extern void (*msp4th_putchar)(uint8_t c);
extern uint8_t (*msp4th_getchar)(void);

void init_msp4th(void);
void processLoop(void);

#endif
