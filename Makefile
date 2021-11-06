
_pc4th: pc4th.c msp4th.c msp4th.h
	gcc -g -Wall -o _pc4th pc4th.c msp4th.c


.PHONY:       clean
clean:
	$(RM) _pc4th

