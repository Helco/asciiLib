#include "asciiLibIntern.h"
#ifdef ASCII_ENABLE_BACKEND_EMSCRIPTEN
#include <GL/glfw.h>
#include <emscripten/emscripten.h>

extern void js_ascii_setConsoleSize (int32_t w,int32_t h);
extern void js_ascii_toggleEvents (int32_t toggle);
extern void js_ascii_changeConsoleText (const char* text,uint32_t len);
extern void js_ascii_changeConsoleColors (const char* backColor,const char* foreColor);
extern void js_ascii_setTimeout (int32_t ms,int32_t id);
extern void js_ascii_onMouseMoveEvent (int32_t dummy); //these functions has to be called by C to ensure the existence
extern void js_ascii_onMouseKeyEvent (int32_t dummy); //as they can be removed by the optimizer (TODO: Replace with inline?)

struct {
	asciiEngine* engine;
	char* buffer;
	uint32_t bufferLen;
	uint32_t bufferPtr;
	int8_t backColor,foreColor;
} asciiWeb = {0}; //badstyle because of glfwKeyHandler
static const uint32_t _web_bufferChunk=4096;
void _ascii_glfwKeyHandler (int glKey,int glAction);
asciiBool _ascii_initWeb (asciiEngine* e,int32_t w,int32_t h) {
	js_ascii_onMouseMoveEvent(0);
	js_ascii_onMouseKeyEvent(0);

	asciiWeb.engine = e;
	asciiWeb.bufferLen=((uint32_t)w)*h;
	asciiWeb.bufferPtr=0;
	asciiWeb.backColor=e->clearChar.backColor;
	asciiWeb.foreColor=e->clearChar.foreColor;
	asciiWeb.buffer=(char*)malloc(asciiWeb.bufferLen+1);
	if (!asciiWeb.buffer)
		return ASCII_FAILED;
	js_ascii_setConsoleSize(w,h);
	glfwInit ();//set up the keyboard event
	glfwSetKeyCallback(_ascii_glfwKeyHandler);
	js_ascii_toggleEvents (1);
	return ASCII_SUCESS;
}
#define _web_keyMappingCount 18
const asciiKeyMap _web_keyMappings [_web_keyMappingCount]={
	{GLFW_KEY_BACKSPACE,ASCII_KEY_BACKSPACE},{GLFW_KEY_TAB,ASCII_KEY_TAB},{GLFW_KEY_ENTER,ASCII_KEY_RETURN},{GLFW_KEY_ESC,ASCII_KEY_ESCAPE},
	{GLFW_KEY_SPACE,ASCII_KEY_SPACE},{GLFW_KEY_UP,ASCII_KEY_UP},{GLFW_KEY_DOWN,ASCII_KEY_DOWN},{GLFW_KEY_RIGHT,ASCII_KEY_RIGHT},
	{GLFW_KEY_LEFT,ASCII_KEY_LEFT},{GLFW_KEY_LSHIFT,ASCII_KEY_SHIFT},{GLFW_KEY_RSHIFT,ASCII_KEY_SHIFT},
	{GLFW_KEY_LCTRL,ASCII_KEY_CTRL},{GLFW_KEY_RCTRL,ASCII_KEY_CTRL},
	{GLFW_KEY_LALT,ASCII_KEY_ALT},{GLFW_KEY_RALT,ASCII_KEY_CTRL},
	{255,ASCII_KEY_ESCAPE},{13,ASCII_KEY_RETURN},{'\b',ASCII_KEY_BACKSPACE}}; //the GLFW_KEY_* doesn't work for ESCAPE, RETURN and BACKSPACE
