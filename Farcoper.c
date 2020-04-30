/*
 * Farcoper.c
 *
 * Created: 02.06.2016 21:59:26
 *  Author: isairon
 
 */ 
#define F_CPU 8000000UL  // 8 MHz, внутренняя RC-цепочка
#include <avr/io.h>


#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "spi.h"
#include "Motion_Sensor.h"
 
#include <stddef.h>
#include <stdint.h>

#include "ds1307.h"
#include "LED_shift.h"
#include "HW_definitions.h"
#include "Button_input.h"
#include "ADC.h"
#include "One_wire.h"
#include "avr/wdt.h"

//------------- Умолчания----------------//

#define ModeOFF	    0		//	Спим - читаем часы иногда.
#define ModeON			1		//	Работаем
#define ModeThresholdSET 2
#define ModeThresholdSET_on_going 7
#define ModeTimeSuspend 3 	
#define ModeTimeEdit 4
#define ModeMorningTimeEdit 5
#define ModeHWerror 9

#define BAUD 9600

#define accelerometr_resolution_FS_1  72
#define accelerometr_resolution_FS_0  18

#define X_axis 0x29
#define Y_axis 0x2B
#define Z_axis 0x2D

#define Chosen_axis 0x2D		// Мы работаем с этой осью в этой программе

#define ChildTime 23
//#define  MorningTime 7
uint8_t MorningTime; 
#define Allowed_number_of_attempts 2
#define RinseCycle_NoActivity_sec 300
#define Min_difference_by_washing 20		// разница в пять градаций. Может быть слишком много. Надо проверить.

//---------------Переменные глобальные-------------------//

unsigned char SPI_IN_buf[4];
uint8_t current_Max_acceleration=4;  // кратно 4

volatile uint8_t Minimum_acceleration;	//Для логики бездействия (если разницы нет- машина простаивает).
volatile uint8_t Maximum_acceleration;

uint8_t G_Flag_acceleration;
unsigned char RAM_AlarmThreshold;
volatile uint16_t G_counter =0;
volatile uint8_t G_counter_char =0;
unsigned char EE_AlarmThreshold  EEMEM = 0x3E;
unsigned char EE_Mode_of_operation  EEMEM = ModeOFF;
unsigned char EE_MorningTime  EEMEM = 7;
volatile unsigned char Mode_of_operation;

unsigned char FlagMode = ModeON;

volatile unsigned char Digit = 0;
volatile unsigned char LED_string[4];

volatile unsigned int BeepDuration;
unsigned char const BeepPattern[] PROGMEM = {70,0,0};
//unsigned char const BeepPattern_LONG[] PROGMEM = {250,1,250,1,0,0};
unsigned char const BeepPattern_DoubleShort[] PROGMEM ={50,20,50,20,50,20,50,20,50,20,50,20,50,20,50,20,0,0};
unsigned char const BeepPattern_DoubleShort_Wrong[] PROGMEM = {100,30,100,30,100,30,100,30,0,0};
unsigned char const BeepPattern_Meloidic[] PROGMEM = {200,30,200,254,100,20,100,1,0,0};	
unsigned char const BeepPattern_SwitchOn[] PROGMEM = {80,250,40,250,40,200,40,150,40,150,40,100,40,100,40,40,40,40,20,20,15,20,10,0};
unsigned char const BeepPattern_SwitchOFF[] PROGMEM = {10,10,15,20,20,10,40,40,40,40,40,100,40,100,40,150,40,150,40,200,40,250,40,250,80, 0};		

unsigned char NumberOfAttempts;

volatile unsigned char Nested_counter =0;
volatile unsigned char Last_GlobalVar = 0;

volatile unsigned int G_time1 = 0;
volatile unsigned int WashTimeWindow = 0;
unsigned char BeeperFlag;
unsigned char BeeperPatternCounter;
unsigned char * BeeperPattern_pointer; // Адресс массива

volatile unsigned char Overacceleration_Occured = 0;

uint8_t TimeSuspendSkipEvening = 0x00;		// Без eeprom - не страшно если машина включится вечером при перевлючении света.

void (*f)(void) ;   // f - указатель

