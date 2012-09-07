# msp430-gcc -nostartfiles -mmcu=msp430x2013 -O2 -g -mendup-at=main test1.c


.SECONDARY:

MAIN=x.c

default: $(MAIN:.c=.hex) $(MAIN:.c=.xout) $(MAIN:.c=.asm)

all: default flash

#msp430-gcc -nostartfiles -mmcu=msp430x2013 -O2 -g -Wall -Wa,-ahlms=$(<:.c=.lst)
%.a43: %.c
	msp430-gcc -O0 -nostartfiles -mmcu=msp430xG438 -Wall -Wa,-ahlms=$(<:.c=.lst) \
	    -mendup-at=main -o $@ $< -L -Xlinker -T ldscript_ns430

%.asm: %.c
	msp430-gcc -S -O0 -nostartfiles -mmcu=msp430xG438 -Wall \
	    -mendup-at=main -o $@ $<

%.hex: %.a43
	msp430-objcopy -O ihex $< $@

%.xout: %.a43
	@#msp430-objdump -dSt $< > $@
	@#msp430-objdump --disassemble-all --architecture=msp:54 $< > $@
	msp430-objdump --disassemble-all $< > $@

flash: $(MAIN:.c=.hex)
	./flash.py $<

clean:
	rm -f *.a43 *.hex *.xout *.lst *.asm


