#include "asciiLib.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <malloc.h>

/*
You can set the backend manually by setting the USE_BACKEND macro under
this commentar to one of the following specific backend macros
*/

#define USE_BACKEND_EMSCRIPTEN 0
#define USE_BACKEND_WIN_CONSOLE 1
#define USE_BACKEND_UNIX_CONSOLE 2 //not implemented
#define USE_BACKEND_SDL 3 //not implemented

//#define FORCE_BACKEND_SDL

#ifndef USE_BACKEND
	#if defined FORCE_BACKEND_SDL
		#define USE_BACKEND USE_BACKEND_SDL
	#elif defined EMSCRIPTEN
		#define USE_BACKEND USE_BACKEND_EMSCRIPTEN
	#elif defined _MSC_VER || defined WIN32 || defined _WIN32
		#define USE_BACKEND USE_BACKEND_WIN_CONSOLE
	#elif defined __APPLE__ && defined TARGET_OS_MAC
		#define USE_BACKEND USE_BACKEND_SDL
	#elif defined LINUX || defined _LINUX || defined __linux
		#define USE_BACKEND USE_BACKEND_UNIX_CONSOLE
	#else
		#error [ASCIILIB]: Couldn't detect platform, please set backend manually to a specific backend (e.g. USE_BACKEND_SDL) 
	#endif
#endif

int8_t _ascii_initSys (uint8_t w,uint8_t h);
void _ascii_runSys ();
void _ascii_quitSys ();
int8_t _ascii_flipSys ();
void _ascii_changeStdColorsSys (int8_t backColor,int8_t foreColor);
const int8_t _ascii_keyAsciiTable[ASCII_KEYCOUNT]={
	'\b','\t','\n',0,' ','0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F','G',
	'H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z',0,0,0,0,0,0};
typedef struct {
	int32_t hardware;
	uint8_t key;
} _ascii_keyMap;

struct {
	asciiPoint consoleSize;
	asciiChar* screenBuffer;
	int8_t stdBackColor,stdForeColor;
	int8_t isInited;
	asciiKeyEventCallback keyEventCallback;
	void* keyEventCallbackContext;
	asciiQuitCallback quitCallback;
	void* quitCallbackContext;
} _ascii={
	{0,0},
	0,ASCII_COLOR_BLACK,ASCII_COLOR_WHITE,0,
	0,0,
	0,0
};

int8_t asciiInit (uint8_t w,uint8_t h) {
	if (_ascii.isInited==0&&w>0&&h>0) {
		_ascii.screenBuffer=(asciiChar*)malloc(w*h*sizeof(asciiChar));
		if (!_ascii.screenBuffer)
			return 0;
		memset(_ascii.screenBuffer,0,w*h*sizeof(asciiChar));
		if (_ascii_initSys(w,h)==0) {
			free(_ascii.screenBuffer);
			return 0;
		}
		_ascii.consoleSize.x=w;
		_ascii.consoleSize.y=h;
		_ascii.isInited=1;
	}
	return 0;
}

void _ascii_quit () {
	if (_ascii.isInited==1) {
		if (_ascii.quitCallback)
			_ascii.quitCallback(_ascii.quitCallbackContext);
		_ascii_quitSys ();
		free(_ascii.screenBuffer);
		_ascii.isInited=0;
	}
}

void asciiRun () {
	if (_ascii.isInited==1) {
		_ascii_runSys ();
	}
}

int8_t asciiFlip () {
	if (_ascii.isInited==1) {
		return _ascii_flipSys ();
	}
	return 0;
}

void asciiFillRect (asciiChar ch,asciiRect rect) {
	if (_ascii.isInited==1&&rect.offset.x<_ascii.consoleSize.x&&rect.offset.y<_ascii.consoleSize.y
		&&rect.size.x>0&&rect.size.y>0&&isprint(ch.character)!=0&&ch.backColor>=0&&ch.backColor<ASCII_COLOR_COUNT&&
		ch.foreColor>=0&&ch.foreColor<ASCII_COLOR_COUNT) {
		asciiChar* ptr;
		uint8_t x,y;
		if (rect.offset.x+rect.size.x>=_ascii.consoleSize.x)
			rect.size.x=_ascii.consoleSize.x-rect.offset.x;
		if (rect.offset.y+rect.size.y>=_ascii.consoleSize.y)
			rect.size.y=_ascii.consoleSize.y-rect.offset.y;
		for (y=rect.offset.y;y<rect.offset.y+rect.size.y;y++) {
			ptr=_ascii.screenBuffer+y*_ascii.consoleSize.x+rect.offset.x;
			for (x=rect.offset.x;x<rect.offset.x+rect.size.x;x++) {
				*ptr=ch;
				ptr++;
			}
		}
	}
}

