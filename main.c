#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "asciiLib.h"

#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 40
#define CONSOLE_WIDTH 76
#define CONSOLE_HEIGHT 19
#define INPUT_MAX_LENGTH (CONSOLE_WIDTH-2)
#define INPUT_POS_X 4
#define INPUT_POS_Y (SCREEN_HEIGHT-1)

#define min(a,b) ((a)<(b)?(a):(b))

asciiEngine* e;
asciiBitmap consoleHeader;
char inputData[INPUT_MAX_LENGTH+1];
uint8_t inputPtr=0;

void drawConsole () {
	asciiDrawBitmapColored(e,consoleHeader,asciiRect(0,SCREEN_HEIGHT-CONSOLE_HEIGHT-1,0,0),ASCII_COLOR_WHITE,ASCII_COLOR_BLACK);
	asciiFillRect (e,asciiChar('|',ASCII_COLOR_WHITE,ASCII_COLOR_BLACK),asciiRect(0,SCREEN_HEIGHT-CONSOLE_HEIGHT,1,CONSOLE_HEIGHT));
	asciiFillRect (e,asciiChar('|',ASCII_COLOR_WHITE,ASCII_COLOR_BLACK),asciiRect(SCREEN_WIDTH-2,SCREEN_HEIGHT-CONSOLE_HEIGHT,1,CONSOLE_HEIGHT));
	asciiFillRect (e,asciiChar(' ',ASCII_COLOR_WHITE,ASCII_COLOR_BLACK),asciiRect(1,SCREEN_HEIGHT-CONSOLE_HEIGHT,CONSOLE_WIDTH+1,CONSOLE_HEIGHT));
}
void writeConsoleLine (const char* text,uint32_t len) {
	uint32_t lines=len/SCREEN_WIDTH+(len%SCREEN_WIDTH==0?0:1);
	uint32_t y;
	if (len>CONSOLE_WIDTH*CONSOLE_HEIGHT) {
		text+=len-CONSOLE_WIDTH*CONSOLE_HEIGHT;
		lines=CONSOLE_HEIGHT;
		len=CONSOLE_WIDTH*CONSOLE_HEIGHT;
	}
	asciiScrollRect(e,lines,asciiRect(2,SCREEN_HEIGHT-CONSOLE_HEIGHT,CONSOLE_WIDTH,CONSOLE_HEIGHT));
	asciiFillRect(e,asciiChar(' ',ASCII_COLOR_WHITE,ASCII_COLOR_BLACK),asciiRect(2,SCREEN_HEIGHT-lines,CONSOLE_WIDTH,lines));
	for (y=0;y<lines;y++) {
		asciiDrawSizedTextColored(e,text,min(len,CONSOLE_WIDTH),asciiPoint(2,SCREEN_HEIGHT-lines+y),ASCII_COLOR_WHITE,ASCII_COLOR_BLACK);
		text+=CONSOLE_WIDTH;
		len-=CONSOLE_WIDTH;
	}
}
void writeConsole (const char* text) {
	uint32_t len;
	const char* ptr=text;
	while (*ptr!=0) {
		len=0;
		while (*ptr!=0&&*ptr!='\n') {
			len++;
			ptr++;
		}
		writeConsoleLine(ptr-len,len);
		if (*ptr=='\n')
			ptr++;
	}
}

void keyEventCallback (uint8_t key,uint8_t pressed,void* context) {
	if (pressed) {
		if (key==ASCII_KEY_ESCAPE)
            asciiSignalQuit (e);
        else if (key>=ASCII_KEY_A&&key<=ASCII_KEY_Z&&inputPtr!=INPUT_MAX_LENGTH) {
            inputData[inputPtr]='A'+(key-ASCII_KEY_A);
            inputPtr++;
            inputData[inputPtr]=0;
            asciiDrawTextColored(e,inputData,asciiPoint(INPUT_POS_X,INPUT_POS_Y),ASCII_COLOR_WHITE,ASCII_COLOR_BLACK);
            asciiFlip (e);
        }
        else if (key==ASCII_KEY_SPACE&&inputPtr!=INPUT_MAX_LENGTH) {
            inputData[inputPtr]=' ';
            inputPtr++;
            inputData[inputPtr]=0;
            asciiDrawTextColored(e,inputData,asciiPoint(INPUT_POS_X,INPUT_POS_Y),ASCII_COLOR_WHITE,ASCII_COLOR_BLACK);
            asciiFlip (e);
        }
        else if (key==ASCII_KEY_BACKSPACE&&inputPtr!=0) {
            inputPtr--;
            inputData[inputPtr]=0;
            asciiDrawChar(e,asciiChar(' ',ASCII_COLOR_WHITE,ASCII_COLOR_BLACK),asciiPoint(INPUT_POS_X+inputPtr,INPUT_POS_Y));
            asciiDrawTextColored(e,inputData,asciiPoint(INPUT_POS_X,INPUT_POS_Y),ASCII_COLOR_WHITE,ASCII_COLOR_BLACK);
            asciiFlip (e);
        }
        else if (key==ASCII_KEY_RETURN&&inputPtr!=0) {
            writeConsole (inputData);
            writeConsole ("> ");
            inputPtr=0;
            inputData[inputPtr]=0;
            asciiFlip (e);
        }
	}
}

void mouseMoveEventCallback (uint8_t buttonPressed,asciiPoint pos,void* context) {
	if (buttonPressed) {
		asciiDrawChar(e,asciiChar('x',ASCII_COLOR_WHITE,ASCII_COLOR_BLACK),pos);
		asciiFlip (e);
	}
}

int main (int argc,char* argv[]) {
	srand((unsigned int)time(0));
	e = asciiInit (ASCII_GRAPHIC_DEFAULT,SCREEN_WIDTH,SCREEN_HEIGHT);
	asciiSetKeyEventCallback (e,keyEventCallback,0);
	asciiSetMouseMoveEventCallback(e,mouseMoveEventCallback,0);
	consoleHeader=asciiLoadBitmapFromFile ("image.txt");
	if (consoleHeader.address==0)
		asciiDrawTextColored(e,"Failed to load bitmap!",asciiPoint(30,0),ASCII_COLOR_RED,ASCII_COLOR_GREEN);
	else {
	    asciiDrawChar(e,asciiChar('^',ASCII_COLOR_RED,ASCII_COLOR_BLUE),asciiPoint(10,10));
		drawConsole ();
		writeConsole("You are likely to be eaten by a grue.");
		writeConsole("> ");
	}
	asciiFlip (e);
	asciiRun (e);
	asciiFreeBitmap(&consoleHeader);
	return 0;
}
