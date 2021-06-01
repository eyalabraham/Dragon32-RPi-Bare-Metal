###############################################################################
#
# Raspberry Pi bare metal Makefile for Dragon 32 emulator on RPi
#
###############################################################################

# Remove existing implicit rules (specifically '%.o: %c')
.SUFFIXES:

#------------------------------------------------------------------------------
# Defines
#   TOOLDIR     - Tool chain directory
#   BINDIR      - Binary output directory
#   CCFLAGS     - C compiler flags
#   ASFLAGS     - Assembler flags
#   LDFLAGS     - Linker flags
#   CC          - Compiler
#   AS          - Assembler
#   LD          - Linker
#   OBJCOPY     - Object code conversion
#   ARMARCH     - Processor architecture
#------------------------------------------------------------------------------
include environment.mk

PIMODEL ?= RPI1

#------------------------------------------------------------------------------
# Define RPi model
#------------------------------------------------------------------------------
CCFLAGS += -D$(PIMODEL) -DRPI_MODEL_ZERO=1 -DRPI_BARE_METAL=1

#------------------------------------------------------------------------------------
# Dependencies
#------------------------------------------------------------------------------------
OBJDRAGON = start.o dragon.o \
            mem.o cpu.o \
            sam.o pia.o vdg.o \
            printf.o sdfat32.o loader.o \
            rpibm.o gpio.o auxuart.o timer.o spi0.o spi1.o mailbox.o irq.o irq_util.o

#------------------------------------------------------------------------------
# New make patterns
#------------------------------------------------------------------------------
%.o: %.c
	$(CC) $(CCFLAGS) -g -I $(INCDIR) -c $< -o $@

%.o: %.S
	$(AS) $(ASFLAGS) $< -o $@

#------------------------------------------------------------------------------
# Build all targets
#------------------------------------------------------------------------------
all: dragon

dragon: $(OBJDRAGON)
#	$(LD) $(LDFLAGS) -L $(LIBDIR1) -L $(LIBDIR2) -o $@.elf $? -lgpio -lprintf -lgcc -lg_nano
	$(LD) $(LDFLAGS) -L $(LIBDIR1) -L $(LIBDIR2) -o $@.elf $? -lgcc -lg_nano
	$(OBJCOPY) $@.elf -O binary $@.img
	cp $@.img $(BOOTDIR)/kernel.img

#------------------------------------------------------------------------------
# Cleanup
#------------------------------------------------------------------------------

.PHONY: clean

clean:
	rm -f *.elf
	rm -f *.o
	rm -f *.map
	rm -f *.lib
	rm -f *.bak
	rm -f *.hex
	rm -f *.out
	rm -f *.img

