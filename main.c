#define F_CPU 8000000L      // 8Mhz clock
#include <string.h>
#include <stdarg.h>
#include <avr/io.h>
#include <util/delay.h>
#include <ds18b20.h>

void initUSART(void){
    #define BAUD 9600
    #include <util/setbaud.h> 
    UBRR0H = UBRRH_VALUE;                             /* defined in setbaud.h */
    UBRR0L = UBRRL_VALUE;
#if USE_2X
    UCSR0A |= (1 << U2X0);
#else
    UCSR0A &= ~(1 << U2X0);
#endif
/* Enable USART transmitter/receiver */
    UCSR0B = (1 << TXEN0) | (1 << RXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);        /* 8 data bits, 1 stop bit */
}

char usartReadChar(){
    while(! (UCSR0A & (1 << RXC0))){}
    return UDR0;
}

void usartWriteChar(char data){
    while(! (UCSR0A & (1 << UDRE0))){}
    UDR0 = data;
}

void print(uint8_t *data, uint8_t size){
    uint8_t i;
    uint8_t *p;
    for(i = 0; i < size; ++i){
        p = data + i;
        if(p){
            usartWriteChar(*p);
        }
    }
}

void printu(const char* format, ...){
#define ___BUFSZ 0xFF
    static char buf[___BUFSZ];
    int16_t sz;
    va_list argptr;
    if(format == NULL){
        return;
    }
    va_start(argptr, format);
    sz = (int16_t)vsnprintf(buf, ___BUFSZ, format, argptr);
    if(sz < 0){
        return;
    }
    va_end(argptr);
    return print((uint8_t*)buf,(uint8_t)sz);
#undef __BUFSZ
}

void printrom(uint8_t* rom){
    printu("%x-%x-%x-%x-%x-%x-%x-%x;\r\n", rom[0], rom[1], rom[2], rom[3], rom[4], rom[5], rom[6], rom[7]);
}

int main(){
    uint16_t temp1, temp2;
    uint8_t retval;

    // If you are reading this, you should write here your 64-bit constants
    // instead of mine constants, while they are meant to be unique.
    uint8_t rom[2][8] = {
        {0x28,0x43,0x8f,0x33,0x8,0x0,0x0,0x73},
        {0x28,0xd7,0x78,0x33,0x8,0x0,0x0,0xe9}
    };
    uint8_t troms[2][8] = {{0},{0}};
    DDRB   = 0xff;

    initUSART();

    printu("Init...\r\nKnown roms:\r\n");
    printrom(rom[0]);
    printrom(rom[1]);
    

    // retval = ds18b20convert( &PORTB, &DDRB, &PINB, ( 1 << 0 ), NULL );
    // printu("convert = %d;\r\n", retval);
    // _delay_ms(2000);

    // retval = ds18b20rom( &PORTB, &DDRB, &PINB, ( 1 << 0 ), troms[0]);
    // printu("readrom = %d;\r\n", retval);
    // printrom(troms[0]);
    while(1){
        //Start conversion (without ROM matching)
        retval = ds18b20convert( &PORTB, &DDRB, &PINB, ( 1 << 0 ), rom[0] );
        // printu("convert1 = %d;\r\n", retval);
        // Delay (sensor needs time to perform conversion)
        _delay_ms(1000);

        //Read temperature (without ROM matching)
        retval = ds18b20read( &PORTB, &DDRB, &PINB, ( 1 << 0 ), rom[0], &temp1 );
        // printu("read1 = %d;\r\n", retval);
        _delay_ms(1000);

        retval = ds18b20convert( &PORTB, &DDRB, &PINB, ( 1 << 0 ), rom[1] );
        // printu("convert2 = %d;\r\n", retval);
        _delay_ms(1000);

        retval = ds18b20read( &PORTB, &DDRB, &PINB, ( 1 << 0 ), rom[1], &temp2 );
        // printu("read2 = %d;\r\n", retval);
        printu("temp1 = %d.%d; temp2 = %d.%d;\r\n", temp1/16, temp1%16, temp2/16, temp2%16);
        _delay_ms(1000);
    }

    return 0;
}
