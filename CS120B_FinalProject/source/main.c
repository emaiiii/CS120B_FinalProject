#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "io.h"
//#include "io.c"
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

const unsigned char taskSize = 2;
task tasks[2];

const unsigned long taskPeriod = 100;
const unsigned long matrixPeriod = 100;
const unsigned long gameStartPeriod = 200;
const unsigned long gamePlayPeriod = 100;

char alpha[] = {0x00, 0x01, 0x02, 0x03};
unsigned char alphaIndex = 0;
unsigned char cursor = 1;
unsigned char string[4];
unsigned char tempString[4];

///////////////////////////////////////////////////////////////////////
//SM for game start
unsigned char startFlag = 0;
unsigned char symbolIndex = 0;
unsigned char score = 0;
unsigned char *menuMessage = "To Play - click the left button";
unsigned char *gameStarted = "Game started";

enum GameStart_State {Game_waitPress, Game_waitRelease, Game_start};

int TickFct_GameStart(int state) {
	switch(state){
		case Game_waitPress:
			if(!button1 && !button2 && !button3 && button4) {
				state = Game_waitRelease;
			}
			else {
				state = Game_waitPress;
			}
			break;
		case Game_waitRelease:
			if(!button1 && !button2 && !button3 && button4) {
				state = Game_waitRelease;
			}
			else {
				state = Game_start;
			}
			break;
		case Game_start:
			if(startFlag == 1) {
				state = Game_start;
			}
			else {
				state = Game_waitPress;
			}
			break;
		default:
			state = Game_waitPress;
			break;
	}
	
	switch (state){
		case Game_waitPress:
			LCD_DisplayString(1, menuMessage);
			LCD_WriteData(menuMessage);
			break;
		case Game_waitRelease:
			startFlag = 1;
			LCD_DisplayString(1, gameStarted);
			LCD_WriteData(gameStarted);
			break;
		case Game_start:
			break;
		default:
			break;
	}
}

///////////////////////////////////////////////////////////////////////
//SM for game logic

enum GamePlay_state{GamePlay_wait, GamePlay_start, GamePlay_scoreInc, GamePlay_scoreDec};
	
int TickFct_GamePlay(int state) {
	switch(state) {
		case GamePlay_wait:
			if(startFlag == 1) {
				state = GamePlay_start;
			}
			else {
				state = GamePlay_wait;
			}
			break;
		case GamePlay_start:
			if(!button1 && !button2 && !button3 && !button4) {
				state = GamePlay_start;
			}
			else if(symbolIndex == 0 && button1 && !button2 && !button3 && !button4) {
				state = GamePlay_scoreInc;
			}
			else if(symbolIndex == 1 && !button1 && button2 && !button3 && !button4) {
				state = GamePlay_scoreInc;
			}
			else if(symbolIndex == 2 && !button1 && !button2 && button3 && !button4) {
				state = GamePlay_scoreInc;
			}
			else {
				state = GamePlay_scoreDec;
			}
			break;
		case GamePlay_scoreInc:
			state = GamePlay_start;
			break;
		case GamePlay_scoreDec:
			state = GamePlay_start;
			break;
		default:
			state = GamePlay_wait;
			break;
	}
	
	switch(state) {
		case GamePlay_wait:
			break;
		case GamePlay_start:
			symbolIndex = rand() % 3;
			LCD_DisplayString(1, alpha[symbolIndex]);
			break;
		case GamePlay_scoreInc:
			score += 100;
			symbolIndex = rand() % 3;
			break;
		case GamePlay_scoreDec:
			score -= 50;
			symbolIndex = rand() % 3;
			break;
		default:
			break;
	}
	
	return state;
}
///////////////////////////////////////////////////////////////////////
//SM for matrix LED
//PORTD D0 - D5 and PORTA A6 and A7 - for rows - 1 turns off and 0 turns on
//PORTB B0 - B7 - for columns - 0 turns off and 1 turns on
unsigned long matrixRowSequence[8] = {0b01111111, 0b00111111, 0b00011111, 0b00001111, 0b00000111, 0b00000011, 0b00000001, 0b00000000};
unsigned long matrixColSequence[8] = {0b00000001, 0b00000011, 0b00000111, 0b00001111, 0b00011111, 0b00111111, 0b01111111, 0b11111111};

