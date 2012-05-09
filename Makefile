# msp430-gcc -nostartfiles -mmcu=msp430x2013 -O2 -g -mendup-at=main test1.c


.SECONDARY:

%.a43: %.c
	msp430-gcc -nostartfiles -mmcu=msp430x2013 -O2 -g -Wall -Wa,-ahlms=$(<:.c=.lst) \
	    -mendup-at=main -o $@ $< -L -Xlinker -T ldscript_ns430

%.hex: %.a43
	msp430-objcopy -O ihex $< $@

%.xout: %.a43
	#msp430-objdump -dSt $< > $@
	#msp430-objdump --disassemble-all --architecture=msp:54 $< > $@
	msp430-objdump --disassemble-all $< > $@


clean:
	rm -f *.a43 *.hex *.xout *.lst


