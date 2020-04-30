/*
 * Button_input.c
 *
 * Created: 05.06.2016 10:09:07
 *  Author: isairton
 Далее эффективный, эффектный и надёжный код :)
 */ 
#include "Button_input.h"
#include <avr/io.h>

// Это вставить в обработчик прерывания таймера
void Within_ISR_button_service (void)
{
	
	//Пока здесь, но на будущее как то нужно будет может быть отвязаться от лед. Допустим, завязать на другом таймере.
	Button_Timer_Counter++;
	// Если переполнился и БЫЛ установлен хотя бы один из таймеров
	if((Button_Timer_Counter>ButtonTimerOverFlow)&&(Button_Timer_Flag&ButtonTimerSet_0))    Button_Timer_Flag|=ButtonLongReady_0;
	if((Button_Timer_Counter>ButtonTimerOverFlow)&&(Button_Timer_Flag&ButtonTimerSet_1))    Button_Timer_Flag|=ButtonLongReady_1;
	
}

//******************************************Интегратор***********************************************************

unsigned int integrator[NumberOfButtons];
unsigned char output[NumberOfButtons] = {1,1}; // Не знаю как по другому(Не теряя два слова при определении). Иначе при запуске регистрируется нажатие. 

void SetButtonTimer(uint8_t T)
{
	Button_Timer_Counter = 0;				// Начинаем с нуля
	switch(T)
	{
		case 0:
		Button_Timer_Flag |=ButtonTimerSet_0;		// Ставим нужный флаг
		break;
		case 1:
		Button_Timer_Flag |=ButtonTimerSet_1;
		break;
		case 11:
		Button_Timer_Flag |=ButtonTimerSet_0|ButtonTimerSet_1; //Оба
		break;
		
	}
	
}



char f_integrator (unsigned char input, unsigned char button_num)
{
	

	if (input == 0)
	{
		if (integrator[button_num] > 0)integrator[button_num]--;
	}
	else if (integrator[button_num] < MAXIMUM) integrator[button_num]++;
	
	
	if (integrator[button_num] == 0) output[button_num] = 0;
	
	else if (integrator[button_num] >= MAXIMUM)
	{
		output[button_num] = 1;
		integrator[button_num] = MAXIMUM;  // defensive code if integrator got corrupted
	}
	return output[button_num];
}