//---------------Прототипы-------------------//
void Acceleration_to_LED (signed char acceleration_output);
void ReadAxis(void); // Прототип
void Get_time (void);
void ONE_WIRE_DO_IT_HERE(void);
void Threshold_reducer (void);
void Change_Mode_of_Operation(uint8_t NewMode);
void Show_Morning_time(void);
//--------------- Процедуры и функции-----------------------//


void Beeper_Activator(const uint8_t *pattern)
{
	BeeperPattern_pointer=(uint8_t *)pattern;
	BeeperFlag|= 0x80;
	BeeperPatternCounter = 0;
}
void Deal_with_Beeper(void)
{
	uint8_t var;
	if ( BeeperFlag & 0x80)
	{
		var = pgm_read_byte(BeeperPattern_pointer+BeeperPatternCounter);
		if (var == 0)
		{BeeperFlag = 0;
		BeeperPattern_pointer = 0;
		BeeperPatternCounter=0;
		Buzzer_OFF;
		return;
		}
		else if(BeeperFlag&0x01)
		{
			Buzzer_OFF;
			BeeperFlag&=0xFE;
		}
		else
		{
			Buzzer_ON;
			BeeperFlag|=0x01;
		}
		BeepDuration = var;
		BeeperPatternCounter++;
	}
	
}



ISR(TIMER0_OVF_vect)		// 2ms  // Обслуживаем индикацию
{
	G_counter_char++;
	
	Within_ISR_button_service();
	
	 Digit++;												//изменение разряда для мультиплексированного вывода.
	 if(Digit==4) Digit=0;
	 
	 if(((Mode_of_operation == ModeThresholdSET)||(Mode_of_operation == ModeTimeEdit)||(Mode_of_operation==ModeMorningTimeEdit))&&(G_counter_char>190)) PutOneDigit(10,Digit,0);				// 190 ms чёрный экран - моргание.
	 else
	 {
															 //Показываем время (точка посередине) или тем-ру
		 if ((Mode_of_operation == ModeOFF)||(Mode_of_operation == ModeTimeSuspend)||(Mode_of_operation==ModeTimeEdit)||(Mode_of_operation==ModeMorningTimeEdit))
		  {
		 
					if ((Last_GlobalVar)&&(Digit==2)&&(G_counter_char>127)) PutOneDigit(LED_string[Digit],Digit,1);
					else  if ((!(Last_GlobalVar))&&(Digit==1))PutOneDigit(LED_string[Digit],Digit,1);
					else PutOneDigit(LED_string[Digit],Digit,0);	  		 
		  }
		 
		 else												//ускорение, точка после первого знака, и моргаем, если настрока.
		  { 
				  if (Digit==3) PutOneDigit(LED_string[Digit],Digit,1);
				  else PutOneDigit(LED_string[Digit],Digit,0);  	  
		  }
		}
 
	  if(Mode_of_operation == ModeTimeSuspend)				//Попеременно светодиодами
	  {
		    if(G_counter_char==0)
		    {Led_Yellow_OFF;Led_RED_ON;}
			if(G_counter_char==127)
			{Led_RED_OFF;Led_Yellow_ON;}
	  }

	    if(Mode_of_operation == ModeHWerror)				// Моргаем КРАСНЫМ - ошибка
	    {
		    if(G_counter_char==0)Led_RED_ON;
		    if(G_counter_char==127)Led_RED_OFF;
	    }

}

