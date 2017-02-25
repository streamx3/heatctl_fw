PROJECT=heatctl
DEPS=Makefile
SOURCES=main.c
CC=avr-gcc
OBJCOPY=avr-objcopy
MMCU=atmega328p
PMCU=m328p
FREQ=8000000L

AVRDUDECMD=avrdude -p $(PMCU) -c usbasp
CFLAGS=-mmcu=$(MMCU) -Os -Wl,--relax -fno-tree-switch-conversion -frename-registers -g -Wall -W -pipe -flto -flto-partition=none -fwhole-program -std=gnu99 -Wno-main

$(PROJECT).hex: $(PROJECT).out
	$(AVRBINDIR)$(OBJCOPY) -j .text -j .data -O ihex $(PROJECT).out $(PROJECT).hex

$(PROJECT).out: $(SOURCES) $(DEPS)
	make -C avr-ds18b20 all F_CPU=$(FREQ) MCU=$(MMCU)
	$(AVRBINDIR)$(CC)  $(CFLAGS) -I./ -I./avr-ds18b20/include/ -o $(PROJECT).out $(SOURCES) avr-ds18b20/lib/libds18b20.a
	$(AVRBINDIR)avr-size $(PROJECT).out

program:
	$(AVRDUDECMD) -U flash:w:$(PROJECT).hex

clean:
	make -C avr-ds18b20 clean
	-rm -f $(PROJECT).* *.o

america_great_again: clean $(PROJECT).hex program
