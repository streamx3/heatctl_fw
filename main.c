#define F_CPU 8000000L      // 8Mhz clock
#include <string.h>
#include <stdarg.h>
#include <avr/io.h>
#include <util/delay.h>

void initUSART(void) {
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
#define ___BUFSZ 256
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
    return print((uint8_t*)buf,(uint16_t)sz);
#undef __BUFSZ
}

int temp;

int main(){
    DDRB   = 0xff;

    char data;
    initUSART();

        printu("Init...\r\n");
        _delay_ms(1000);
    while(1){

                //Start conversion (without ROM matching)
        ds18b20convert( &PORTB, &DDRB, &PINB, ( 1 << 0 ), NULL );

        //Delay (sensor needs time to perform conversion)
        _delay_ms( 1000 );

        //Read temperature (without ROM matching)
        ds18b20read( &PORTB, &DDRB, &PINB, ( 1 << 0 ), NULL, &temp );
      // data = usartReadChar();
// actually, never reach

      // PORTB  = (1 << 0);
    }

    return 0;
}
