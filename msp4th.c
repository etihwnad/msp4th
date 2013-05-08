/*
 *
 * TODO:
 *  - use enum for VM opcodes
 *
 *  X speed up pop/pushMathStack (need bounds check??)
 *      only bounds check is for mathStack underflow
 *
 *  X UART usage is blocking, convert to interrupt-based
 *      user may provide msp4th_{putchar,getchar} function pointers
 *      the default one remains blocking
 *
 *  X allow configurable user-code space
 *      mathStack[], addrStack[]
 *      prog[], cmdList[], progOpcodes[]
 *      array locations come from a vector table instead of hard-coded
 *      locations in bss space.  init_msp4th() populates the pointers
 *      with values from the table (which can be user-written to effect
 *      changing user code sizes.
 *
 */


#if defined(MSP430)
#include "ns430.h"
#include "ns430-atoi.h"

#else
// mixins to test msp4th on PC
#include <stdint.h>
typedef uint8_t str_t;
#endif


#include "msp4th.h"



/* 
 * Hard-coded constants
 *
 * buffer lengths don't need to be configurable, right?
 */
#define LINE_SIZE 128
#define WORD_SIZE 32


/*
 * Local function prototypes
 */
uint8_t getKeyB(void);
void getLine(void);
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
void pushnFunc(void);
void overFunc(void);
void dfnFunc(void);
void printNumber(int16_t n);
void printHexChar(int16_t n);
void printHexByte(int16_t n);
void printHexWord(int16_t n);
void execN(int16_t n);
void execFunc(void);

/****************************************************************************
 *
 * Module-level global constants (in ROM)
 *
 ***************************************************************************/

// The order matches the execN function and determines the opcode value.
// NOTE: must end in a space !!!!
const uint8_t cmdListBi[] = 
             {"exit + - * / "                       // 1 -> 5
              ". dup drop swap < "                  // 6 -> 10
              "> == .hb gw dfn "                    // 11 -> 15
              "keyt , p@ p! not "                   // 16 -> 20
              "list if then else begin "            // 21 -> 25
              "until eram .h ] num "                // 26 -> 30
              "push0 goto exec lu pushn "           // 31 -> 35
              "over push1 pwrd emit ; "             // 36 -> 40
              "@ ! h@ do loop "                     // 41 -> 45
              "i ~ ^ & | "                          // 46 -> 50
              "*/ key cr *2 /2 "                    // 51 -> 55
              "call0 call1 call2 call3 call4 "      // 56 -> 60
              "ndrop "                              // 61 -> 61
             };
#define LAST_PREDEFINED 61	// update this when we add commands to the built in list


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
 * Module-level global variables (in RAM)
 *
 ***************************************************************************/
// our "special" pointer, direct word access to all address space
volatile int16_t *dirMemory;

// set to 1 to kill program
int16_t xit;

uint16_t progCounter;

uint8_t lineBuffer[LINE_SIZE];      /* input line buffer */
uint16_t lineBufferIdx;             /* input line buffer pointer */

uint8_t wordBuffer[WORD_SIZE];      // just get a word



/* The following utilize a vector table to allow re-configuring the
 * location/size of these arrays.  Then the stack sizes and user program space
 * sizes can be (re-)specified by changing the table and calling init_msp4th()
 * again.
 */
#if defined(MSP430)
int16_t register *mathStackPtr asm("r6");
#else
int16_t *mathStackPtr;
#endif

#if defined(MSP430)
int16_t register *addrStackPtr asm("r7");
#else
int16_t *addrStackPtr;
#endif

int16_t *prog;          // user programs (opcodes) are placed here
uint16_t progIdx;       // next open space for user opcodes

int16_t *progOpcodes;   // mapping between user word index and program opcodes
                        // start index into prog[]

uint8_t *cmdList;       // string of user defined word names
uint16_t cmdListIdx;    // next open space for user word strings








static int16_t RAMerrors(void){
    int16_t errors;
#if defined(MSP430)
    __asm__ __volatile__ ( "mov r9, %0\n" : [res] "=r" (errors));
#else
    errors = 0;
#endif
    return errors;
}


void msp4th_puts(uint8_t *s)
{
    uint16_t i = 0;
    uint8_t c = 1;

    while (c != 0) {
        c = s[i++];
        msp4th_putchar(c);
    }
    msp4th_putchar('\r');
    msp4th_putchar('\n');
}


uint8_t getKeyB(){
    uint8_t c;

    c = lineBuffer[lineBufferIdx++];
    if (c == 0) {
        getLine();
        c = lineBuffer[lineBufferIdx++];
    }

    return (c);
}


