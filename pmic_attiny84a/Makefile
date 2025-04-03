# Author: Norman Zhi (normanzyb@gmail.com)

TRG	= pmic
SRC	= main.cpp led.cpp key.cpp dev.cpp i2c.cpp i2c_regs.cpp adc.cpp timer1.cpp wdt.cpp sht.cpp

MCU_TARGET	= attiny84a
MCU_FREQ	= 8000000

EFUSE		= 0xff
# DWEN disabled to save power
# Keep EEPROM content through chip erase
# BOD disabled
HFUSE		= 0xd7
# Internal RC, 8MHz
LFUSE		= 0xe2

CROSS_COMPILE	= avr-

OPTIMIZE	= -O3 -flto
DEFS		+= -DF_CPU=$(MCU_FREQ)

AR		= $(CROSS_COMPILE)ar
AS		= $(CROSS_COMPILE)gcc
CC		= $(CROSS_COMPILE)gcc
CXX		= $(CROSS_COMPILE)g++
LD		= $(CROSS_COMPILE)g++
SIZE	= $(CROSS_COMPILE)size
OBJCOPY	= $(CROSS_COMPILE)objcopy
OBJDUMP	= $(CROSS_COMPILE)objdump

AVRDUDE	= sudo avrdude

CPPFLAGS	= -mmcu=$(MCU_TARGET) -std=gnu++11 $(OPTIMIZE) $(DEFS) #-fstack-usage
LDFLAGS		= -mmcu=$(MCU_TARGET) -std=gnu++11 $(OPTIMIZE) #-Wl,--verbose
LIBS		=

.PHONY: all
all: $(TRG).elf $(TRG).hex eeprom.bin

%.hex: %.elf
	$(OBJCOPY) -j .text -j .data -O ihex $< $@

%.elf: $(SRC:%.cpp=%.o)
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)
	$(SIZE) $@

%.o: %.cpp
	$(CXX) $(CPPFLAGS) -c -o $@ $<

eeprom.bin: eeprom.py
	./$< -o $@

.PHONY: clean
clean:
	rm -f $(SRC:%.cpp=%.o) $(TRG).elf $(TRG).hex eeprom.bin
