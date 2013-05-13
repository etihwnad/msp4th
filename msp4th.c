/*
 *  msp4th
 *
 *  forth-like interpreter for the msp430
 *
 *  Source originally from Mark Bauer, beginning life in the z80 and earlier.
 *  Nathan Schemm used and modified for use in his series of msp430-compatible
 *  processors.
 *
 *  This version by Dan White (2013).  Cleaned-up, expanded, and given
 *  capabilities to allow live re-configuration and directly calling user C
 *  functions.
 *
 *  * Used in Dan's "atoi" chip loaded from flash into RAM.
 *  * Fabbed in ROM as part of the Gharzai/Schmitz "piranha" imager chip.
 *
 *  If cpp symbol "MSP430" is not defined, it compiles to a version for testing
 *  on PC as a console program via the setup and main() in "test430.c".
 *
 * TODO:
 *  - use enum/symbols for VM opcodes (?builtins tagged as negative numbers?)
 *
 */


#if defined(MSP430)
#include "ns430.h"
#include "ns430-atoi.h"

// our "special" pointer, direct word access to all absolute address space
#define dirMemory ((int16_t *) 0)

#else
// mixins to test msp4th on PC
#include <stdint.h>
// the OS does not like direct, absolute, memory addressing...
// just create a sham space for testing
int16_t dirMemory[65536];

#endif


#include "msp4th.h"


/****************************************************************************
 *
 * Module-level global variables (in RAM)
 *
 ***************************************************************************/
int16_t xit;  // set to 1 to kill program
int16_t echo; // boolean: false -> no interactive echo/prompt

uint16_t progCounter;  // value determines builtin/user and opcode to execute

int16_t lineBufferIdx; // input line buffer index
int16_t progIdx;       // next open space for user opcodes
int16_t cmdListIdx;    // next open space for user word strings

/* The following allow re-configuring the location/size of all configurable
 * arrays.  Then the stack sizes and user program space sizes can be
 * (re-)specified by changing the table and calling init_msp4th() again.  See
 * test430.c for a configuration example.
 *
 * THE ONLY BOUNDARY CHECKS ARE:
 *  - math stack underflow
 *  - line buffer overflow
 *  - word buffer overflow
 *
 * The user is therefore responsible for:
 *  - keeping stack depths in range
 *  - not underflowing the address stack
 *  - allocating sufficient space in prog[], progOpcodes[], cmdList[]
 */
#if defined(MSP430)
/* Register notes.
 * r0-3 are special (pc, sp, sr, cg)
 * In mspgcc:
 *  r4-5 are (sometimes) used as frame and argument pointers.
 *  r12-15 are call clobbered.
 *  * Function arguments are allocated left-to-right assigned r15--r12, others
 *    are passed on the stack.
 *  * Return values are passed in r15, r15:r14, r15:r14:r13, or r15:r14:r13:r12
 *    depending on size.
 */
int16_t register *mathStackPtr asm("r6"); // used enough to keep in registers
int16_t register *addrStackPtr asm("r7");
#else
int16_t *mathStackPtr;
int16_t *addrStackPtr;
#endif

int16_t *mathStackStart; // original locations for calculating stack depth
int16_t *addrStackStart;

int16_t *prog;           // user programs (opcodes) are placed here

int16_t *progOpcodes;    // mapping between cmdList word index and program
                         // opcodes start index into prog[]

uint8_t *cmdList;        // string containing user-defined words

uint8_t *lineBuffer;     // where interactive inputs are buffered
int16_t lineBufferLength;// to prevent overflow

uint8_t *wordBuffer;     // the currently-parsed word
int16_t wordBufferLength;

void (*msp4th_putchar)(uint8_t c); // the sole interactive output avenue
uint8_t (*msp4th_getchar)(void);   // the sole interactive input avenue
void (*msp4th_puts)(uint8_t *s); // the sole interactive output avenue




/****************************************************************************
 *
 * Module-level global constants (in ROM)
 *
 ***************************************************************************/

// The order matches the execN function and determines the opcode value.
// NOTE: must end in a space !!!!
const uint8_t cmdListBi[] = {
              "bye + - * /% "                       // 1 -> 5
              ". dup drop swap < "                  // 6 -> 10
              "> == hb. gw dfn "                    // 11 -> 15
              "abs , p@ p! not "                    // 16 -> 20
              "list if then else begin "            // 21 -> 25
              "until depth h. ] num "               // 26 -> 30
              "push0 goto exec lu pushn "           // 31 -> 35
              "over push1 pwrd emit ; "             // 36 -> 40
              "@ ! h@ do loop "                     // 41 -> 45
              "+loop i j k ~ "                      // 46 -> 50
              "^ & | */ key "                       // 51 -> 55
              "cr 2* 2/ call0 call1 "               // 56 -> 60
              "call2 call3 call4 ndrop swpb "       // 61 -> 65
              "+! roll pick tuck max "              // 66 -> 70
              "min s. sh. neg echo "                // 71 -> 75
              "init o2w "                           // 76 -> 77
             };