void asciiDrawSizedTextColored (asciiString text,uint32_t len,asciiPoint offset,int8_t backColor,int8_t foreColor) {
	uint32_t i,offsetIndex,consoleLen;
	asciiChar* targetPtr;
	asciiString sourcePtr=text;
	if (backColor<0||backColor>=ASCII_COLOR_COUNT||foreColor<0||foreColor>=ASCII_COLOR_COUNT||
		offset.x>=_ascii.consoleSize.x||offset.y>=_ascii.consoleSize.y||_ascii.isInited==0||text==0||len==0)
		return;
	offsetIndex=offset.y*_ascii.consoleSize.x+offset.x;
	consoleLen=((uint32_t)_ascii.consoleSize.x)*_ascii.consoleSize.y;
	if (offsetIndex+len>consoleLen)
		len=consoleLen-offsetIndex;
	targetPtr=_ascii.screenBuffer+offsetIndex;
	for (i=0;i<len;i++) {
		if (isprint(*sourcePtr))
			targetPtr->character=*sourcePtr;
		targetPtr->backColor=backColor;
		targetPtr->foreColor=foreColor;
		targetPtr++;
		sourcePtr++;
	}
}

void asciiDrawBitmapColored (asciiBitmap bitmap,asciiRect rect,int8_t backColor,int8_t foreColor) {
	uint8_t x,y;
	int8_t* sourcePtr;
	asciiChar* targetPtr;
	if (backColor<0||backColor>=ASCII_COLOR_COUNT||foreColor<0||foreColor>=ASCII_COLOR_COUNT||
		rect.offset.x>=_ascii.consoleSize.x||rect.offset.y>=_ascii.consoleSize.y||_ascii.isInited==0||
		bitmap.address==0||bitmap.bounds.size.x==0||bitmap.bounds.size.y==0)
		return;
	if (rect.size.x==0)
		rect.size.x=bitmap.bounds.size.x;
	if (rect.size.y==0)
		rect.size.y=bitmap.bounds.size.y;
	if (rect.offset.x+rect.size.x>=_ascii.consoleSize.x)
		rect.size.x=_ascii.consoleSize.x-rect.offset.x;
	if (rect.offset.y+rect.size.y>=_ascii.consoleSize.y)
		rect.size.y=_ascii.consoleSize.y-rect.offset.y;
	for (y=0;y<rect.size.y;y++) {
		targetPtr=_ascii.screenBuffer+(rect.offset.y+y)*_ascii.consoleSize.x+rect.offset.x;
		for (x=0;x<rect.size.x;x++) {
			sourcePtr=bitmap.address+
				((y%bitmap.bounds.size.y)+bitmap.bounds.offset.y)*bitmap.bounds.size.x+
				((x%bitmap.bounds.size.x)+bitmap.bounds.offset.x);
			if (isprint(*sourcePtr)&&*sourcePtr!=bitmap.trans) {
				targetPtr->character=*sourcePtr;
				targetPtr->backColor=backColor;
				targetPtr->foreColor=foreColor;
			}
			targetPtr++;
		}
	}
}

void asciiDrawColoredBitmap(asciiColoredBitmap bitmap,asciiRect rect) {
	uint8_t x,y;
	asciiChar* sourcePtr;
	asciiChar* targetPtr;
	if (rect.offset.x>=_ascii.consoleSize.x||rect.offset.y>=_ascii.consoleSize.y||_ascii.isInited==0||
		bitmap.address==0||bitmap.bounds.size.x==0||bitmap.bounds.size.y==0)
		return;
	if (rect.size.x==0)
		rect.size.x=bitmap.bounds.size.x;
	if (rect.size.y==0)
		rect.size.y=bitmap.bounds.size.y;
	if (rect.offset.x+rect.size.x>=_ascii.consoleSize.x)
		rect.size.x=_ascii.consoleSize.x-rect.offset.x;
	if (rect.offset.y+rect.size.y>=_ascii.consoleSize.y)
		rect.size.y=_ascii.consoleSize.y-rect.offset.y;
	for (y=0;y<rect.size.y;y++) {
		targetPtr=_ascii.screenBuffer+(rect.offset.y+y)*_ascii.consoleSize.x+rect.offset.x;
		for (x=0;x<rect.size.x;x++) {
			sourcePtr=bitmap.address+
				((y%bitmap.bounds.size.y)+bitmap.bounds.offset.y)*bitmap.bounds.size.x+
				((x%bitmap.bounds.size.x)+bitmap.bounds.offset.x);
			if (isprint(sourcePtr->character)&&sourcePtr->character!=bitmap.trans) {
				targetPtr->character=sourcePtr->character;
				targetPtr->backColor=sourcePtr->backColor;
				targetPtr->foreColor=sourcePtr->foreColor;
			}
			targetPtr++;
		}
	}
}