unsigned char row = 0;
unsigned char column = 0;
unsigned char stopFlag = 0;

enum Matrix_States {Matrix_start};

int TickFct_Matrix(int state) {
	
	switch(state) {
		case Matrix_start:
			state = Matrix_start;
			break;
		default:
			state = Matrix_start;
			break;
	}
	
	switch(state) {
		case Matrix_start:
			if(startFlag == 1) {
				if(row == 8 && column != 7) {
					row = 0;
					column++;
				}
				else if(row < 8) {
					PORTB = matrixColSequence[column]; // PORTB CONTROLS COLUMNS
					PORTD = matrixRowSequence[row]; // 1 turns off the LED AND CONTROLS ROWS
					row++;
				}
				
				if(row == 8 && column == 7) {
					startFlag = 0;
					stopFlag = 1;
				}
			}
			break;
		default:
			break;
	}
	
	return state;
}

//////////////////////////////////////////////////////////////////////////
// ex stuff
/*enum MENU_States {MENU_WAIT,MENU_INC,MENU_DEC,MENU_WRITE,MENU_CLEAR,MENU_EEPROM_WRITE_PRESS,MENU_EEPROM_WRITE_RELEASE,MENU_EEPROM_READ,SMILEY};

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
}*/

int main(void) {
	DDRA = 0xF0; PORTA = 0x0F;
	DDRB = 0xFF; PORTB = 0x00;
	DDRC = 0xFF; PORTC = 0x00;
	DDRD = 0xFF; PORTD = 0x00;
	
	static task task1;
	static task task2;
	task *tasklist[] = {&task1, &task2};		

	task1.state = Game_start;
	task1.period = gameStartPeriod;
	task1.elapsedTime = 0;
	task1.TickFct = &TickFct_GameStart;
	
	/*task2.state = Matrix_start;
	task2.period = matrixPeriod;
	task2.elapsedTime = 0;
	task2.TickFct = &TickFct_Matrix;*/
	
	task2.state = GamePlay_wait;
	task2.period = gamePlayPeriod;
	task2.elapsedTime = 0;
	task2.TickFct = &TickFct_GamePlay;

	TimerOn();
	TimerSet(100);
	LCD_init();

	unsigned char i;

	while (1) {		
		for(i = 0; i < 2; i++){
			if ( tasklist[i]->elapsedTime == tasklist[i]->period ) {
				tasklist[i]->state = tasklist[i]->TickFct(tasklist[i]->state);
				tasklist[i]->elapsedTime = 0;
			}
			tasklist[i]->elapsedTime += taskPeriod;
		}

		while(!TimerFlag);
		TimerFlag = 0;
	}
	return 1;
}

	
	/*unsigned char i = 0;
	
	tasks[i].state = Matrix_start;
	tasks[i].period = matrixPeriod;
	tasks[i].elapsedTime = 0;
	tasks[i].TickFct = &TickFct_Matrix;
	i++;
	
	tasks[i].state = MENU_WAIT;
	tasks[i].period = gamePeriod;
	tasks[i].elapsedTime = 0;
	tasks[i].TickFct = &Tick_MENU;
	i++;
	
	TimerOn();
	TimerSet(taskPeriod);
	LCD_init();
	
	BuildAllChar();
	string[0] = alpha[0];*/
	
	//BuildAllChar();
	//string[0] = alpha[0];

	// in while loop
	/*LCD_DisplayString(1,string);
	if(!button1 && !button2){
		LCD_Cursor(cursor);
		LCD_WriteData(alpha[alphaIndex]);
	}*/