void getLine()
{
    int16_t waiting;
    uint8_t c;

    lineBufferIdx = 0;

    msp4th_putchar('\r');
    msp4th_putchar('\n');
    msp4th_putchar('>');   // this is our prompt

    waiting = 1;
    while (waiting) {  // just hang in loop until we get CR
        c = msp4th_getchar();

        if ((c == '\b') && (lineBufferIdx > 0)) {
            msp4th_putchar('\b');
            msp4th_putchar(' ');
            msp4th_putchar('\b');
            lineBufferIdx--;
        } else if ( ((c == 255) || (c == '')) && (lineBufferIdx == 0)) {
            xit = 1;
            waiting = 0;
        } else {
            msp4th_putchar(c);
            if ( (c == '\r') ||
                 (c == '\n') ||
                 (lineBufferIdx >= (LINE_SIZE - 1))) { // prevent overflow of line buffer
                waiting = 0;
            }
            lineBuffer[lineBufferIdx++] = c;
            lineBuffer[lineBufferIdx] = 0;
        }
    }
    msp4th_putchar('\n');
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


    for (k=0; k < WORD_SIZE; k++) {
        wordBuffer[k] = 0;
    }

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

    k = 0;
    wordBuffer[k++] = c;
    wordBuffer[k] = 0;

    c = getKeyB();
    while (c > ' ') {
        wordBuffer[k++] = c;
        wordBuffer[k] = 0;
        c = getKeyB();
    }
}


void listFunction()
{
    msp4th_puts((str_t *)cmdListBi);
    msp4th_puts((str_t *)cmdListBi2);
    msp4th_puts((str_t *)cmdList);
}


#define TOS (*mathStackPtr)
#define NOS (*(mathStackPtr + 1))
#define STACK(n) (*(mathStackPtr + n))

int16_t popMathStack(void)
{
    int16_t i;

    i = *mathStackPtr;

    // prevent stack under-flow
    if (mathStackPtr < (int16_t *)mathStackStartAddress) {
        mathStackPtr++;
    }

    return(i);
}


void pushMathStack(int16_t n)
{
    mathStackPtr--;
    *mathStackPtr = n;
}


#define ATOS (addrStackPtr)
#define ANOS (*(addrStackPtr + 1))
#define ASTACK(n) (*(addrStackPtr + n))

int16_t popAddrStack(void)
{
    int16_t i;

    i = *addrStackPtr;
    addrStackPtr++;

    return(i);
}


void pushAddrStack(int16_t n)
{
    addrStackPtr--;
    *addrStackPtr = n;
}


void ndropFunc(void)
{
    int16_t n;

    n = TOS + 1; // drop the *drop count* also
    mathStackPtr += n;
}


int16_t lookupToken(uint8_t *x, uint8_t *l){    // looking for x in l
  int16_t i,j,k,n;
  j = 0;
  k = 0;
  n=1;
  i=0;
  while(l[i] != 0){
    if(x[j] != 0){   
      // we expect the next char to match
      if(l[i] == ' '){
        // can't match x is longer than the one we were looking at
        j = 0;
        n++;
        while(l[i] > ' '){ i++; }
      } else {
        if(l[i] == x[j]){
          j++;
        } else {
          j = 0;
          while(l[i] > ' '){ i++; }
          n++;
        }
      }
    } else {
      // ran out of input ... did we hit the space we expected???
      if(l[i] == ' '){
        // we found it.
        k = n;
        while(l[i] != 0){
          i++;
        }
      } else {
        // missed it
        j = 0;
        n++;
        while(l[i] > ' '){ i++; }

      }
    }
    i++;
  }

  return(k);
}


void luFunc(){
  int16_t i;
  
  i = lookupToken(wordBuffer, (uint8_t *)cmdListBi);
  
  if(i){
    i += 20000;
    pushMathStack(i);
    pushMathStack(1);
  } else {
    // need to test internal interp commands
    i = lookupToken(wordBuffer, (uint8_t *)cmdListBi2);
    if(i){
      i += 10000;
      pushMathStack(i);
      pushMathStack(1);
    } else {
      i = lookupToken(wordBuffer, cmdList);
      if(i){
        pushMathStack(i);
        pushMathStack(1);
      } else {
        pushMathStack(0);
      }
    }
  }  
} 