void asciiScrollRect (uint8_t amount,asciiRect rect) {
	if (_ascii.isInited==1&&rect.offset.x<_ascii.consoleSize.x&&rect.offset.y<_ascii.consoleSize.y&&
		rect.size.x>0&&rect.size.y>0&&amount>0) {
			if (rect.offset.x+rect.size.x>_ascii.consoleSize.x)
				rect.size.x=_ascii.consoleSize.x-rect.offset.x;
			if (rect.offset.y+rect.size.y>_ascii.consoleSize.y)
				rect.size.y=_ascii.consoleSize.y-rect.offset.y;
			if (rect.size.y<=amount)
				asciiFillRect(asciiChar(' ',_ascii.stdBackColor,_ascii.stdForeColor),rect);
			else {
				uint8_t y;
				for (y=0;y<rect.size.y-amount;y++) {
					memcpy(_ascii.screenBuffer+(rect.offset.y+y)*_ascii.consoleSize.x+rect.offset.x,
						_ascii.screenBuffer+(rect.offset.y+y+amount)*_ascii.consoleSize.x+rect.offset.x,
						sizeof(asciiChar)*rect.size.x);
				}
				rect.offset.y+=rect.size.y-amount;
				rect.size.y=amount;
				asciiFillRect(asciiChar(' ',_ascii.stdBackColor,_ascii.stdForeColor),rect);
			}
	}
}

void asciiDrawCharacter (asciiChar c,asciiPoint offset) {
	if (_ascii.isInited==1&&offset.x<_ascii.consoleSize.x&&offset.y<_ascii.consoleSize.y&&isprint(c.character)&&
		c.backColor>=0&&c.backColor<ASCII_COLOR_COUNT&&c.foreColor>=0&&c.foreColor<ASCII_COLOR_COUNT) {
			_ascii.screenBuffer[offset.y*_ascii.consoleSize.x+offset.x]=c;
	}
}

void asciiDrawChar (int8_t c,asciiPoint offset) {
	asciiDrawCharacter(asciiChar(c,_ascii.stdBackColor,_ascii.stdForeColor),offset);
}

void asciiDrawCharColored (int8_t c,asciiPoint offset,int8_t backColor,int8_t foreColor) {
	asciiDrawCharacter(asciiChar(c,backColor,foreColor),offset);
}

void asciiScrollScreen (uint8_t amount) {
	asciiRect rect={{0,0},{_ascii.consoleSize.x,_ascii.consoleSize.y}};
	asciiScrollRect(amount,rect);
}

void asciiDrawText (asciiString text,asciiPoint offset) {
	asciiDrawSizedTextColored(text,strlen((const char*)text),offset,_ascii.stdBackColor,_ascii.stdForeColor);
}

void asciiDrawTextColored (asciiString text,asciiPoint offset,int8_t backColor,int8_t foreColor) {
	asciiDrawSizedTextColored(text,strlen((const char*)text),offset,backColor,foreColor);
}

void asciiDrawSizedText (asciiString text,uint32_t len,asciiPoint offset) {
	asciiDrawSizedTextColored(text,len,offset,_ascii.stdBackColor,_ascii.stdForeColor);
}

void asciiDrawBitmap (asciiBitmap bitmap,asciiRect rect) {
	asciiDrawBitmapColored (bitmap,rect,_ascii.stdBackColor,_ascii.stdForeColor);
}

void asciiClearRect (asciiRect rect) {
	asciiChar ch={' ',_ascii.stdBackColor,_ascii.stdForeColor,0};
	asciiFillRect(ch,rect);
}

void asciiSetKeyEventCallback (asciiKeyEventCallback callback,void* context) {
	_ascii.keyEventCallback=callback;
	_ascii.keyEventCallbackContext=context;
}

void asciiSetQuitCallback (asciiQuitCallback callback,void* context) {
	_ascii.quitCallback=callback;
	_ascii.quitCallbackContext=context;
}

int8_t asciiKeyToAscii (uint8_t key) {
	if (key>=ASCII_KEYCOUNT)
		return 0;
	return _ascii_keyAsciiTable[key];
}

asciiPoint asciiGetSize () {
	return _ascii.consoleSize;
}

asciiChar* asciiGetConsoleBuffer () {
	return _ascii.screenBuffer;
}

int8_t asciiGetStdBackColor () {
	return _ascii.stdBackColor;
}

int8_t asciiGetStdForeColor () {
	return _ascii.stdForeColor;
}

void asciiSetStdBackColor(int8_t backColor) {
	_ascii.stdBackColor=backColor;
	_ascii_changeStdColorsSys(_ascii.stdBackColor,_ascii.stdForeColor);
}

void asciiSetStdForeColor(int8_t foreColor) {
	_ascii.stdForeColor=foreColor;
	_ascii_changeStdColorsSys(_ascii.stdBackColor,_ascii.stdForeColor);
}

void asciiSetBitmapTransparent (asciiBitmap* bm,int8_t tr) {
	if (bm)
		bm->trans=tr;
}

void asciiSetColoredBitmapTransparent (asciiColoredBitmap* bm,int8_t tr) {
	if (bm)
		bm->trans=tr;
}

void asciiFreeBitmap (asciiBitmap* bm) {
	if (bm&&bm->address) {
		free(bm->address);
		bm->address=0;
		bm->trans=0;
		bm->bounds=asciiRect(0,0,0,0);
	}
}

