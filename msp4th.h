#if !defined(__MSP4TH)
#define __MSP4TH

/*
 * Provide definitions for these arrays in user code and assign the
 * pointers, then call msp4th_init(&config).
 */

struct msp4th_config {
    int16_t *mathStackStart;
    int16_t *addrStackStart;
    int16_t *prog;
    int16_t *progOpcodes;
    uint8_t *cmdList;
    uint8_t *lineBuffer;
    int16_t lineBufferLength;
    uint8_t *wordBuffer;
    int16_t wordBufferLength;
    void (*putchar)(uint8_t);
    uint8_t (*getchar)(void);
    void (*puts)(uint8_t *);
};

void msp4th_init(struct msp4th_config *);
int16_t msp4th_processLoop(void);

/* Suppress specific warnings (callX words use function pointers)
 *
 * from:
 * Suppressing GCC Warnings, by Patrick Horgan
 * http://dbp-consulting.com/tutorials/SuppressingGCCWarnings.html
 */

#if !defined(MSP430) && ((__GNUC__ * 100) + __GNUC_MINOR__) >= 402
#define GCC_DIAG_STR(s) #s
#define GCC_DIAG_JOINSTR(x,y) GCC_DIAG_STR(x ## y)
# define GCC_DIAG_DO_PRAGMA(x) _Pragma (#x)
# define GCC_DIAG_PRAGMA(x) GCC_DIAG_DO_PRAGMA(GCC diagnostic x)
# if ((__GNUC__ * 100) + __GNUC_MINOR__) >= 406
#  define GCC_DIAG_OFF(x) GCC_DIAG_PRAGMA(push) \
        GCC_DIAG_PRAGMA(ignored GCC_DIAG_JOINSTR(-W,x))
#  define GCC_DIAG_ON(x) GCC_DIAG_PRAGMA(pop)
# else
#  define GCC_DIAG_OFF(x) GCC_DIAG_PRAGMA(ignored GCC_DIAG_JOINSTR(-W,x))
#  define GCC_DIAG_ON(x)  GCC_DIAG_PRAGMA(warning GCC_DIAG_JOINSTR(-W,x))
# endif
#else
# define GCC_DIAG_OFF(x)
# define GCC_DIAG_ON(x)
#endif

#endif