#define LAST_PREDEFINED 77	// update this when we add commands to the built in list


// these commands are interps
const uint8_t cmdListBi2[] = {"[ : var "};

// these values point to where in progBi[] these routines start
const int16_t cmdList2N[] = {0,10000,10032,10135};  // need an extra zero at the front


// to flag the initial built in functions from the rest, save the negative of them in the program space (prog).

const int16_t progBi[] = { // address actually start at 10000

   // this is the monitor in compiled forth code (by hand)

   20025,        //   0 begin
   20014,        //   1 gw      get word
   20030,        //   2 num     test if number
   20022,10008,  //   3 if
 
   20031,        //   5 push0    push a zero on math stack
   20032,10030,  //   6 goto     jump to until function

   20008,        //   8 drop
   20034,        //   9 lu       look up word
   20022,10026,  //  10 if       did we find the word in the dictionary
   
   20035,']',    //  12 pushn    next value on math stack  look for ]

   20036,        //  14 over
   20012,        //  15 equal    test if function was a ']'
   20022,10022,  //  16 if

   20008,        //  18 drop     it was the ']' exit function
   20037,        //  19 push1    put a true on the math stack 
   20032,10030,  //  20 goto     jump to until func

   20033,        //  22 exec     execute the function on the math stack (it is a call so we return to here)
   20031,        //  23 push0
   20032,10030,  //  24 goto     jump to until func
   
   // undefined string
   
   20035,'?',    //  26 pushn    put the '?' on math stack
   20039,        //  28 emit     output the ? to the terminal
   20031,        //  29 push0
   
   20026,        //  30 until
   20040,        //  31 return function   



   // this is the ':' function hand compiled
   
   20035,0x5555, //  32 just push a known value on the stack, will test at the end
   20014,        //  34 get a word from the input

   20015,        //  35 define it
   20025,        //  36 begin

   20014,        //  37 get a word 
   20030,        //  38 see if number
   20022,10047,  //  39 if
   
   // it is a number
   
   20035,20035,  //  41 put the push next number opcode on stack
   20017,        //  43 put that opcode in the def
   20017,        //  44 put the actual value next
   20031,        //  45 push 0
   20026,        //  46 until     // we can have many untils for one begin
   
   // wasn't a number, we need to test for many other things

   20008,        //  47 drop   
   20034,        //  48 look in dictionary
   20020,        //  49 not


   20022,10058,  //  50 if        not found .... let them know and just ignore
   20035,'?',    //  52 push a '?' on the stack
   20039,        //  54 emit
   20038,        //  55 tell them what we couldn't find
   20031,        //  56 push0
   20026,        //  57 until
   
   // we found it in the dictionary
   
   20035,20022,  //  58 pushn     see if it is an if function
   20036,        //  60 over
   20012,        //  61 equal
   20022,10070,  //  62 if
   
   // it is an if function

   20017,        //  64 append the if statement to the stack (it was still on the stack
   20043,        //  65 h@ get location of next free word
   20007,        //  66 dup    ( leave a copy on the math stack for the "then" statement
   20017,        //  67 append it to memory
   20031,        //  68 push0
   20026,        //  69 until
   
   // ********************** 
     
   20035,20024,  //  70 pushn     see if it is an "else" function
   20036,        //  72 over
   20012,        //  73 equal
   20022,10088,  //  74 if
   
    //  it is an "else" statement
    
   20035,20032,  //  76 push a goto command on the math stack
   20017,        //  78 append it to the program
   20043,        //  79 h@ get location of next free word
   20009,        //  80 swap
   20017,        //  81 append
   20009,        //  82 swap
   20043,        //  83 h@
   20009,        //  84 swap
   20019,        //  85 !    this will be in prog space
   20031,        //  86 push0
   20026,        //  87 until
   
   // *******************************   

   20035,20023,  //  88 pushn    see if it is a "then" function

   20036,        //  90 over
   20012,        //  91 equal    test if function was a 'then'
   20022,10100,  //  92 if

      // it is a "then"

   20008,        //  94 drop
   20043,        //  95 h@
   20009,        //  96 swap
   20019,        //  97 !
   20031,        //  98 push0
   20026,        //  99 until
   
   // *********************************
   
   20035,10001,  // 100 pushn    see if it is a "[" function

   20036,        // 102 over
   20012,        // 103 equal   
   20022,10109,  // 104 if

      // it is a "["
   
   10001,        // 106 recurse into the monitor
   20031,        // 107 push0
   20026,        // 108 until
   
   // ********************************************   
   
   20035,20040,  // 109 pushn    next value on math stack  look for built in func ';'

   20036,        // 111 over
   20012,        // 112 equal    test if function was a ';'
   20020,        // 113 not
   20022,10119,  // 114 if      

         // this must be just an ordinary function ..... just push it in the prog

   20017,        // 116 append   
   20031,        // 117 push0
   20026,        // 118 until
   
   //  must be the ';'

   20017,        // 119 append return function to prog

   20035,0x5555, // 120 just push a known value on the stack, will test at the end
   20012,        // 122 equal
   20020,        // 123 not
   20022,10132,  // 124 if
   
   20035,'?',    // 126 push a '?' on the stack
   20039,        // 128 emit
   20035,'s',    // 129 push a 's' on the stack
   20039,        // 131 emit

   20037,        // 132 push1
   20026,        // 133 until
   20040,        // 134 return


   // ***********************************************
   // var    create a variable
   
   20043,        // 135 get address of variable
   20031,        // 136 push0
   20017,        // 137 append  ","
   
   20014,        // 138 get a word from the input
   20015,        // 139 define it
   20035,20035,  // 140 put the push next number opcode on stack
   20017,        // 142 append the pushn instruction    
   20017,        // 143 append the address we want to push
   20035,20040,  // 144 put a return instruction on stack
   20017,        // 146 put the return instruction in prog
   20040,        // 147 return
   
   };   



