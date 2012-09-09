#
# Makefile for msp430
#
# 'make' builds everything
# 'make clean' deletes everything except source files and Makefile
# You need to set TARGET, MCU and SOURCES for your project.
# TARGET is the name of the executable file to be produced 
# $(TARGET).elf $(TARGET).hex and $(TARGET).txt and $(TARGET).map are all generated.
# The TXT file is used for BSL loading, the ELF can be used for JTAG use
# 
SHELL = /bin/bash

TARGET     = main
#MCU        = msp430f5529
MCU        = msp2
# List all the source files here
# eg if you have a source file foo.c then list it here
SOURCES = main.c ns430-uart.c msp4th.c
# Include are located in the Include directory
#INCLUDES = -IInclude
INCLUDES = -I.
# Add or subtract whatever MSPGCC flags you want. There are plenty more
#######################################################################################
#CFLAGS   = -mmcu=$(MCU) -g -Os -Wall -Wunused $(INCLUDES)   
CFLAGS   = -mmcu=$(MCU) -Wall -Wunused -mendup-at=main $(INCLUDES)
#ASFLAGS  = -mmcu=$(MCU) -x assembler-with-cpp -Wa,-gstabs
ASFLAGS  = -mmcu=$(MCU) -Wall -Wunused -mendup-at=main $(INCLUDES)
LDFLAGS  = -mmcu=$(MCU) -Wl,-Map=$(TARGET).map -T ldscript_ns430
########################################################################################
CC       = msp430-gcc
LD       = msp430-ld
AR       = msp430-ar
AS       = msp430-gcc
GASP     = msp430-gasp
NM       = msp430-nm
OBJCOPY  = msp430-objcopy
RANLIB   = msp430-ranlib
STRIP    = msp430-strip
SIZE     = msp430-size
READELF  = msp430-readelf
MAKETXT  = srec_cat
CP       = cp -p
RM       = rm -f
MV       = mv
########################################################################################
# the file which will include dependencies
DEPEND = $(SOURCES:.c=.d)

# all the object files
OBJECTS = $(SOURCES:.c=.o)

# all asm files
ASSEMBLYS = $(SOURCES:.c=.lst)

#all: $(TARGET).elf $(TARGET).hex $(TARGET).txt 
all: $(TARGET).elf $(TARGET).hex $(TARGET).xout $(ASSEMBLYS)

$(TARGET).elf: $(OBJECTS)
	@echo "Linking $@"
	$(CC) $(OBJECTS) $(LDFLAGS) $(LIBS) -o $@
	@echo
	@echo ">>>> Size of Firmware <<<<"
	$(SIZE) $(TARGET).elf
	@echo

%.hex: %.elf
	$(OBJCOPY) -O ihex $< $@

%.xout: %.elf
	msp430-objdump --disassemble-all $< \
	    | ../inc2syms.py ../ns430-atoi.inc \
	    | ../rename_regs.sh > $@

%.txt: %.hex
	$(MAKETXT) -O $@ -TITXT $< -I
	unix2dos $(TARGET).txt
#  The above line is required for the DOS based TI BSL tool to be able to read the txt file generated from linux/unix systems.
#
%.o: %.c
	@echo "Compiling $<"
	$(CC) -c $(CFLAGS) -o $@ $<

# rule for making assembler source listing, to see the code
%.lst: %.c
	@#$(CC) -S $(ASFLAGS) -Wa,-anlhd -o $@ $<
	$(CC) -S $(ASFLAGS) -o $@ $<

# include the dependencies unless we're going to clean, then forget about them.
ifneq ($(MAKECMDGOALS), clean)
-include $(DEPEND)
endif

# dependencies file
# includes also considered, since some of these are our own
# (otherwise use -MM instead of -M)
%.d: %.c
	@echo "Generating dependencies $@ from $<"
	$(CC) -M ${CFLAGS} $< >$@

.PHONY:	flash
flash: $(TARGET).hex
	./flash.py -e $(TARGET).hex

#.SILENT:
.PHONY:       clean
clean:
	$(RM) $(OBJECTS)
	$(RM) $(TARGET).{elf,hex,txt,map}
	$(RM) $(SOURCES:.c=.lst)
	$(RM) $(DEPEND)

