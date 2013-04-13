
#if defined(MSP430)
/* use devboard uart */
#include "ns430-atoi.h"
#include "ns430-uart.h"

#else
/* mixins to test msp4th on PC */
#include <stdio.h>
#include <stdint.h>
typedef uint8_t str_t;
void uart_putchar(uint8_t c) { putchar(c); }
uint8_t uart_getchar(void) { return (uint8_t)getchar(); }
void uart_puts(uint8_t *s) { puts((char *)s); }
#endif


#include "msp4th.h"


#define ALIGN_2 __attribute__ ((aligned (2)))

/* 
 * Configuration constants
 */
#define CMD_LIST_SIZE 128
#define MATH_STACK_SIZE 32
#define ADDR_STACK_SIZE 64
#define PROG_SPACE 256
#define USR_OPCODE_SIZE 32

#define LINE_SIZE 128
#define WORD_SIZE 32

#define BI_PROG_SHIFT 10000

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

/*
 * Module-level global vars
 */
// must end in a space !!!!
// The order is important .... don't insert anything!
// the order matches the execN function

const uint8_t cmdListBi[] = 
             {"exit + - * / "                       // 1 -> 5
              ". dup drop swap < "                  // 6 -> 10
              "> = .hb gw dfn "                     // 11 -> 15
              "keyt , p@ p! not "                   // 16 -> 20
              "list if then else begin "            // 21 -> 25
              "until clrb .h ] num "                // 26 -> 30
              "push0 goto exec lu pushn "           // 31 -> 35
              "over push1 pwrd emit ; "             // 36 -> 40
              "@ ! h@ do loop "                     // 41 -> 45
              "i b@ a! and or "                     // 46 -> 50
              "*/ key cr hist histclr "             // 51 -> 55
              "fasttimer slowtimer stat hstat fec " // 56 -> 60
              "fecset fecbset fecbclr "             // 61 -> 65
              };

// these commands are interps

const uint8_t cmdListBi2[] = {"[ : var "};

// these values point to where in progBi[] these routines start

const int16_t cmdList2N[] = {0,10000,10032,10135};  // need an extra zero at the front

#define LAST_PREDEFINED 40	// update this when we add commands to the built in list

int16_t ALIGN_2 mathStack[MATH_STACK_SIZE];

int16_t ALIGN_2 addrStack[ADDR_STACK_SIZE];
volatile uint16_t addrStackPtr;

int16_t ALIGN_2 prog[PROG_SPACE];  // user programs are placed here
volatile uint16_t progPtr;           // next open space for user opcodes
int16_t ALIGN_2 progOps[USR_OPCODE_SIZE];
volatile uint16_t progOpsPtr;
uint8_t ALIGN_2 cmdList[CMD_LIST_SIZE];  // just a string of user defined names
volatile uint16_t cmdListPtr;

int16_t fastTimer;
int16_t slowTimer;

int16_t *dirMemory;

uint16_t buckets[260];  // use buckets[256] for total


// to flag the initial built in functions from the rest, save the negative of them in the program space (prog).

