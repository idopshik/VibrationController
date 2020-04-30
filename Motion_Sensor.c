/*
 * Motion_Sensor.c
 *
 * Created: 22.02.2016 1:31:09
 *  Author: isairon
 Используется библиотека Stefan Engelke по SPI
 */ 
// Выбрать чип - ОПУСТИТЬ строб CS
// При поднятом стробе чип ожидает приёма по TWI. Опускаем и обращаемся по SPI
//MSB вперёд

#include <avr/io.h>
#include <avr/interrupt.h>
#include "spi.h"
#include "Motion_Sensor.h"
// Для записи после отпускания СS начинается передача первого бита. До опускания CS - SCK тоже должен быть High.
// При его опускнии начинается считывание бита. (Чип работает по falling edge). Надо именно так настроить SPI модуль.

// Первый бит 0. Это чтение. Второй бит если нуль, то не будет происходить автоинкремента адресса при multiple sync

void SPI_accelerometr_Write(uint8_t Addr, uint8_t data)
{
	CS_PORT &= ~(1<<CS_unaffected_bit );
	spi_fast_shift (Addr & 0x3F ); // Отсекаем два старших бита (0-запись, 0- не инкрементировать адресс)
	spi_fast_shift (data); // Пишем байт.
	CS_PORT |= (1<<CS_unaffected_bit);
}

void SPI_accelerometr_Read(uint8_t Addr, uint8_t *reg)// Тело этой проще скопировать в основную прогу. Без всяких указателей сразу присвоить переменной возвращённый байт.
{
	CS_PORT &= ~(1<<CS_unaffected_bit );			 // LOW - выбора FLASH-чипа
	spi_fast_shift ((Addr & 0xBF)|0x80 ); // Ставим старший бит, и отсекаем следующий. (1-чтение, 0- не инкрементировать адресс)
	// Расскоментить и допилить, чтобы варрнинг не вылетал, не могу сейчас в пол третьего ночи догнать, почему варрнинг.
	///*reg = spi_fast_shift (Addr); // Пишем любой мусор, не важно, мы ВОЗВРАЩАЕМ прочитанное. Это важно.
	spi_transfer_sync (reg, reg, 1); // не понял почему, но через фастШифт не работает, хотя одно и то же, если всматриваться
	// Может как-то линковка или оптимизация что-то обрезают.
	
	CS_PORT |= (1<<CS_unaffected_bit); // Убрали выбор чипа - High
}
void SPI_accelerometr_Read_char(uint8_t Addr, signed char *reg) 
{
	CS_PORT &= ~(1<<CS_unaffected_bit );			 // LOW - выбора FLASH-чипа
	spi_fast_shift ((Addr & 0xBF)|0x80 ); // Ставим старший бит, и отсекаем следующий. (1-чтение, 0- не инкрементировать адресс)
	// Расскоментить и допилить, чтобы варрнинг не вылетал, не могу сейчас в пол третьего ночи догнать, почему варрнинг.
	///*reg = spi_fast_shift (Addr); // Пишем любой мусор, не важно, мы ВОЗВРАЩАЕМ прочитанное. Это важно.
	
	SPDR = *reg; //суём мусор
	while((SPSR & (1<<SPIF))==0);  // ждём
	*reg = SPDR; // присваиваем переменной по адрессу указателя значение принятого байта
	CS_PORT |= (1<<CS_unaffected_bit); // Убрали выбор чипа - High
}

uint8_t spiMEMS_ReadByte(uint8_t memAddr)
{
	uint8_t data;
	CS_PORT &= ~(1<<CS_unaffected_bit); // LOW - выбора акселерометра. 
	// send address
	
	//spi_fast_shift (((memAddr & 0xBF)|0x80));// Отсекаем два старших бита (0-запись, 0- не инкрементировать адресс)
	spi_fast_shift (memAddr);// вдруг накосячил
	
	// read contents of memory address
	data = spi_fast_shift(0xFF);// шлём no_care байт и возвращённый пишем в data
	
	CS_PORT |= (1<<CS_unaffected_bit); // Убрали выбор чипа - High
		
		
		return data;
	}
	
	uint8_t LIS_read__who_i_am (void)
	{
		uint8_t data;
		CS_PORT &= ~(1<<CS_unaffected_bit );			 // LOW - выбора FLASH-чипа
		spi_fast_shift(0x8F);    // Шлём опкод и адрес регистра "Кто я"
		data = spi_fast_shift(0x00);// шлём no_care байт и возвращённый пишем в data
		CS_PORT |= (1<<CS_unaffected_bit); // опустили строб
		return data;// read contents of memory address		
	}
	
	void LIS_read__who_i_am_2 (uint8_t * dataout)
	{
		CS_PORT &= ~(1<<CS_unaffected_bit );			 // LOW - выбора FLASH-чипа
		spi_fast_shift(0xCF);    // Шлём опкод и адрес регистра "Кто я"
		
		
		// read contents of memory address

		spi_transfer_sync( dataout, dataout,1);
		CS_PORT |= (1<<CS_unaffected_bit); // опустили строб

	}
		void LIS_read__who_i_am_3 (uint8_t * dataout)
		{
			CS_PORT &= ~(1<<CS_unaffected_bit );			 // LOW - выбора FLASH-чипа
			spi_transfer_sync( dataout, dataout, 2);
			CS_PORT |= (1<<CS_unaffected_bit); // Подняли строб

		}
