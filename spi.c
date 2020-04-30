/*
    Copyright (c) 2007 Stefan Engelke <mbox@stefanengelke.de>

*/

#include "spi.h"


#include <avr/io.h>
#include <avr/interrupt.h>
/////////Эти определения исключительно для spi_init и для spi_extended_init не нужны
#define PORT_SPI    PORTB
#define DDR_SPI     DDRB
/*
#define DD_MISO     PB6
#define DD_MOSI     PB5
#define DD_SS       PB4
#define DD_SCK      PB7
*/

#define DD_MISO     PB4
#define DD_MOSI     PB3
#define DD_SS       PB2
#define DD_SCK      PB5
void spi_extended_init(uint8_t mode,uint8_t clock_devider)
{
	
	////////////////////////////////////////// HARDWARE ////////////////////////////
	#ifdef __AVR_ATmega128__
	// setup SPI I/O pins
	PORTB|=(1<<1); // set SCK hi   sbi(DDRB, 1);  // set SCK as output
	DDRB&=~(1<<3);  // set MISO as input
	DDRB|=(1<<2);  // set MOSI as output
	DDRB|=(1<<0);  // SS must be output for Master mode to work
	#elif __AVR_ATmega8__||__AVR_ATmega8A__||__AVR_ATmega48__ ||__AVR_ATmega88__ || __AVR_ATmega168__
	// setup SPI I/O pins
	PORTB|=(1<<5);  // set SCK hi
	DDRB|=(1<<5);  // set SCK as output
	DDRB&=~(1<<4);  // set MISO as input
	DDRB|=(1<<3);  // set MOSI as output
	DDRB|=(1<<2);  // SS must be output for Mastermode to work
	 #elif __AVR_ATmega328P__
	 DDRB|=(1<<5);  // set SCK as output
	 PORTB|=(1<<5);  // set SCK hi
	 DDRB&=~(1<<4);  // set MISO as input
	 DDRB|=(1<<3);  // set MOSI as output
	 DDRB|=(1<<2);  // SS must be output for Mastermode to work
	#else
	// setup SPI I/O pins
	PORTB|=(1<<7);  // set SCK hi
	DDRB|=(1<<7);  // set SCK as output
	DDRB&=~(1<<6);  // set MISO as input
	DDRB|=(1<<5);  // set MOSI as output
	DDRB|=(1<<4);  // SS must be output for Master mode to work
	#endif
	

	////////////////////////////////////////// SPI CONTROL REGISTER SET ////////////////////////////
	//Подходит и для 328 и для 16 меги.

	 SPCR |= ((1<<SPE)|(1<<MSTR));  // SPI Enable Data Order:MSB first    Master select
	 SPCR = (SPCR & ~SPI_MODE_MASK) | mode;      // Маской убрали биты, а потом где надо, снова поставили
     SPCR = (SPCR & ~SPI_CLOCK_MASK) | (clock_devider & SPI_CLOCK_MASK);
     SPSR = (SPSR & ~SPI_2XCLOCK_MASK) | ((clock_devider >> 2) & SPI_2XCLOCK_MASK);

	
}


void spi_init()   // Только АтМега16
// Initialize pins for spi communication
{
	
    DDR_SPI &= ~(1<<DD_MISO);
    // Define the following pins as output
    DDR_SPI |= ((1<<DD_MOSI)|(1<<DD_SS)|(1<<DD_SCK));

  
    SPCR = ((1<<SPE)|               // SPI Enable
            (0<<SPIE)|              // SPI Interupt Enable (Нет)
            (0<<DORD)|              // Data Order (0:MSB first / 1:LSB first)
            (1<<MSTR)|              // Master/Slave select   
           // (1<<SPR1)|		// Убрали этот бит, теперь вкупе с удвоением скрость у нас будет /8  А это будет 1.5МГц.
			(1<<SPR0)|    // SPI Clock Rate f/128
			(1<<SPI2X)|               // ВСЁ ЭТО ВРЕМЯ СКОРОСТЬ БЫЛА МИНИМАЛЬНАЯ 93.7кГц.	Значит дело не в скорости.
            (1<<CPOL)| // Здесь      // Clock Polarity (0:SCK low / 1:SCK hi when idle)
            (1<<CPHA));				 // Clock Phase (0:leading / 1:trailing edge sampling)
	  /*
	   SPCR |= (1<<SPE)|(1<<MSTR)|(1<<CPOL)|(1<<SPR1)|(1<<SPR0); // 
	//PORT_SPI |= (1<<DD_SS); // НЕЛЬЗЯ поднимать CS!!!!! атмега переходит в слейв!!!!
	*/
}
/* Тело удачно работало с 93с66
    DDR_SPI &= ~(1<<DD_MISO);9b
    // Define the following pins as output
    DDR_SPI |= ((1<<DD_MOSI)|(1<<DD_SS)|(1<<DD_SCK));

    
    SPCR = ((1<<SPE)|               // SPI Enable
    (0<<SPIE)|              // SPI Interupt Enable (Нет)
    (0<<DORD)|              // Data Order (0:MSB first / 1:LSB first)
    (1<<MSTR)|              // Master/Slave select
    (1<<SPR0)|(0<<SPR1)|    // SPI Clock Rate f/16
    (0<<CPOL)|              // Clock Polarity (0:SCK low / 1:SCK hi when idle)
    (0<<CPHA));             // Clock Phase (0:leading / 1:trailing edge sampling)

    SPSR = (0<<SPI2X);              // Double Clock Rate( нет).
*/
/* Лень писать, надо как нибудь универсальную функцию написать

void spi_advanced_init()
{
	
}
*/

void spi_transfer_sync (uint8_t * dataout, uint8_t * datain, uint8_t len)
// Shift full array through target device
{
       uint8_t i;      
       for (i = 0; i < len; i++) {
             SPDR = dataout[i];
             while((SPSR & (1<<SPIF))==0);
             datain[i] = SPDR;
       }
}

void spi_transmit_sync (uint8_t * dataout, uint8_t len)
// Shift full array to target device without receiving any byte
{
       uint8_t i;      
       for (i = 0; i < len; i++) {
             SPDR = dataout[i];
             while((SPSR & (1<<SPIF))==0);
       }
}

uint8_t spi_fast_shift (uint8_t data)
// Clocks only one byte to target device and returns the received one
{
    SPDR = data;
    while((SPSR & (1<<SPIF))==0);
    return SPDR;
}

uint16_t spiTransferWord(uint16_t data)
{
	uint16_t rxData = 0;
	// send MS byte of given data
	rxData = (spi_fast_shift((data>>8) &
	0x00FF))<<8;
	// send LS byte of given data
	rxData |= (spi_fast_shift(data & 0x00FF));
	// return the received data
	return rxData;
}