const int16_t ALIGN_2 progBi[] = { // address actually start at 10000

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
         
volatile uint16_t progCounter;

uint8_t lineBuffer[LINE_SIZE];      /* input line buffer */

volatile uint16_t lineBufferPtr;                 /* input line buffer pointer */
// uint8_t xit;                    /* set to 1 to kill program */

uint8_t wordBuffer[WORD_SIZE];		// just get a word


uint8_t getKeyB(){
  volatile uint8_t c;
  c = lineBuffer[lineBufferPtr];
  if (c != 0) {
      lineBufferPtr = lineBufferPtr + 1;
  }
  return(c);
}


void getLine()
{
    volatile uint16_t waiting;
    volatile uint8_t c;

    lineBufferPtr = 0;

    uart_putchar('\r');
    uart_putchar('\n');
    uart_putchar('>');   // this is our prompt

    waiting = 1;
    while (waiting) {  // just hang in loop until we get CR
        c = uart_getchar();

        if (c == '\b') {
            if (lineBufferPtr > 0) {
                uart_putchar('\b');
                uart_putchar(' ');
                uart_putchar('\b');
                lineBufferPtr--;
            }
        } else {
            uart_putchar(c);
            if ((c == '\r') || (c == '\n')) {
                // hit cr
                lineBuffer[lineBufferPtr] = 0;
                waiting = 0;
            } else {

                lineBuffer[lineBufferPtr++] = c;
                lineBuffer[lineBufferPtr] = 0;

                if(lineBufferPtr >= 127) {  // prevent overflow of line buffer
                    waiting = 0;
                }
            }
        }
    }
    uart_putchar('\n');
    lineBufferPtr = 0;
}


void getWord(void)
{
    volatile uint16_t k;
    volatile uint8_t c;
    volatile uint16_t waiting;

    wordBuffer[0] = 0;

    /*uart_putchar('.');*/
    /*uart_putchar('.');*/

    /*while (wordBuffer[0] == 0) {*/
    waiting = 1;
    while (waiting) {
        k = 0;
        c = getKeyB();

        while ((c <= ' ') && (c != 0)) {
            c = getKeyB();    /* strip leading spaces */
            uart_putchar('a');
            /*uart_putchar(c);*/
        }

        if (c > 0) {
            /*if (0 && (c == '"')) {*/
                /*c = getKeyB();*/
                /*uart_putchar(c);*/

                /*while ((c != '"') && (c != 0)) {*/
                    /*if (c != '"') {*/
                        /*wordBuffer[k++] = c;*/
                    /*}*/
                    /*c = getKeyB();*/
                    /*uart_putchar(c);*/
                /*}*/
            /*} else {*/
                while (c > ' ') {
                    uart_putchar('b');
                    wordBuffer[k++] = c;
                    /*uart_putchar(c);*/
                    c = getKeyB();
                }
            /*}*/

            wordBuffer[k] = 0;
            waiting = 0;
        } else {
            uart_putchar('c');
            wordBuffer[0] = 0;
            getLine();
        }
    }
}

void inline listFunction()
{
    uart_puts((str_t *)cmdListBi);
    uart_puts((str_t *)cmdListBi2);
    uart_puts((str_t *)cmdList);
}
  
int16_t popMathStack(void)
{
    volatile uint16_t i;
    volatile int16_t j,k;

    k = 1;
    j = mathStack[0];
    /*uart_putchar('1');*/
    /*uart_putchar('2');*/

    for (i=1;i<MATH_STACK_SIZE;i++) {
    /*uart_putchar('s');*/
        mathStack[i-1] = mathStack[i];
    }
    /*uart_putchar('3');*/
    k = 0;
    return(j);
}

void pushMathStack(int16_t n)
{
    volatile uint16_t i;
    volatile uint16_t tmp;
    /*printNumber(n);*/
    /*uart_puts((str_t *)"<-- push math");*/
    for (i=MATH_STACK_SIZE - 2; i > 0; i--) {
        tmp = i - 1;
        mathStack[i] = mathStack[tmp];
    }
    mathStack[0] = n;
}

int16_t popAddrStack(void)
{
    volatile int16_t j;
    j = addrStack[addrStackPtr];
    addrStackPtr = addrStackPtr + 1;
    return(j);
}

void pushAddrStack(int16_t n)
{
    addrStackPtr = addrStackPtr - 1;
    addrStack[addrStackPtr] = n;
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
    volatile uint16_t i;
    volatile int16_t j;
    volatile int16_t n;

    /*uart_puts((str_t *)"in numFunc()");*/

    // first check for neg sign
    i = 0;
    if (wordBuffer[i] == '-') {
        i = i + 1;
    }

    /*uart_puts((str_t *)"there");*/

    if ((wordBuffer[i] >= '0') && (wordBuffer[i] <= '9')) {
        /*uart_puts((str_t *)"num");*/
        // it is a number 
        j = 1;
        // check if hex
        if(wordBuffer[0] == '0' && wordBuffer[1] == 'x'){
            /*uart_puts((str_t *)"hex");*/
            // base 16 number ... just assume all characters are good
            i = 2;
            n = 0;
            while(wordBuffer[i]){
                n = n << 4;
                n = n + wordBuffer[i] - '0';
                if(wordBuffer[i] > '9'){
                    n = n - 7;
                }
                i = i + 1;
            }
        } else {
            /*uart_puts((str_t *)"dec");*/
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
        /*uart_puts((str_t *)"not number");*/
        n = 0;
        j = 0;
    }

    /*uart_putchar('.');*/
    pushMathStack(n);
    /*uart_putchar('.');*/
    pushMathStack(j);
    /*uart_putchar('.');*/
}

void ifFunc(int16_t x){     // used as goto if x == 1
    volatile uint16_t addr;
    volatile uint16_t tmp;
    volatile int16_t i;

    /*uart_puts((str_t *)"in ifFunc");*/

    if(progCounter > 9999){
        tmp = progCounter - 10000;
        addr = progBi[tmp];
    } else {
        addr = prog[progCounter];
    }

    /*printNumber(addr);*/
    /*uart_puts((str_t *)"<-- addr");*/

    progCounter = progCounter + 1;
    /*uart_putchar('.');*/
    if(x == 1){
        // this is a goto
        /*uart_putchar('g');*/
        progCounter = addr;
        /*uart_puts((str_t *)"goto");*/
    } else {
        // this is the "if" processing
        /*uart_putchar('i');*/
        i = popMathStack();
        /*uart_putchar('a');*/
        /*printNumber((int16_t)addr);*/
        if(i == 0){
            progCounter = addr;
        }
        /*uart_puts((str_t *)"<-- if");*/
    }
    /*uart_putchar('.');*/
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
  i = mathStack[1];
  pushMathStack(i);
}

void dfnFunc(){
  uint16_t i;
  // this function adds a new def to the list and creats a new opcode
  i = 0;
  while(wordBuffer[i]){
    cmdList[cmdListPtr] = wordBuffer[i];
    cmdListPtr = cmdListPtr + 1;
    i = i + 1;
  }
  cmdList[cmdListPtr] = ' ';
  cmdListPtr = cmdListPtr + 1;
  cmdList[cmdListPtr] = 0;
  i = lookupToken(wordBuffer,cmdList);
  progOps[i] = progPtr;
}


void printNumber(register int16_t n)
{
    volatile uint16_t i;
    volatile int16_t rem;
    /*int16_t k;*/
    uint8_t x[7];

    //uart_putchar('.');
    //uart_putchar('.');

    /*k = n;*/
    if (n < 0) {
        uart_putchar('-');
        n = -n;
    }

    i = 0;
    do {
        rem = n % 10;
        //uart_putchar('r');
        //printHexWord(rem);
        //uart_putchar(' ');
        //uart_putchar('n');
        //printHexWord(n);
        //uart_putchar('.');
        x[i] = (uint8_t)rem + (uint8_t)'0';
        n = n / 10;
        i = i + 1;
    } while((n != 0) && (i < 7));

    /*i = i - 1;*/
    do{
        i = i - 1;
        uart_putchar(x[i]);
    } while (i > 0);

    //uart_putchar('.');
    uart_putchar(' ');
}

void printHexChar(int16_t n){
  n &= 0x0F;
  if(n > 9){
    n += 7;
  }
  n += '0';
  uart_putchar(n);
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
    progCounter = progOps[opcode];

  }

}


void execN(int16_t n){
  volatile int16_t i,j,k,m;
  int16_t x,y,z;
  /*uart_puts((str_t *)"execN: ");*/
  /*printNumber(n);*/
  /*uart_puts((str_t *)"");*/
  switch(n){
    case 0:
      break;
    case 1:
  //    xit = 1;
      break;
    case 2:
      // +
      mathStack[1] += mathStack[0];
      popMathStack();
      break;
    case 3:
      // -
      mathStack[1] += -mathStack[0];
      popMathStack();
      break;
    case 4:
      // *
      mathStack[1] = mathStack[0] * mathStack[1];
      popMathStack();
      break;
    case 5:
      // /
      mathStack[1] = mathStack[1] / mathStack[0];
      popMathStack();
      break;
    case 6:
      // .
      printNumber(popMathStack());
      break;
    case 7:
      // dup
      pushMathStack(mathStack[0]);
      break;
    case 8:
      // drop
      i = popMathStack();
      break;
    case 9:
      // swap
      i = mathStack[0];
      mathStack[0] = mathStack[1];
      mathStack[1] = i;
      break;
    case 10:
      // <
      i = popMathStack();
      if(mathStack[0] < i){
        mathStack[0] = 1;
      } else {
        mathStack[0] = 0;
      }
      break;      
    case 11:
      // >
      i = popMathStack();
      if(mathStack[0] > i){
        mathStack[0] = 1;
      } else {
        mathStack[0] = 0;
      }
      break;      
    case 12:
      // =
      i = popMathStack();
      if(i == mathStack[0]){
        mathStack[0] = 1;
      } else {
        mathStack[0] = 0;
      }
      break;      

    case 13:
      printHexByte(popMathStack());
      break;

    case 14:
      getWord();
      break;

    case 15:
      dfnFunc();
      break;

    case 16: // keyt
     // return a 1 if keys are in ring buffer
     //i = (inputRingPtrXin - inputRingPtrXout) & 0x0F;    // logical result assigned to i
     i = 0;
     pushMathStack(i);
     break;

    case 17: // allot
      prog[progPtr] = popMathStack();
      progPtr = progPtr + 1;
      if(progPtr >= PROG_SPACE){
        uart_puts((str_t *)"prog mem");
      }
      break;

    case 18:  // @
      i = mathStack[0];
      mathStack[0] = prog[i];
      break;

    case 19:  // !
      i = popMathStack();
      j = popMathStack();
      prog[i] = j;
      break;

    case 20: // not
      if(mathStack[0]){
        mathStack[0] = 0;
      } else {
        mathStack[0] = 1;
      }
      break;

    case 21: // list
      listFunction();
      break;

    case 22: // if
      ifFunc(0);
      break;

    case 23: // then      ( trapped in ':')
      break;

    case 24: // else      ( trapped in ':')
      break;

    case 25:  // begin
      pushAddrStack(progCounter);
      break;

    case 26:  // until
      i = popAddrStack();
      j = popMathStack();
      if(j == 0){
        addrStackPtr--;  // number is still there ... just fix the pointer
        progCounter = i;
      }
      break;    

    case 27:  // clrb   clear a/d converter buckets
      for(i=0;i<256;i++){
        buckets[i] = 0;
      }
      break;
      
    case 28:  // .h
      printHexWord(popMathStack());
      break;

    case 29:
      break;

    case 30:  // num
      /*uart_puts((str_t *)"in case 30");*/
      numFunc();
      break;

    case 31:  // push0
      pushMathStack(0);
      break;

    case 32:  // goto   ( for internal use only )
      ifFunc(1);
      break;

    case 33: // exec
      execFunc();
      break;

    case 34: // lu
      luFunc();
      break;

    case 35: // pushn   ( internal use only )
      pushnFunc();
      break;

    case 36: // over
      overFunc();
      break;

    case 37:  // push1
      pushMathStack(1);
      break;

    case 38: // pwrd
      uart_puts(wordBuffer);
      break;

    case 39: // emit
      uart_putchar(popMathStack());
      break;

    case 40: // ;
      i = progCounter;
      progCounter = popAddrStack();
      break;

    case 41: // @ read directly from memory address
      i = popMathStack();
      i = i >> 1;  // divide by to   
      j = dirMemory[i];
      pushMathStack(j);
      break;
      
    case 42: // ! write directly to memory address words only!
      i = popMathStack();  //  address to write to
      i = i >> 1;
      j = popMathStack();  //  value to write
      dirMemory[i] = j;
      break;

    case 43: // h@
      pushMathStack(progPtr);
      break;

    case 44: // do
      i = popMathStack();  // start of count
      j = popMathStack();  // end count
      k = progCounter;

      pushAddrStack(j);  // limit on count
      pushAddrStack(i);  // count  (I)
      pushAddrStack(k);  // address to remember for looping
      break;

    case 45: // loop
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
      
    case 46: // i
      j = addrStack[addrStackPtr+1];
      pushMathStack(j);
      break;

    case 47: // b@
      i = mathStack[0];
      mathStack[0] = buckets[i];
      break;
      
    case 48: // a!
      i = popMathStack();  // address
      j = popMathStack();  // value
      /*setDAC(i,j);*/
      break;

    case 49: // and
      mathStack[1] &= mathStack[0];
      popMathStack();
      break;

    case 50: // or
      mathStack[1] |= mathStack[0];
      popMathStack();
      break;

    case 51: // */    scale function
      x = popMathStack();
      y = popMathStack();
      z = mathStack[0];
      z = (z*y)/x;
      mathStack[0] = z;
      break;
      
    case 52: // key     get a key from input .... (wait for it)
      pushMathStack(uart_getchar());
      break;

    case 53: // cr
      uart_putchar(0x0D);
      uart_putchar(0x0A);
      break;

    case 54: // hist
      i = mathStack[0];
      mathStack[0] = buckets[i];
      break;
      
    case 55: // histclr
      for(i=0;i<260;i++){
        buckets[i] = 0;
      }
      break;

    case 56: // fasttimer
      i = (int16_t )fastTimer;
      i = i>>1;
      pushMathStack(i);
      break;

    case 57:  // slowtimer
      i = (int16_t )slowTimer;
      i = i>>1;
      pushMathStack(i);
      break;

    case 58: // hstat
      for(i=256;i<260;i++){
        printHexWord(buckets[i]);
        uart_putchar(' ');
      }
      break;

    case 59:
      for(i=0;i<256;i++){
        if(buckets[i]){
          printHexByte(i);
          uart_putchar(' ');
          printHexWord(buckets[i]);
          uart_putchar(0x0D);
          uart_putchar(0x0A);
        }
      }
      break;

    case 60: // fec
      /*
      printHexWord(fecShadow[2]);
      uart_putchar(' ');
      printHexWord(fecShadow[1]);
      uart_putchar(' ');
      printHexWord(fecShadow[0]);
      */
      break;      

    case 61: // fecset
      /*
      fecShadow[0] = popMathStack();   // lsb
      fecShadow[1] = popMathStack();
      fecShadow[2] = popMathStack();   //msb
      */
      /*sendToFEC(fecShadow);*/
      break;

    case 62: // fecbset
      /*
      i = popMathStack();
      if(i < 48 && i >= 0){
        j = i >> 4;  // find the byte
        i = i & 0x0F; // find the bit
        i = 1 << i;   // get the bit location
        fecShadow[j] |= i;
      }
      */
      /*sendToFEC(fecShadow);*/
      break;

    case 63: // fecbclr
      /*
      i = popMathStack();
      if(i < 48 && i >= 0){
        j = i >> 4;  // find the byte
        i = i & 0x0F; // find the bit
        i = 1 << i;   // get the bit location
        fecShadow[j] &= ~i;
      }
      */
      break;

    default:
      /*uart_puts((str_t *)"opcode ");      */
      break;
  }
}


#if defined(MSP430)
static int16_t __inline__ RAMerrors(void){
    int16_t errors;
    __asm__ __volatile__ ( "mov r9, %0\n" : [res] "=r" (errors));
    return errors;
}
#endif


void init_msp4th(void)
{
    volatile uint16_t i;

#if defined(MSP430)
    printNumber(RAMerrors());
    uart_puts((uint8_t *)"<-- RAM errors");
#endif


//  xit = 0;
    addrStackPtr = ADDR_STACK_SIZE;    // this is one past the end !!!! as it should be
    progCounter = 10000;
    progPtr = 1;			// this will be the first opcode
    i=0;
    cmdListPtr = 0;
    cmdList[0] = 0;
    progOpsPtr = 1;      // just skip location zero .... it makes it easy for us

    dirMemory = (void *) 0;   // its an array starting at zero

    lineBufferPtr = 0;
    for (i=0; i < LINE_SIZE; i++) {
        lineBuffer[i] = 0;
    }

    for (i=0; i < WORD_SIZE; i++) {
        wordBuffer[i] = 0;
    }

    getLine();
    pushMathStack(0);
}


void processLoop() // this processes the forth opcodes.
{
    volatile uint16_t opcode;
    volatile uint16_t tmp;

    while(1){
        uart_puts((str_t *)"processLoop()");

        printNumber(progCounter);
        uart_puts((str_t *)"<-- progCounter");

        if(progCounter > 9999){
            tmp = progCounter - 10000;
            opcode = progBi[tmp];
        } else {
            opcode = prog[progCounter];
        }

        printNumber(opcode);
        uart_puts((str_t *)"<-- opcode");

        progCounter = progCounter + 1;

        if(opcode > 19999){
            // this is a built in opcode
            execN(opcode - 20000);
        } else {
            pushAddrStack(progCounter);
            progCounter = progOps[opcode];
        }
    } // while ()
}






