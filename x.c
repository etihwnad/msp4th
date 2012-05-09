
// forth interp, written as simple as it can be.

// This version works!

// special version for debugging the Nathan chip.
// last update 3/9/08

#include <signal.h>

#include <io.h>
#include <iomacros.h>


#define DEBUG_STUFF 1		// just print lots of junk
#define CMD_LIST_SIZE 128
#define MATH_STACK_SIZE 16
#define ADDR_STACK_SIZE 32
#define PROG_SPACE 256
#define USR_OPCODE_SIZE 32

#define BI_PROG_SHIFT 10000

// expected I/O stuff
//   Port B 0x0001 in
//   Port B 0x0002 in
//   Port B 0x0004 in
//   Port B 0x0008 in
//   Port B 0x0010 out serial output
//   Port B 0x0020 in  serial input
//   Port B 0x0040 out main loop toggle
//   Port B 0x0080 out interrupt toggle
//   Port B 0x0100
//   Port B 0x0200
//   Port B 0x0400
//   Port B 0x0800
//   Port B 0x1000
//   Port B 0x2000
//   Port B 0x4000
//   Port B 0x8000

#define PADSR_ 0x2000       
#define PADIR_ 0x2004      // OEN
#define PAOUT_ 0x2008      // was ODR
#define PAPER_ 0x200C 

#define PBDSR_ 0x2002
#define PBDIR_ 0x2006
#define PBOUT_ 0x200A
#define PBIER_ 0x200E

#define SPI_SCR_ 0x4000
#define SPI_RDR_ 0x4002
#define SPI_TDR_ 0x4004
#define SPI_SR_  0x4006

#define TMR0_TCR_ 0x6000
#define TMR0_SR_  0x6002
#define TMR0_CNT_ 0x6004
#define TMR0_RA_  0x6006
#define TMR0_RB_  0x6008
#define TMR0_RC_  0x600A

sfrw(PADSR,PADSR_);
sfrw(PADIR,PADIR_);
sfrw(PAOUT,PAOUT_);
sfrw(PAPER,PAPER_);	// interrupt enable register

sfrw(PBDSR,PBDSR_);
sfrw(PBDIR,PBDIR_);
sfrw(PBOUT,PBOUT_);
sfrw(PBIER,PBIER_);	// interrupt enable register

sfrw(SPI_SCR,SPI_SCR_);
sfrw(SPI_RDR,SPI_RDR_);
sfrw(SPI_TDR,SPI_TDR_);
sfrw(SPI_SR ,SPI_SR_ );

sfrw(TMR0_TCR,TMR0_TCR_);
sfrw(TMR0_SR,TMR0_SR_);
sfrw(TMR0_CNT,TMR0_CNT_);
sfrw(TMR0_RA,TMR0_RA_);
sfrw(TMR0_RB,TMR0_RB_);
sfrw(TMR0_RC,TMR0_RC_);

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

int16_t mathStack[MATH_STACK_SIZE];

int16_t addrStack[ADDR_STACK_SIZE];
int16_t addrStackPtr;

int16_t prog[PROG_SPACE];  // user programs are placed here
int16_t progPtr;           // next open space for user opcodes
int16_t progOps[USR_OPCODE_SIZE];
int16_t progOpsPtr;
uint8_t cmdList[CMD_LIST_SIZE];  // just a string of user defined names
int16_t cmdListPtr;

int16_t subSecondClock;
int16_t fastTimer;
int16_t slowTimer;


int16_t *dirMemory;


uint16_t buckets[260];  // use buckets[256] for total


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
   20022,10026,  //  10 if       did we found the word in the dictionary
   
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
         
int16_t progCounter;

uint8_t lineBuffer[128];      /* input line buffer */

uint16_t lineBufferPtr;                 /* input line buffer pointer */
// uint8_t xit;                    /* set to 1 to kill program */

uint8_t wordBuffer[32];		// just get a word




// variables for the non interrupt driven output

volatile uint16_t outputCharN;
volatile uint16_t outputCharCntrN;

// variables for the interrupt driven I/O

volatile uint16_t outputChar;
volatile uint16_t outputCharCntr;
volatile uint16_t clicks;      // counts at 9,600 hz

uint16_t outputRing[16];
volatile uint16_t outputRingPtrXin;    // this is where the next char goes in 
volatile uint16_t outputRingPtrXout;   // where the next char will come out


volatile uint16_t inputChar;
volatile uint16_t inputCharX;
volatile uint16_t inputCharCntr;

volatile uint16_t inputCharBit;

uint8_t inputRing[16];
volatile uint16_t inputRingPtrXin;   // if Xin == Xout, the buffer is empty
volatile uint16_t inputRingPtrXout;


