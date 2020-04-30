/*

 *  Author: isairton
// Здесь не все на одном порту. Поэтому нюанс в сишнике. Смотри сишник.
 */ 

#ifndef LED_SHIFT_H_
#include <avr/io.h>
#define LED_SHIFT_H_

#define  ShiftDDR		 DDRD
#define  ShiftPort		 PORTD
#define  DS				 PB6        // Serial data in (about 595)
#define  ST_CP			 PD5		// Storage clock (latch)
#define  SH_CP			 PD6		// Shift clock (clock)

void Hardware_set_for_shift (void);
void shift( unsigned int data);
void Data_Out (unsigned char d);
void PutOneDigit(unsigned char Num,unsigned char Digit,unsigned char DOT);


#endif /* LED_SHIFT_H_ */
