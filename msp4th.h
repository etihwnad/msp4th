#if !defined(__MSP4TH)
#define __MSP4TH

void init_msp4th(void);
void processLoop(void);

uint8_t getKeyB(void);
void getLine(void);
void getWord(void);
void listFunction(void);
int16_t popMathStack(void);
void pushMathStack(int16_t n);
int16_t popAddrStack(void);
void pushAddrStack(int16_t n);
int16_t lookupToken(volatile uint8_t *x, volatile uint8_t *l);
void luFunc(void);
void numFunc(void);
void ifFunc(int16_t x);
void pushnFunc(void);
void overFunc(void);
void dfnFunc(void);
void printNumber(int16_t n);
void printHexChar(int16_t n);
void printHexByte(int16_t n);
void printHexWord(int16_t n);
void execN(int16_t n);
void execFunc(void);
#endif
