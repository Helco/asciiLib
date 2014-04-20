#include "asciiLibIntern.h"
#ifndef ASCII_ENABLED_BACKEND_WIN_CONSOLE
#include <Windows.h>
#pragma comment (lib,"winmm.lib")

struct _asciiWin {
	asciiEngine* engine;
	asciiBool isRunning;
	HANDLE hConsoleOut;
	HANDLE hConsoleIn;
	CHAR_INFO* consoleScreenBuffer;
	COORD dwBufferCoord,dwBufferSize;
	SMALL_RECT dwConsoleRect;
	BYTE keyboardState[ASCII_KEYCOUNT];
} asciiWin; //why do I keep this badstyle? because of ConsoleCtrlHander

void _ascii_signalQuitWin (asciiEngine* e) {
	asciiWin.isRunning = 0;
}
BOOL WINAPI _ascii_console_handler (DWORD ev) {
	asciiQuit (asciiWin.engine);
	return FALSE;
}
asciiBool _ascii_initWin (asciiEngine* e,int32_t w,int32_t h) {
	uint16_t i;
	CONSOLE_CURSOR_INFO cursor;
	asciiWin.engine = e;
	for (i=0;i<ASCII_KEYCOUNT;i++)
		asciiWin.keyboardState[i] = 0;
	asciiWin.consoleScreenBuffer = (CHAR_INFO*)malloc(w*h*sizeof(CHAR_INFO));
	if (!asciiWin.consoleScreenBuffer)
		return 0;
	asciiWin.hConsoleOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (!asciiWin.hConsoleOut)
		return 0;
	asciiWin.hConsoleIn = GetStdHandle(STD_INPUT_HANDLE);
	if (!asciiWin.hConsoleIn)
		return 0;
	asciiWin.dwBufferCoord.X = 0;
	asciiWin.dwBufferCoord.Y = 0;
	asciiWin.dwBufferSize.X = w;
	asciiWin.dwBufferSize.Y = h;
	asciiWin.dwConsoleRect.Top = 0;
	asciiWin.dwConsoleRect.Left = 0;
	asciiWin.dwConsoleRect.Bottom = h - 1;
	asciiWin.dwConsoleRect.Right = w - 1;
	cursor.bVisible = 0;
	cursor.dwSize = 1;
	if (SetConsoleScreenBufferSize(asciiWin.hConsoleOut, asciiWin.dwBufferSize)==0 ||	// Set Buffer Size
		SetConsoleWindowInfo(asciiWin.hConsoleOut, TRUE, &asciiWin.dwConsoleRect)==0 ||	// Set Window Size
		SetConsoleCursorInfo(asciiWin.hConsoleOut,&cursor)==0 ||					// Disable console cursor
		SetConsoleCtrlHandler(_ascii_console_handler,TRUE)==0 ||				// Handle closing
		SetConsoleMode(asciiWin.hConsoleIn,ENABLE_MOUSE_INPUT)==0)				// Enable mouse
		return 0;
	return 1;
}
void _ascii_quitWin (asciiEngine* e) {
	if (asciiWin.consoleScreenBuffer) {
		free(asciiWin.consoleScreenBuffer);
		asciiWin.consoleScreenBuffer = 0;
	}
}
const int8_t _win_colorMappings [ASCII_COLOR_COUNT]={0x00,0x0c,0x0a,0x0e,0x09,0x05,0x0b,0x07};
asciiBool _ascii_flipWin (asciiEngine* e) {
	asciiChar* sourcePtr = e->screen.address;
	CHAR_INFO* targetPtr = asciiWin.consoleScreenBuffer;
	WORD attributes = 0xffff;
	int8_t backColor,foreColor;
	int32_t x,y;
	for (y=0;y<e->screen.bounds.size.y;y++) {
		for (x=0;x<e->screen.bounds.size.x;x++) {
			if (attributes==0xffff || backColor!=sourcePtr->backColor || foreColor!=sourcePtr->foreColor) {
				backColor = _win_colorMappings[sourcePtr->backColor];
				foreColor = _win_colorMappings[sourcePtr->foreColor];
				attributes = (WORD)((backColor<<4)|foreColor);
			}
			targetPtr->Char.AsciiChar = sourcePtr->character;
			targetPtr->Attributes = attributes;
			sourcePtr++;
			targetPtr++;
		}
	}
	return (int8_t)WriteConsoleOutputA(asciiWin.hConsoleOut,asciiWin.consoleScreenBuffer,
		asciiWin.dwBufferSize,asciiWin.dwBufferCoord,&asciiWin.dwConsoleRect);
}
#define _win_keyMappingCount 12 //a constant doesn't work in msvc
#define _win_inputBufferSize 16
const asciiKeyMap _win_keyMappings [_win_keyMappingCount]={
	{VK_BACK,ASCII_KEY_BACKSPACE},{VK_TAB,ASCII_KEY_TAB},{VK_RETURN,ASCII_KEY_RETURN},{VK_ESCAPE,ASCII_KEY_ESCAPE},
	{VK_SPACE,ASCII_KEY_SPACE},{VK_UP,ASCII_KEY_UP},{VK_DOWN,ASCII_KEY_DOWN},{VK_RIGHT,ASCII_KEY_RIGHT},
	{VK_LEFT,ASCII_KEY_LEFT},{VK_SHIFT,ASCII_KEY_SHIFT},{VK_CONTROL,ASCII_KEY_CTRL},{VK_MENU,ASCII_KEY_ALT}};
