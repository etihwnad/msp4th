#if !defined(__MSP4TH)
#define __MSP4TH

#if defined(MSP430)
extern volatile int16_t *mathStackStartAddress asm("__mathStackStartAddress");
extern volatile int16_t *addrStackStartAddress asm("__addrStackStartAddress");
extern volatile int16_t *progStartAddress asm("__progStartAddress");
extern volatile int16_t *progOpcodesStartAddress asm("__progOpcodesStartAddress");
extern volatile uint8_t *cmdListStartAddress asm("__cmdListStartAddress");
#else
extern volatile int16_t *mathStackStartAddress;
extern volatile int16_t *addrStackStartAddress;
extern volatile int16_t *progStartAddress;
extern volatile int16_t *progOpcodesStartAddress;
extern volatile uint8_t *cmdListStartAddress;
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