uint8_t inputBuf[128];  // hold input line for processing
uint8_t inputBufPtr;


int16_t fecShadow[3];



NAKED(_reset_vector__){
  __asm__ __volatile__("br #main"::);
}

uint16_t ad_int_tmp;

// the vector number does not matter .... we create the
// table at the end of the code, but they cant match


interrupt (0) junkInterrupt(void){
  // I just trap unused interrupts here
//  dumy1++;    // xxxxxx
}

interrupt(2) adcInterrupt(void){
  // read all 4 a/d converter ports
}


interrupt (4) timerInterrupt(void){

  // if the PBDSR is read in the next instruction after TMR0_SR
  // something didn't work ..... so I put 1 instruction next
  // and it worked much better.


  TMR0_SR = 0;

  clicks++;

  inputCharBit = PADSR;   

  inputCharBit = inputCharBit & 0x0020;

  if((clicks & 0x03) == 0){   // we are going to work at 2400 baud


    if(outputCharCntr){
      if(outputChar & 1){
        PAOUT |= 0x0010;
      } else {
        PAOUT &= 0xFFEF;
      }
      outputCharCntr--;
      outputChar = outputChar >> 1;
    } else {
      // we are not outputting anything .... check the buffer
      if(outputRingPtrXin != outputRingPtrXout){
        outputChar = outputRing[outputRingPtrXout];
        outputRingPtrXout++;
        outputRingPtrXout &= 0x0F;
        outputChar = outputChar << 1;
        outputChar += 0xFE00;
        outputCharCntr = 10;
      }
    } 

    // clock stuff     2400 hz
    if(fastTimer){
      fastTimer--;
    }
    subSecondClock--;
    if(subSecondClock == 0){
      subSecondClock = 2400;
      if(slowTimer){
        slowTimer--;
      }
    }
  }

  if(inputCharCntr){   // did we already get a start bit ??????
    // now get the data bits
    inputCharCntr--;

    if((inputCharCntr & 0x0003) == 0 && (inputCharCntr & 0x00FC) != 0){
      if(inputCharBit){
        // shift in a 1
        inputCharX |= 0x0100;
      }
      inputCharX = inputCharX >> 1;
     
      if(inputCharCntr == 0x04){  // last data bit
        inputRing[inputRingPtrXin] = inputCharX;
        inputRingPtrXin = (inputRingPtrXin+1) & 0x0F;
      }

    }

  } else {
    // waiting for start bit  
    if(inputCharBit == 0){
      // We have a start bit .... 
      inputCharCntr = 38;
      inputCharX = 0;
    }      
  } 





}

static void __inline__ delay(register unsigned int n){
  __asm__ __volatile__ (
      "1: \n"
      " dec	%[n] \n"
      " jne	1b \n"
      : [n] "+r"(n));
}

void emit(uint8_t c){
  uint8_t i;
  i = outputRingPtrXin;
  i = (i+1) & 0x0F;
  while(outputRingPtrXout != outputRingPtrXin){   // wait for output buffer to have space
    eint();
  }
  outputRing[outputRingPtrXin] = c;
  outputRingPtrXin = i;
}


uint8_t getKey(){
  uint8_t i;

  while(inputRingPtrXin == inputRingPtrXout){  // hang until we get a char
    eint();
  }
  i = inputRing[inputRingPtrXout++];
  inputRingPtrXout &= 0x0F;
  return(i);
} 




void initVars(){

  // I override the C startup code .... so I must init all vars.

  outputCharCntrN = 0;

  outputCharCntr = 0;
  inputCharCntr = 0;

  inputRingPtrXin = 0;
  inputRingPtrXout = 0;

  outputRingPtrXin = 0;
  outputRingPtrXout = 0;

  inputBufPtr = 0;


  
}


uint8_t getKeyB(){
  uint8_t i;
  i = lineBuffer[lineBufferPtr];
  if(i != 0) lineBufferPtr++;
  return(i);
}


void printHexByte(int16_t n);





void getLine(){
  int16_t i;
  lineBufferPtr = 0;

  emit(0x0D);
  emit(0x0A);
  emit('>');   // this is our prompt

  i = 1;
  while(i){  // just hang in loop until we get CR
    i = getKey();
    if(i == 0x08){
      if(lineBufferPtr > 0){
        emit(0x08);
        emit(' ');
        emit(0x08);
        lineBufferPtr--;
      }
    } else {
      emit(i);
      if(i == 0x0D){
        // hit cr
        lineBuffer[lineBufferPtr] = 0;
        i = 0;
      } else {

        lineBuffer[lineBufferPtr++] = i;
        lineBuffer[lineBufferPtr] = 0;

        if(lineBufferPtr > 125){  // prevent overflow of line buffer
          i=0;
        }
      }
    }
  }
  emit(0x0A);
  lineBufferPtr = 0;
}