void asciiFreeColoredBitmap (asciiColoredBitmap* bm) {
	if (bm&&bm->address) {
		free(bm->address);
		bm->address=0;
		bm->trans=0;
		bm->bounds=asciiRect(0,0,0,0);
	}
}

/*
	Backends
*/

#if USE_BACKEND == USE_BACKEND_EMSCRIPTEN
    #include <GL/glfw.h>
	#include <emscripten/emscripten.h>
	extern void js_ascii_setConsoleSize (uint8_t w,uint8_t h);
	extern void js_ascii_changeConsoleText (const char* text);
	extern void js_ascii_changeConsoleColors (const char* backColor,const char* foreColor);
	struct {
	    char* buffer;
	    uint32_t bufferLen;
	    uint32_t bufferPtr;
	    int8_t backColor,foreColor;
	} _web;
	static const uint32_t _web_bufferChunk=512;
	int8_t _ascii_initSys (uint8_t w,uint8_t h) {
	    _web.bufferLen=((uint32_t)w)*h;
	    _web.bufferPtr=0;
	    _web.backColor=_ascii.stdBackColor;
	    _web.foreColor=_ascii.stdForeColor;
	    _web.buffer=(char*)malloc(_web.bufferLen+1);
	    if (!_web.buffer)
            return 0;
        js_ascii_setConsoleSize(w,h);
        glfwInit ();//set up the keyboard event
        return 1;
	}
	#define _web_keyMappingCount 16
	const _ascii_keyMap _web_keyMappings [_web_keyMappingCount]={
			{GLFW_KEY_BACKSPACE,ASCII_KEY_BACKSPACE},{GLFW_KEY_TAB,ASCII_KEY_TAB},{GLFW_KEY_ENTER,ASCII_KEY_RETURN},{GLFW_KEY_ESC,ASCII_KEY_ESCAPE},
			{GLFW_KEY_SPACE,ASCII_KEY_SPACE},{GLFW_KEY_UP,ASCII_KEY_UP},{GLFW_KEY_DOWN,ASCII_KEY_DOWN},{GLFW_KEY_RIGHT,ASCII_KEY_RIGHT},
			{GLFW_KEY_LEFT,ASCII_KEY_LEFT},{GLFW_KEY_LSHIFT,ASCII_KEY_SHIFT},{GLFW_KEY_RSHIFT,ASCII_KEY_SHIFT},
			{GLFW_KEY_LCTRL,ASCII_KEY_CTRL},{GLFW_KEY_RCTRL,ASCII_KEY_CTRL},
			{255,ASCII_KEY_ESCAPE},{13,ASCII_KEY_RETURN},{'\b',ASCII_KEY_BACKSPACE}}; //he GLFW_KEY_* doesn't work for ESCAPE, RETURN and BACKSPACE
    #define GLFW_KEY_0 ((int)'0')
    #define GLFW_KEY_9 ((int)'9')
    #define GLFW_KEY_A ((int)'A')
    #define GLFW_KEY_Z ((int)'Z')
	void _ascii_glfwKeyHandler (int glKey,int glAction) {
	    if (_ascii.isInited==1&&_ascii.keyEventCallback&&(glAction==GLFW_PRESS||glAction==GLFW_RELEASE)) {
	        uint8_t key=ASCII_KEYCOUNT,action=(glAction==GLFW_PRESS?ASCII_KEYPRESSED:ASCII_KEYRELEASED);
	        if (glKey>=GLFW_KEY_A&&glKey<=GLFW_KEY_Z)
                key=ASCII_KEY_A+(glKey-GLFW_KEY_A);
            else if (glKey>=GLFW_KEY_0&&glKey<=GLFW_KEY_9)
                key=ASCII_KEY_0+(glKey-GLFW_KEY_0);
            else {
                uint8_t i;
                for (i=0;i<_web_keyMappingCount;i++) {
                    if (_web_keyMappings[i].hardware==glKey) {
                        key=_web_keyMappings[i].key;
                        break;
                    }
                }
            }
            if (key<ASCII_KEYCOUNT)
                _ascii.keyEventCallback(key,action,_ascii.keyEventCallbackContext);
	    }
	}
	void _ascii_runSys () {
	    if (_ascii.keyEventCallback) {
            glfwSetKeyCallback(_ascii_glfwKeyHandler);
	    }
	}
	void _ascii_quitSys () {
	    const char* message="User closed ASCII web application";
	    const uint8_t len=(uint8_t)strlen(message);
	    glfwSetKeyCallback(0);
	    free(_web.buffer);
	    _web.buffer=0;
	    _web.bufferLen=0;
	    js_ascii_setConsoleSize(len,1);
	    js_ascii_changeConsoleText (message);
	}
	void asciiSignalQuit () {
	    _ascii_quit ();
	}
	const char* _ascii_getWebColor (int8_t c) {
	    static const char* webColors[ASCII_COLOR_COUNT]={
	        "000000","CD0000","00CD00","CDCD00","0000EE","CD00CD","00CDCD","E5E5E5"
        };
        if (c<0||c>=ASCII_COLOR_COUNT)
            c=0;
        return webColors[c];
	}
	void _ascii_changeStdColorsSys(int8_t backColor,int8_t foreColor) {
	    js_ascii_changeConsoleColors(_ascii_getWebColor(backColor),_ascii_getWebColor(foreColor));
	}
	int8_t _ascii_writeSized_webBuffer (const char* str,uint32_t len) {
	    uint32_t newLen;char* newBuffer;
	    while (_web.bufferPtr+len>_web.bufferLen) {
	        newLen=_web.bufferLen+_web_bufferChunk;
	        newBuffer=realloc(_web.buffer,newLen+1);
	        if (!newBuffer)
                return 0;
            _web.buffer=newBuffer;
            _web.bufferLen=newLen;
	    }
	    if (len>1)
            memcpy(_web.buffer+_web.bufferPtr,str,len);
        else
            _web.buffer[_web.bufferPtr]=*str;
	    _web.bufferPtr+=len;
	    return 1;
	}
	int8_t _ascii_write_webBuffer (const char* str) {
	    return _ascii_writeSized_webBuffer(str,strlen(str));
	}
	int8_t _ascii_writeChar_webBuffer (char c) {
	    return _ascii_writeSized_webBuffer(&c,1);
	}
	int8_t _ascii_flipSys () {
	    //TODO: Optimize: No (fore)color changing if spaces aren't displayed...
	    uint8_t x,y,x2;
	    char c;
	    int8_t bc,fc,curBc=_web.backColor,curFc=_web.foreColor,inSpan=0;
	    _web.bufferPtr=0;
	    asciiChar* sourcePtr,* srcLinePtr;
	    for (y=0;y<_ascii.consoleSize.y;y++) {
	        srcLinePtr=_ascii.screenBuffer+((uint32_t)y)*_ascii.consoleSize.x;
	        sourcePtr=srcLinePtr;
	        for (x=0;x<_ascii.consoleSize.x;x++) {
	            c=sourcePtr->character;
	            bc=sourcePtr->backColor;
	            fc=sourcePtr->foreColor;
	            if (!isprint(sourcePtr->character))
	                c=' ';
	            if (bc!=curBc||fc!=curFc) {
	                if (inSpan==1&&_ascii_write_webBuffer("</span>")==0) return 0;
	                if ((_web.backColor==bc&&_web.foreColor==fc)||
                        (_web.backColor==bc&&c==' ')){
	                    inSpan=0;
                        curBc=_web.backColor;
                        curFc=_web.foreColor;
	                }
	                else {
                        if (_ascii_write_webBuffer("<span style=\"color:#")==0||
                            _ascii_write_webBuffer(_ascii_getWebColor(fc))==0||
                            _ascii_write_webBuffer("; background-color:#")==0||
                            _ascii_write_webBuffer(_ascii_getWebColor(bc))==0||
                            _ascii_write_webBuffer(";\">")==0)
                            return 0;
                        curBc=bc;
                        curFc=fc;
                        inSpan=1;
                    }
	            }
	            if (c==' ')
                    _ascii_write_webBuffer("&nbsp;");
	            else
                    _ascii_writeChar_webBuffer(c);
	            sourcePtr++;
	        }
	        if (_ascii_write_webBuffer("<br>")==0) return 0;
	    }
	    if (inSpan==1&&_ascii_write_webBuffer("</span>")==0) return 0;
	    _web.buffer[_web.bufferPtr]=0;
	    js_ascii_changeConsoleText(_web.buffer);
	    return 1;
	}