ISR(TIMER2_OVF_vect)					//1ms // Логика датчика и задержек.
{
	
	
	G_time1++;
	if(G_time1>1000)					// Секунда
	{	
		WashTimeWindow++;																		// время без сработок.Если давно их не было, даёт больше попыток - позволяет делать больше сработок.Чтобы не подряд.
		G_time1 = 0;
		//Логика восстановления питания после сработки и разрыва.
		if (Overacceleration_Occured)
		{
			Overacceleration_Occured--;														   // будильник паузы (загружается при сработке)
			if ((!(Overacceleration_Occured))&&(Mode_of_operation==ModeON))
			{RelayCut_IN;  // Если уже нуль И	машину можно включать.
			 Beeper_Activator(BeepPattern_SwitchOn);
			}
		}
		
	}
	if ((WashTimeWindow>RinseCycle_NoActivity_sec )&&(Mode_of_operation == ModeON))				// только для режими "вкл"
	{
		
		WashTimeWindow = 0;
		if(NumberOfAttempts)NumberOfAttempts --; // Даём по ещё одной попытке на каждые RinseCycle_NoActivity_sec (5 минут или около).
		
		// Заодно затулим сюда автовыключение при бездействии!!!!
		//Логика для автоотключения
		if ((Maximum_acceleration-Minimum_acceleration)<Min_difference_by_washing)Change_Mode_of_Operation(ModeOFF); //вырубаемся!
		Maximum_acceleration=0;		//Каждые Rinse_cycle_sec обнуляем переменные и начинаем прослушивание бездействия снова.
		Minimum_acceleration=50;	// 0.9G. И в программе может уменьшаться.
	}
	
	
	
	
	
	
	
// Таймер для зуммера	
if(BeepDuration>0)BeepDuration--;
if(BeepDuration ==0)Deal_with_Beeper();

G_counter++;


	if (G_counter > 200) // каждые 200ms
	{
		switch (Mode_of_operation)     //выбор, чё выводим на LED дисп, по режимам работы.
			{
				case ModeTimeEdit:				//on display:Только время
				
				Nested_counter++;
				if (Nested_counter >3)
				{
					f = Get_time;
					Nested_counter =0;
					Last_GlobalVar = 1;	
				}			
				break;	
				case ModeMorningTimeEdit:			
					f = Show_Morning_time;
				break;			
				
				case ModeOFF:					//on display: Время и тем-ра
				case ModeTimeSuspend:				
	
				Nested_counter++;
				if (Nested_counter >40)
				{
					Nested_counter =0;
					f = ONE_WIRE_DO_IT_HERE;			// начало таймслота для 1-wire
				}
				else if (Nested_counter >13)
				{
					f = Get_time;
					Last_GlobalVar = 1;
				}
				else if (Nested_counter >3)
				{
					Last_GlobalVar = 0; // Показываем тем-ру
				}
				break;
				
				case ModeON:
				case ModeThresholdSET:														//on display: Ускорение 
			
					if (G_Flag_acceleration>0x10)									//  секунда без прерывания по FF_WU_2
						{
							G_Flag_acceleration=0xFF;								 //  будем снижать, пока прерывание не сработает снова.
							f = Threshold_reducer;
						}
							G_Flag_acceleration<<=1;									 // Сдвигаем влево в конце блока для следующей итерации


						if (Mode_of_operation==ModeThresholdSET)Acceleration_to_LED(RAM_AlarmThreshold);
						else Acceleration_to_LED(current_Max_acceleration);
						
				
				break;
			
				case ModeHWerror:
				RelayCut_OUT;
				LED_string[0]=LED_string[1]=10;											// Пустышки
				LED_string[2]=11;														// E (error)
				LED_string[3]=14;														// H (hardware)
				break;			
			}
		G_counter =0;
	}	
}


inline void setupTimer_0 (void)
{			//переполнение каждые 2,048мс
	TCCR0 = 0;
	TCCR0 |= (1<<CS01)|(1<<CS00);	//64, 32 у этого таймера нет.
	TIMSK|=(1<<TOIE0);            // Enable Counter Overflow Interrupt
}

inline void SetupTimer_2 (void)
{			//переполнение каждые 1,024мс
	TCCR2 = 0;
	TCCR2 |= (1<<CS21)|(1<<CS20);	//32
	TIMSK|=(1<<TOIE2);            // Enable Counter Overflow Interrupt
}



void Acceleration_to_LED (signed char acceleration_output)
{
	//if (Current_resolution == accelerometr_resolution_FS_0)Led_Yellow_OFF;
	//else Led_Yellow_ON; // Full scale
Led_RED_OFF; // Первоначально - положительное
	if (acceleration_output<0)	 //Сразу преобразуем, если отрицательное.
	{
		 // индикация отрицательного
		// positiv=0;
		acceleration_output^= 0xFF; // инвертируем
		acceleration_output+=0x01; // прибавляем единицу
	}
	signed int thrue_G_output = acceleration_output;
	thrue_G_output*=accelerometr_resolution_FS_0;
	
	
	
	// Код для вывода при настройке AlarmThreshold
	if (Mode_of_operation==ModeThresholdSET)thrue_G_output=RAM_AlarmThreshold*accelerometr_resolution_FS_0;
	
	uint8_t i;// локальная переменаная											
	for(i=0; i<4; i++)									// цикл по длинне строки
	{
		LED_string[i] = (char) ((thrue_G_output % 10UL));		//Остаток от деления на десять ,
		//приводим к char и грузим начиная с конца строки
		thrue_G_output/=10;
	}
}