void getWord(){
  int16_t k;
  uint8_t c;
  wordBuffer[0] = 0;
  while(wordBuffer[0] == 0){
    k = 0;
    c = getKeyB();
    while(( c <= ' ') && ( c != 0 )) c = getKeyB();    /* strip leading spaces */
    if( c > 0 ){
      if( c == '"' ){
        c = getKeyB();
        while((c != '"')&&(c != 0)){
          if(c != '"') wordBuffer[k++] = c;
          c = getKeyB();
        }
      } else {
        while(c > ' ' && c != 0){
          wordBuffer[k++] = c;
          c = getKeyB();
        }
      }
      wordBuffer[k] = 0;
    } else {
      wordBuffer[0] = 0;
      getLine();     
    }
  }
}

void printString(const uint8_t *c){
  while(c[0]){
    emit(c[0]);
    c++;
  }
}


int16_t sc(uint8_t *x,uint8_t *y){
  int16_t i;
  i = 1;
  while(x[0] != 0 && y[0] != 0){
    if(x[0] != y[0]){
      i = 0;
    }
    x++;
    y++;
  }
  return(i);
}

void inline listFunction(){
  printString(cmdListBi);
  printString(cmdListBi2);
  printString(cmdList);
}
  
int16_t popMathStack(){
  int16_t i,j;

  j = mathStack[0];
  for(i=1;i<MATH_STACK_SIZE;i++){
    mathStack[i-1] = mathStack[i];
  }

  return(j);
}

void pushMathStack(int16_t n){
  uint16_t i;
  for(i=MATH_STACK_SIZE - 2;i > 0;i--){
    mathStack[i] = mathStack[i-1];
  }
  mathStack[0] = n;
}

int16_t popAddrStack(){
  int16_t j;
  j = addrStack[addrStackPtr];
  addrStackPtr++;
  return(j);
}

void pushAddrStack(int16_t n){
  addrStackPtr--;
  addrStack[addrStackPtr] = n;
}

int16_t lookupToken(uint8_t *x,uint8_t *l){    // looking for x in l
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
  
  i = lookupToken(wordBuffer,(uint8_t *)cmdListBi);
  
  if(i){
    i += 20000;
    pushMathStack(i);
    pushMathStack(1);
  } else {
    // need to test internal interp commands
    i = lookupToken(wordBuffer,(uint8_t *)cmdListBi2);
    if(i){
      i += 10000;
      pushMathStack(i);
      pushMathStack(1);
    } else {
      i = lookupToken(wordBuffer,cmdList);
      if(i){
        pushMathStack(i);
        pushMathStack(1);
      } else {
        pushMathStack(0);
      }
    }
  }  
} 

