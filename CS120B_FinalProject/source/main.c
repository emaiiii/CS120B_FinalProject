#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <string.h>
#include <stdio.h>
#include "io.h"
#include "timer.h"
#ifdef _SIMULATE_
#include "simAVRHeader.h"
#endif

#define button1 (~PINA & 0x01)
#define button2 (~PINA & 0x02)
#define button3 (~PINA & 0x04)
#define button4 (~PINA & 0x08)

///////////////////////////////////////////////////////////////////////
// special character arrays
unsigned char downward[] = {0x04,0x04,0x04,0x04,0x15,0x0e,0x04,0x00};
unsigned char square[] = {0x00,0x1f,0x11,0x11,0x11,0x1f,0x00,0x00};
unsigned char curvy[] = {0x01,0x02,0x04,0x08,0x10,0x11,0x1f,0x00};
unsigned char phone[] = {0x1f,0x1f,0x12,0x04,0x09,0x10,0x1f,0x00};
///////////////////////////////////////////////////////////////////////
// special character function
void BuildAllChar(){
	LCD_BuildChar(0, downward);
	LCD_BuildChar(1, square);
	LCD_BuildChar(2, curvy);
	LCD_BuildChar(3, phone);
}
//////////////////////////////////////////////////////////////////////

typedef struct task{
	int state;
	unsigned long period;
	unsigned long elapsedTime;
	int (*TickFct) (int);
} task;

char alpha[] = {0x00, 0x01, 0x02, 0x03};
unsigned char alphaIndex = 0;
unsigned char cursor = 1;
unsigned char string[4];
unsigned char tempString[4];

///////////////////////////////////////////////////////////////////////
//SM for game logic
enum MENU_States {MENU_WAIT,MENU_INC,MENU_DEC,MENU_WRITE,MENU_CLEAR,MENU_EEPROM_WRITE_PRESS,MENU_EEPROM_WRITE_RELEASE,MENU_EEPROM_READ,SMILEY};

int Tick_MENU(int state){
	switch(state){
		case MENU_WAIT:
		if(button1 && !button2 && !button3 && !button4) state = MENU_WRITE;
		else if(!button1 && button2 && !button3 && !button4) state = MENU_DEC;
		else if(!button1 && !button2 && button3 && !button4) state = MENU_INC;
		else if(!button1 && button2 && button3 && !button4) state = MENU_CLEAR;
		else if(button1 && button2 && !button3 && !button4){state = MENU_EEPROM_READ; strcpy(tempString,string);}
		else if(!button1 && !button2 && !button3 && button4) state = MENU_EEPROM_WRITE_PRESS;
		else if(button1 && !button2 && !button3 && button4){state = SMILEY; strcpy(tempString,string);}
		else state = MENU_WAIT;
		break;
		case MENU_INC:
		state = MENU_WAIT;
		break;
		case MENU_DEC:
		state = MENU_WAIT;
		break;
		case MENU_WRITE:
		state = MENU_WAIT;
		break;
		case MENU_CLEAR:
		state = MENU_WAIT;
		break;
		case MENU_EEPROM_WRITE_PRESS:
		state = (button4)?(MENU_EEPROM_WRITE_PRESS):(MENU_EEPROM_WRITE_RELEASE);
		break;
		case MENU_EEPROM_WRITE_RELEASE:
		state = MENU_WAIT;
		break;
		case MENU_EEPROM_READ:
		state = (button1 && button2 && !button3)?(MENU_EEPROM_READ):(MENU_WAIT);
		if(state == MENU_WAIT){memset(string,0,16); strcpy(string,tempString);}
		break;
		case SMILEY:
		state = (button1 && button4)?(SMILEY):(MENU_WAIT);
		if(state == MENU_WAIT){memset(string,0,16); strcpy(string,tempString);}
		break;
		default:
		state = MENU_WAIT;
		break;
	}

	switch(state){
		case MENU_WAIT:
		break;
		case MENU_INC:
		if(alphaIndex < 51) alphaIndex++;
		string[cursor-1] = alpha[alphaIndex];
		break;
		case MENU_DEC:
		if(alphaIndex > 0) alphaIndex--;
		string[cursor-1] = alpha[alphaIndex];
		break;
		case MENU_WRITE:
		string[cursor - 1] = alpha[alphaIndex];
		if(cursor < 16) cursor++;
		break;
		case MENU_CLEAR:
		memset(string,0,16);
		alphaIndex = 0;
		cursor = 1;
		break;
		case MENU_EEPROM_WRITE_PRESS:
		break;
		case MENU_EEPROM_WRITE_RELEASE:
		eeprom_write_block((void*)&string,(const void*)12,16);
		break;
		case MENU_EEPROM_READ:
		eeprom_read_block((void*)&string,(const void*)12,16);
		break;
		case SMILEY:
		LCD_WriteData(0);
		break;
	}

	return state;
}

//////////////////////////////////////////////////////////////////////////////
//SM for LED matrix logic
const unsigned char ex1 = 0b11111111;
const unsigned char topMask = 0b1000000;
const unsigned char bottomMask = 0b01111111;

enum MATRIX_States{MATRIX_START};
	
int TickFct_Matrix(int state) {
	
	switch (state) {
		case MATRIX_START:
			state = MATRIX_START;
			break;
		default:
			state = MATRIX_START;
			break;
	}
	
	switch(state) {
		case MATRIX_START:
			PORTD = ex1 & bottomMask;
			PORTA = ex1 & topMask;
			break;
		default:
			break;
	}
}

int main(void) {
	DDRA = 0x00; PORTA = 0xFF;
	DDRB = 0xFF; PORTB = 0x00;
	DDRC = 0xFF; PORTC = 0x00;
	DDRD = 0xFF; PORTD = 0x00;
	
	TimerOn();
	TimerSet(1);
	
	/*task task1;
	task1.state = MENU_WAIT;
	task1.period = 100;
	task1.elapsedTime = task1.period;
	task1.TickFct = &Tick_MENU;

	task task2;
	task2.state = MATRIX_START;
	task2.period = 1;
	task2.elapsedTime = task2.period;
	task2.TickFct = &TickFct_Matrix;

	task* tasklist[] = {&task1};

	TimerOn();
	TimerSet(100);
	LCD_init();
	
	BuildAllChar();
	string[0] = alpha[0];
	 
	unsigned char i;*/
	while (1) {
		PORTB = 0b00000001; // PORTB CONTROLS COLUMNS
		PORTD = 0b11111110; // PORTD - 1 turns off the LED AND CONTROLS ROWS
		
		/*for(i = 0; i < 1; i++){
			if ( tasklist[i]->elapsedTime == tasklist[i]->period ) {
				tasklist[i]->state = tasklist[i]->TickFct(tasklist[i]->state);
				tasklist[i]->elapsedTime = 0;
			}
			tasklist[i]->elapsedTime += 100;
		}

		LCD_DisplayString(1,string);
		if(!button1 && !button2){
			LCD_Cursor(cursor);
			LCD_WriteData(alpha[alphaIndex]);
		}*/
		while(!TimerFlag);
		TimerFlag = 0;
	}
	return 1;
}
