
00003000 <.sec1>:
;setup stack pointer to end of RAM
    3000:       31 40 b0 ff     mov     #-80,sp ;#0xffb0

;set PA(0) = 0 and enable output
;(should be) wired to flash /CS pin
    3004:       92 43 08 1a     mov     #1,     &PAOUT  ;r3 As==01
    3008:       82 43 0a 1a     mov     #0,     &PBOUT  ;r3 As==00
    300c:       92 43 04 1a     mov     #1,     &PAOEN  ;r3 As==01
    3010:       b2 40 00 80     mov     #-32768,&PBOEN  ;#0x8000
    3014:       06 1a 

;enable peripherals:
; MISO0
; MOSI0
; SCLK0
    3016:       b2 40 0e 00     mov     #14,    &PAPER  ;#0x000e
    301a:       0c 1a 
    301c:       0e 43           clr     r14             

;check PA(8), setup oscillator
; NOT CONTROLLING FABBED HARDWARE
    301e:       b2 b0 00 01     bit     #256,   &PADSR  ;#0x0100
    3022:       00 1a 
    3024:       04 24           jz      $+10            ;abs 0x302e
    3026:       1e 43           mov     #1,     r14     ;r3 As==01
;from above
; effect here is GPOUT0 = 0x00ca
    3028:       b2 40 ca 00     mov     #202,   &0x2416 ;#0x00ca
    302c:       16 24 

;enable SPI0
; set CPOL,CPHA = 1,1
; 8-bit transfers
    302e:       b2 40 70 00     mov     #112,   &SPI0_CR        ;#0x0070
    3032:       00 20 
;bring flash /CS pin low
    3034:       92 c3 08 1a     bic     #1,     &PAOUT  ;r3 As==01
;release flash from deep sleep
    3038:       b2 40 00 ab     mov     #-21760,&SPI0_TDR       ;#0xab00
    303c:       04 20 
; wait until command done sending
    303e:       a2 b3 06 20     bit     #2,     &SPI0_SR        ;r3 As==10
    3042:       fd 27           jz      $-4             ;abs 0x303e
;now use 16-bit transfers
    3044:       b2 40 f0 00     mov     #240,   &SPI0_CR        ;#0x00f0
    3048:       00 20 
;de-select flash /CS
    304a:       92 d3 08 1a     bis     #1,     &PAOUT  ;r3 As==01

;check RAM for errors
;writes error count to 0x263e
;address is not present in fabbed mmap
; r9 is not modified later in bootcode
;  (to read count later in early user code)
;  all function calls push/pop r9
;  user code may initially read r9 to get the RAM error count
    304e:       09 43           clr     r9              
    3050:       35 40 aa aa     mov     #-21846,r5      ;#0xaaaa
    3054:       37 40 00 40     mov     #16384, r7      ;#RAMStart
    3058:       38 40 00 60     mov     #24576, r8      ;#0x6000
    305c:       18 83           dec     r8              
    305e:       c8 1e 07 45     .rpt    r8
                                movx    r5,     r7      
    3062:       38 c0 0f 00     bic     #15,    r8      ;#0x000f
    3066:       08 93           tst     r8              
    3068:       f9 23           jnz     $-12            ;abs 0x305c
    306a:       37 40 00 40     mov     #16384, r7      ;#RAMStart
    306e:       38 40 00 60     mov     #24576, r8      ;#0x6000
    3072:       0b 45           mov     r5,     r11     
    3074:       5b 0e           rlam    #4,     r11     
    3076:       0a 43           clr     r10             
    3078:       18 83           dec     r8              
    307a:       c8 18 3a 57     addx    @r7+,   r10     
    307e:       0b 9a           cmp     r10,    r11     
    3080:       01 24           jz      $+4             ;abs 0x3084
    3082:       19 53           inc     r9              
    3084:       38 c0 0f 00     bic     #15,    r8      ;#0x000f
    3088:       08 93           tst     r8              
    308a:       f5 23           jnz     $-20            ;abs 0x3076
    308c:       35 93           cmp     #-1,    r5      ;r3 As==11
    308e:       08 24           jz      $+18            ;abs 0x30a0
    3090:       35 90 55 55     cmp     #21845, r5      ;#0x5555
    3094:       03 24           jz      $+8             ;abs 0x309c
    3096:       35 40 55 55     mov     #21845, r5      ;#0x5555
    309a:       dc 3f           jmp     $-70            ;abs 0x3054
    309c:       35 43           mov     #-1,    r5      ;r3 As==11
    309e:       da 3f           jmp     $-74            ;abs 0x3054
    30a0:       82 49 3e 26     mov     r9,     &0x263e 

;check state of PA(7)
; PA.7 low -> invoke bootstrap loader
; PA.7 high -> copy code from flash
    30a4:       b2 b0 80 00     bit     #128,   &PADSR  ;#0x0080
    30a8:       00 1a 
    30aa:       4e 24           jz      $+158   ;#gotoMain    ;abs 0x3148