void _ascii_runWin (asciiEngine* e) {
	INPUT_RECORD inputBuffer[_win_inputBufferSize];
	DWORD i,inputLen,chunkLen;
	uint8_t key,mapI;
	asciiPoint mousePos;
	asciiTimerID timerID;
	asciiWin.isRunning=1;
	while (asciiWin.isRunning) {
		for (timerID=0;timerID<ASCII_MAX_TIMER;timerID++) {
			if (e->timers[timerID].callback!=0 && e->timers[timerID].timeout<=timeGetTime ()) {
				e->timers[timerID].callback(e->timers[timerID].context);
				e->timers[timerID].callback=0;
			}
		}
		if (GetNumberOfConsoleInputEvents (asciiWin.hConsoleIn,&inputLen)!=0 && inputLen>0) {
				while (inputLen>0) {
					if (ReadConsoleInputA(asciiWin.hConsoleIn,inputBuffer,_win_inputBufferSize,&chunkLen)==0)
						break;
					if (inputLen >= chunkLen)
						inputLen -= chunkLen;
					else
						inputLen = 0;
					for (i=0;i<chunkLen;i++) {
						if (inputBuffer[i].EventType==KEY_EVENT) {
							if (inputBuffer[i].Event.KeyEvent.uChar.AsciiChar>='a' && inputBuffer[i].Event.KeyEvent.uChar.AsciiChar<='z')
								key = ASCII_KEY_A + (inputBuffer[i].Event.KeyEvent.uChar.AsciiChar-'a');
							else if (inputBuffer[i].Event.KeyEvent.uChar.AsciiChar>='A' && inputBuffer[i].Event.KeyEvent.uChar.AsciiChar<='Z')
								key = ASCII_KEY_A + (inputBuffer[i].Event.KeyEvent.uChar.AsciiChar-'A');
							else if (inputBuffer[i].Event.KeyEvent.uChar.AsciiChar>='0' && inputBuffer[i].Event.KeyEvent.uChar.AsciiChar<='9')
								key = ASCII_KEY_0 + (inputBuffer[i].Event.KeyEvent.uChar.AsciiChar-'0');
							else if (inputBuffer[i].Event.KeyEvent.wVirtualKeyCode>=VK_F1 && inputBuffer[i].Event.KeyEvent.wVirtualKeyCode<=VK_F12)
								key = ASCII_KEY_F1 + (inputBuffer[i].Event.KeyEvent.wVirtualKeyCode-VK_F1);
							else {
								for (mapI=0;mapI<_win_keyMappingCount;mapI++) {
									if (_win_keyMappings[mapI].hardware == inputBuffer[i].Event.KeyEvent.wVirtualKeyCode) {
										key = _win_keyMappings[mapI].key;
										break;
									}
								}
								if (mapI>=_win_keyMappingCount)
									continue;
							}
							if (inputBuffer[i].Event.KeyEvent.bKeyDown==TRUE)
								asciiOnKeyDown (asciiWin.engine,key);
							else
								asciiOnKeyUp (asciiWin.engine,key);
						} //key event
						else if (inputBuffer[i].EventType==MOUSE_EVENT) {
							if (inputBuffer[i].Event.MouseEvent.dwEventFlags == 0) {
								if ((inputBuffer[i].Event.MouseEvent.dwButtonState&FROM_LEFT_1ST_BUTTON_PRESSED)>0)
									asciiOnMouseDown(asciiWin.engine,ASCII_MOUSE_BUTTON_LEFT);
								else
									asciiOnMouseUp(asciiWin.engine,ASCII_MOUSE_BUTTON_LEFT);
								if ((inputBuffer[i].Event.MouseEvent.dwButtonState&RIGHTMOST_BUTTON_PRESSED)>0)
									asciiOnMouseDown(asciiWin.engine,ASCII_MOUSE_BUTTON_RIGHT);
								else
									asciiOnMouseUp(asciiWin.engine,ASCII_MOUSE_BUTTON_RIGHT);
							}
							else if (inputBuffer[i].Event.MouseEvent.dwEventFlags == MOUSE_MOVED) {
								mousePos.x = (int32_t)inputBuffer[i].Event.MouseEvent.dwMousePosition.X;
								mousePos.y = (int32_t)inputBuffer[i].Event.MouseEvent.dwMousePosition.Y;
								asciiOnMouseMove(asciiWin.engine,mousePos);
							}
						}
					} //for (i=0;i<chunklen;i++)
				} //while (inputLen>0)
		}
		Sleep(16); //equals about 60 frames per second
	}
	asciiQuit (e);
}
int8_t _ascii_setTimeoutWin (asciiEngine* e,asciiTimerID id) {
	e->timers[id].timeout+=timeGetTime ();
	return 1;
}
void _ascii_changedEventWin (asciiEngine* e,asciiEvent ev) {
}
const asciiGBackend asciiGraphicBackendWinConsole = {
	_ascii_initWin,
	_ascii_runWin,
	_ascii_quitWin,
	_ascii_flipWin,
	_ascii_setTimeoutWin,
	_ascii_signalQuitWin,
	_ascii_changedEventWin
};
#endif