#define GLFW_KEY_0 ((int)'0')
#define GLFW_KEY_9 ((int)'9')
#define GLFW_KEY_A ((int)'A')
#define GLFW_KEY_Z ((int)'Z')
void _ascii_glfwKeyHandler (int glKey,int glAction) {
	if (asciiWeb.engine && (glAction==GLFW_PRESS || glAction==GLFW_RELEASE)) {
		uint8_t key = ASCII_KEYCOUNT,
			action = (glAction==GLFW_PRESS);
		if (glKey>=GLFW_KEY_A && glKey<=GLFW_KEY_Z)
			key = ASCII_KEY_A + (glKey-GLFW_KEY_A);
		else if (glKey>=GLFW_KEY_0 && glKey<=GLFW_KEY_9)
			key = ASCII_KEY_0 + (glKey-GLFW_KEY_0);
		else if (glKey>=GLFW_KEY_F1 && glKey<=GLFW_KEY_F12)
			key = ASCII_KEY_F1 + (glKey-GLFW_KEY_F1);
		else {
			uint8_t i;
			for (i=0;i<_web_keyMappingCount;i++) {
				if (_web_keyMappings[i].hardware==glKey) {
					key = _web_keyMappings[i].key;
					break;
				}
			}
		}
		if (key<ASCII_KEYCOUNT) {
			if (action)
				asciiOnKeyDown(asciiWeb.engine,key);
			else
				asciiOnKeyUp(asciiWeb.engine,key);
		}
	}
}
void _ascii_runWeb (asciiEngine* engine) {
}
void _ascii_quitWeb (asciiEngine* engine) {
	const char* message="User closed ASCII web application";
	const uint8_t len=(uint8_t)strlen(message);
	glfwSetKeyCallback(0);
	js_ascii_toggleEvents(0);
	free(asciiWeb.buffer);
	asciiWeb.buffer=0;
	asciiWeb.bufferLen=0;
	js_ascii_setConsoleSize(len,1);
	js_ascii_changeConsoleText (message,len);
}
void _ascii_signalQuitWeb (asciiEngine* e) {
	asciiQuit (e);
}
asciiBool _ascii_setTimeoutWeb (asciiEngine* e,asciiTimerID id) {
	js_ascii_setTimeout(e->timers[id].timeout,id);
	return ASCII_SUCESS;
}
int32_t _onjs_fireTimeout (int32_t id) { //ATTENTION ATTENTION ATTENTION: Make sure these functions are not removed by emscripten optimizer
	if (asciiWeb.engine->timers[id].callback!=0) {
		asciiWeb.engine->timers[id].callback(asciiWeb.engine->timers[id].context);
		asciiWeb.engine->timers[id].callback=0;
	}
	return 0;
}
int32_t _onjs_fireMouseMove (int32_t X,int32_t Y) {
	asciiOnMouseMove(asciiWeb.engine,asciiPoint(X,Y));
	return 0;
}
int32_t _onjs_fireMouseKey (int32_t buttonPressed,int32_t isDown) {
	if (isDown)
		asciiOnMouseDown(asciiWeb.engine,buttonPressed);
	else
		asciiOnMouseUp(asciiWeb.engine,buttonPressed);
	return 0;
}
const char* _ascii_getWebColorString (int8_t c) {
	static const char* webColors[ASCII_COLOR_COUNT]={
		"000000","CD0000","00CD00","CDCD00","0000EE","CD00CD","00CDCD","E5E5E5"
	};
	if (c<0||c>=ASCII_COLOR_COUNT)
		c=0;
	return webColors[c];
}
#define _ascii_getWebColorClass(c) (((c<0 || c>=ASCII_COLOR_COUNT) ? 0 : c) +'0')
/*
 * Of course this big function is a huge bad-style, but inline is not available in emscripten
 * and with this forced inline it reduces the average runtime of asciiFlip in my test profiles
 * from 52.854ms to 7.489ms !
 */
