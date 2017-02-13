#define F_CPU 8000000L      // 8Mhz clock
#include <string.h>
#include <stdarg.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h> // cli()
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
    printu("%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x;\r\n", rom[0], rom[1], rom[2], rom[3], rom[4], rom[5], rom[6], rom[7]);
}



#define ONEWIRE_ERROR_ENDSEARCH 	2

#define ONEWIRE_COMMAND_SEARCH_NORMAL 0xF0
#define ONEWIRE_COMMAND_SEARCH_ALERT  0xEC

union u_rom
{
    volatile uint64_t i64;
    volatile uint8_t i8[8];
};

struct onewireMap
{
    union u_rom rom;
    union u_rom cch; // cache
    union u_rom bmk; // bookmark
};

typedef struct onewireMap onewireMap;

uint8_t onewireSearchInit( onewireMap *map )
{
    map->rom.i64 = 0;
    map->bmk.i64 = 0;
    map->cch.i64 = 0;
}

uint8_t onewireSearch(
    volatile uint8_t *port,
    volatile uint8_t *direction,
    volatile uint8_t *portin,
    uint8_t mask,
    uint8_t alert,
    onewireMap* map)
{
    /***
    * For better understanding, please see:
    * https://www.maximintegrated.com/en/app-notes/index.mvp/id/187
    * http://pdfserv.maximintegrated.com/en/an/AN937.pdf
    */

    uint64_t cmask; // cache mask
    uint8_t sreg;
    uint8_t b; // temporal storage for next bit
    uint8_t rsp = 0; // next bit to select in address
    volatile uint64_t i = 1;

    sreg = SREG;
    // Communication check
    if ( onewireInit( port, direction, portin, mask ) == ONEWIRE_ERROR_COMM )
        return DS18B20_ERROR_COMM;

    onewireWrite( port, direction, portin, mask,
        (alert) ? ONEWIRE_COMMAND_SEARCH_ALERT : ONEWIRE_COMMAND_SEARCH_NORMAL );


    cli();

    map->rom.i64 = 0;
    for( ; i!= 0; i <<= 1 )
    {
        rsp = 0;
        b = 0;

        // Read first bit
        *port |= mask; //Write 1 to output
        *direction |= mask;
        *port &= ~mask; //Write 0 to output
        _delay_us( 2 );
        *direction &= ~mask; //Set port to input
        _delay_us( 5 );
        b |= *portin & mask; //Read input
        _delay_us( 60 );

        b <<= 1;

        // Read complementary bit
        *port |= mask; //Write 1 to output
        *direction |= mask;
        *port &= ~mask; //Write 0 to output
        _delay_us( 2 );
        *direction &= ~mask; //Set port to input
        _delay_us( 5 );
        b |= *portin & mask; //Read input
        _delay_us( 60 );


        if( map->bmk.i64 & i ){
            rsp = 1;
            // map->bmk.i64 = 0;
            printu("skipping\r\n");
            goto onewireSearch_wr_rsp;
        }
        switch(b){ // ... | bit | complementary bit;
            case(2):
                // printu("case 2\r\n");
                rsp = 1; // only 1s in current bit
                break;
            case(0):
                // 0s & 1s in current bit
                if(map->cch.i64 & i){
                    printu("-");
                    // dirty stupid hack,
                    // while zero comparison does not work for me
                    asm("nop");
                }else{
                    map->bmk.i64 = i;
                    printu("+");
                }
                // printu("]\r\n");
            case(1):
                // printu("case 1\r\n");
                break; // only 0s in current bit
            default:
            case(4):
                printu("case 4\r\n");
                // 4 is for no devices left in selected multiplicity
                return ONEWIRE_ERROR_ENDSEARCH;
        }


// Writing a bit we choose to follow this time
onewireSearch_wr_rsp:
        *port |= mask; //Write 1 to output
        *direction |= mask;
        *port &= ~mask; //Write 0 to output

        if ( rsp ) _delay_us( 8 );
        else _delay_us( 80 );

        *port |= mask;

        if ( rsp ) _delay_us( 80 );
        else _delay_us( 2 );

        if(rsp){
            map->rom.i64 |= i;
        }
    }
    map->cch.i64 = map->rom.i64;
    cmask = map->bmk.i64;
    // clear higher bits of cache, which are less significant for bookmark
    cmask <<= 1;
    while(cmask){
        map->cch.i64 &= ~(cmask);
        cmask <<= 1;
    }

onewireSearch_return:
    SREG = sreg;
    return ONEWIRE_ERROR_OK;
}

onewireMap map1;

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

    printu("Init...\r\n");


    printu("first rom [%d]:\r\n", onewireSearch(&PORTB, &DDRB, &PINB, ( 1 << 0 ), 0, &map1));
    printrom(&(map1.rom.i8));
    printu("bookmark:\r\n");
    printrom(&(map1.bmk.i8));
    printu("cache:\r\n");
    printrom(&(map1.cch.i8));

    printu("\nsecond rom [%d]:\r\n", onewireSearch(&PORTB, &DDRB, &PINB, ( 1 << 0 ), 0, &map1));
    printrom(&(map1.rom.i8));
    printu("bookmark:\r\n");
    printrom(&(map1.bmk.i8));
    printu("cache:\r\n");
    printrom(&(map1.cch.i8));

    printu("\nthird rom [%d]:\r\n", onewireSearch(&PORTB, &DDRB, &PINB, ( 1 << 0 ), 0, &map1));
    printrom(&(map1.rom.i8));
    printu("bookmark:\r\n");
    printrom(&(map1.bmk.i8));
    printu("cache:\r\n");
    printrom(&(map1.cch.i8));

    // printu("Known roms:\r\n");
    // printrom(rom[0]);
    // printrom(rom[1]);

    // retval = ds18b20convert( &PORTB, &DDRB, &PINB, ( 1 << 0 ), NULL );
    // printu("convert = %d;\r\n", retval);
    // _delay_ms(2000);

    // retval = ds18b20rom( &PORTB, &DDRB, &PINB, ( 1 << 0 ), troms[0]);
    // printu("readrom = %d;\r\n", retval);
    // printrom(troms[0]);
    while(1){
        // //Start conversion (without ROM matching)
        // retval = ds18b20convert( &PORTB, &DDRB, &PINB, ( 1 << 0 ), rom[0] );
        // // printu("convert1 = %d;\r\n", retval);
        // // Delay (sensor needs time to perform conversion)
        // _delay_ms(1000);

        // //Read temperature (without ROM matching)
        // retval = ds18b20read( &PORTB, &DDRB, &PINB, ( 1 << 0 ), rom[0], &temp1 );
        // // printu("read1 = %d;\r\n", retval);
        // _delay_ms(1000);

        // retval = ds18b20convert( &PORTB, &DDRB, &PINB, ( 1 << 0 ), rom[1] );
        // // printu("convert2 = %d;\r\n", retval);
        // _delay_ms(1000);

        // retval = ds18b20read( &PORTB, &DDRB, &PINB, ( 1 << 0 ), rom[1], &temp2 );
        // // printu("read2 = %d;\r\n", retval);
        // printu("temp1 = %d.%02d; temp2 = %d.%02d;\r\n", temp1/16, temp1%16, temp2/16, temp2%16);
        // _delay_ms(1000);
        asm("nop");
    }

    return 0;
}