/****************************************************************************
 *
 * Local function prototypes
 *
 ***************************************************************************/
uint8_t getKeyB(void);
void getLine(void);
uint8_t nextPrintableChar(void);
uint8_t skipStackComment(void);
void getWord(void);
void listFunction(void);
int16_t popMathStack(void);
void pushMathStack(int16_t n);
int16_t popAddrStack(void);
void pushAddrStack(int16_t n);
void ndropFunc(void);
int16_t lookupToken(uint8_t *x, uint8_t *l);
void luFunc(void);
void numFunc(void);
void ifFunc(int16_t x);
void loopFunc(int16_t n);
void rollFunc(int16_t n);
void pushnFunc(void);
void dfnFunc(void);
void printNumber(int16_t n);
void printHexChar(int16_t n);
void printHexByte(int16_t n);
void printHexWord(int16_t n);
void execN(int16_t n);
void execFunc(void);



/****************************************************************************
 *
 * Public functions
 *
 ***************************************************************************/

void msp4th_init(struct msp4th_config *c)
{
    /*
     * Get addresses of user-configurable arrays from the pre-known vector
     * table locations.
     *
     * Changing the values in the msp4th_* locations and calling
     * init_msp4th() again restarts the interpreter with the new layout;
     */
    mathStackPtr = c->mathStackStart;
    addrStackPtr = c->addrStackStart;
    mathStackStart = c->mathStackStart;
    addrStackStart = c->addrStackStart;
    prog = c->prog;
    progOpcodes = c->progOpcodes;
    cmdList = c->cmdList;
    lineBuffer = c->lineBuffer;
    lineBufferLength = c->lineBufferLength;
    wordBuffer = c->wordBuffer;
    wordBufferLength = c->wordBufferLength;
    msp4th_putchar = c->putchar;
    msp4th_getchar = c->getchar;
    msp4th_puts = c->puts;


    xit = 0;
    echo = 1;
    progCounter = 10000;
    progIdx = 1;			// this will be the first opcode
    cmdListIdx = 0;

    lineBufferIdx = 0;
    msp4th_puts((uint8_t *)"msp4th!");
}


void msp4th_processLoop(void) // this processes the forth opcodes.
{
    uint16_t opcode;
    uint16_t tmp;

    while(xit == 0){
        if(progCounter > 9999){
            tmp = progCounter - 10000;
            opcode = progBi[tmp];
        } else {
            opcode = prog[progCounter];
        }

        progCounter = progCounter + 1;

        if(opcode > 19999){
            // this is a built in opcode
            execN(opcode - 20000);
        } else {
            pushAddrStack(progCounter);
            progCounter = progOpcodes[opcode];
        }
    } // while ()
}



/****************************************************************************
 *
 * The rest of the implementation
 *
 ***************************************************************************/

uint8_t getKeyB(void)
{
    uint8_t c;

    c = lineBuffer[lineBufferIdx++];
    if (c == 0) {
        getLine();
        c = lineBuffer[lineBufferIdx++];
    }

    return (c);
}