#define _ascii_writeSized_webBuffer(str,len) do { \
	uint32_t newLen;char* newBuffer; \
	if (asciiWeb.bufferPtr+len > asciiWeb.bufferLen) { \
		newLen = asciiWeb.bufferLen+_web_bufferChunk; \
		while (newLen < asciiWeb.bufferPtr+len) newLen += _web_bufferChunk; \
		newBuffer = (char*)realloc(asciiWeb.buffer,newLen+1); \
		if (!newBuffer) \
			return 0; \
		asciiWeb.buffer = newBuffer; \
		asciiWeb.bufferLen = newLen; \
	} \
	if (len>1) \
		memcpy(asciiWeb.buffer+asciiWeb.bufferPtr,str,len); \
	else \
		asciiWeb.buffer[asciiWeb.bufferPtr] = *str;\
		asciiWeb.bufferPtr += len; \
	} while (0)
#define _ascii_write_webBuffer(str) _ascii_writeSized_webBuffer(str,strlen(str)) //deprecated
#define _ascii_writeChar_webBuffer(c) _ascii_writeSized_webBuffer(&c,1)
#define isprint(c) (c>=0x20 && c<=0x7e)
#define SPAN_STR "<span class=\"cbx cfx\";>x"
#define SPAN_STR_LEN 24
#define SPAN_STR_BACKCOLOR_OFF 15
#define SPAN_STR_FORECOLOR_OFF 19
#define SPAN_STR_CHARACTER_OFF 23
int8_t _ascii_flipWeb (asciiEngine* e) {
	int32_t x,y;
	char c;
	int8_t bc,fc,curBc = asciiWeb.backColor,
		curFc = asciiWeb.foreColor,
		inSpan = 0;
	asciiPoint consoleSize = e->screen.bounds.size;
	asciiChar* sourcePtr = e->screen.address;
	char spanStr[] = SPAN_STR;
	asciiWeb.bufferPtr = 0;
	js_ascii_changeConsoleColors(_ascii_getWebColorString(e->clearChar.backColor),_ascii_getWebColorString(e->clearChar.foreColor));
	_ascii_writeSized_webBuffer("<pre>",5);
	for (y=0;y<consoleSize.y;y++) {
		for (x=0;x<consoleSize.x;x++) {
			c = sourcePtr->character;
			bc = sourcePtr->backColor;
			fc = sourcePtr->foreColor;
			if (!isprint(sourcePtr->character))
				c = ' ';
			if (bc!=curBc || fc!=curFc) {
				if (inSpan==1)
					_ascii_writeSized_webBuffer("</span>",7);
				if (asciiWeb.backColor==bc && (asciiWeb.foreColor==fc || c==' ')){
					inSpan = 0;
					curBc = asciiWeb.backColor;
					curFc = asciiWeb.foreColor;
					_ascii_writeChar_webBuffer(c);
				}
				else {
					spanStr[SPAN_STR_BACKCOLOR_OFF] = _ascii_getWebColorClass(bc);
					spanStr[SPAN_STR_FORECOLOR_OFF] = _ascii_getWebColorClass(fc);
					spanStr[SPAN_STR_CHARACTER_OFF] = c;
					_ascii_writeSized_webBuffer(spanStr,SPAN_STR_LEN);
					curBc = bc;
					curFc = fc;
					inSpan = 1;
				}
			}
			else
				_ascii_writeChar_webBuffer(c);
			sourcePtr++;
		}
		_ascii_writeSized_webBuffer("<br>",4);
	}
	if (inSpan==1)
		_ascii_writeSized_webBuffer("</span>",7);
	_ascii_writeSized_webBuffer("</pre>",6);
	asciiWeb.buffer[asciiWeb.bufferPtr] = 0;
#ifdef _DEBUG
	printf("web buffer size: %u\n",asciiWeb.bufferPtr);
#endif
	js_ascii_changeConsoleText(asciiWeb.buffer,asciiWeb.bufferPtr);
	return 1;
}
void _ascii_eventChangedWeb (asciiEngine* e,asciiEvent ev) {
}
const asciiGBackend asciiGraphicBackendEmscripten = {
	_ascii_initWeb,
	_ascii_runWeb,
	_ascii_quitWeb,
	_ascii_flipWeb,
	_ascii_setTimeoutWeb,
	_ascii_signalQuitWeb,
	_ascii_eventChangedWeb
};
#endif