#elif USE_BACKEND == USE_BACKEND_WIN_CONSOLE
	#include <Windows.h>
	struct {
		int8_t isRunning;
		HANDLE hConsole;
		CHAR_INFO* consoleScreenBuffer;
		COORD dwBufferCoord,dwBufferSize;
		SMALL_RECT dwConsoleRect;
		BYTE keyboardState[ASCII_KEYCOUNT];
	} _win;
	void asciiSignalQuit () {
		_win.isRunning=0;
	}
	BOOL WINAPI _ascii_console_handler (DWORD ev) {
		_ascii_quit ();
		return FALSE;
	}
	int8_t _ascii_initSys (uint8_t w,uint8_t h) {
		uint16_t i;
		CONSOLE_CURSOR_INFO cursor;
		for (i=0;i<ASCII_KEYCOUNT;i++)
			_win.keyboardState[i]=0;
		_win.consoleScreenBuffer=(CHAR_INFO*)malloc(w*h*sizeof(CHAR_INFO));
		if (!_win.consoleScreenBuffer)
			return 0;
		_win.hConsole=GetStdHandle(STD_OUTPUT_HANDLE);
		if (!_win.hConsole)
			return 0;
		_win.dwBufferCoord.X = 0;
		_win.dwBufferCoord.Y = 0;
		_win.dwBufferSize.X = w;
		_win.dwBufferSize.Y = h;
		_win.dwConsoleRect.Top = 0;
		_win.dwConsoleRect.Left = 0;
		_win.dwConsoleRect.Bottom = h - 1;
		_win.dwConsoleRect.Right = w - 1;
		cursor.bVisible=0;
		cursor.dwSize=1;
		if (SetConsoleScreenBufferSize(_win.hConsole, _win.dwBufferSize)==0||	// Set Buffer Size
			SetConsoleWindowInfo(_win.hConsole, TRUE, &_win.dwConsoleRect)==0||	// Set Window Size
			SetConsoleCursorInfo(_win.hConsole,&cursor)==0||					// Disable console cursor
			SetConsoleCtrlHandler(_ascii_console_handler,TRUE)==0)				// Handle closing
			return 0;
		return 1;
	}
	void _ascii_quitSys () {
		if (_win.consoleScreenBuffer) {
			free(_win.consoleScreenBuffer);
			_win.consoleScreenBuffer=0;
		}
	}
	const int8_t _win_colorMappings [ASCII_COLOR_COUNT]={0x00,0x0c,0x0a,0x0e,0x09,0x05,0x0b,0x07};
	int8_t _ascii_flipSys () {
		asciiChar* sourcePtr=_ascii.screenBuffer;
		CHAR_INFO* targetPtr=_win.consoleScreenBuffer;
		WORD attributes=0xffff;
		int8_t backColor,foreColor;
		uint8_t x,y;
		for (y=0;y<_ascii.consoleSize.y;y++) {
			for (x=0;x<_ascii.consoleSize.x;x++) {
				if (attributes==0xffff||backColor!=sourcePtr->backColor||foreColor!=sourcePtr->foreColor) {
					backColor=_win_colorMappings[sourcePtr->backColor];
					foreColor=_win_colorMappings[sourcePtr->foreColor];
					attributes=(WORD)((backColor<<4)|foreColor);
				}
				targetPtr->Char.AsciiChar=sourcePtr->character;
				targetPtr->Attributes=attributes;
				sourcePtr++;
				targetPtr++;
			}
		}
		return (int8_t)WriteConsoleOutputA(_win.hConsole,_win.consoleScreenBuffer,_win.dwBufferSize,_win.dwBufferCoord,&_win.dwConsoleRect);
	}
	#define _win_keyMappingCount 11 //a constant doesn't work in msvc
	const _ascii_keyMap _win_keyMappings [_win_keyMappingCount]={
		{VK_BACK,ASCII_KEY_BACKSPACE},{VK_TAB,ASCII_KEY_TAB},{VK_RETURN,ASCII_KEY_RETURN},{VK_ESCAPE,ASCII_KEY_ESCAPE},
		{VK_SPACE,ASCII_KEY_SPACE},{VK_UP,ASCII_KEY_UP},{VK_DOWN,ASCII_KEY_DOWN},{VK_RIGHT,ASCII_KEY_RIGHT},
		{VK_LEFT,ASCII_KEY_LEFT},{VK_SHIFT,ASCII_KEY_SHIFT},{VK_CONTROL,ASCII_KEY_CTRL}};
	void _ascii_runSys () {
		uint8_t i;
		uint8_t key;
		BYTE state;
		_win.isRunning=1;
		while (_win.isRunning) {
			if (_ascii.keyEventCallback) {
				for (i='0';i<='9';i++) {
					state=GetKeyState(i)&0x80;
					key=ASCII_KEY_0+(i-'0');
					if (state>0&&_win.keyboardState[key]==0)
						_ascii.keyEventCallback(key,ASCII_KEYPRESSED,_ascii.keyEventCallbackContext);
					else if (state==0&&_win.keyboardState[key]>0)
						_ascii.keyEventCallback(key,ASCII_KEYRELEASED,_ascii.keyEventCallbackContext);
					_win.keyboardState[key]=state;
				}
				for (i='A';i<='Z';i++) {
					state=GetKeyState(i)&0x80;
					key=ASCII_KEY_A+(i-'A');
					if (state>0&&_win.keyboardState[key]==0)
						_ascii.keyEventCallback(key,ASCII_KEYPRESSED,_ascii.keyEventCallbackContext);
					else if (state==0&&_win.keyboardState[key]>0)
						_ascii.keyEventCallback(key,ASCII_KEYRELEASED,_ascii.keyEventCallbackContext);
					_win.keyboardState[key]=state;
				}
				for (i=0;i<_win_keyMappingCount;i++) {
					state=GetKeyState(_win_keyMappings[i].hardware)&0x80;
					key=_win_keyMappings[i].key;
					if (state>0&&_win.keyboardState[key]==0)
						_ascii.keyEventCallback(key,ASCII_KEYPRESSED,_ascii.keyEventCallbackContext);
					else if (state==0&&_win.keyboardState[key]>0)
						_ascii.keyEventCallback(key,ASCII_KEYRELEASED,_ascii.keyEventCallbackContext);
					_win.keyboardState[key]=state;
				}
			}
			Sleep(30); //equals about 30 frames per second
		}
		_ascii_quit  ();
	}
	void _ascii_changeStdColorsSys (int8_t backColor,int8_t foreColor) {
	}