void numFunc()
{  // the word to test is in wordBuffer
    uint16_t i;
    int16_t j;
    int16_t n;

    // first check for neg sign
    i = 0;
    if (wordBuffer[i] == '-') {
        i = i + 1;
    }

    if ((wordBuffer[i] >= '0') && (wordBuffer[i] <= '9')) {
        // it is a number 
        j = 1;
        // check if hex
        if(wordBuffer[0] == '0' && wordBuffer[1] == 'x'){
            // base 16 number ... just assume all characters are good
            i = 2;
            n = 0;
            while(wordBuffer[i]){
                n = n << 4;
                n = n + wordBuffer[i] - '0';
                if(wordBuffer[i] > '9'){
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
            while(wordBuffer[i]){
                n = n * 10;
                n = n + wordBuffer[i] - '0';
                i = i + 1;
            }
            if(wordBuffer[0] == '-'){
                n = -n;
            }
        }
    } else {
        n = 0;
        j = 0;
    }

    pushMathStack(n);
    pushMathStack(j);
}


void ifFunc(int16_t x){     // used as goto if x == 1
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

    if(x == 1){
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


void pushnFunc(){
  int16_t i;
  if(progCounter > 9999){
    i = progBi[progCounter - 10000];
  } else {
    i = prog[progCounter];
  }
  progCounter = progCounter + 1;
  pushMathStack(i);
}


void overFunc(){
  int16_t i;
  i = NOS;
  pushMathStack(i);
}


void dfnFunc(){
  uint16_t i;
  // this function adds a new def to the list and creats a new opcode
  i = 0;
  while(wordBuffer[i]){
    cmdList[cmdListIdx++] = wordBuffer[i];
    i = i + 1;
  }
  cmdList[cmdListIdx++] = ' ';
  cmdList[cmdListIdx] = 0;
  i = lookupToken(wordBuffer,cmdList);
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


void printHexChar(int16_t n){
  n &= 0x0F;
  if(n > 9){
    n += 7;
  }
  n += '0';
  msp4th_putchar(n);
}


void printHexByte(int16_t n){
  n &= 0xFF;
  printHexChar(n >> 4);
  printHexChar(n);
}


void printHexWord(int16_t n){
  printHexByte(n >> 8);
  printHexByte(n);
}


void execFunc(){
  int16_t opcode;
  opcode = popMathStack();

  if(opcode > 19999){
    // this is a built in opcode
    execN(opcode - 20000);

  } else if(opcode > 9999){

    pushAddrStack(progCounter);
    progCounter = cmdList2N[opcode-10000];

  } else {

    pushAddrStack(progCounter);
    progCounter = progOpcodes[opcode];

  }

}


void execN(int16_t opcode){
  int16_t i,j,k,m,n;
  int32_t x;

  switch(opcode){
    case  0: // unused
      break;

    case  1: // exit
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

    case  5: // /  ( a b -- a/b )
      NOS = NOS / TOS;
      popMathStack();
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
      break;

    case 14: // gw  ( -- ) \ get word from input
      getWord();
      break;

    case 15: // dfn  ( -- ) \ create opcode and store word to cmdList
      dfnFunc();
      break;

    case 16: // keyt  ( -- flag )
      // TODO: for interrupt-based UART I/O
      // return a 1 if keys are in ring buffer
      //i = (inputRingPtrXin - inputRingPtrXout) & 0x0F;    // logical result assigned to i
      i = 0;
      pushMathStack(i);
      break;

    case 17: // allot  ( opcode -- ) \ push opcode to prog space
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

    case 27: // eram  ( -- nRAMerrors )
      pushMathStack(RAMerrors());
      break;
      
    case 28: // .h  ( a -- )
      printHexWord(popMathStack());
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
      overFunc();
      break;

    case 37: // push1  ( -- 1 )
      pushMathStack(1);
      break;

    case 38: // pwrd  ( -- ) \ print word buffer
      msp4th_puts((str_t *)wordBuffer);
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

    case 44: // do  ( limit cnt -- ) ( -a- limit cnt pcnt )
      i = popMathStack();  // start of count
      j = popMathStack();  // end count
      k = progCounter;

      pushAddrStack(j);  // limit on count
      pushAddrStack(i);  // count  (I)
      pushAddrStack(k);  // address to remember for looping
      break;

    case 45: // loop  ( -- ) ( limit cnt pcnt -a- | limit cnt+1 pcnt )
      j = popAddrStack();  // loop address
      k = popAddrStack();  // count
      m = popAddrStack();  // limit
      k = k + 1;           // up the count
      if(k >= m){
        // we are done
      } else {
        // put it all back and loop
        pushAddrStack(m);
        pushAddrStack(k);
        pushAddrStack(j);
        progCounter = j;

      }
      break;
      
    case 46: // i  ( -- cnt ) \ loop counter value
      j = ANOS;
      pushMathStack(j);
      break;

    case 47: // ~  ( a -- ~a ) \ bitwise complement
      TOS = ~TOS;
      break;

    case 48: // ^  ( a b -- a^b ) \ bitwise xor
      NOS ^= TOS;
      popMathStack();
      break;

    case 49: // &  ( a b -- a&b ) \ bitwise and
      NOS &= TOS;
      popMathStack();
      break;

    case 50: // |  ( a b -- a|b ) \bitwise or
      NOS |= TOS;
      popMathStack();
      break;

    case 51: // */  ( a b c -- reshi reslo ) \ (a*b)/c scale function
#if defined(MSP430)
      asm("dint");
      MPYS = popMathStack();
      OP2 = NOS;
      x = (int32_t)(((int32_t)RESHI << 16) | RESLO);
      x = (int32_t)(x / TOS);
      NOS = (int16_t)((x >> 16) & 0xffff);
      TOS = (int16_t)(x & 0xffff);
      asm("eint");
#else
      i = popMathStack();
      j = TOS;
      k = NOS;
      x = (int32_t)(j * k);
      x = (int32_t)(x / i);
      NOS = (int16_t)((x >> 16) & 0xffff);
      TOS = (int16_t)(x & 0xffff);
#endif
      break;
      
    case 52: // key  ( -- c ) \ get a key from input .... (wait for it)
      pushMathStack(msp4th_getchar());
      break;

    case 53: // cr  ( -- )
      msp4th_putchar(0x0D);
      msp4th_putchar(0x0A);
      break;

    case 54: // *2  ( a -- a<<1 )
      TOS <<= 1;
      break;

    case 55: // /2  ( a -- a>>1 )
      TOS >>= 1;
      break;

    case 56: // call0  ( &func -- *func() )
      i = TOS;
      TOS = (*(int16_t(*)(void)) i) ();
      break;

    case 57: // call1  ( a &func -- *func(a) )
      i = TOS;
      j = NOS;
      NOS = (*(int16_t(*)(int16_t)) i) (j);
      popMathStack();
      break;

    case 58: // call2  ( a b &func -- *func(a,b) )
      i = TOS;
      j = NOS;
      k = STACK(2);
      STACK(2) = (*(int16_t(*)(int16_t, int16_t)) i) (k, j);
      TOS = 1;
      ndropFunc();
      break;

    case 59: // call3  ( a b c &func -- *func(a,b,c) )
      i = TOS;
      j = NOS;
      k = STACK(2);
      m = STACK(3);
      STACK(3) = (*(int16_t(*)(int16_t, int16_t, int16_t)) i) (m, k, j);
      TOS = 2;
      ndropFunc();
      break;

    case 60: // call4  ( a b c d &func -- *func(a,b,c,d) )
      i = TOS;
      j = NOS;
      k = STACK(2);
      m = STACK(3);
      n = STACK(4);
      STACK(4) = (*(int16_t(*)(int16_t, int16_t, int16_t, int16_t)) i) (n, m, k, j);
      TOS = 3;
      ndropFunc();
      break;

    case 61: // ndrop  ( (x)*n n -- )  drop n math stack cells
      ndropFunc();
      break;

    default:
      break;
  }
}



void init_msp4th(void)
{
    uint16_t i;

    xit = 0;

    /*
     * Get addresses of user-configurable arrays from the pre-known vector
     * table locations.
     *
     * Changing the values in the xStartAddress locations and calling
     * init_msp4th() again restarts the interpreter with the new layout;
     */
    mathStackPtr = (int16_t *)mathStackStartAddress;
    addrStackPtr = (int16_t *)addrStackStartAddress;
    prog = (int16_t *)progStartAddress;
    progOpcodes = (int16_t *)progOpcodesStartAddress;
    cmdList = (uint8_t *)cmdListStartAddress;



    progCounter = 10000;
    progIdx = 1;			// this will be the first opcode


    cmdListIdx = 0;

    dirMemory = (void *) 0;   // its an array starting at zero


    lineBufferIdx = 0;
    for (i=0; i < LINE_SIZE; i++) {
        lineBuffer[i] = 0;
    }

    for (i=0; i < WORD_SIZE; i++) {
        wordBuffer[i] = 0;
    }

    getLine();
}


void processLoop() // this processes the forth opcodes.
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