// 
// void ReadAxis(void)
// {
// 	signed char acceleration_output;
// 	SPI_accelerometr_Read_char(Chosen_axis,&acceleration_output);
// 	Acceleration_to_LED(acceleration_output);
// }

uint8_t LIS331_SetUP (void)
{
	
	// Врубаем акселерометр....................................
	SPI_accelerometr_Write(0x20, 0xC7); // включить генерацию всех трёх измерений и отключить POWER DOWN. Шкала до 2.3 G
	
	SPI_IN_buf[0]=0x8F;
	LIS_read__who_i_am_3(SPI_IN_buf);   // Работает только двоекратно. Найти почему не хватает ума!!!
	SPI_IN_buf[0]=0x8F;
	LIS_read__who_i_am_3(SPI_IN_buf);
	uint8_t var_lis;
	var_lis=SPI_IN_buf[1];
	if (var_lis==0x3B)return 1;
	else return 0;
	
	/*
	uint8_t var_lis;
	var_lis=SPI_IN_buf[1];
	if (var_lis==0x3B)uart_puts_P("SPI accelerometer LIS on the bus");
	else{uart_puts_P("No answer from accelerometer :-(   Wrong byte is - ");
		uart_puthex_byte(var_lis);
	}
	uart_putc(0x0A);								//line feed
	uart_putc(0x0D);
	*/
}

void LIS331_int2_SET (void)  // Изменил настроку. Теперь на меге подтяг. А акселерометр - Open Drain. Надо проверить.
{
	SPI_accelerometr_Write(0x22, 0xD0); // прерывание -активный LOW. Open DRAIN Для INT2 выбор активности -FF_WU_2
	SPI_accelerometr_Write(0x34, 0x20); // FF_WU_2 : прерывание только по Z по верхнему уровню
	SPI_accelerometr_Write(0x36, current_Max_acceleration); // FF_WU_2: FF_WU_THS_2.
	SPI_accelerometr_Write(0x37, 0); // duration. 0 ms при 400 Hz ODR
}



void Threshold_incrementer (void)
{
		if (current_Max_acceleration<124) // Проверить на выход из диапазона (127+перестраховка от дефекта датчика)
		{
			current_Max_acceleration+=4; // получается 28 шагов.
			SPI_accelerometr_Write(0x36, current_Max_acceleration);			// Увеличиваем порог прерывания FF_WU_2
		}
		else		//Диапазон выбран, косяк с настройкой,
					// Либо линия упала на массу. Так или иначе - 
					//  спасаемся.
		{
			
			Change_Mode_of_Operation(ModeOFF); // вырубаемся с индикацией ошибки
			Led_RED_ON; // индикация
			RelayCut_OUT;		// Дополнительно, мало ли.
		}
		
	//Логика для автоотключения
	if (current_Max_acceleration>Maximum_acceleration)Maximum_acceleration=current_Max_acceleration; //Обновляем максимальное значение
	
}

void Threshold_reducer (void)
{
		if (current_Max_acceleration<10)return;	// опасаемся прехода через нуль.
		current_Max_acceleration-=4; // получается 28 шагов тоже
		SPI_accelerometr_Write(0x36, current_Max_acceleration);			//  порог FF_WU_2
		
		//Логика для автоотключения
		if (current_Max_acceleration<Minimum_acceleration)Minimum_acceleration=current_Max_acceleration; //Обновляем минимальное значение
	
}


void Time_set_enabler (void)
{
 if (Mode_of_operation==ModeOFF)
 {
	 Mode_of_operation = ModeTimeEdit;
	 Beeper_Activator(BeepPattern_DoubleShort);
 }
 else if (Mode_of_operation==ModeTimeEdit) Change_Mode_of_Operation(ModeOFF);
  
 else Beeper_Activator(BeepPattern_DoubleShort_Wrong);
 
}