#elif USE_BACKEND == USE_BACKEND_UNIX_CONSOLE
	void asciiSignalQuit ();
	int8_t _ascii_initSys (uint8_t w,uint8_t h);
	void _ascii_quitSys ();
	void _ascii_runSys ();
	int8_t _ascii_flipSys ();
	void _ascii_changeStdColorsSys (int8_t backColor,int8_t foreColor);
#elif USE_BACKEND == USE_BACKEND_SDL
	void asciiSignalQuit ();
	int8_t _ascii_initSys (uint8_t w,uint8_t h);
	void _ascii_quitSys ();
	void _ascii_runSys ();
	int8_t _ascii_flipSys ();
	void _ascii_changeStdColorsSys (int8_t backColor,int8_t foreColor);
#else
	#error [ASCIILIB]: No backend specified
#endif //BACKEND SWITCH
#ifdef _MSC_VER
	asciiPoint _ascii_make_asciiPoint (uint8_t x,uint8_t y) {
		asciiPoint p;
		p.x=x;
		p.y=y;
		return p;
	}
	asciiRect _ascii_make_asciiRect (uint8_t x,uint8_t y,uint8_t w,uint8_t h) {
		asciiRect r;
		r.offset.x=x;
		r.offset.y=y;
		r.size.x=w;
		r.size.y=h;
		return r;
	}
	asciiChar _ascii_make_asciiChar (int8_t ch,int8_t bc,int8_t fc) {
		asciiChar c;
		c.character=ch;
		c.backColor=bc;
		c.foreColor=fc;
		c.reserved=0;
		return c;
	}