void getLine(void)
{
    int16_t waiting;
    uint8_t c;

    lineBufferIdx = 0;

    if (echo) {
        msp4th_putchar('\r');
        msp4th_putchar('\n');
        msp4th_putchar('>');   // this is our prompt
    }

    waiting = 1;
    while (waiting) {  // just hang in loop until we get CR
        c = msp4th_getchar();

        if (echo && (c == '\b') && (lineBufferIdx > 0)) {
            msp4th_putchar('\b');
            msp4th_putchar(' ');
            msp4th_putchar('\b');
            lineBufferIdx--;
        } else if ( ((c == 255) || (c == '')) && (lineBufferIdx == 0)) {
            xit = 1;
            waiting = 0;
            // add a sham word so we make it back to processLoop() to exit
            lineBuffer[lineBufferIdx++] = 'x';
            lineBuffer[lineBufferIdx] = ' ';
        } else {
            if (echo) {
                msp4th_putchar(c);
            }

            if ( (c == '\r') ||
                 (c == '\n') ||
                 (lineBufferIdx >= (lineBufferLength - 1))) { // prevent overflow of line buffer

                waiting = 0;

                if (echo) { msp4th_putchar('\n'); }
            }

            lineBuffer[lineBufferIdx++] = c;
            lineBuffer[lineBufferIdx] = 0;
        }
    }

    lineBufferIdx = 0;
}


uint8_t nextPrintableChar(void)
{
    uint8_t c;

    c = getKeyB();
    while (c <= ' ') {
        c = getKeyB();
    }

    return (c);
}


uint8_t skipStackComment(void)
{
    uint8_t c;

    c = getKeyB();
    while (c != ')') {
        c = getKeyB();
    }

    c = nextPrintableChar();

    return (c);
}


void getWord(void)
{
    int16_t k;
    uint8_t c;

    k = 0;
    c = nextPrintableChar();

    // ignore comments
    while ((c == '(') || (c == 92)) {
        switch (c) {
            case '(': // '(' + anything + ')'
                c = skipStackComment();
                break;

            case 92: // '\' backslash -- to end of line
                getLine();
                c = nextPrintableChar();
                break;

            default:
                break;
        }
    }

    do {
        wordBuffer[k++] = c;
        wordBuffer[k] = 0;
        c = getKeyB();
    } while ((c > ' ') && (k < wordBufferLength));
}


void listFunction(void)
{
    msp4th_puts((uint8_t *)cmdListBi);
    msp4th_puts((uint8_t *)cmdListBi2);
    msp4th_puts(cmdList);
}


#define TOS (*mathStackPtr)
#define NOS (*(mathStackPtr + 1))
#define STACK(n) (*(mathStackPtr + n))

int16_t popMathStack(void)
{
    int16_t i;

    i = *mathStackPtr;

    // prevent stack under-flow
    if (mathStackPtr < mathStackStart) {
        mathStackPtr++;
    }

    return(i);
}


void pushMathStack(int16_t n)
{
#if defined(MSP430)
    asm("decd %[ms]\n"
        "mov %[n],  @%[ms]\n"
        : /* outputs */
        : /* inputs */ [n] "r" (n), [ms] "r" (mathStackPtr)
        : /* clobbers */
       );
#else
    mathStackPtr--;
    *mathStackPtr = n;
#endif
}


#define ATOS (*addrStackPtr)
#define ANOS (*(addrStackPtr + 1))
#define ASTACK(n) (*(addrStackPtr + n))

int16_t popAddrStack(void)
{
    int16_t i;

#if defined(MSP430)
    asm("mov @%[as],  %[out]\n"
        "incd %[as]\n"
        : /* outputs */ [out] "=r" (i)
        : /* inputs */  [as] "r" (addrStackPtr)
        : /* clobbers */
       );
#else
    i = *addrStackPtr;
    addrStackPtr++;
#endif

    return(i);
}


void pushAddrStack(int16_t n)
{
#if defined(MSP430)
    asm("decd %[as]\n"
        "mov %[n],  @%[as]\n"
        : /* outputs */
        : /* inputs */ [n] "r" (n), [as] "r" (addrStackPtr)
        : /* clobbers */
       );
#else
    addrStackPtr--;
    *addrStackPtr = n;
#endif
}


void ndropFunc(void)
{
    int16_t n;

    n = TOS + 1; // drop the *drop count* also
    mathStackPtr += n;
}


int16_t lookupToken(uint8_t *word, uint8_t *list)
{
    // looking for word in list
    // Matches FIRST OCCURENCE of word in list

    int16_t i;
    int16_t j;
    int16_t k;
    int16_t n;

    i = 0;
    j = 0;
    k = 0;
    n = 1;

#define CONSUMETO(c) do { while (list[i] > c) { i++; } } while (0)
    while (list[i] != 0) {
        if ((word[j] != 0) && (list[i] == word[j])) {
            // keep matching
            j++;
        } else if ((word[j] == 0) && (list[i] == ' ')){
            // end of word, match iff it's the end of the list item
            k = n;
            //just break the while early
            break;
        } else {
            j = 0;
            n++;
            CONSUMETO(' ');
        }

        i++;
    }

    return(k);
#undef CONSUMETO
}