void numFunc(){  // the word to test is in wordBuffer
  int16_t i,j,n;
  // first check for neg sign
  i = 0;
  if(wordBuffer[0] == '-'){
    i++;
  }
  if((wordBuffer[i] >= '0') && (wordBuffer[i] <= '9')){
    // it is a number 
    j = 1;
    // check if hex
    if(wordBuffer[0] == '0' && wordBuffer[1] == 'x'){
      // base 16 number ... just assume all characters are good
      i=2;
      n = 0;
      while(wordBuffer[i]){
        n = n << 4;
        n += wordBuffer[i] - '0';
        if(wordBuffer[i] > '9'){
          n += -7;
        }
        i++;
      }
    } else {
      // base 10 number
      n = 0;
      while(wordBuffer[i]){
        n *= 10;
        n += wordBuffer[i] - '0';
        i++;
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

void ifFunc(uint8_t x){     // used as goto if x == 1
  int16_t addr;
  int16_t i;
  if(progCounter > 9999){
    addr = progBi[progCounter - 10000];
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
    if(!i){
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
  progCounter++;
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
    cmdList[cmdListPtr++] = wordBuffer[i];
    i++;
  }
  cmdList[cmdListPtr++] = ' ';
  cmdList[cmdListPtr] = 0;
  i = lookupToken(wordBuffer,cmdList);
  progOps[i] = progPtr;
}


void printNumber(int16_t n){
  int16_t k,x[7];
  int16_t i,j;
  k = n;
  if(k < 0){
    k = -k;
  }

  i=0;
  do{
    j = k % 10;
    k = k / 10;

    x[i++] = j + '0';
  }while(k);
  i--;
  
  if(n < 0){
    emit('-');
  }
  do{
    emit(x[i--]);
  }while(i >= 0);
  emit(' ');
}

void printHexChar(int16_t n){
  n &= 0x0F;
  if(n > 9){
    n += 7;
  }
  n += '0';
  emit(n);
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

void execN(int16_t n); // proto ... this could get recursive

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
  int16_t i,j,k,m;
  int32_t x,y,z;
  switch(n){
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
     i = (inputRingPtrXin - inputRingPtrXout) & 0x0F;    // logical result assigned to i
     pushMathStack(i);
     break;

    case 17: // allot
      prog[progPtr++] = popMathStack();
      if(progPtr >= PROG_SPACE){
        printString((uint8_t *)"prog mem");
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

//    case 23: // then      ( trapped in ':')
//      break;

//    case 24: // else      ( trapped in ':')
//      break;

    case 25:  // begin
      pushAddrStack(progCounter);
      break;

    case 26:  // until
      i = popAddrStack();
      j = popMathStack();
      if(!j){
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


    case 30:  // num
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
      printString(wordBuffer);
      break;

    case 39: // emit
      emit(popMathStack());
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
      k++;		   // up the count
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
      pushMathStack(getKey());
      break;

    case 53: // cr
      emit(0x0D);
      emit(0x0A);
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
      i = (int16_t )&fastTimer;
      i = i>>1;
      pushMathStack(i);
      break;

    case 57:  // slowtimer
      i = (int16_t )&slowTimer;
      i = i>>1;
      pushMathStack(i);
      break;

    case 58: // hstat
      for(i=256;i<260;i++){
        printHexWord(buckets[i]);
        emit(' ');
      }
      break;

    case 59:
      for(i=0;i<256;i++){
        if(buckets[i]){
          printHexByte(i);
          emit(' ');
          printHexWord(buckets[i]);
          emit(0x0D);
          emit(0x0A);
        }
      }
      break;

    case 60: // fec
      printHexWord(fecShadow[2]);
      emit(' ');
      printHexWord(fecShadow[1]);
      emit(' ');
      printHexWord(fecShadow[0]);
      break;      

    case 61: // fecset
      fecShadow[0] = popMathStack();   // lsb
      fecShadow[1] = popMathStack();
      fecShadow[2] = popMathStack();   //msb
      /*sendToFEC(fecShadow);*/
      break;

    case 62: // fecbset
      i = popMathStack();
      if(i < 48 && i >= 0){
        j = i >> 4;  // find the byte
        i = i & 0x0F; // find the bit
        i = 1 << i;   // get the bit location
        fecShadow[j] |= i;
      }
      /*sendToFEC(fecShadow);*/
      break;

    case 63: // fecbclr
      i = popMathStack();
      if(i < 48 && i >= 0){
        j = i >> 4;  // find the byte
        i = i & 0x0F; // find the bit
        i = 1 << i;   // get the bit location
        fecShadow[j] &= ~i;
      }
      break;

    default:
      printString((uint8_t *)"opcode ");      
      break;
  }
}

void processLoop(){            // this processes the forth opcodes.
  int16_t opcode;


  while(1){

    if(progCounter > 9999){
      opcode = progBi[progCounter - 10000];
    } else {
      opcode = prog[progCounter];
    }

    progCounter++;

    if(opcode > 19999){
      // this is a built in opcode
      execN(opcode - 20000);
    } else {
      pushAddrStack(progCounter);
      progCounter = progOps[opcode];
    }
  }
}


int main(void){
  int16_t i;

  PAPER = 0x000C;
  PAOUT = 0x0000;
  PADIR = 0x001F;  // set data direction registers

  initVars();

  TMR0_CNT = 0x0000;
  TMR0_SR = 0;
  TMR0_RC = 1059;
  TMR0_TCR = 0x003C;

  emit(0x00);   


//  xit = 0;
  addrStackPtr = ADDR_STACK_SIZE;    // this is one past the end !!!! as it should be
  progCounter = 10000;
  progPtr = 1;			// this will be the first opcode
  i=0;
  cmdListPtr = 0;
  cmdList[0] = 0;
  progOpsPtr = 1;      // just skip location zero .... it makes it easy for us

  dirMemory = (void *) 0;   // its an array starting at zero

  processLoop();

  return 0;
}

NAKED(_unexpected_){
 __asm__ __volatile__("br #main"::);

}


INTERRUPT_VECTORS = { 

   (void *)0x3C00,     // RST          just jump to next
   (void *)0x4030,     // NMI          restart at main
   main,               // External IRQ
   (void *)0x3C00,     // SPI IRQ
   (void *)0x3C00,     // PIO IRQ
   (void *)0x4030,     // Timer IRQ
   timerInterrupt,     // UART IRQ
   (void *)0x4030,      // ADC IRQ
   adcInterrupt  ,      // UMB IRQ
   (void *)0x3C00,
   (void *)0x3C00,
   (void *)0x3C00,
   (void *)0x3C00,
   (void *)0x3C00,
   (void *)0x4030,
   junkInterrupt
 };

