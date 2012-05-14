
// forth interp, written as simple as it can be.

// This version works!

// special version for debugging the Nathan chip.
// last update 3/9/08

#include <signal.h>

#include <io.h>
#include <iomacros.h>

#include <msp430/common.h>

#include "ns430-atoi.h"
#include "ns430-uart.h"


int16_t *dirMemory;


NAKED(_reset_vector__){
  __asm__ __volatile__("br #main"::);
}

// the vector number does not matter .... we create the
// table at the end of the code, but they cant match
interrupt (0) junkInterrupt(void){
  // I just trap unused interrupts here
}

interrupt(2) adcInterrupt(void){
  // read all 4 a/d converter ports
}



interrupt (4) timerInterrupt(void){
} 


static void __inline__ delay(register unsigned int n){
  __asm__ __volatile__ (
      "1: \n"
      " dec	%[n] \n"
      " jne	1b \n"
      : [n] "+r"(n));
}

void emit(uint8_t c){
}


uint8_t getKey(void){
    return 0;
} 




void initVars(){
  // I override the C startup code .... so I must init all vars.
}



void printString(const uint8_t *c){
  while(c[0]){
    emit(c[0]);
    c++;
  }
}




int main(void){

  PAPER = 0x0000;
  PAOUT = 0x0000;
  PAOEN = BIT5 | BIT4;

  initVars();

  /*emit(0x00);   */

  /*dirMemory = (void *) 0;   // its an array starting at zero*/

  while (1) {
      PAOUT = 0x0000;
      delay(10);
      PAOUT = 0xffff;
      delay(10);
  }

  return 0;
}

NAKED(_unexpected_){
 __asm__ __volatile__("br #main"::);

}


INTERRUPT_VECTORS = { 

   main,     // RST          just jump to next
   main,     // NMI          restart at main
   main,               // External IRQ
   main,     // SPI IRQ
   main,     // PIO IRQ
   main,     // Timer IRQ
   main,     // UART IRQ
   main,      // ADC IRQ
   main,      // UMB IRQ
   main,
   main,
   main,
   main,
   main,
   main,
   main
 };