void luFunc(void)
{
    int16_t opcode;

    opcode = lookupToken(wordBuffer, (uint8_t *)cmdListBi);

    if (opcode) {
        opcode += 20000;
        pushMathStack(opcode);
        pushMathStack(1);
    } else {
        // need to test internal interp commands
        opcode = lookupToken(wordBuffer, (uint8_t *)cmdListBi2);
        if (opcode) {
            opcode += 10000;
            pushMathStack(opcode);
            pushMathStack(1);
        } else {
            opcode = lookupToken(wordBuffer, cmdList);
            if (opcode) {
                pushMathStack(opcode);
                pushMathStack(1);
            } else {
                pushMathStack(0);
            }
        }
    }
}


void opcode2word(void)
{
    // given an opcode, print corresponding ASCII word name

    int16_t i;
    int16_t n;
    int16_t opcode;
    uint8_t *list;

    i = 0;
    n = 1; // opcode indices are 1-based
    opcode = popMathStack();

#define BUILTIN_OPCODE_OFFSET 20000
#define BUILTIN_INTERP_OFFSET 10000

    // where is the opcode defined?
    // remove offset
    if (opcode >= BUILTIN_OPCODE_OFFSET) {
        list = (uint8_t *)cmdListBi;
        opcode -= BUILTIN_OPCODE_OFFSET;
    } else if (opcode >= BUILTIN_INTERP_OFFSET) {
        list = (uint8_t *)cmdListBi2;
        opcode -= BUILTIN_INTERP_OFFSET;
    } else {
        list = cmdList;
    }

    // walk list to get word
    // skip to start of the expected location
    while (n < opcode) {
        while (list[i] > ' ') {
            i++;
        }
        i++;
        n++;
    }

    if (list[i] !=0) {
        // not end of list, print next word
        while (list[i] > ' ') {
            msp4th_putchar(list[i++]);
        }
    } else {
        msp4th_putchar('?');
    }

    msp4th_putchar(' ');
}


void numFunc(void)
{
    // the word to test is in wordBuffer

    uint16_t i;
    int16_t isnum;
    int16_t n;

    i = 0;
    isnum = 0;
    n = 0;

    // first check for neg sign
    if (wordBuffer[i] == '-') {
        i = i + 1;
    }

    if ((wordBuffer[i] >= '0') && (wordBuffer[i] <= '9')) {
        // it is a number 
        isnum = 1;
        // check if hex
        if (wordBuffer[0] == '0' && wordBuffer[1] == 'x') {
            // base 16 number ... just assume all characters are good
            i = 2;
            n = 0;
            while (wordBuffer[i]) {
                n = n << 4;
                n = n + wordBuffer[i] - '0';
                if (wordBuffer[i] > '9') {
                    n = n - 7;

                    // compensate for lowercase digits
                    if (wordBuffer[i] >= 'a') {
                        n -= 0x20;
                    }
                }
                i = i + 1;
            }
        } else {
            // base 10 number
            n = 0;
            while (wordBuffer[i]) {
                n = n * 10;
                n = n + wordBuffer[i] - '0';
                i = i + 1;
            }
            if (wordBuffer[0] == '-') {
                n = -n;
            }
        }
    }

    pushMathStack(n);
    pushMathStack(isnum);
}


void ifFunc(int16_t x){
    // used as goto if x == 1

    uint16_t addr;
    uint16_t tmp;
    int16_t i;

    if(progCounter > 9999){
        tmp = progCounter - 10000;
        addr = progBi[tmp];
    } else {
        addr = prog[progCounter];
    }

    progCounter++;

    if (x == 1) {
        // this is a goto
        progCounter = addr;
    } else {
        // this is the "if" processing
        i = popMathStack();
        if(i == 0){
            progCounter = addr;
        }
    }
}


void loopFunc(int16_t n)
{
#define j ATOS      // loop address
#define k ANOS      // count
#define m ASTACK(2) // limit

    ANOS += n;     // inc/dec the count

    if ( ! (   ((n >= 0) && (k >= m))
            || ((n <= 0) && (k <= m)))) {
        // loop
        progCounter = j;
    } else {
        // done, cleanup
        popAddrStack();
        popAddrStack();
        popAddrStack();
    }
#undef j
#undef k
#undef m
}


void rollFunc(int16_t n)
{
    int16_t *addr;
    int16_t tmp;

    tmp = STACK(n);
    addr = (mathStackPtr + n);

    while (addr > mathStackPtr) {
        *addr = *(addr - 1);
        addr--;
    }

    TOS = tmp;
}


void pushnFunc(void)
{
    int16_t i;

    if (progCounter > 9999) {
        i = progBi[progCounter - 10000];
    } else {
        i = prog[progCounter];
    }

    progCounter = progCounter + 1;
    pushMathStack(i);
}


