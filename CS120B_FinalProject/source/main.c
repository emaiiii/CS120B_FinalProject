#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "io.h"
#include "io.c"
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
unsigned char sq[] = {0x00,0x1f,0x11,0x11,0x11,0x1f,0x00,0x00};
unsigned char curvy[] = {0x01,0x02,0x04,0x08,0x10,0x11,0x1f,0x00};
	
// special character function
void BuildAllChar(){
	LCD_BuildChar(0, downward);
	LCD_BuildChar(1, sq);
	LCD_BuildChar(2, curvy);
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
const unsigned long gameStartPeriod = 100;
const unsigned long gamePlayPeriod = 100;

unsigned char alpha[] = {0, 1, 2};
unsigned char cursor = 1;

///////////////////////////////////////////////////////////////////////
//SM for game start
unsigned char startFlag = 0;
unsigned char stopFlag = 0;

unsigned char *menuMessage = "To Play - click the left button";
unsigned char *gameStarted = "Game started";

enum GameStart_State {Game_waitPress, Game_waitRelease, Game_start, Game_checkScore};

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
			break;
		case Game_waitRelease:
			startFlag = 1;
			break;
		case Game_start:
			break;
		default:
			break;
	}

	return state; // doesnt work when using atmels
}

///////////////////////////////////////////////////////////////////////
//SM for game logic
unsigned char cnt = 0;
unsigned short score = 0;
unsigned short highScore = 0;
unsigned char symbolIndex = 0;

unsigned char displayBuffer[32];
unsigned char tempBuffer[16];

unsigned char *scoreString = "Score: ";
unsigned char *newHighScoreString = "New High Score: ";

enum GamePlay_state{GamePlay_init, GamePlay_start, GamePlay_wait, GamePlay_scoreInc, GamePlay_scoreDec, GamePlay_checkScore};
	
int TickFct_GamePlay(int state) {
	
	switch(state) {
		case GamePlay_init:
			if(startFlag == 1) {
				state = GamePlay_start;
			}
			else {
				state = GamePlay_init;
			}
			break;
		case GamePlay_start:
			if(stopFlag != 1) {
				state = GamePlay_wait;
			}
			else {
				cnt = 0;
				state = GamePlay_checkScore;
			}
			break;
		case GamePlay_wait:
			if(symbolIndex == 0 && button1 && !button2 && !button3 && !button4) {
				state = GamePlay_scoreInc;
			}
			else if(symbolIndex == 1 && !button1 && button2 && !button3 && !button4) {
				state = GamePlay_scoreInc;
			}
			else if(symbolIndex == 2 && !button1 && !button2 && button3 && !button4) {
				state = GamePlay_scoreInc;
			}
			else if(cnt == 5) {
				state = GamePlay_scoreDec;
			}
			break;
		case GamePlay_scoreInc:
			state = GamePlay_start;
			break;
		case GamePlay_scoreDec:
			state = GamePlay_start;
			break;
		case GamePlay_checkScore:
			if(cnt == 50) {
				state = GamePlay_init;
			}
			else {
				state = GamePlay_checkScore;
			}
			break;
		default:
			state = GamePlay_init;
			break;
	}
	
	switch(state) {
		case GamePlay_init:
			break;
		case GamePlay_start:
			cnt = 0;
			symbolIndex = floor(rand() % 3);
			
			// operations to show the score
			strcpy(displayBuffer,"                SCORE: ");
			sprintf(tempBuffer, "%hu", score);
			strcat(displayBuffer, tempBuffer);
			
			LCD_DisplayString(1, displayBuffer);
			LCD_Cursor(1);
			LCD_WriteData(alpha[symbolIndex]);
			break;
		case GamePlay_wait:
			cnt += 1;
			break;
		case GamePlay_scoreInc:
			score += 100;
			symbolIndex = rand() % 3;
			break;
		case GamePlay_scoreDec:
			if(score - 50 < 0) {
				score -= 50;
			}
			symbolIndex = rand() % 3;
			break;
		case GamePlay_checkScore:
			startFlag = 0;
			stopFlag = 0;
			
			highScore = eeprom_read_word((uint16_t*)69);
			
			if(score > highScore) {
				eeprom_write_word((uint16_t*)69, score);
				
				strcpy(displayBuffer,"NEW HIGH SCORE: ");
				sprintf(tempBuffer, "%hu", score);
				strcat(displayBuffer, tempBuffer);
				
				LCD_DisplayString(1, displayBuffer);
			}
			else {
				strcpy(displayBuffer,"SCORE: ");
				sprintf(tempBuffer, "%hu", score);
				strcat(displayBuffer, tempBuffer);
				
				LCD_DisplayString(1, displayBuffer);
			}
			break;
		default:
			break;
	}
	
	return state;
}
	
	//eeprom_write_word((uint16_t*)69, score);
	//eeprom_read_word((uint16_t*)69);
///////////////////////////////////////////////////////////////////////
//SM for matrix LED
//PORTD D0 - D5 and PORTA A6 and A7 - for rows - 1 turns off and 0 turns on
//PORTB B0 - B7 - for columns - 0 turns off and 1 turns on
unsigned long matrixRowSequence[8] = {0b01111111, 0b00111111, 0b00011111, 0b00001111, 0b00000111, 0b00000011, 0b00000001, 0b00000000};
unsigned long matrixColSequence[8] = {0b00000001, 0b00000011, 0b00000111, 0b00001111, 0b00011111, 0b00111111, 0b01111111, 0b11111111};

unsigned char row = 0;
unsigned char column = 0;

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

int main(void) {
	DDRA = 0xF0; PORTA = 0x0F;
	DDRB = 0xFF; PORTB = 0x00;
	DDRC = 0xFF; PORTC = 0x00;
	DDRD = 0xFF; PORTD = 0x00;
	
	BuildAllChar();

	static task task1;
	static task task2;
	static task task3;
	task *tasklist[] = {&task1, &task2, &task3};		

	task1.state = Game_start;
	task1.period = gameStartPeriod;
	task1.elapsedTime = 0;
	task1.TickFct = &TickFct_GameStart;
		
	task2.state = GamePlay_wait;
	task2.period = gamePlayPeriod;
	task2.elapsedTime = 0;
	task2.TickFct = &TickFct_GamePlay;

	task3.state = Matrix_start;
	task3.period = matrixPeriod;
	task3.elapsedTime = 0;
	task3.TickFct = &TickFct_Matrix;

	TimerOn();
	TimerSet(100);
	LCD_init();

	unsigned char i;

	while (1) {		
		for(i = 0; i < 3; i++){
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

	