// Возвращает коды кнопок
//Самый младший- кнопка1. Далее - кнопка2. Длинная кнопка1. Длинная кнопка2
uint8_t ButtonCheck (void)
{
	uint8_t var0,var1,Button_output;
	Button_output = 0;			// Каждый раз с нулём
	//Антидребезг
	
	var0 = f_integrator(PINB &(1<<0),0);			// Чистые состояния без bounce
	var1 = f_integrator(PIND &(1<<7),1);
	
	
	// Если кнопка нажата
	 if(!var0)
	{
		// Предохранитель на повторное действие
		if(!(Button_Timer_Flag&ButtonRepeatPrevention_0))
		{
			//Первичная установка таймера и флага нажатия.
			if (!(Button_Timer_Flag&ButtonTimerSet_0))
			{
				//Устанавливаем таймер,Как и маску о его установке
				SetButtonTimer(0); 
				Button_state |= ButtonPressed_0_MASK; // Заявка на короткое нажатие
			}
			//Вышел ли таймер Длинного нажатия
			if(Button_Timer_Flag&ButtonLongReady_0)
			{
				
				// А нету ли у нас запрета на это действие, потому что сделано другое?
				if (!(Button_state&ButtonPressed_SHORT_Double_MASK))
				{
					//ДЕЙСТВИЕ ПО ДЛИТЕЛЬНОМУ НАЖАТИЮ
					Button_output|=ButtonPressed_0_LONG_MASK;
				}
				//вышел, ставим предохранитель
				Button_Timer_Flag |= ButtonRepeatPrevention_0;
				// Отмена заявки на короткое нажатие
				Button_state &= ~ButtonPressed_0_MASK;
			}
			
		}
	}

	// Если кнопка отпущена
	if(var0)
	{
		//Проверяем состояние, было ли короткое нажатие
		if (Button_state&ButtonPressed_0_MASK)
		{
			
			//Может быть в это время нажата и не прошла LONG вторая кнопка, а так же пока нет регистрации SHORT_Double
			 if ((Button_state&ButtonPressed_1_MASK)&&(!(Button_state&ButtonPressed_SHORT_Double_MASK)))
			{
				Button_state|= ButtonPressed_SHORT_Double_MASK;		//ставим флаг двойного нажатия (для второй кнопки и уходим
				// БЫЛО КОРОТКОЕ ДВОЙНОЕ НАЖАТИЕ
				Button_output|=ButtonPressed_SHORT_Double_MASK;
				// Вторая кнопка должна по выходу убрать этот флаг.
				// Отсечь возможное длинное второй кнопки
			
			}
			//Нет не нажата, и у нас нет флага игнорирования 
			 
			else if(!(Button_state&ButtonPressed_SHORT_Double_MASK))
			{
				//Было, КОРОТКОЕ НАЖАТИЕ ДЕЙСТВИЕ, 
				Button_output|=ButtonPressed_0_MASK;
				
			}
			// видимо всё таки есть
			else 
			{
				Button_state=0;// Убирает это событие для исключения повтора, уходим
			}
			// Убирает это событие для исключения повтора так или иначе
				Button_state&= ~ButtonPressed_0_MASK;
		
		
		}
		// Гарантированно очищаем таймерные флаги этой кнопки
		Button_Timer_Flag &= ~ (ButtonLongReady_0 | ButtonTimerSet_0 | ButtonRepeatPrevention_0); // Их нужно всегда очищать! Да, потеря времени на каждой итерации. Но если каждый раз проверять перед очисткой флаг, едва ли будет меньше кода
		
	}


	// Если кнопка нажата
	if(!var1)
	{
		// Предохранитель на повторное действие
		if(!(Button_Timer_Flag&ButtonRepeatPrevention_1))
		{
			//Вышел ли таймер Длинного нажатия
			if(Button_Timer_Flag&ButtonLongReady_1)
			{
				//вышел, ставим предохранитель
				Button_Timer_Flag |= ButtonRepeatPrevention_1;
				// Отмена заявки на короткое нажатие
				Button_state &= ~ButtonPressed_1_MASK;
				// А нету ли у нас запрета на это действие, потому что сделано другое?
				if (!(Button_state&ButtonPressed_SHORT_Double_MASK))
				{
					//ДЕЙСТВИЕ ПО ДЛИТЕЛЬНОМУ НАЖАТИЮ
				Button_output|=ButtonPressed_1_LONG_MASK;
				}
				
			}
			// Может быть он даже не установлен?
			if (!(Button_Timer_Flag&ButtonTimerSet_1))
			{
				//Устанавливаем таймер
				SetButtonTimer(1);
				Button_state |= ButtonPressed_1_MASK; // Заявка на короткое нажатие
			}
		}
	}

	// Если кнопка отпущена
	if(var1)
	{
		//Проверяем состояние, было ли короткое нажатие
		if (Button_state&ButtonPressed_1_MASK)
		{
		
			//Может быть в это время нажата и не прошла LONG вторая кнопка, а так же пока нет регистрации SHORT_Double
			if ((Button_state&ButtonPressed_0_MASK)&&(!(Button_state&ButtonPressed_SHORT_Double_MASK)))
			{
				Button_state|= ButtonPressed_SHORT_Double_MASK;		//ставим флаг двойного нажатия и уходим
				// БЫЛО КОРОТКОЕ ДВОЙНОЕ НАЖАТИЕ
				Button_output|=ButtonPressed_SHORT_Double_MASK;
				// Вторая кнопка должна по выходу убрать этот флаг.
				// Отсечь возможное длинное второй кнопки
				
			}
			//может быть у нас флаг игнорирования от второй кнопки?
			else if (!(Button_state&ButtonPressed_SHORT_Double_MASK))
			{
				//Было, КОРОТКОЕ НАЖАТИЕ ДЕЙСТВИЕ,
				Button_output|=ButtonPressed_1_MASK;
				
			}
			// Тогда игнорируем
			else
			{
				Button_state=0;// Убираем все события. Уходим
			}
			// Убирает это событие для исключения повтора
				Button_state&= ~ButtonPressed_1_MASK;	
		}
		// Гарантированно очищаем таймерные флаги этой кнопки (Потому что нажатие могло быть и длинным. Придётся на каждой итерации
		Button_Timer_Flag &= ~  (ButtonLongReady_1 | ButtonTimerSet_1 | ButtonRepeatPrevention_1);
	}

	return Button_output;
}