void MorningTimeSet_enabler (void)
{
	if (Mode_of_operation==ModeOFF)
	{
		Mode_of_operation = ModeMorningTimeEdit;
		Beeper_Activator(BeepPattern_DoubleShort_Wrong);
	}
	else if (Mode_of_operation==ModeMorningTimeEdit)
	{ 
		TimeSuspendSkipEvening|=0x01;	// Ставим метку-принуждение
		Change_Mode_of_Operation(ModeTimeSuspend);   // Переходим принудительно и раньше времени 23:00
	}
	
	else Beeper_Activator(BeepPattern_DoubleShort_Wrong);
	
}

void AlarmThreshold_changer (uint8_t side)
{
	
	
	if(Mode_of_operation == ModeThresholdSET) // Вызов при любом одиночном нажатии. Или так, или алгоритм менять.
	{

		if(side)
		{
			if(RAM_AlarmThreshold<123)RAM_AlarmThreshold+=4;		//получается 28 шагов // Увеличение порога	
		}
		else
		{
			if(RAM_AlarmThreshold>4) RAM_AlarmThreshold-=4;			//получается 28 шагов // Уменьшение	
		}
	eeprom_update_byte(&EE_AlarmThreshold,RAM_AlarmThreshold);  // Обновляем
	}
	if(Mode_of_operation == ModeTimeEdit) // Вызов при любом одиночном нажатии. Или так, или алгоритм менять.
	{
	
	uint8_t var,hour,minute;

	ds1307_getdate(&var, &var, &var, &hour, &minute, &var);

		if(side) //на этой кнопке часы
		{
			hour++;
			if(hour>23)hour =0;
		}
		else // На этой минуты
		{
			minute++;
			if(minute>59) minute=0;			//получается 28 шагов // Уменьшение
		}
		//ds1307_setdate(var, var, var, hour, minute, 0);  // Обновляем

		 ds1307_SPECIAL_setdate(hour,minute);

	}


	if(Mode_of_operation == ModeMorningTimeEdit) 
	{
	
		if(side) //на этой кнопке часы
		{
			MorningTime++;
			if(MorningTime>11)MorningTime=11;
		}
		else 
		{
			MorningTime--;
			if(MorningTime<7)MorningTime=7;
		}
		eeprom_update_byte(&EE_MorningTime,MorningTime);

	}
	
}


void Get_time (void)
{	
		uint8_t var,hour,minute,second;

		ds1307_getdate(&var, &var, &var, &hour, &minute, &second);
		
		LED_string[3]=(hour % 100 / 10);
		LED_string[2]=(hour % 10);
		LED_string[1]=(minute % 100 / 10);
		LED_string[0]=(minute % 10);	
}

void Show_Morning_time (void)
{

	
	LED_string[3]=(MorningTime % 100 / 10);
	LED_string[2]=(MorningTime % 10);
	LED_string[1]=0;
	LED_string[0]=0;
}


void SwitchOnRoutine(void)
{
	Mode_of_operation = ModeON;
 				Beeper_Activator(BeepPattern_SwitchOn);
 				RelayCut_IN;
				Led_RED_OFF;Led_Yellow_ON;		//во время работы горит жёлтый.
				WashTimeWindow = 0;				// Начинаем новый квант стирки (иначе будет непостоянный баг).
				NumberOfAttempts = 0;				// Обнуляем число попыток, если работаем без резета, а мы к этом и стремимся
				// Врубаем датчик!
				LIS331_int2_SET();
				LIS331_SetUP();
}

void SwitchOFFRoutine(void)
{
	Mode_of_operation = ModeOFF;
			Beeper_Activator(BeepPattern_SwitchOFF);
			RelayCut_OUT;
			SPI_accelerometr_Write(0x20, 0x07); // включить POWER DOWN. Должно LIS вырубить	
}

void finite_state_machine(void)
{
	// Сбросим здесь светодиоды, чтобы не вводили в заблуждение
	Led_Yellow_OFF;
	Led_RED_OFF;
	if(Mode_of_operation == ModeOFF)SwitchOnRoutine();			// Переключаемся на "Включено"
	else SwitchOFFRoutine();														//Вырубаемся
	eeprom_update_byte(&EE_Mode_of_operation,Mode_of_operation);
}