CopyFromFlash:
;setup CRC
    30ac:       82 43 04 00     mov     #0,     &CRCINIRES    ;r3 As==00
;no effect, port B is NC
    30b0:       82 43 0a 1a     mov     #0,     &PBOUT  ;r3 As==00
    30b4:       b2 40 00 80     mov     #-32768,&PBOEN  ;#0x8000
    30b8:       06 1a 
;check state of PA.9 (aka I2C SCL)
;r13 holds the flag
; PA.9 low -> do not byteswap
; PA.9 high -> byteswap words from flash
    30ba:       0d 43           clr     r13             
    30bc:       b2 b0 00 02     bit     #512,   &PADSR  ;#0x0200
    30c0:       00 1a 
    30c2:       01 24           jz      $+4             ;abs 0x30c6
    30c4:       1d 43           mov     #1,     r13     ;r3 As==01
;no effect, port B is NC
    30c6:       82 43 06 1a     mov     #0,     &PBOEN  ;r3 As==00
;r7 holds start of RAM address
;r8 (end of RAM)+1
    30ca:       37 40 00 40     mov     #16384, r7      ;#RAMStart
    30ce:       88 01 00 00     mova    #0x10000,r8     
;select flash /CS line
    30d2:       92 c3 08 1a     bic     #1,     &PAOUT  ;r3 As==01
;flash command 0x03 - read data bytes
;                00 - address MSB
;                r7 - address low word (=RAMStart)
    30d6:       b2 40 00 03     mov     #768,   &SPI0_TDR       ;#0x0300
    30da:       04 20 
    30dc:       82 47 04 20     mov     r7,     &SPI0_TDR       
;wait for transmissions to finish
    30e0:       92 b3 06 20     bit     #1,     &SPI0_SR        ;r3 As==01
    30e4:       fd 27           jz      $-4             ;abs 0x30e0
;send 0x0000 to flash
    30e6:       82 43 04 20     mov     #0,     &SPI0_TDR       ;r3 As==00
    30ea:       92 b3 06 20     bit     #1,     &SPI0_SR        ;r3 As==01
    30ee:       fd 27           jz      $-4             ;abs 0x30ea
;send second 0x0000 to flash
    30f0:       82 43 04 20     mov     #0,     &SPI0_TDR       ;r3 As==00
;clear status register (any r/w does so)
    30f4:       82 43 06 20     mov     #0,     &SPI0_SR        ;r3 As==00

MainLoop1:
;wait for return word
    30f8:       a2 b2 06 20     bit     #4,     &SPI0_SR        ;r2 As==10
    30fc:       fd 27           jz      $-4             ;abs 0x30f8
;store received word
    30fe:       15 42 02 20     mov     &SPI0_RDR,r5    
;send another 0x0000 to SPI
    3102:       82 43 04 20     mov     #0,     &SPI0_TDR       ;r3 As==00
;?swap bytes?
    3106:       0d 93           tst     r13             
    3108:       01 24           jz      $+4     ;#SPINoSwap        ;abs 0x310c
    310a:       85 10           swpb    r5              
SPINoSwap:
;copy received word to RAM address in r7 (ini=RAMStart)
    310c:       87 45 00 00     mov     r5,     0(r7)   ;CRCDI_L(r7)
;compute CRC of data
;increment r7
    3110:       b2 47 00 00     mov     @r7+,   &CRCDI        
;?reached end of RAM?
    3114:       d8 07           cmpa    r7,     r8      
    3116:       f0 23           jnz     $-30    ;#MainLoop1     ;abs 0x30f8

;wait for last transmission to finish
    3118:       a2 b3 06 20     bit     #2,     &SPI0_SR        ;r3 As==10
    311c:       fd 27           jz      $-4             ;abs 0x3118
;deselect flash /CS line
    311e:       92 d3 08 1a     bis     #1,     &PAOUT  ;r3 As==01

;enable SPI0
; set CPOL,CPHA = 1,1
; 8-bit transfers
    3122:       b2 40 70 00     mov     #112,   &SPI0_CR        ;#0x0070
    3126:       00 20 
;select flash /CS line
    3128:       92 c3 08 1a     bic     #1,     &PAOUT  ;r3 As==01
;flash command: deep power down
    312c:       b2 40 00 b9     mov     #-18176,&SPI0_TDR       ;#0xb900
    3130:       04 20 
;wait for completion
;deselect flash /CS line
    3132:       a2 b3 06 20     bit     #2,     &SPI0_SR        ;r3 As==10
    3136:       fd 27           jz      $-4             ;abs 0x3132
    3138:       92 d3 08 1a     bis     #1,     &PAOUT  ;r3 As==01

;disable PA peripherals
    313c:       82 43 0c 1a     mov     #0,     &PAPER  ;r3 As==00
;disable PA output drivers
    3140:       82 43 04 1a     mov     #0,     &PAOEN  ;r3 As==00
;execute user code starting at address pointed to by 0xfffe
    3144:       10 42 fe ff     br      &0xfffe 
