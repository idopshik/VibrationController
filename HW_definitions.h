/*
 * HW_definitions.h
 *
 * Created: 05.06.2016 10:30:05
 *  Author: isairton
 */ 

#ifndef HW_DEFINITIONS_H_
#define HW_DEFINITIONS_H_

#include <avr/io.h>

 /*
 ///////////////////////////// Карта физического подключения////////////////////////////////////////////////
 Выводы на LIS
 PD3 - вход прерывания от LIS
 PB1 - выход чип селект. PB2 - не подключен
 PC1 - выход на реле, рядом параллельно выведен и PC0. Можно и его туда вывести
 PC2 - потенциометр.
 PC7 - Термодатчик
 PC3 - база npn для зуммера
 Светодиоды
 pd2 -верхний LED
 pd4 - нижний LED. Они уходят к земле
 Линия к 595
 PB6 DS
 PB7 EN
 PD5	Latch
 PD6	Schift
 Кнопки, на землю, уже с конденсаторами
 PD7- верхняя
 PB0 - нижняя
 */
  #define uart_new_line_Macro {uart_putc(0x0A); uart_putc(0x0D);}

 #define Led_Yellow_ON PORTD|=(1<<PD2);
 #define Led_Yellow_OFF PORTD&=~(1<<PD2);
 #define Led_RED_ON PORTD|=(1<<PD4);
 #define Led_RED_OFF PORTD&=~(1<<PD4);
 #define Buzzer_ON PORTC|=(1<<PC3);
 #define Buzzer_OFF PORTC&=~(1<<PC3);
 #define RelayCut_IN PORTC|=(1<<PC1);
 #define RelayCut_OUT PORTC &=~(1<<PC1);
 
 

void Hardware_init (void)
{
	// Для SPI
	DDRB |= (1<<1);								 // чип-селект для lis
	PORTB|= (1<<1);								 // Неактивный - HIGH
	DDRD &= ~(1<<3);							 // Вход прерываний от ЛИС. Возможно использовать INT1.
	PORTD|=(1<<3);	//нужно поствить подтяг здесь?
	
	// Подтяг для TWI
	PORTC |= (1<<4)|(1<<5);						 // Включим подтяжку на ноги TWI
	DDRC &=~(1<<4|1<<5);
	
	//АЦП
	//DDRC &=~((1<<2)|(1<<7));					 // Ножка АЦП   //Термодатчик
	PORTC &=~((1<<2)|(1<<7));					 
	DDRC |= (1<<7);
	
	// Кнопки
	DDRB &=~(1<<0);								//Кнопка1
	DDRD &=~(1<<7);								//Кнопка2
	PORTB |= (1<<0);
	PORTD |= (1<<7);
	
	DDRC |= (1<<3);								 // Зуммер

	DDRD|= (1<<2)|(1<<4);						 // Светодиоды
	
	DDRC |= (1<<3);								 // Зуммер
	
	DDRC |= (1<<1);								 //База npn транзистора в управлении РЕЛЕ
	PORTC &=~(1<<1);
}
#endif /* HW_DEFINITIONS_H_ */