void Change_Mode_of_Operation(uint8_t NewMode)
{
	// Сбросим здесь светодиоды, чтобы не вводили в заблуждение
	Led_Yellow_OFF;
	Led_RED_OFF;
	if (NewMode == ModeOFF)SwitchOFFRoutine();
	else if (NewMode == ModeON) SwitchOnRoutine();			// При загрузке при чтении ЕЕПРОМ - восстановление после выключения света.
	
	//Если выход из режима настройки (ветвление для настройки для ходу)
	else if (Mode_of_operation==ModeThresholdSET)
	{
		if (PORTC&(1<<1))									//Если выход на реле активен 
		{Mode_of_operation=ModeON;
		Led_RED_OFF;Led_Yellow_ON		//во время работы горит жёлтый.  
		// Не трогая реле переходим в режим. Подразумевается, что настройкой пользуемся обычно на ходу прибора. Иначе не забываем выключить.
		}
		else Change_Mode_of_Operation(ModeOFF);          // МОЖНО ВЫЗВАТЬ ФУНКЦИЮ ИЗ ФУНКЦИИ? РЕКУРСИЯ РАЗРЕШЕНА В СТУДИИ???? ВПЕЧАТЛЁН.
		
		
	}
	// Ну тогда просто меняем режим работы и всё. Без доп действий. 
	
	else Mode_of_operation = NewMode;
	
//Сохраняем в энергонезависимой памяти.		 
eeprom_update_byte(&EE_Mode_of_operation,Mode_of_operation);		 
}

void ShowAlarm(void)
{
if (Mode_of_operation==ModeThresholdSET)					// Либо сам наклацал либо кнопка западает при работе. Не должно такого быть - сработка в режиме настройки.
	{
	 RelayCut_OUT;
	 Led_Yellow_ON;
	 Mode_of_operation=ModeHWerror;					
	}
	// Временно вырубаем машину без изменения режима работы.
	RelayCut_OUT // Выключить реле
	Beeper_Activator(BeepPattern_DoubleShort);
	if(!Overacceleration_Occured)NumberOfAttempts ++;				// Боремся с возможным ложным повторением.

Overacceleration_Occured = 5; // 

	
	WashTimeWindow = 0; // Не позволяем тут же сделать декремент, если таймер вдруг на этом месте.
	if (NumberOfAttempts>Allowed_number_of_attempts)			// Приплыли, выключаемся и показываем почему.
	{
		Change_Mode_of_Operation(ModeOFF);// вырубаемся
		Led_Yellow_ON;		//Вдруг юзер спит, чтобы когда проснулся, знал, что если два горят - порог превышения был и выключение.
		Led_RED_ON;
		return;
	}
	
}

void Check_Children_Time(void)
{
	uint8_t var,hour;
	ds1307_getdate(&var, &var, &var, &hour, &var, &var);
	if(((hour>= ChildTime)||(hour< MorningTime))&&(Mode_of_operation == ModeON))
	{
		RelayCut_OUT;
		Change_Mode_of_Operation(ModeTimeSuspend);
		Beeper_Activator(BeepPattern_SwitchOFF);
	}

	 if(hour>= ChildTime)TimeSuspendSkipEvening&=~0x01;	// Сбрасываем флаг пропуска вечера.
	 if((hour>= MorningTime)&&(hour< ChildTime)&&(Mode_of_operation == ModeTimeSuspend))
	 {
			if (TimeSuspendSkipEvening&0x01)return;			//У нас вечер! 
	 		Change_Mode_of_Operation(ModeON);
	 		RelayCut_IN;
			Beeper_Activator(BeepPattern_SwitchOn);
	 }
}



uint16_t DIG_digit(uint16_t dig, uint16_t sub,uint8_t DIGIT) {
	char c = 0;
	while (dig >= sub) {
		dig -= sub;
		c++;
	}
	LED_string[DIGIT]=c;				// Вожделенная строчка!
	return dig;
}



void DIG_num(int16_t num) {
	uint16_t unum; // число без знака

	unum = num;
	uint16_t snum =  unum >> 4; // отбрасывает дробную часть
	if (snum >= 10) {
		LED_string[3]=10;						 // Нафиг не нужен этот разряд в комнате.
		snum = DIG_digit(snum, 10,2); // 2й разряд
	}
	DIG_digit(snum, 1,1); // 1й разряд
	DIG_digit((((uint8_t)(unum & 0x0F)) * 10) >> 4, 1,0); // дробная часть
}


void ONE_WIRE_DO_IT_HERE(void)

