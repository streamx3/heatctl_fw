PROJECT=heatctl
DEPS=Makefile
SOURCES=main.c avr-ds18b20/src/onewire.c avr-ds18b20/src/ds18b20.c
CC=avr-gcc
OBJCOPY=avr-objcopy
MMCU=atmega328p

AVRDUDECMD=avrdude -p m328p -c usbasp
CFLAGS=-mmcu=$(MMCU) -Os -Wl,--relax -fno-tree-switch-conversion -frename-registers -g -Wall -W -pipe -flto -flto-partition=none -fwhole-program -std=gnu99 -Wno-main

$(PROJECT).hex: $(PROJECT).out
	$(AVRBINDIR)$(OBJCOPY) -j .text -j .data -O ihex $(PROJECT).out $(PROJECT).hex

$(PROJECT).out: $(SOURCES) $(DEPS)
	$(AVRBINDIR)$(CC)  $(CFLAGS) -I./ -I./avr-ds18b20/include/ -o $(PROJECT).out $(SOURCES)
	$(AVRBINDIR)avr-size $(PROJECT).out

program:
	$(AVRDUDECMD) -U flash:w:$(PROJECT).hex

clean:
	-rm -f $(PROJECT).* *.o

all: clean $(PROJECT).hex program
