#if !defined(__MSP4TH)
#define __MSP4TH

/*
 * Provide definitions for these arrays in user code and assign the
 * pointers, then call msp4th_init(&config).
 */

struct msp4th_config {
    int16_t *mathStackStartAddress;
    int16_t *addrStackStartAddress;
    int16_t *prog;
    int16_t *progOpcodes;
    uint8_t *cmdList;
    uint8_t *lineBuffer;
    int16_t lineBufferLength;
    uint8_t *wordBuffer;
    int16_t wordBufferLength;
    void (*putchar)(uint8_t);
    uint8_t (*getchar)(void);
};

void msp4th_init(struct msp4th_config *);
void msp4th_processLoop(void);

#endif
