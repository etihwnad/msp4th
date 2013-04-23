.include "ns430-atoi.inc"


; msp430-gcc uses r[0,1,2] not pc,sp,sr
#define pc r0
#define sp r1
#define sr r2



;.global FlashLoadStart
;FlashLoadStart:
;setup stack pointer to end of RAM
    mov     #-80,r1 ;#0xffb0

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
    mov     #0x000e,&PAPER  ;#0x000e

;enable SPI0
; set CPOL,CPHA = 1,1
; 8-bit transfers
    mov     #0x0070,&SPI0_CR
;bring flash /CS pin low
    bic     #1,     &PAOUT  ;r3 As==01
;release flash from deep sleep
    mov     #0xab00,&SPI0_TDR       ;#0xab00
; wait until command done sending
    bit     #1,     &SPI0_SR        ;r3 As==01
    jz      $-4             ;abs 0x30ea
;now use 16-bit transfers
    mov     #0x00f0,&SPI0_CR        ;#0x00f0
;de-select flash /CS
    bis     #1,     &PAOUT  ;r3 As==01

;check RAM for errors
;writes error count to 0x263e
;address is not present in fabbed mmap
; r9 is not modified later in bootcode
;  (to read count later in early user code)
;  all function calls push/pop r9
;  user code may initially read r9 to get the RAM error count
    clr     r9              
    mov     #-21846,r5      ;#0xaaaa
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
    mov     r9,     &0x263e 

;check state of PA(7)
; PA.7 low -> invoke bootstrap loader
; PA.7 high -> copy code from flash
    bit     #128,   &PADSR  ;#0x0080
    ;jz      $+158   ;#gotoMain    ;abs 0x3148
    jz      4f   ;#gotoMain    ;abs 0x3148

;CopyFromFlash:
1:
;setup CRC
    mov     #0,     &CRCINIRES    ;r3 As==00
;check state of PA.9 (aka I2C SCL)
;r13 holds the flag
; PA.9 low -> do not byteswap
; PA.9 high -> byteswap words from flash
    clr     r13             
    bit     #512,   &PADSR  ;#0x0200
    jz      $+4             ;abs 0x30c6
    mov     #1,     r13     ;r3 As==01
;r7 holds start of RAM address
;r8 (end of RAM)+1
    mov     #16384, r7      ;#RAMStart
    mova    #0x10000,r8     
;select flash /CS line
    bic     #1,     &PAOUT  ;r3 As==01
;flash command 0x03 - read data bytes
;                00 - address MSB
;                r7 - address low word (=RAMStart)
    mov     #768,   &SPI0_TDR       ;#0x0300
    mov     r7,     &SPI0_TDR       
;wait for transmissions to finish
    bit     #1,     &SPI0_SR        ;r3 As==01
    jz      $-4             ;abs 0x30e0
;send 0x0000 to flash
    mov     #0,     &SPI0_TDR       ;r3 As==00
    bit     #1,     &SPI0_SR        ;r3 As==01
    jz      $-4             ;abs 0x30ea
;send second 0x0000 to flash
    mov     #0,     &SPI0_TDR       ;r3 As==00
;clear status register (any r/w does so)
    mov     #0,     &SPI0_SR        ;r3 As==00

;MainLoop1:
2:
;wait for return word
    bit     #4,     &SPI0_SR        ;r2 As==10
    jz      $-4             ;abs 0x30f8
;store received word
    mov     &SPI0_RDR,r5    
;send another 0x0000 to SPI
    mov     #0,     &SPI0_TDR       ;r3 As==00
;?swap bytes?
    tst     r13             
    ;jz      $+4     ;#SPINoSwap        ;abs 0x310c
    jz      3f     ;#SPINoSwap        ;abs 0x310c
    swpb    r5              

;SPINoSwap:
3:
;copy received word to RAM address in r7 (ini=RAMStart)
    mov     r5,     0(r7)   ;CRCDI_L(r7)
;compute CRC of data
;increment r7
    mov     @r7+,   &CRCDI        
;?reached end of RAM?
    cmpa    r7,     r8      
    ;jnz     $-30    ;#MainLoop1     ;abs 0x30f8
    jnz     2b   ;#MainLoop1     ;abs 0x30f8

;wait for last transmission to finish
    bit     #2,     &SPI0_SR        ;r3 As==10
    jz      $-4             ;abs 0x3118
;deselect flash /CS line
    bis     #1,     &PAOUT  ;r3 As==01

;enable SPI0
; set CPOL,CPHA = 1,1
; 8-bit transfers
    mov     #112,   &SPI0_CR        ;#0x0070
;select flash /CS line
    bic     #1,     &PAOUT  ;r3 As==01
;flash command: deep power down
    mov     #-18176,&SPI0_TDR       ;#0xb900
;wait for completion
;deselect flash /CS line
    bit     #2,     &SPI0_SR        ;r3 As==10
    jz      $-4             ;abs 0x3132
    bis     #1,     &PAOUT  ;r3 As==01

;disable PA peripherals
    mov     #0,     &PAPER  ;r3 As==00
;disable PA output drivers
    mov     #0,     &PAOEN  ;r3 As==00
;execute user code starting at address pointed to by 0xfffe
    br      &0xfffe 

4:

; syntax from:  git://github.com/vim-scripts/msp.vim.git
; vim: ft=msp