#endif

/*
	Format stuff
*/
asciiBitmap _ascii_emptyBitmap={{{0,0},{0,0}},0,0};
asciiColoredBitmap _ascii_emptyColoredBitmap={{{0,0},{0,0}},0,0};
int8_t _ascii_filePtrInStream (void* context) {
	FILE* f=(FILE*) context;
	int8_t ret;
	if (feof(f))
		return 0;
	ret=(int8_t)fgetc(f);
	if (feof(f))
		return 0;
	return ret;
}
int8_t _ascii_stream_skip_to_eol (asciiTextInStreamCallback stream,void* context) {
	int8_t eos=0;
	int8_t c;
	do {
		c=stream(context);
		if (c==0)
			eos=1;
	} while (c!=0&&c!='\n');
	return eos;
}
int16_t _ascii_stream_read_number (asciiTextInStreamCallback stream,void* context) { //actually reads only 8bit unsigned
	int8_t c;
	int16_t ret=0;
	uint8_t i=0;
	c=stream(context);
	while (c>=0&&!isdigit(c)) {
		if (_ascii_stream_skip_to_eol(stream,context))
			return -1;
		c=stream(context);
	}
	if (c<0)
		return -1;
	ret=c-'0';
	for (i=0;i<2;i++) {
		c=stream(context);
		if (c<0)
			return -1;
		else if (isdigit(c))
			ret=ret*10+(c-'0');
		else {
			if (c!='\n'&&_ascii_stream_skip_to_eol(stream,context))
				return -1;
			break;
		}
	}
	if (i==2)
		_ascii_stream_skip_to_eol(stream,context);
	return (ret>0xff?-1:ret);
}

asciiBitmap asciiLoadBitmapFromFile (asciiString fn) {
	asciiBitmap ret=_ascii_emptyBitmap;
#ifdef _MSC_VER
	FILE* fp=0;
	fopen_s(&fp,fn,"r");
#else
	FILE* fp=fopen((const char*)fn,"r");
#endif
	if (fp) {
		ret=asciiLoadBitmapFromFilePtr(fp);
		fclose(fp);
	}
	return ret;
}
asciiBitmap asciiLoadBitmapFromFilePtr (void* fp) {
	if (!fp) {
		asciiBitmap bitmap=_ascii_emptyBitmap;
		return bitmap;
	}
	return asciiLoadBitmapFromStream(_ascii_filePtrInStream,fp);
}
asciiBitmap asciiLoadBitmapFromStream (asciiTextInStreamCallback stream,void* context) {
	asciiBitmap bitmap=_ascii_emptyBitmap;
	int16_t i,lineEnd;
	uint8_t x,y;
	int8_t c;
	int8_t* ptr;
	if (stream==0||context==0)
		return bitmap;
	//read first number (width)
	i=_ascii_stream_read_number (stream,context);
	if (i<0)
		return bitmap;
	bitmap.bounds.size.x=(uint8_t)i;
	//read second number (height)
	i=_ascii_stream_read_number (stream,context);
	if (i<0)
		return bitmap;
	bitmap.bounds.size.y=(uint8_t)i;
	//read character data
	bitmap.address=(int8_t*)malloc(bitmap.bounds.size.x*bitmap.bounds.size.y);
	if (bitmap.address) {
		memset(bitmap.address,' ',bitmap.bounds.size.x*bitmap.bounds.size.y);
		for (y=0;y<bitmap.bounds.size.y;y++) {
			lineEnd=0;
			ptr=bitmap.address+y*bitmap.bounds.size.x;
			for (x=0;x<bitmap.bounds.size.x;x++) {
				c=stream(context);
				if (c=='\n') {
					lineEnd=1;
					break;
				}
				else if (c<0||!isprint(c)) {
					free(bitmap.address);
					bitmap.address=0;
					return bitmap;
				}
				else
					*ptr=c;
				ptr++;
			}
			if (lineEnd==0)
				_ascii_stream_skip_to_eol (stream,context);
		}
	}
	return bitmap;
}