{

//	one_w_i = 0; // инициализируем индекс
cli(); /// запретим прерывания, чтобы одновайр работал нормально
	if (onewire_skip()) { // Если у нас на шине кто-то присутствует,...
		onewire_send(0x44); // ...запускается замер температуры на всех термодатчиках
sei(); // Чтобы не висло
 uint8_t i;
 	for(i=0;i<3;i++)
 	{
 		_delay_ms(300); // Минимум на 750 мс.
 		wdt_reset();		// Внутри блокирующей функции обеспечиваем работу
 	}
	//	_delay_ms(900); // Минимум на 750 мс.

 /// запретим прерывания, чтобы одновайр работал нормально
	cli(); // снова блокируем	
		onewire_enum_init(); // Начало перечисления устройств
		for(;;) {
			uint8_t * p = onewire_enum_next(); // Очередной адрес
			if (!p)
			break;
			// Вывод шестнадцатиричной записи адреса в UART и рассчёт CRC
			uint8_t d = *(p++);
			uint8_t crc = 0;
			uint8_t family_code = d; // Сохранение первого байта (код семейства)
			for (uint8_t i = 0; i < 8; i++) {
				
				crc = onewire_crc_update(crc, d);
				d = *(p++);
			}
			if (crc) {
				// в итоге должен получиться ноль. Если не так, вывод сообщения об ошибке
						LED_string[3]=10;
						LED_string[2]=10;
						LED_string[1]=10;
						LED_string[0]=11;		// E
				} else {
				if ((family_code == 0x28) || (family_code == 0x22) || (family_code == 0x10)) {
					// Если код семейства соответствует одному из известных...
					// 0x10 - DS18S20, 0x28 - DS18B20, 0x22 - DS1822
					// проведём запрос scratchpad'а, считая по ходу crc
					onewire_send(0xBE);
					uint8_t scratchpad[8];
					crc = 0;
					for (uint8_t i = 0; i < 8; i++) {
						uint8_t b = onewire_read();
						scratchpad[i] = b;
						crc = onewire_crc_update(crc, b);
					}
					if (onewire_read() != crc) {
						// Если контрольная сумма скретчпада не совпала.
 						LED_string[3]=10;
 						LED_string[2]=12;		// С
						LED_string[1]=13;		// Г
						LED_string[0]=12;		// С
						} else {
						int16_t t = (scratchpad[1] << 8) | scratchpad[0];
						if (family_code == 0x10) { // 0x10 - DS18S20
							// у DS18S20 значение температуры хранит 1 разряд в дробной части.
							// повысить точность показаний можно считав байт 6 (COUNT_REMAIN) из scratchpad
							t <<= 3;
							if (scratchpad[7] == 0x10) { // проверка на всякий случай
								t &= 0xFFF0;
								t += 12 - scratchpad[6];
							}
						} // для DS18B20 DS1822 значение по умолчанию 4 бита в дробной части
						// Вывод температуры
						DIG_num(t);
					}
					} else {

					// Неизвестное устройство
 				LED_string[3]=10;
				LED_string[2]=10;
				LED_string[1]=10;
				LED_string[0]=12;		// E
				}
			}
			
		}
		//One_wire_buf[5]='.';
		
		} 
		else // На шине молчок
		{
		//  One_wire_buf[6]='-'; 		LED_string[3]=10;
		LED_string[2]=10;
		LED_string[1]=11;
 		LED_string[0]=10;		// E
		
		}

	sei(); /// разрешим прерывания, чтобы всё работало нормально.
	
}



 int main (void)
 
 {
	wdt_enable(WDTO_500MS);
	 
	 
	uint8_t var;
//Восстанавливаемся если питание пропадало
RAM_AlarmThreshold = eeprom_read_byte(&EE_AlarmThreshold);		// До включения логических ветвлений.
var=eeprom_read_byte(&EE_Mode_of_operation);	 
MorningTime = eeprom_read_byte(&EE_MorningTime);  

	 	 //////////////////////////   H A R D W A R E   I N I T   /////////////////////////////////
Hardware_set_for_shift();
Hardware_init();
Buzzer_ON;  
sei();		  
spi_extended_init(SPI_MODE3,SPI_CLOCK_DIV4);

//if (!(LIS331_SetUP())) Mode_of_operation = ModeHWerror;	

Change_Mode_of_Operation(var);// Только после настройки переферии. 

	
_delay_ms(3);
setupTimer_0(); // LED и опрос кнопок
SetupTimer_2(); // Работа с акселерометром
ds1307_init();

f=NULL;

Buzzer_OFF;// зуммер выключился, загрузка окончена
unsigned int Time_watcher = 0;
unsigned int ProgWatchDog = 0;


	while(1)
	{
		wdt_reset(); // сбрасываем собачий таймер

		if ((Mode_of_operation == ModeON)||(Mode_of_operation == ModeTimeSuspend))
		{
			Time_watcher++;
			if(Time_watcher == 0) Check_Children_Time();
		}
		
		if (PORTC&(1<<1))									//Если выход на реле активен (подразумеваю ModeOn но и гарантирую выключение при косяках в логике).
		{
			ProgWatchDog++;
			if(ProgWatchDog > 0xFF00)
			{		 
			RelayCut_OUT;
			Led_Yellow_ON;
			Mode_of_operation=ModeHWerror;					// Програмный вачдог на дефект датчика(нет прерываний)
			}
		}
		
	// Обрабатываем LIS-прерывание. В том числе и при настройке порога "на горячую". Из-за логики самодиагностики датчика.
		if ((!(PIND& (1<<3)))&&((Mode_of_operation==ModeON)||(Mode_of_operation==ModeThresholdSET))	)				
		{
			ProgWatchDog = 0;							// От дефекта датчика (отсутствие прерываний).

			Threshold_incrementer();
			SPI_accelerometr_Read(0x35, NULL);		// сбрасываем прерывание FF_WU_2, читая регистр его статуса 0x00 - адресс куда писать мусор. Может быть опасно
			G_Flag_acceleration=0x01;				// Сбрасываем битовый счётчик "время без прерываний"
			Led_RED_ON;								 //Индикация
			if(current_Max_acceleration > 65)Buzzer_ON; // Чтобы не крякало при кружении возле нулевой вибрации.
			_delay_ms(2);							// Иначе лис не успевает одуплиться
			Led_RED_OFF; //
			Buzzer_OFF;
													// Проверить на предельную вибрацию											
			if(current_Max_acceleration>RAM_AlarmThreshold)ShowAlarm();
			
			// Неправильная настрока, закоротка линии, выход датчика из строя на замыкание линии - 
			// Threshold_incrementer() переполнит переменную (которая уменьшается только таймером) 
			// мы получаем вечный ShowAlarm(), точнее выход по allowedNumberOfAttempts через пятнадцать секунд. Это не очень хорошо. 
			
			// Хотя предполагалось вырубание систему с красным светодиодом.
			
			// Отсутствие этого прерывания тоже указыает на отказ датчика и тоже приводит в отключению.
			// Гарантия этого прерывания (от зависания программы) - WATCH DOG.

		}


	 if(f)
		 {
			cli();
			f();	// вызов функции если там есть чё.
			f=NULL;   // Предотвращаем цикличность.
			sei();
		 }
		 

	
//Обработчик нажатия кнопок/////	
		  switch(ButtonCheck())
		 {
			 case ButtonPressed_0_MASK:
				 AlarmThreshold_changer(0);
				 Beeper_Activator(BeepPattern);
			 break;
			 case ButtonPressed_1_MASK:
				 AlarmThreshold_changer(1);
				 Beeper_Activator(BeepPattern);
			 break;
			 case ButtonPressed_0_LONG_MASK:
				Beeper_Activator(BeepPattern_Meloidic);
				Change_Mode_of_Operation(ModeThresholdSET);
			 break;
			 case ButtonPressed_1_LONG_MASK:
				finite_state_machine();
			 break;
			 case (ButtonPressed_0_LONG_MASK| ButtonPressed_1_LONG_MASK):
				 Time_set_enabler();
			 break;
			  case ButtonPressed_SHORT_Double_MASK:
			   
			  MorningTimeSet_enabler();
			  break;
				//Beeper_Activator(BeepPattern_DoubleShort);    /// Было вот так, эти три строки. Это была, наверное, просто заготовка
				// if (Mode_of_operation==ModeON)RelayCut_IN // Включить реле
				// else Beeper_Activator(BeepPattern_DoubleShort_Wrong);		
		 }
	}
 }
	 
