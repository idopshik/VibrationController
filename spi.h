/*
    Copyright (c) 2007 Stefan Engelke <mbox@stefanengelke.de>

*/

#ifndef _SPI_H_
#define _SPI_H_

#include <avr/io.h>
 
// Третий бит - это удвоение. Он в функции сдвигается >> дважды и накладывается на статус-регистр.
#define SPI_CLOCK_DIV4 0x00
#define SPI_CLOCK_DIV16 0x01
#define SPI_CLOCK_DIV64 0x02
#define SPI_CLOCK_DIV128 0x03
#define SPI_CLOCK_DIV2 0x04
#define SPI_CLOCK_DIV8  0x05
#define SPI_CLOCK_DIV32 0x06
//#define SPI_CLOCK_DIV64 0x07

#define SPI_MODE0 0x00
#define SPI_MODE1 0x04
#define SPI_MODE2 0x08
#define SPI_MODE3 0x0C

#define SPI_MODE_MASK 0x0C  // CPOL = bit 3, CPHA = bit 2 on SPCR // И 16МЕГА ТОЖЕ
#define SPI_CLOCK_MASK 0x03  // SPR1 = bit 1, SPR0 = bit 0 on SPCR // И 16МЕГА ТОЖЕ
#define SPI_2XCLOCK_MASK 0x01  // SPI2X = bit 0 on SPSR // И 16МЕГА ТОЖЕ


extern void spi_init();
void spi_extended_init(uint8_t mode,uint8_t clock_devider);
extern void spi_transfer_sync (uint8_t * dataout, uint8_t * datain, uint8_t len);
extern void spi_transmit_sync (uint8_t * dataout, uint8_t len);
extern uint8_t spi_fast_shift (uint8_t data);

uint16_t spiTransferWord(uint16_t data);

#endif /* _SPI_H_ */
