#####################################################################################
#
#  This make file should be included in order to define GCC tool-chain environment
#
#####################################################################################

#
# TODO Update creation of ARMARCH according to script logic below.
#
# Determine if the model is a b+ model or not
# We need to know if the raspberry pi model is b+ because the early RPI1 models have a different IO
# pin layout between the original units and the later B+ units. Similarly the RPI3 models have a
# different IO arrangement on the 3B+ units compared to the original 3B. We need to be able to
# be able to adjust the code for whichever model we're targeting
#case "${model}" in
#    rpi0*)
#        cflags="${cflags} -mfpu=vfp"
#        cflags="${cflags} -march=armv6zk"
#        cflags="${cflags} -mtune=arm1176jzf-s"
#        ;;
#
#    rpi1*)
#        cflags="${cflags} -mfpu=vfp"
#        cflags="${cflags} -march=armv6zk"
#        cflags="${cflags} -mtune=arm1176jzf-s"
#        ;;
#
#    rpi2*)
#        cflags="${cflags} -mfpu=neon-vfpv4"
#        cflags="${cflags} -march=armv7-a"
#        cflags="${cflags} -mtune=cortex-a7"
#        ;;
#
#    rpi3*)
#        cflags="${cflags} -mfpu=crypto-neon-fp-armv8"
#        cflags="${cflags} -march=armv8-a+crc"
#        cflags="${cflags} -mcpu=cortex-a53"
#        ;;
#
#    rpi4*) cflags="${cflags} -DRPI4"
#        cflags="${cflags} -mfpu=crypto-neon-fp-armv8"
#        cflags="${cflags} -march=armv8-a+crc"
#        cflags="${cflags} -mcpu=cortex-a72"
#        ;;
#
#    *) echo "Unknown model type ${model}" >&2 && exit 1
#        ;;
#esac

#------------------------------------------------------------------------------------
# GCC cross compiler tool chain installation 
#------------------------------------------------------------------------------------

TOOLDIR = ~/bin/gcc-arm-none-eabi-9-2019-q4-major

CC = $(TOOLDIR)/bin/arm-none-eabi-gcc
LD = $(TOOLDIR)/bin/arm-none-eabi-ld
AS = $(TOOLDIR)/bin/arm-none-eabi-as
AR = $(TOOLDIR)/bin/arm-none-eabi-ar
OBJCOPY = $(TOOLDIR)/bin/arm-none-eabi-objcopy

LIBDIR1 = $(TOOLDIR)/arm-none-eabi/lib				# libc_nano (Newlib)
LIBDIR2 = $(TOOLDIR)/lib/gcc/arm-none-eabi/9.2.1	# libgcc

#------------------------------------------------------------------------------------
# Project directory structure
#------------------------------------------------------------------------------------

BASE = ~/data/projects/dragon/code/dragon32bm
INCDIR = $(BASE)/include
BOOTDIR = $(BASE)/boot

#------------------------------------------------------------------------------------
# Environment architecture
# TODO Remove the dead/unused functions:
#      - Compile with “-fdata-sections” to keep the data in separate data sections and “-ffunction-sections” to keep functions in separate sections, so they (data and functions) can be discarded if unused.
#      - Link with “–gc-sections” to remove unused sections.
#------------------------------------------------------------------------------------

# *** The settings commented out below are not compatible with the stock newlib ***
#ARMARCH = -march=armv6zk -mtune=arm1176jzf-s -mfpu=vfp
#CCFLAGS = -I $(INCDIR) -O0 -Wall -mfloat-abi=hard $(ARMARCH)

ARMARCH = -march=armv6zk -mtune=arm1176jzf-s
CCFLAGS = -O2 -Wall -ffreestanding -mfloat-abi=soft $(ARMARCH)

ASFLAGS = --no-pad-sections
LDFLAGS = -z max-page-size=1024