void dfnFunc(void)
{
    // this function adds a new def to the list and creates a new opcode

    uint16_t i;

    i = 0;

    while (wordBuffer[i]) {
        cmdList[cmdListIdx++] = wordBuffer[i];
        i = i + 1;
    }

    cmdList[cmdListIdx++] = ' ';
    cmdList[cmdListIdx] = 0;
    i = lookupToken(wordBuffer, cmdList);
    progOpcodes[i] = progIdx;
}


void printNumber(register int16_t n)
{
    uint16_t nu;
    int16_t i;
    int16_t rem;
    uint8_t x[7];

    if (n < 0) {
        msp4th_putchar('-');
        nu = -n;
    } else {
        nu = n;
    }

    i = 0;
    do {
        rem = nu % 10;
        x[i] = (uint8_t)rem + (uint8_t)'0';
        nu = nu / 10;
        i = i + 1;
    } while((nu != 0) && (i < 7));

    do{
        i = i - 1;
        msp4th_putchar(x[i]);
    } while (i > 0);

    msp4th_putchar(' ');
}


void printHexChar(int16_t n)
{
    n &= 0x0F;

    if (n > 9) {
        n += 7;
    }

    n += '0';
    msp4th_putchar(n);
}


void printHexByte(int16_t n)
{
    n &= 0xFF;
    printHexChar(n >> 4);
    printHexChar(n);
}


void printHexWord(int16_t n)
{
    printHexByte(n >> 8);
    printHexByte(n);
}


void execFunc(void) {
    int16_t opcode;

    opcode = popMathStack();

    if (opcode > 19999) {
        // this is a built in opcode
        execN(opcode - 20000);
    } else if (opcode > 9999) {
        // built in interp
        pushAddrStack(progCounter);
        progCounter = cmdList2N[opcode-10000];
    } else {
        pushAddrStack(progCounter);
        progCounter = progOpcodes[opcode];
    }
}