asciiColoredBitmap asciiLoadColoredBitmapFromFile (asciiString fn) {
	asciiColoredBitmap ret=_ascii_emptyColoredBitmap;
#ifdef _MSC_VER
	FILE* fp=0;
	fopen_s(&fp,fn,"r");
#else
	FILE* fp=fopen((const char*)fn,"r");
#endif
	if (fp) {
		ret=asciiLoadColoredBitmapFromFilePtr(fp);
		fclose(fp);
	}
	return ret;
}
asciiColoredBitmap asciiLoadColoredBitmapFromFilePtr (void* fp) {
	if (!fp) {
		asciiColoredBitmap bitmap=_ascii_emptyColoredBitmap;
		return bitmap;
	}
	return asciiLoadColoredBitmapFromStream(_ascii_filePtrInStream,fp);
}
asciiColoredBitmap asciiLoadColoredBitmapFromStream (asciiTextInStreamCallback stream,void* context) {
	asciiColoredBitmap bitmap=_ascii_emptyColoredBitmap;
	int16_t i,lineEnd;
	uint8_t x,y,colorCount;
	int8_t c;
	asciiChar* ptr;
	struct {
		int8_t backColor;
		int8_t foreColor;
	} colorTable[36]; //0-9 && a-z
	if (stream==0||context==0)
		return bitmap;
	//read first number (width)
	i=_ascii_stream_read_number (stream,context);
	if (i<=0)
		return bitmap;
	bitmap.bounds.size.x=(uint8_t)i;
	//read second number (height)
	i=_ascii_stream_read_number (stream,context);
	if (i<=0)
		return bitmap;
	bitmap.bounds.size.y=(uint8_t)i;
	//read third number (color count)
	i=_ascii_stream_read_number (stream,context);
	if (i<=0||i>36)
		return bitmap;
	colorCount=(uint8_t)i;
	//read color table
	for (i=0;i<colorCount;i++) {
		c=stream(context);
		if (c<'0'||c>'9')
			return bitmap;
		colorTable[i].backColor=c-'0';
		c=stream(context);
		if (c<'0'||c>'9')
			return bitmap;
		colorTable[i].foreColor=c-'0';
		if (_ascii_stream_skip_to_eol(stream,context))
			return bitmap;
	}
	bitmap.address=(asciiChar*)malloc(bitmap.bounds.size.x*bitmap.bounds.size.y*sizeof(asciiChar));
	if (bitmap.address) {
		//read character data
		memset(bitmap.address,' ',bitmap.bounds.size.x*bitmap.bounds.size.y*sizeof(asciiChar));
		for (y=0;y<bitmap.bounds.size.y;y++) {
			lineEnd=0;
			ptr=bitmap.address+y*bitmap.bounds.size.x;
			for (x=0;x<bitmap.bounds.size.x;x++) {
				c=stream(context);
				if (c=='\n') {
					lineEnd=1;
					break;
				}
				else if (c<0||!isprint(c)) {
					free(bitmap.address);
					bitmap.address=0;
					return bitmap;
				}
				else {
					ptr->character=c;
					ptr->backColor=0;
					ptr->foreColor=0;
				}
				ptr++;
			}
			for (;x<bitmap.bounds.size.x;x++) {
				ptr->backColor=0;
				ptr->foreColor=0;
				ptr++;
			}
			if (lineEnd==0)
				_ascii_stream_skip_to_eol (stream,context);
		}
		//read color data
		for (y=0;y<bitmap.bounds.size.y;y++) {
			lineEnd=0;
			ptr=bitmap.address+y*bitmap.bounds.size.x;
			for (x=0;x<bitmap.bounds.size.x;x++) {
				c=stream(context);
				if (c=='\n') {
					lineEnd=1;
					break;
				}
				else if (c<0||(!isdigit(c)&&!isalpha(c))) {
					free(bitmap.address);
					bitmap.address=0;
					return bitmap;
				}
				else {
					if (c<='9')
						c-='0';
					else
						c=10+c-(islower(c)?'a':'A');
					if (c>=colorCount) {
						free(bitmap.address);
						bitmap.address=0;
						return bitmap;
					}
					ptr->backColor=colorTable[c].backColor;
					ptr->foreColor=colorTable[c].foreColor;
				}
				ptr++;
			}
			if (lineEnd==0)
				_ascii_stream_skip_to_eol (stream,context);
		}
	}
	return bitmap;
}
