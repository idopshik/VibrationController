/*
 * ADC.h
 *
 * Created: 01.06.2016 14:44:41
 *  Author: idopshik
 */ 

#ifndef ADC_H_
#define ADC_H_


// СНИЗИТЬ ЧАСТОТУ АЦП ЧЕРЕЗ ДРУГОЕ СОБЫТИЕ ЗАПУСКА

void adcInit(void)
{
	//*************Регистр ADCSRA************************//
	
			// ЭТО ТОЛЬКО ДЛЯ MEGA16 !

	// SC - start conversion
	// IE - interrupt enable
	// ADATE - автоход. Здесь он выключен.
	//           7    6     5     4     3     2     1     0
	//ADCSRA = ADEN ADSC  ADATE  ADIF  ADIE  ADPS2 ADPS1 ADPS0
	//************************************************************//
	
	//ADCSRA&= ~(1<<ADATE); //убираем автоход
ADCSRA|= (1<<ADEN)| (1<<ADSC)|(1<<ADIE)|(1<<ADPS0)|(1<<ADPS1)|(1<<ADPS2);
//ADMUX|= (1<<REFS0); // AVCC
//ADMUX |=(1<<MUX1); // Џеременник на PC2

ADMUX|= (1<<REFS0)|(1<<REFS1);
ADMUX |=(1<<MUX0)|(1<<MUX1)|(1<<MUX2); // Термодатчик
ADCSRA|=(1<ADSC);//ADSC  Ќачало преобразования
}

#endif /* ADC_H_ */