void execN(int16_t opcode)
{
  int16_t i,j,k,m,n;
  int32_t x;

  switch(opcode){
    case  0: // unused
      break;

    case  1: // bye
      xit = 1;
      break;

    case  2: // +  ( a b -- a+b )
      NOS += TOS;
      popMathStack();
      break;

    case  3: // -  ( a b -- a-b )
      NOS += -TOS;
      popMathStack();
      break;

    case  4: // *  ( a b -- reshi reslo )
#if defined(MSP430)
      asm("dint");
      MPYS = NOS;
      OP2 = TOS;
      NOS = RESHI;
      TOS = RESLO;
      asm("eint");
#else
      x = TOS * NOS;
      NOS = (int16_t)((x >> 16) & 0xffff);
      TOS = (int16_t)(x & 0xffff);
#endif
      break;

    case  5: // /%  ( a b -- a/b a%b )
#if defined(MSP430)
      /* directly call divmodhi4, gcc calls it twice even though the fn returns
       * both values in one call */
      asm("mov 2(%[ms]), r12\n"
          "mov 0(%[ms]), r10\n"
          "call #__divmodhi4\n"
          "mov r12, 2(%[ms])\n"
          "mov r14, 0(%[ms])\n"
          : /* outputs */
          : [ms] "r" (mathStackPtr) /* inputs */
          : /* clobbers */ "r10","r11","r12","r13","r14"
         );
#else
      i = NOS;
      j = TOS;
      NOS = i / j;
      TOS = i % j;
#endif
      break;

    case  6: // .  ( a -- )
      printNumber(popMathStack());
      break;

    case  7: // dup  ( a -- a a )
      pushMathStack(TOS);
      break;

    case  8: // drop  ( a -- )
      i = popMathStack();
      break;

    case  9: // swap  ( a b -- b a )
      i = TOS;
      TOS = NOS;
      NOS = i;
      break;

    case 10: // <  ( a b -- a<b )
      i = popMathStack();
      if(TOS < i){
        TOS = 1;
      } else {
        TOS = 0;
      }
      break;      

    case 11: // >  ( a b -- a>b )
      i = popMathStack();
      if(TOS > i){
        TOS = 1;
      } else {
        TOS = 0;
      }
      break;      

    case 12: // ==  ( a b -- a==b )
      i = popMathStack();
      if(i == TOS){
        TOS = 1;
      } else {
        TOS = 0;
      }
      break;      

    case 13: // .hb  ( a -- )
      printHexByte(popMathStack());
      msp4th_putchar(' ');
      break;

    case 14: // gw  ( -- ) \ get word from input
      getWord();
      break;

    case 15: // dfn  ( -- ) \ create opcode and store word to cmdList
      dfnFunc();
      break;

    case 16: // abs  ( a -- |a| ) \ -32768 is unchanged
      if (TOS < 0) {
          TOS = -TOS;
      }
      break;

    case 17: // ,  ( opcode -- ) \ push opcode to prog space
      prog[progIdx++] = popMathStack();
      break;

    case 18: // p@  ( opaddr -- opcode )
      i = TOS;
      TOS = prog[i];
      break;

    case 19: // p!  ( opcode opaddr -- )
      i = popMathStack();
      j = popMathStack();
      prog[i] = j;
      break;

    case 20: // not  ( a -- !a ) \ logical not
      if(TOS){
        TOS = 0;
      } else {
        TOS = 1;
      }
      break;

    case 21: // list  ( -- ) \ show defined words
      listFunction();
      break;

    case 22: // if  ( flag -- )
      ifFunc(0);
      break;

    case 23: // then      ( trapped in ':')
      break;

    case 24: // else      ( trapped in ':')
      break;

    case 25: // begin  ( -- ) ( -a- pcnt )
      pushAddrStack(progCounter);
      break;

    case 26: // until  ( flag -- ) ( addr -a- )
      i = popAddrStack();
      j = popMathStack();
      if(j == 0){
        addrStackPtr--;  // number is still there ... just fix the pointer
        progCounter = i;
      }
      break;    

    case 27: // depth  ( -- n ) \ math stack depth
      pushMathStack(mathStackStart - mathStackPtr);
      break;
      
    case 28: // .h  ( a -- )
      printHexWord(popMathStack());
      msp4th_putchar(' ');
      break;

    case 29: // ] ( trapped in interp )
      break;

    case 30: // num  ( -- n flag ) \ is word in buffer a number?
      numFunc();
      break;

    case 31: // push0  ( -- 0 )
      pushMathStack(0);
      break;

    case 32: // goto   ( for internal use only )
      ifFunc(1);
      break;

    case 33: // exec  ( opcode -- )
      execFunc();
      break;

    case 34: // lu  ( -- opcode 1 | 0 )
      luFunc();
      break;

    case 35: // pushn   ( internal use only )
      pushnFunc();
      break;

    case 36: // over  ( a b -- a b a )
      pushMathStack(NOS);
      break;

    case 37: // push1  ( -- 1 )
      pushMathStack(1);
      break;

    case 38: // pwrd  ( -- ) \ print word buffer
      msp4th_puts(wordBuffer);
      break;

    case 39: // emit  ( c -- )
      msp4th_putchar(popMathStack());
      break;

    case 40: // ;  ( pcnt -a- )
      i = progCounter;
      progCounter = popAddrStack();
      break;

    case 41: // @  ( addr -- val ) \ read directly from memory address
      i = TOS >> 1;
      TOS = dirMemory[i];
      break;
      
    case 42: // !  ( val addr -- ) \ write directly to memory address words only!
      i = popMathStack();  //  address to write to
      i = i >> 1;
      j = popMathStack();  //  value to write
      dirMemory[i] = j;
      break;

    case 43: // h@  ( -- prog ) \ end of program code space
      pushMathStack(progIdx);
      break;

    //////// end of words used in progBi[] ///////////////////////////////////

    case 44: // do  ( limit cnt -- ) ( -a- limit cnt pcnt )
      i = popMathStack();  // start of count
      j = popMathStack();  // end count
      k = progCounter;

      pushAddrStack(j);  // limit on count
      pushAddrStack(i);  // count  (I)
      pushAddrStack(k);  // address to remember for looping
      break;

    case 45: // loop  ( -- ) ( limit cnt pcnt -a- | limit cnt+1 pcnt )
      loopFunc(1);
      break;

    case 46: // +loop  ( n -- ) ( limit cnt pcnt -a- | limit cnt+n pcnt ) \ decrement loop if n<0
      loopFunc(popMathStack());
      break;

    case 47: // i  ( -- cnt ) \ loop counter value
      i = ANOS;
      pushMathStack(i);
      break;

    case 48: // j  ( -- cnt ) \ next outer loop counter value
      i = ASTACK(4);
      pushMathStack(i);
      break;

    case 49: // k  ( -- cnt ) \ next next outer loop counter value
      i = ASTACK(7);
      pushMathStack(i);
      break;

    case 50: // ~  ( a -- ~a ) \ bitwise complement
      TOS = ~TOS;
      break;

    case 51: // ^  ( a b -- a^b ) \ bitwise xor
      NOS ^= TOS;
      popMathStack();
      break;

    case 52: // &  ( a b -- a&b ) \ bitwise and
      NOS &= TOS;
      popMathStack();
      break;

    case 53: // |  ( a b -- a|b ) \bitwise or
      NOS |= TOS;
      popMathStack();
      break;

    case 54: // */  ( a b c -- (a*b)/c ) \ 32b intermediate
#if defined(MSP430)
      asm("dint");
      MPYS = popMathStack();
      OP2 = NOS;
      x = (((int32_t)RESHI << 16) | RESLO);
      asm("eint");
      x = (x / TOS);
      popMathStack();
      TOS = (int16_t)(x & 0xffff);
#else
      i = popMathStack();
      j = TOS;
      k = NOS;
      x = j * k;
      x = x / i;
      popMathStack();
      TOS = (int16_t)(x & 0xffff);
#endif
      break;
      
    case 55: // key  ( -- c ) \ get a key from input .... (wait for it)
      pushMathStack(msp4th_getchar());
      break;

    case 56: // cr  ( -- )
      msp4th_putchar(0x0D);
      msp4th_putchar(0x0A);
      break;

    case 57: // 2*  ( a -- a<<1 )
      TOS <<= 1;
      break;

    case 58: // 2/  ( a -- a>>1 )
      TOS >>= 1;
      break;

    case 59: // call0  ( &func -- *func() )
      i = TOS;
      TOS = (*(int16_t(*)(void)) i) ();
      break;

    case 60: // call1  ( a &func -- *func(a) )
      i = TOS;
      j = NOS;
      NOS = (*(int16_t(*)(int16_t)) i) (j);
      popMathStack();
      break;

    case 61: // call2  ( a b &func -- *func(a,b) )
      i = TOS;
      j = NOS;
      k = STACK(2);
      STACK(2) = (*(int16_t(*)(int16_t, int16_t)) i) (k, j);
      TOS = 1;
      ndropFunc();
      break;

    case 62: // call3  ( a b c &func -- *func(a,b,c) )
      i = TOS;
      j = NOS;
      k = STACK(2);
      m = STACK(3);
      STACK(3) = (*(int16_t(*)(int16_t, int16_t, int16_t)) i) (m, k, j);
      TOS = 2;
      ndropFunc();
      break;

    case 63: // call4  ( a b c d &func -- *func(a,b,c,d) )
      i = TOS;
      j = NOS;
      k = STACK(2);
      m = STACK(3);
      n = STACK(4);
      STACK(4) = (*(int16_t(*)(int16_t, int16_t, int16_t, int16_t)) i) (n, m, k, j);
      TOS = 3;
      ndropFunc();
      break;

    case 64: // ndrop  ( (x)*n n -- ) \ drop n math stack cells
      ndropFunc();
      break;

    case 65: // swpb  ( n -- n ) \ byteswap TOS
#if defined(MSP430)
      asm("swpb %[s]\n":  : [s] "r" (mathStackPtr) :);
#else
      TOS = ((TOS >> 8) & 0x00ff) | ((TOS << 8) & 0xff00);
#endif
      break;

    case 66: // +!  ( n addr -- ) \ *addr += n
      i = popMathStack();
      j = popMathStack();
      dirMemory[i] += j;
      break;

    case 67: // roll  ( n -- ) \ nth stack removed and placed on top
      rollFunc(popMathStack());
      break;

    case 68: // pick  ( n -- ) \ nth stack copied to top
      i = popMathStack();
      pushMathStack(STACK(i));
      break;

    case 69: // tuck  ( a b -- b a b ) \ insert copy TOS to after NOS
      i = NOS;
      pushMathStack(TOS);
      STACK(2) = TOS;
      NOS = i;
      break;

    case 70: // max  ( a b -- c ) \ c = a ? a>b : b
      i = popMathStack();
      if (i > TOS) {
          TOS = i;
      }
      break;

    case 71: // min  ( a b -- c ) \ c = a ? a<b : b
      i = popMathStack();
      if (i < TOS) {
          TOS = i;
      }
      break;

    case 72: // s.  ( -- ) \ print stack contents, TOS on right
      { // addr is strictly local to this block
          int16_t *addr;
          addr = mathStackStart;
          while (addr >= mathStackPtr) {
              printNumber(*addr);
              addr--;
          }
      }
      break;

    case 73: // sh.  ( -- ) \ print stack contents in hex, TOS on right
      { // addr is strictly local to this block
          int16_t *addr;
          addr = mathStackStart;
          while (addr >= mathStackPtr) {
              printHexWord(*addr);
              msp4th_putchar(' ');
              addr--;
          }
      }
      break;

    case 74: // neg  ( a -- -a ) \ twos complement
      TOS *= -1;
      break;

    case 75: // echo  ( bool -- ) \ ?echo prompts and terminal input?
      echo = popMathStack();
      break;

    case 76: // init  ( &config -- ) \ clears buffers and calls msp4th_init
      *lineBuffer = 0; // if location is same, the call is recursive otherwise
      msp4th_init((struct msp4th_config *)popMathStack());
      break;

    case 77: // o2w  ( opcode -- ) \ leaves name of opcode in wordBuffer
      opcode2word();
      break;

    default:
      break;
  }
}



