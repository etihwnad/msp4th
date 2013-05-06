#if !defined(__MSP4TH)
#define __MSP4TH

#if defined(MSP430)
extern volatile uint16_t mathStackStartAddress asm("__mathStackStartAddress");
extern volatile uint16_t addrStackStartAddress asm("__addrStackStartAddress");
extern volatile uint16_t progStartAddress asm("__progStartAddress");
extern volatile uint16_t progOpcodesStartAddress asm("__progOpcodesStartAddress");
extern volatile uint16_t cmdListStartAddress asm("__cmdListStartAddress");
#else
extern volatile uint16_t mathStackStartAddress;
extern volatile uint16_t addrStackStartAddress;
extern volatile uint16_t progStartAddress;
extern volatile uint16_t progOpcodesStartAddress;
extern volatile uint16_t cmdListStartAddress;
#endif

void init_msp4th(void);
void processLoop(void);

#endif
