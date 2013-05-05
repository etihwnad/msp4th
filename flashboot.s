.include "ns430-atoi.inc"


.equ StackStart, 0xff00
.equ RAMCodeStart, 0x4000

; msp430-gcc uses r[0,1,2] not pc,sp,sr


; setup stack pointer to end of RAM
; not used in this asm code, but arguably necessary for proper init of the C
; environment later
    mov     #StackStart,r1
    dint


; NOTE: we wakeup the flash chip (M25PE80) now to ensure we allow the
;       requisite 30us for wakeup from deep sleep into idle/active mode.
;set PA(0) = 0 and enable output
;(should be) wired to flash /CS pin
    mov     #1,     &PAOUT
    mov     #1,     &PAOEN

;enable peripherals:
; MISO0
; MOSI0
; SCLK0
;   output pins MOSI0 and SCLK0 are hard-wired as outputs
;   when in peripheral mode, no need to change PAOUT
    mov     #0x000e,&PAPER

;enable SPI0
; set CPOL,CPHA = 1,1
; 8-bit transfers
    mov     #0x0070,&SPI0_CR
;bring flash /CS pin low
    bic     #1,     &PAOUT
;release flash from deep sleep
    mov     #0xab00,&SPI0_TDR
; wait until command done sending
1:
    bit     #1,     &SPI0_SR
    jz      1b
;now use 16-bit transfers
    mov     #0x00f0,&SPI0_CR
;de-select flash /CS
    bis     #1,     &PAOUT

/*
 * old code.
 * I (Dan) do not quite fully understand what's going on.
 * The idea is to write/read RAM and accumulate the number of bad positions
 * in r9, for reading in later user code.
 *
 * If we *really* need a RAM tester, we can code one in ASM and run from the
 * beginning of RAM.  Writing user code to read r9 kindof implies *some* RAM is
 * working...
 *
;check RAM for errors
; r9 is not modified later in bootcode
;  (to read count later in early user code)
;  all function calls push/pop r9
;  user code may initially read r9 to get the RAM error count
    clr     r9              
    mov     #0xaaaa,r5      ;#0xaaaa
    mov     #16384, r7      ;#RAMStart
    mov     #24576, r8      ;#0x6000
    dec     r8              
    .rpt    r8
    movx    r5,     r7      
    bic     #15,    r8      ;#0x000f
    tst     r8              
    jnz     $-12            ;abs 0x305c
    mov     #16384, r7      ;#RAMStart
    mov     #24576, r8      ;#0x6000
    mov     r5,     r11     
    rlam    #4,     r11     
    clr     r10             
    dec     r8              
    addx    @r7+,   r10     
    cmp     r10,    r11     
    jz      $+4             ;abs 0x3084
    inc     r9              
    bic     #15,    r8      ;#0x000f
    tst     r8              
    jnz     $-20            ;abs 0x3076
    cmp     #-1,    r5      ;r3 As==11
    jz      $+18            ;abs 0x30a0
    cmp     #21845, r5      ;#0x5555
    jz      $+8             ;abs 0x309c
    mov     #21845, r5      ;#0x5555
    jmp     $-70            ;abs 0x3054
    mov     #-1,    r5      ;r3 As==11
    jmp     $-74            ;abs 0x3054
*/

;check state of PA(7)
; PA.7 low -> continue into default msp4th interpreter aka main()
; PA.7 high -> copy code from flash
    bit     #0x0080,&PADSR
    jz      8f  ;#ContinueMain

;CopyFromFlash:
;setup CRC
    mov     #0,     &CRCINIRES
;check state of PA.9 (aka I2C SCL)
;r13 holds the flag
; PA.9 low -> do not byteswap
; PA.9 high -> byteswap words from flash
    clr     r13             
    bit     #0x0200,&PADSR
    jz      2f
    mov     #1,     r13
2:
;r7 holds start of RAM address
;r8 (end of RAM)+1
;    mov     #RAMStart,r7
    mov     #0x6000,r7
    mova    #(RAMStart+RAMSize),r8
;select flash /CS line
    bic     #1,     &PAOUT
;flash command 0x03 - read data bytes
;                00 - address MSB
;                r7 - address low word (=RAMStart)
    mov     #0x0300,&SPI0_TDR
;    mov     r7,     &SPI0_TDR
    mov     #0x4000,&SPI0_TDR
;wait for transmissions to finish
3:
    bit     #1,     &SPI0_SR
    jz      3b
;send 0x0000 to flash
    mov     #0,     &SPI0_TDR
4:
    bit     #1,     &SPI0_SR
    jz      4b
;send second 0x0000 to flash
    mov     #0,     &SPI0_TDR
;clear status register (any r/w does so)
    mov     #0,     &SPI0_SR

;MainLoop1:
;wait for return word
5:
    bit     #4,     &SPI0_SR
    jz      5b
;store received word
    mov     &SPI0_RDR,r5    
;send another 0x0000 to SPI
    mov     #0,     &SPI0_TDR
;?swap bytes?
    tst     r13             
    jz      6f     ;#SPINoSwap
    swpb    r5              

;SPINoSwap:
6:
;copy received word to RAM address in r7 (ini=RAMStart)
    mov     r5,     0(r7)
;compute CRC of data
;increment r7
    mov     @r7+,   &CRCDI        
;?reached end of RAM?
    cmpa    r7,     r8      
    jnz     5b   ;#MainLoop1

;wait for last transmission to finish
    bit     #2,     &SPI0_SR
    jz      $-4
;deselect flash /CS line
    bis     #1,     &PAOUT

;enable SPI0
; set CPOL,CPHA = 1,1
; 8-bit transfers
    mov     #0x0070,&SPI0_CR
;select flash /CS line
    bic     #1,     &PAOUT  ;r3 As==01
;flash command: deep power down
    mov     #0xb900,&SPI0_TDR
;wait for completion
;deselect flash /CS line
7:
    bit     #2,     &SPI0_SR
    jz      7b
    bis     #1,     &PAOUT

;disable PA peripherals
    mov     #0,     &PAPER
;disable PA output drivers
    mov     #0,     &PAOEN
;execute user code starting at address pointed to by 0xfffe
;    br      &0xfffe

;ContinueMain
8:

; syntax from:  git://github.com/vim-scripts/msp.vim.git
; vim: ft=msp
