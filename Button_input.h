/*
 * Button_input.h
 *
 * Created: 05.06.2016 10:03:22
 *  Author: isairton
 */ 


#ifndef BUTTON_INPUT_H_
#define BUTTON_INPUT_H_


#define Button1_Pressed (!(PINB&(1<<0)))
#define Button1_Released (PINB&(1<<0))

#define Button2_Pressed (!(PIND&(1<<7)))
#define Button2_Released (PIND&(1<<7))


/*
Button_Timer_Flag
------------7------------6------------5------------4------------3------------2------------1------------0------------
	TimerSet_1		TimerSet_0	RepPrevDouble	RepPrev1		RepPrev0		LongReady_Double			LongReady_0
	
	
	
ButtonState
------------7------------6------------5------------4------------3------------2------------1------------0-----------	
											 SHORT_Double	            	   		  Pressed_1	    Pressed_0		
*/
//////////////////////////////////////________Прототипы
void Within_ISR_button_service (void);
unsigned char ButtonCheck (void);

//////////////////////////////////////_________Переменные
volatile unsigned char Button_state;
volatile unsigned char Button_Timer_Flag;
volatile unsigned int Button_Timer_Counter;



#define DEBOUNCE_TIME       0.3     
#define SAMPLE_FREQUENCY    3000
#define MAXIMUM         (DEBOUNCE_TIME * SAMPLE_FREQUENCY)
#define NumberOfButtons      2

char f_integrator (unsigned char input, unsigned char button_num);


 #define DDRButton DDRD
 #define PortButton PORTD
 #define PinButton PIND
 #define Button_1 2
 #define Button_2 3

// Маски ButtonState
#define ButtonPressed_0_MASK		  0b00000001
#define ButtonPressed_1_MASK	   	  0b00000010
#define ButtonPressed_0_LONG_MASK		  0b00000100
#define ButtonPressed_1_LONG_MASK	   	  0b00001000
#define ButtonPressed_SHORT_Double_MASK  0b00010000

// Маски Button_Timer_Flag
#define ButtonTimerSet_0	  0b01000000
#define ButtonTimerSet_1	  0b10000000


#define ButtonLongReady_0		 0b00000001
#define ButtonLongReady_1	   	 0b00000010
#define ButtonLongReady_Double   0b00000100

#define ButtonRepeatPrevention_0		 0b00001000
#define ButtonRepeatPrevention_1		 0b00010000
#define ButtonRepeatPrevention_Double    0b00100000

#define ButtonTimerOverFlow 400


#endif /* BUTTON_INPUT_H_ */
