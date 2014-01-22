#include "asciiLib.h"
#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/*
	COMPILE-SWITCH CONFIGURATION
You can set the backend manually by setting the USE_BACKEND macro under
this commentar to one of the following specific backend macros
(e.g. #define USE_BACKEND USE_BACKEND_SDL)
*/

#define USE_BACKEND_EMSCRIPTEN 0
#define USE_BACKEND_WIN_CONSOLE 1
#define USE_BACKEND_SDL 2 //not implemented

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
		#define USE_BACKEND USE_BACKEND_SDL
	#else
		#error [ASCIILIB]: Couldnt detect platform, please set backend manually to a specific backend (e.g. USE_BACKEND_SDL)
	#endif
#endif

/*
	BACKEND INTERFACE
*/
typedef enum _ascii_Event{
    ASCII_EVENT_KEY,
    ASCII_EVENT_MOUSEMOVE,
    ASCII_EVENT_MOUSEKEY,
    ASCII_EVENT_QUIT
} _ascii_Event;
asciiResult _ascii_initSys (int32_t w,int32_t h);
void _ascii_runSys ();
void _ascii_quitSys ();
asciiResult _ascii_flipSys ();
asciiResult _ascii_setTimeoutSys (asciiTimerID id);
void _ascii_changedEvent(_ascii_Event ev);
//void _asciiChange
//void asciiSignalQuit ();

/*
	HELPER STRUCTURES/DATA/MACROS
*/
#define ASCII_IS_OPT(bit) ((_ascii.optBitmask&bit)>0)
#define ASCII_IS_CHARACTER_LOCK ASCII_IS_OPT(ASCII_CHARACTER_LOCK)
#define ASCII_IS_BACKCOLOR_LOCK ASCII_IS_OPT(ASCII_BACKCOLOR_LOCK)
#define ASCII_IS_FORECOLOR_LOCK ASCII_IS_OPT(ASCII_FORECOLOR_LOCK)
#define ASCII_IS_TARGET_BITMAP ASCII_IS_OPT(ASCII_TARGET_BITMAP)
#define ASCII_TARGET (ASCII_IS_TARGET_BITMAP?_ascii.targetBitmap:&_ascii.screen)
const int8_t _ascii_keyAsciiTable[ASCII_KEYCOUNT]={
	'\b','\t','\n',0,' ','0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F','G',
	'H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z',0,0,0,0,0,0};
typedef struct {
	int32_t hardware;
	uint8_t key;
} _ascii_keyMap;
typedef struct {
	uint32_t timeout;
	asciiTimeoutCallback callback;
	void* context;
} _ascii_timerData;

/*
	LIBRARY RUNTIME DATA
*/
struct {
	int8_t isInited;
	//graphic
	asciiColoredBitmap screen;
	asciiColoredBitmap* targetBitmap;
	asciiChar clearChar;
	uint32_t optBitmask;
	//callbacks
	asciiKeyEventCallback keyEventCallback;
	void* keyEventCallbackContext;
	asciiMouseEventCallback mouseKeyEventCallback;
	void* mouseKeyEventCallbackContext;
	asciiMouseEventCallback mouseMoveEventCallback;
	void* mouseMoveEventCallbackContext;
	asciiQuitCallback quitCallback;
	void* quitCallbackContext;
	//timers
	_ascii_timerData timers[ASCII_MAX_TIMER];
} _ascii={
	0, //isInited
	{0},0,{' ',ASCII_COLOR_BLACK,ASCII_COLOR_WHITE},0, //screen/target/clearChar/optBitmask
	0,0, 0,0, 0,0, 0,0, //callbacks
	{0} //timers
};

//this helper functions needs _ascii
#if defined WIN32
#define inline __inline
#elif defined EMSCRIPTEN
#define inline
void* realloc_emscripten (void* old,unsigned int oldSize,unsigned int newSize) {
    void* newBlock=malloc(newSize);
    if (newBlock==0)
      return 0;
    if (old!=0) {
        memcpy(newBlock,old,oldSize);
        free(old);
    }
    return newBlock;
}
#endif
inline void _ascii_copyChar (asciiChar* target,asciiChar source) {
	if (!ASCII_IS_CHARACTER_LOCK)
		target->character=source.character;
	if (!ASCII_IS_BACKCOLOR_LOCK)
		target->backColor=source.backColor;
	if (!ASCII_IS_FORECOLOR_LOCK)
		target->foreColor=source.foreColor;
}

/*
	SYSTEM FUNCTIONS
*/
asciiResult asciiInit (int32_t w,int32_t h) {
	if (_ascii.isInited==0&&w>0&&h>0) {
		_ascii.screen=asciiCreateColoredBitmap (asciiPoint(w,h));
		if (!_ascii.screen.address)
			return ASCII_FAILED;
		if (_ascii_initSys(w,h)==0) {
			asciiFreeColoredBitmap (&_ascii.screen);
			return ASCII_FAILED;
		}
		_ascii.isInited=1;
		return ASCII_SUCESS;
	}
	return ASCII_FAILED;
}
void asciiRun () {
	if (_ascii.isInited==1) {
		_ascii_runSys ();
	}
}
//asciiSignalQuit is completly implemented by the interface, which then calls _ascii_quit ()
void _ascii_quit () {
	if (_ascii.isInited==1) {
		if (_ascii.quitCallback)
			_ascii.quitCallback(_ascii.quitCallbackContext);
		_ascii_quitSys ();
		asciiFreeColoredBitmap(&_ascii.screen);
		_ascii.isInited=0;
	}
}
void asciiSetKeyEventCallback (asciiKeyEventCallback callback,void* context) {
	_ascii.keyEventCallback=callback;
	_ascii.keyEventCallbackContext=context;
	_ascii_changedEvent(ASCII_EVENT_KEY);
}
void asciiSetMouseKeyEventCallback (asciiMouseEventCallback callback,void* context) {
	_ascii.mouseKeyEventCallback=callback;
	_ascii.mouseKeyEventCallbackContext=context;
	_ascii_changedEvent(ASCII_EVENT_MOUSEKEY);
}
void asciiSetMouseMoveEventCallback (asciiMouseEventCallback callback,void* context) {
	_ascii.mouseMoveEventCallback=callback;
	_ascii.mouseMoveEventCallbackContext=context;
	_ascii_changedEvent(ASCII_EVENT_MOUSEMOVE);
}
void asciiSetQuitCallback (asciiQuitCallback callback,void* context) {
	_ascii.quitCallback=callback;
	_ascii.quitCallbackContext=context;
	_ascii_changedEvent(ASCII_EVENT_QUIT);
}

/*
	GRAPHIC MANAGEMENT FUNCTIONS
*/
void asciiEnable (uint32_t bit) {
	//not used bits can be ignored
	_ascii.optBitmask|=bit;
}
void asciiDisable (uint32_t bit) {
	int32_t i;
	for (i=0;i<ASCII_OPT_COUNT;i++) {
		if ((bit&(1<<i))>0)
			_ascii.optBitmask&=~(1<<i);
	}
}
void asciiSetClearChar (asciiChar ch) {
	_ascii.clearChar=ch;
}
void asciiSetTargetBitmap (asciiColoredBitmap* target) {
	_ascii.targetBitmap=target;
	if (target==0)
		asciiDisable(ASCII_TARGET_BITMAP);
}
asciiChar asciiGetClearChar () {
	return _ascii.clearChar;
}
asciiColoredBitmap* asciiGetTargetBitmap () {
	return _ascii.targetBitmap;
}
asciiPoint asciiGetTargetSize () {
	return ASCII_TARGET->bounds.size;
}

/*
	BITMAP FUNCTIONS
*/
void asciiSetBitmapTransparent (asciiBitmap* bitmap,asciiTextchar ch) {
	if (bitmap)
		bitmap->trans=ch;
}
void asciiSetColoredBitmapTransparent (asciiColoredBitmap* bitmap,asciiChar ch) {
	if (bitmap)
		bitmap->trans=ch;
}
asciiBitmap asciiCreateBitmap (asciiPoint size) {
	return asciiCreateFilledBitmapEx(size,_ascii.clearChar);
}
asciiBitmap asciiCreateBitmapEx (asciiPoint size,asciiColor backColor,asciiColor foreColor) {
	return asciiCreateFilledBitmapEx(size,asciiChar(_ascii.clearChar.character,backColor,foreColor));
}
asciiBitmap asciiCreateFilledBitmap (asciiPoint size,asciiTextchar fillChar) {
	return asciiCreateFilledBitmapEx(size,asciiChar(fillChar,_ascii.clearChar.backColor,_ascii.clearChar.foreColor));
}
asciiBitmap asciiCreateFilledBitmapEx (asciiPoint size,asciiChar ch) {
	asciiBitmap bitmap;
	uint32_t memlen=((uint32_t)size.x)*size.y*sizeof(asciiTextchar);
	bitmap.address=(asciiTextchar*)malloc(memlen);
	if (bitmap.address)
		memset(bitmap.address,0,memlen);
	bitmap.bounds=asciiRect(0,0,size.x,size.y);
	bitmap.trans=0;
	bitmap.ownMemory=1;
	bitmap.pitch=bitmap.bounds.size.x;
	return bitmap;
}
asciiColoredBitmap asciiCreateColoredBitmap (asciiPoint size) {
	return asciiCreateFilledColoredBitmap (size,_ascii.clearChar);
}
asciiColoredBitmap asciiCreateFilledColoredBitmap (asciiPoint size,asciiChar fillChar) {
	asciiColoredBitmap bitmap;
	uint32_t memlen=((uint32_t)size.x)*size.y*sizeof(asciiChar);
	bitmap.address=(asciiChar*)malloc(memlen);
	if (bitmap.address)
		memset(bitmap.address,0,memlen);
	bitmap.bounds=asciiRect(0,0,size.x,size.y);
	bitmap.trans=asciiChar(0,0,0);
	bitmap.ownMemory=1;
	bitmap.pitch=bitmap.bounds.size.x;
	return bitmap;
}
asciiBitmap asciiCreateSubBitmap (asciiBitmap source,asciiRect bounds) {
	asciiBitmap bitmap;
	bounds=asciiClipRect(bounds,source.bounds);
	if (source.address!=0&&bounds.size.x>0&&bounds.size.y>0) {
		bitmap.bounds=bounds;
		bitmap.address=source.address;
		bitmap.trans=source.trans;
		bitmap.ownMemory=0;
		bitmap.pitch=source.bounds.size.x;
	}
	else
		bitmap.address=0;
	return bitmap;
}
asciiColoredBitmap asciiCreateSubColoredBitmap (asciiColoredBitmap source,asciiRect bounds) {
	asciiColoredBitmap bitmap;
	bounds=asciiClipRect(bounds,source.bounds);
	if (source.address!=0&&bounds.size.x>0&&bounds.size.y>0) {
		bitmap.bounds=bounds;
		bitmap.address=source.address;
		bitmap.trans=source.trans;
		bitmap.ownMemory=0;
		bitmap.pitch=source.bounds.size.x;
	}
	else
		bitmap.address=0;
	return bitmap;
}
void asciiFreeBitmap (asciiBitmap* bm) {
	if (bm&&bm->address) {
		if (bm->ownMemory==1)
			free(bm->address);
		bm->address=0;
		bm->trans=0;
		bm->bounds=asciiRect(0,0,0,0);
	}
}
void asciiFreeColoredBitmap (asciiColoredBitmap* bm) {
	if (bm&&bm->address) {
		if (bm->ownMemory==1)
			free(bm->address);
		bm->address=0;
		bm->trans.character=0;
		bm->bounds=asciiRect(0,0,0,0);
	}
}

/*
	TIME FUNCTIONS
*/
asciiTimerID asciiSetTimeout (uint32_t ms,asciiTimeoutCallback callback,void* context) {
	asciiTimerID i;
	for (i=0;i<ASCII_MAX_TIMER;i++) {
		if (_ascii.timers[i].callback==0) {
			_ascii.timers[i].callback=callback;
			_ascii.timers[i].context=context;
			_ascii.timers[i].timeout=ms;
			if (!_ascii_setTimeoutSys(i)) {
				_ascii.timers[i].callback=0;
				i=ASCII_TIMER_INVALID;
			}
			return i;
		}
	}
	return ASCII_TIMER_INVALID;
}
void asciiCancelTimer (asciiTimerID id) {
	if (id>=0&&id<ASCII_MAX_TIMER)
		_ascii.timers[id].callback=0;
}
void asciiCancelAllTimer () {
	asciiTimerID i;
	for (i=0;i<ASCII_MAX_TIMER;i++)
		_ascii.timers[i].callback=0;
}

/*
	RENDER FUNCTIONS
*/
asciiResult asciiFlip () {
	if (_ascii.isInited==1) {
		return _ascii_flipSys ();
	}
	return ASCII_FAILED;
}
void asciiFillRect (asciiChar ch,asciiRect rect) {
	asciiColoredBitmap* target=ASCII_TARGET;
	asciiChar* ptr;
	int32_t x,y;
	if (_ascii.isInited==1&& //asciiLib is inited
		rect.offset.x<target->bounds.size.x&&rect.offset.y<target->bounds.size.y&& //the rect isn't out of bounds
		rect.size.x>0&&rect.size.y>0) { //the rect has an area>0

		//clip rect to target size
		rect=asciiClipRect (rect,asciiRect(0,0,target->bounds.size.x,target->bounds.size.y));
		//render
		for (y=0;y<rect.size.y;y++) {
			ptr=target->address+
				(target->bounds.offset.y+rect.offset.y+y)*target->pitch+
				target->bounds.offset.x+rect.offset.x;
			for (x=0;x<rect.size.x;x++)
				_ascii_copyChar(ptr++,ch);
		}
	}
}
void asciiClearRect (asciiRect rect) {
	asciiFillRect(_ascii.clearChar,rect);
}
void asciiDrawChar (asciiChar c,asciiPoint offset) {
	asciiColoredBitmap* target=ASCII_TARGET;
	asciiChar* targetPtr;
	if (_ascii.isInited==1&& //asciiLib is inited
		offset.x>=0&&offset.y>=0&& //offset is in target bounds
		offset.x<target->bounds.size.x&&offset.y<target->bounds.size.y) {
			targetPtr=target->address+
				(target->bounds.offset.y+offset.y)*target->bounds.size.x+
				target->bounds.offset.x+offset.x;
			_ascii_copyChar(targetPtr,c);
	}
}
void asciiDrawTextchar (asciiTextchar c,asciiPoint offset) {
	asciiDrawChar(asciiChar(c,_ascii.clearChar.backColor,_ascii.clearChar.foreColor),offset);
}
void asciiDrawText (asciiString text,asciiPoint offset) {
	asciiDrawSizedTextColored(text,strlen(text),offset,_ascii.clearChar.backColor,_ascii.clearChar.foreColor);
}
void asciiDrawTextColored (asciiString text,asciiPoint offset,asciiColor backColor,asciiColor foreColor) {
	asciiDrawSizedTextColored(text,strlen(text),offset,backColor,foreColor);
}
void asciiDrawSizedText (asciiString text,uint32_t len,asciiPoint offset) {
	asciiDrawSizedTextColored(text,len,offset,_ascii.clearChar.backColor,_ascii.clearChar.foreColor);
}
void asciiDrawSizedTextColored (asciiString text,uint32_t len,asciiPoint offset,asciiColor backColor,asciiColor foreColor) {
	asciiColoredBitmap* target=ASCII_TARGET;
	uint32_t i;
	asciiChar* targetPtr;
	asciiString sourcePtr=text;
	if (_ascii.isInited!=0&& //asciiLib is inited
		text!=0&&len!=0&& //text and len are valid
		offset.x>=0&&offset.y>=0&&offset.x<target->bounds.size.x&&offset.y<target->bounds.size.y) { //offset is in target bounds
			if (offset.x<0) {
				text+=-offset.x;
				len+=offset.x; //len gets decreased
			}
			targetPtr=target->address+
				(target->bounds.offset.y+offset.y)*target->pitch+
				target->bounds.offset.x+offset.x;
			for (i=0;i<len;i++) {
				_ascii_copyChar(targetPtr,asciiChar(*sourcePtr,backColor,foreColor));
				targetPtr++;
				sourcePtr++;
			}
	}
}
void asciiDrawBitmap (asciiBitmap bitmap,asciiRect rect) {
	asciiDrawBitmapColored (bitmap,rect,_ascii.clearChar.backColor,_ascii.clearChar.foreColor);
}
void asciiDrawBitmapColored (asciiBitmap bitmap,asciiRect rect,asciiColor backColor,asciiColor foreColor) {
	asciiColoredBitmap* target=ASCII_TARGET;
	int32_t x,y,sx=0,sy=0;
	asciiTextchar* sourcePtr;
	asciiChar* targetPtr;
	//if rect has no size, it gets set to the size of the bitmap
	if (rect.size.x==0)
		rect.size.x=bitmap.bounds.size.x;
	if (rect.size.y==0)
		rect.size.y=bitmap.bounds.size.y;
	//try to render
	if (_ascii.isInited==1&& //asciiLib is inited
		bitmap.address!=0&&bitmap.bounds.size.x>0&&bitmap.bounds.size.y&& //bitmap is inited and has a size
		rect.offset.x<target->bounds.size.x&&rect.offset.y<target->bounds.size.y&& //rect is in target bounds
		rect.offset.x+rect.size.x>=0&&rect.offset.y+rect.size.y>=0) {
			//manually clip rect to bounds to gather the render start coordinates
			if (rect.offset.x<0) {
				sx=-rect.offset.x;
				rect.size.x+=rect.offset.x;
			}
			if (rect.offset.y<0) {
				sy=-rect.offset.y;
				rect.size.y+=rect.offset.y;
			}
			if (rect.offset.x+rect.size.x>=target->bounds.size.x)
				rect.size.x=target->bounds.size.x-rect.offset.x;
			if (rect.offset.y+rect.size.y>=target->bounds.size.y)
				rect.size.y=target->bounds.size.y-rect.offset.y;
			//render
			for (y=sy;y<sy+rect.size.y;y++) {
				targetPtr=target->address+
					(target->bounds.offset.x+rect.offset.y+y)*target->pitch+
					target->bounds.offset.x+rect.offset.x;
				for (x=sx;x<sx+rect.size.x;x++) {
					sourcePtr=bitmap.address+
						((y%bitmap.bounds.size.y)+bitmap.bounds.offset.y)*bitmap.pitch+
						((x%bitmap.bounds.size.x)+bitmap.bounds.offset.x);
					if (*sourcePtr!=bitmap.trans)
						_ascii_copyChar(targetPtr,asciiChar(*sourcePtr,backColor,foreColor));
					targetPtr++;
				}
			}
	}
}
void asciiDrawColoredBitmap(asciiColoredBitmap bitmap,asciiRect rect) {
	asciiColoredBitmap* target=ASCII_TARGET;
	int32_t x,y,sx=0,sy=0;
	asciiChar* sourcePtr;
	asciiChar* targetPtr;
	//if rect has no size, it gets set to the size of the bitmap
	if (rect.size.x==0)
		rect.size.x=bitmap.bounds.size.x;
	if (rect.size.y==0)
		rect.size.y=bitmap.bounds.size.y;
	//try to render
	if (_ascii.isInited==1&& //asciiLib is inited
		bitmap.address!=0&&bitmap.bounds.size.x>0&&bitmap.bounds.size.y&& //bitmap is inited and has a size
		rect.offset.x<target->bounds.size.x&&rect.offset.y<target->bounds.size.y&& //rect is in target bounds
		rect.offset.x+rect.size.x>=0&&rect.offset.y+rect.size.y>=0) {
			//manually clip rect to bounds to gather the render start coordinates
			if (rect.offset.x<0) {
				sx=-rect.offset.x;
				rect.size.x+=rect.offset.x;
			}
			if (rect.offset.y<0) {
				sy=-rect.offset.y;
				rect.size.y+=rect.offset.y;
			}
			if (rect.offset.x+rect.size.x>=target->bounds.size.x)
				rect.size.x=target->bounds.size.x-rect.offset.x;
			if (rect.offset.y+rect.size.y>=target->bounds.size.y)
				rect.size.y=target->bounds.size.y-rect.offset.y;
			//render
			for (y=sy;y<sy+rect.size.y;y++) {
				targetPtr=target->address+
					(target->bounds.offset.x+rect.offset.y+y)*target->pitch+
					target->bounds.offset.x+rect.offset.x;
				for (x=sx;x<sx+rect.size.x;x++) {
					sourcePtr=bitmap.address+
						((y%bitmap.bounds.size.y)+bitmap.bounds.offset.y)*bitmap.pitch+
						((x%bitmap.bounds.size.x)+bitmap.bounds.offset.x);
					if (sourcePtr->character!=bitmap.trans.character||
						sourcePtr->backColor!=bitmap.trans.backColor||
						sourcePtr->foreColor!=bitmap.trans.foreColor)
						_ascii_copyChar(targetPtr,*sourcePtr);
					targetPtr++;
				}
			}
	}
}
void asciiScrollScreen (uint32_t amount) {
	asciiPoint size=asciiGetTargetSize();
	asciiRect rect=asciiRect(0,0,size.x,size.y);
	asciiScrollRect(amount,rect);
}
void asciiScrollRect (uint32_t amount,asciiRect rect) {
	asciiColoredBitmap* target=ASCII_TARGET;
	int32_t x,y;
	asciiChar* sourcePtr,
			 * targetPtr;
	if (_ascii.isInited==1&& //asciiLib is inited
		rect.offset.x<target->bounds.size.x&&rect.offset.y<target->bounds.size.y&& //rect is in target bounds
		rect.offset.x+rect.size.x>=0&&rect.offset.y+rect.size.y>=0&&
		rect.size.x>0&&rect.size.y>0&&amount>0) { //rect has a size and the screen has to be scrolled
			//clip rect to target size
			rect=asciiClipRect(rect,asciiRect(0,0,target->bounds.size.x,target->bounds.size.y));
			//render
			if (rect.size.y<=(int32_t)amount) //if everything is scrolled away anyway we can just clear everything
				asciiClearRect(rect);
			else {
				//copy the lines
				for (y=0;y<rect.size.y-(int32_t)amount;y++) {
					sourcePtr=target->address+
						(target->bounds.offset.y+rect.offset.y+amount+y)*target->bounds.size.x+
						target->bounds.offset.x+rect.offset.x;
					targetPtr=target->address+
						(target->bounds.offset.y+rect.offset.y+y)*target->bounds.size.x+
						target->bounds.offset.x+rect.offset.x;
					for (x=0;x<rect.size.x;x++) {
						_ascii_copyChar(targetPtr,*sourcePtr);
						targetPtr++;
						sourcePtr++;
					}
				}
				//clear the upscrolled lines
				rect.offset.y=rect.size.y-amount;
				rect.size.y=amount;
				asciiClearRect(rect);
			}
	}
}

/*
	FORMAT FUNCTIONS
*/
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
	asciiBitmap ret;
#ifdef _MSC_VER
	FILE* fp=0;
	fopen_s(&fp,fn,"r");
#else
	FILE* fp=fopen((const char*)fn,"r");
#endif
	ret.address=0;
	if (fp) {
		ret=asciiLoadBitmapFromFilePtr(fp);
		fclose(fp);
	}
	return ret;
}
asciiBitmap asciiLoadBitmapFromFilePtr (void* fp) {
	if (!fp) {
		asciiBitmap bitmap;
		bitmap.address=0;
		return bitmap;
	}
	return asciiLoadBitmapFromStream(_ascii_filePtrInStream,fp);
}
asciiBitmap asciiLoadBitmapFromStream (asciiTextInStreamCallback stream,void* context) {
	asciiBitmap bitmap;
	int16_t i,lineEnd;
	uint8_t x,y;
	int8_t c;
	int8_t* ptr;
	bitmap.address=0;
	bitmap.trans=0;
	bitmap.bounds=asciiRect(0,0,0,0);
	if (stream==0||context==0)
		return bitmap;
	//read first number (width)
	i=_ascii_stream_read_number (stream,context);
	if (i<0)
		return bitmap;
	bitmap.bounds.size.x=i;
	bitmap.pitch=bitmap.bounds.size.x;
	//read second number (height)
	i=_ascii_stream_read_number (stream,context);
	if (i<0)
		return bitmap;
	bitmap.bounds.size.y=i;
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
	asciiColoredBitmap ret;
#ifdef _MSC_VER
	FILE* fp=0;
	fopen_s(&fp,fn,"r");
#else
	FILE* fp=fopen((const char*)fn,"r");
#endif
	ret.address=0;
	if (fp) {
		ret=asciiLoadColoredBitmapFromFilePtr(fp);
		fclose(fp);
	}
	return ret;
}
asciiColoredBitmap asciiLoadColoredBitmapFromFilePtr (void* fp) {
	if (!fp) {
		asciiColoredBitmap bitmap;
		bitmap.address=0;
		return bitmap;
	}
	return asciiLoadColoredBitmapFromStream(_ascii_filePtrInStream,fp);
}
asciiColoredBitmap asciiLoadColoredBitmapFromStream (asciiTextInStreamCallback stream,void* context) {
	asciiColoredBitmap bitmap;
	int16_t i,lineEnd;
	uint8_t x,y,colorCount;
	int8_t c;
	asciiChar* ptr;
	struct {
		int8_t backColor;
		int8_t foreColor;
	} colorTable[36]; //0-9 && a-z
	bitmap.address=0;
	bitmap.trans=asciiChar(0,0,0);
	bitmap.bounds=asciiRect(0,0,0,0);
	if (stream==0||context==0)
		return bitmap;
	//read first number (width)
	i=_ascii_stream_read_number (stream,context);
	if (i<=0)
		return bitmap;
	bitmap.bounds.size.x=i;
	bitmap.pitch=i;
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

/*
	MISC FUNCTIONS
*/
int8_t asciiKeyToAscii (uint8_t key) {
	if (key>=ASCII_KEYCOUNT)
		return 0;
	return _ascii_keyAsciiTable[key];
}
asciiRect asciiClipRect (asciiRect toClip,asciiRect clipRect) {
	if (toClip.offset.x<clipRect.offset.x) {
		toClip.size.x-=clipRect.offset.x-toClip.offset.x;
		toClip.offset.x=clipRect.offset.x;
	}
	if (toClip.offset.y<clipRect.offset.y) {
		toClip.size.y-=clipRect.offset.y-toClip.offset.y;
		toClip.offset.y=clipRect.offset.y;
	}
	if (toClip.offset.x+toClip.size.x>clipRect.offset.x+clipRect.size.x)
		toClip.size.x=clipRect.offset.x+clipRect.size.x-toClip.offset.x;
	if (toClip.offset.y+toClip.size.y>clipRect.offset.y+clipRect.size.y)
		toClip.size.y=clipRect.offset.y+clipRect.size.y-toClip.offset.y;
	return toClip;
}
#ifdef _MSC_VER
	asciiPoint _ascii_make_asciiPoint (int32_t x,int32_t y) {
		asciiPoint p;
		p.x=x;
		p.y=y;
		return p;
	}
	asciiRect _ascii_make_asciiRect (int32_t x,int32_t y,int32_t w,int32_t h) {
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
	BACKENDS
*/
#if USE_BACKEND == USE_BACKEND_EMSCRIPTEN
    #include <GL/glfw.h>
	#include <emscripten/emscripten.h>
	extern void js_ascii_setConsoleSize (int32_t w,int32_t h);
	extern int32_t js_ascii_getCharacterWidth ();
	extern int32_t js_ascii_getCharacterHeight ();
	extern void js_ascii_changeConsoleText (const char* text,uint32_t len);
	extern void js_ascii_changeConsoleColors (const char* backColor,const char* foreColor);
	extern void js_ascii_setTimeout (int32_t ms,int32_t id);
	extern void js_ascii_toggleMouseKeyEvent (int32_t toggle);
	extern void js_ascii_toggleMouseMoveEvent (int32_t toggle);
	extern void js_ascii_onMouseEvent (int32_t dummy); //this function has to be called by C to ensure the existence for javascript functions
	extern void js_ascii_onDocumentMouseKey (int32_t dummy);
	struct {
	    char* buffer;
	    uint32_t bufferLen;
	    uint32_t bufferPtr;
	    int8_t backColor,foreColor;
	} _web;
	static const uint32_t _web_bufferChunk=4096;
	asciiResult _ascii_initSys (int32_t w,int32_t h) {
	    js_ascii_onMouseEvent(0);
	    js_ascii_onDocumentMouseKey(0);
	    _web.bufferLen=((uint32_t)w)*h;
	    _web.bufferPtr=0;
	    _web.backColor=_ascii.clearChar.backColor;
	    _web.foreColor=_ascii.clearChar.foreColor;
	    _web.buffer=(char*)malloc(_web.bufferLen+1);
	    if (!_web.buffer)
            return ASCII_FAILED;
        js_ascii_setConsoleSize(w,h);
        glfwInit ();//set up the keyboard event
        return ASCII_SUCESS;
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
	    //no need to check if the callback is registered
	    if (_ascii.isInited==1&&(glAction==GLFW_PRESS||glAction==GLFW_RELEASE)) {
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
	}
	void _ascii_quitSys () {
	    const char* message="User closed ASCII web application";
	    const uint8_t len=(uint8_t)strlen(message);
	    glfwSetKeyCallback(0);
	    free(_web.buffer);
	    _web.buffer=0;
	    _web.bufferLen=0;
	    js_ascii_setConsoleSize(len,1);
	    js_ascii_changeConsoleText (message,len);
	}
	void asciiSignalQuit () {
	    _ascii_quit ();
	}
	asciiResult _ascii_setTimeoutSys (asciiTimerID id) {
	    js_ascii_setTimeout(_ascii.timers[id].timeout,id);
	    return ASCII_SUCESS;
	}
	int32_t _onjs_fireTimeout (int32_t id) {
	    if (_ascii.timers[id].callback!=0) {
	      _ascii.timers[id].callback(_ascii.timers[id].context);
	      _ascii.timers[id].callback=0;
	    }
	    return 0;
	}
	int32_t _onjs_fireMouseKey (int32_t buttonPressed,int32_t posX,int32_t posY) {
	    //No need to check if the callback is registerd
	    _ascii.mouseKeyEventCallback((int8_t)buttonPressed,asciiPoint(posX,posY),_ascii.mouseKeyEventCallbackContext);
	    return 0;
	}
	int32_t _onjs_fireMouseMove (int32_t buttonPressed,int32_t posX,int32_t posY) {
	    //No need to check if the callback is registered
	    _ascii.mouseMoveEventCallback((int8_t)buttonPressed,asciiPoint(posX,posY),_ascii.mouseMoveEventCallbackContext);
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
	#define _ascii_getWebColorClass(c) (((c<0||c>=ASCII_COLOR_COUNT)?0:c) +'0')
	/*
        Of course this big function is a huge bad-style, but inline is not available in emscripten
        and with a forced inline this reduces the average runtime of asciiFlip in my test profiles
        from 52.854ms to 7.489ms !
	*/
	#define _ascii_writeSized_webBuffer(str,len) {\
	    uint32_t newLen;char* newBuffer;\
	    if (_web.bufferPtr+len>_web.bufferLen) {\
	        newLen=_web.bufferLen+_web_bufferChunk;\
	        while (newLen<_web.bufferPtr+len) newLen+=_web_bufferChunk;\
	        newBuffer=realloc_emscripten(_web.buffer,_web.bufferLen,newLen+1);\
	        if (!newBuffer)\
                return 0;\
            _web.buffer=newBuffer;\
            _web.bufferLen=newLen;\
	    }\
	    if (len>1)\
            memcpy(_web.buffer+_web.bufferPtr,str,len);\
        else\
            _web.buffer[_web.bufferPtr]=*str;\
	    _web.bufferPtr+=len;\
	}
	#define _ascii_write_webBuffer(str) _ascii_writeSized_webBuffer(str,strlen(str))
	#define _ascii_writeChar_webBuffer(c) _ascii_writeSized_webBuffer(&c,1)
	#define isprint(c) (c>=0x20&&c<=0x7e)
	#define SPAN_STR "<span class=\"cbx cfx\";>x"
	#define SPAN_STR_LEN 24
	#define SPAN_STR_BACKCOLOR_OFF 15
	#define SPAN_STR_FORECOLOR_OFF 19
	#define SPAN_STR_CHARACTER_OFF 23
	int8_t _ascii_flipSys () {
	    //TODO: Optimize: No (fore)color changing if spaces aren't displayed...
	    int32_t x,y,x2;
	    char c;
	    int8_t bc,fc,curBc=_web.backColor,curFc=_web.foreColor,inSpan=0;
	    asciiPoint consoleSize=_ascii.screen.bounds.size;
	    asciiChar* sourcePtr=_ascii.screen.address;
	    char spanStr[]=SPAN_STR;
	    _web.bufferPtr=0;
	    js_ascii_changeConsoleColors(_ascii_getWebColorString(_ascii.clearChar.backColor),_ascii_getWebColorString(_ascii.clearChar.foreColor));
	    _ascii_writeSized_webBuffer("<pre>",5);
	    for (y=0;y<consoleSize.y;y++) {
	        for (x=0;x<consoleSize.x;x++) {
	            c=sourcePtr->character;
	            bc=sourcePtr->backColor;
	            fc=sourcePtr->foreColor;
	            if (!isprint(sourcePtr->character))
	                c=' ';
	            if (bc!=curBc||fc!=curFc) {
	                if (inSpan==1)
                        _ascii_writeSized_webBuffer("</span>",7);
	                if (_web.backColor==bc&&(_web.foreColor==fc||c==' ')){
	                    inSpan=0;
                        curBc=_web.backColor;
                        curFc=_web.foreColor;
                        _ascii_writeChar_webBuffer(c);
	                }
	                else {
	                    spanStr[SPAN_STR_BACKCOLOR_OFF]=_ascii_getWebColorClass(bc);
	                    spanStr[SPAN_STR_FORECOLOR_OFF]=_ascii_getWebColorClass(fc);
	                    spanStr[SPAN_STR_CHARACTER_OFF]=c;
                        _ascii_writeSized_webBuffer(spanStr,SPAN_STR_LEN);
                        curBc=bc;
                        curFc=fc;
                        inSpan=1;
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
	    _web.buffer[_web.bufferPtr]=0;
#ifdef DEBUG
        printf("web buffer size: %u\n",_web.bufferPtr);
#endif
	    js_ascii_changeConsoleText(_web.buffer,_web.bufferPtr);
	    return 1;
	}
	void _ascii_changedEvent (_ascii_Event ev) {
	    if (ev==ASCII_EVENT_KEY)
            glfwSetKeyCallback(_ascii.keyEventCallback==0?0:_ascii_glfwKeyHandler);
        else if (ev==ASCII_EVENT_MOUSEKEY)
            js_ascii_toggleMouseKeyEvent (_ascii.mouseKeyEventCallback!=0);
        else if (ev==ASCII_EVENT_MOUSEMOVE)
            js_ascii_toggleMouseMoveEvent (_ascii.mouseMoveEventCallback!=0);
	}
#elif USE_BACKEND == USE_BACKEND_WIN_CONSOLE
	#include <Windows.h>
	#pragma comment (lib,"winmm.lib")
	struct {
		int8_t isRunning;
		HANDLE hConsoleOut;
		HANDLE hConsoleIn;
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
	int8_t _ascii_initSys (int32_t w,int32_t h) {
		uint16_t i;
		CONSOLE_CURSOR_INFO cursor;
		for (i=0;i<ASCII_KEYCOUNT;i++)
			_win.keyboardState[i]=0;
		_win.consoleScreenBuffer=(CHAR_INFO*)malloc(w*h*sizeof(CHAR_INFO));
		if (!_win.consoleScreenBuffer)
			return 0;
		_win.hConsoleOut=GetStdHandle(STD_OUTPUT_HANDLE);
		if (!_win.hConsoleOut)
			return 0;
		_win.hConsoleIn=GetStdHandle(STD_INPUT_HANDLE);
		if (!_win.hConsoleIn)
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
		if (SetConsoleScreenBufferSize(_win.hConsoleOut, _win.dwBufferSize)==0||	// Set Buffer Size
			SetConsoleWindowInfo(_win.hConsoleOut, TRUE, &_win.dwConsoleRect)==0||	// Set Window Size
			SetConsoleCursorInfo(_win.hConsoleOut,&cursor)==0||					// Disable console cursor
			SetConsoleCtrlHandler(_ascii_console_handler,TRUE)==0||				// Handle closing
			SetConsoleMode(_win.hConsoleIn,ENABLE_MOUSE_INPUT)==0)				// Enable mouse
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
		asciiChar* sourcePtr=_ascii.screen.address;
		CHAR_INFO* targetPtr=_win.consoleScreenBuffer;
		WORD attributes=0xffff;
		int8_t backColor,foreColor;
		int32_t x,y;
		for (y=0;y<_ascii.screen.bounds.size.y;y++) {
			for (x=0;x<_ascii.screen.bounds.size.x;x++) {
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
		return (int8_t)WriteConsoleOutputA(_win.hConsoleOut,_win.consoleScreenBuffer,_win.dwBufferSize,_win.dwBufferCoord,&_win.dwConsoleRect);
	}
	#define _win_keyMappingCount 11 //a constant doesn't work in msvc
	#define _win_inputBufferSize 16
	const _ascii_keyMap _win_keyMappings [_win_keyMappingCount]={
		{VK_BACK,ASCII_KEY_BACKSPACE},{VK_TAB,ASCII_KEY_TAB},{VK_RETURN,ASCII_KEY_RETURN},{VK_ESCAPE,ASCII_KEY_ESCAPE},
		{VK_SPACE,ASCII_KEY_SPACE},{VK_UP,ASCII_KEY_UP},{VK_DOWN,ASCII_KEY_DOWN},{VK_RIGHT,ASCII_KEY_RIGHT},
		{VK_LEFT,ASCII_KEY_LEFT},{VK_SHIFT,ASCII_KEY_SHIFT},{VK_CONTROL,ASCII_KEY_CTRL}};
	void _ascii_runSys () {
		INPUT_RECORD inputBuffer[_win_inputBufferSize];
		DWORD i,inputLen,chunkLen;
		uint8_t key,state,mapI;
		asciiPoint mousePos;
		asciiTimerID timerID;
		_win.isRunning=1;
		while (_win.isRunning) {
			for (timerID=0;timerID<ASCII_MAX_TIMER;timerID++) {
				if (_ascii.timers[timerID].callback!=0&&_ascii.timers[timerID].timeout<=timeGetTime ()) {
					_ascii.timers[timerID].callback(_ascii.timers[timerID].context);
					_ascii.timers[timerID].callback=0;
				}
			}
			if ((_ascii.keyEventCallback||_ascii.mouseKeyEventCallback||_ascii.mouseMoveEventCallback)&&
				GetNumberOfConsoleInputEvents (_win.hConsoleIn,&inputLen)!=0&&inputLen>0) {
				while (inputLen>0) {
					if (ReadConsoleInputA(_win.hConsoleIn,inputBuffer,_win_inputBufferSize,&chunkLen)==0)
						break;
					inputLen-=chunkLen;
					for (i=0;i<chunkLen;i++) {
						if (inputBuffer[i].EventType==KEY_EVENT&&_ascii.keyEventCallback!=0) {
							state=inputBuffer[i].Event.KeyEvent.bKeyDown==TRUE;
							if (inputBuffer[i].Event.KeyEvent.uChar.AsciiChar>='a'&&inputBuffer[i].Event.KeyEvent.uChar.AsciiChar<='z')
								key=ASCII_KEY_A+(inputBuffer[i].Event.KeyEvent.uChar.AsciiChar-'a');
							else if (inputBuffer[i].Event.KeyEvent.uChar.AsciiChar>='A'&&inputBuffer[i].Event.KeyEvent.uChar.AsciiChar<='Z')
								key=ASCII_KEY_A+(inputBuffer[i].Event.KeyEvent.uChar.AsciiChar-'A');
							else if (inputBuffer[i].Event.KeyEvent.uChar.AsciiChar>='0'&&inputBuffer[i].Event.KeyEvent.uChar.AsciiChar<='9')
								key=ASCII_KEY_0+(inputBuffer[i].Event.KeyEvent.uChar.AsciiChar-'0');
							else {
								for (mapI=0;mapI<_win_keyMappingCount;mapI++) {
									if (_win_keyMappings[mapI].hardware==inputBuffer[i].Event.KeyEvent.wVirtualKeyCode) {
										key=_win_keyMappings[mapI].key;
										break;
									}
								}
								if (mapI>=_win_keyMappingCount)
									continue;
							}
							_ascii.keyEventCallback(key,state,_ascii.keyEventCallbackContext);
						} //key event
						else if (inputBuffer[i].EventType==MOUSE_EVENT&&(_ascii.mouseKeyEventCallback!=0||_ascii.mouseMoveEventCallback!=0)) {
							state=(inputBuffer[i].Event.MouseEvent.dwButtonState&FROM_LEFT_1ST_BUTTON_PRESSED)>0;
							mousePos.x=(int32_t)inputBuffer[i].Event.MouseEvent.dwMousePosition.X;
							mousePos.y=(int32_t)inputBuffer[i].Event.MouseEvent.dwMousePosition.Y;
							if (inputBuffer[i].Event.MouseEvent.dwEventFlags==0&&_ascii.mouseKeyEventCallback!=0)
								_ascii.mouseKeyEventCallback(state,mousePos,_ascii.mouseKeyEventCallbackContext);
							else if (inputBuffer[i].Event.MouseEvent.dwEventFlags==MOUSE_MOVED&&_ascii.mouseMoveEventCallback)
								_ascii.mouseMoveEventCallback(state,mousePos,_ascii.mouseMoveEventCallbackContext);
						}
					} //for (i=0;i<chunklen;i++)
				} //while (inputLen>0)
			}
			Sleep(30); //equals about 30 frames per second
		}
		_ascii_quit  ();
	}
	int8_t _ascii_setTimeoutSys (asciiTimerID id) {
		_ascii.timers[id].timeout+=timeGetTime ();
		return 1;
	}
	void _ascii_changedEvent (_ascii_Event ev) {
	}
#elif USE_BACKEND == USE_BACKEND_SDL
	void asciiSignalQuit ();
	asciiResult _ascii_initSys (int32_t w,int32_t h);
	void _ascii_quitSys ();
	void _ascii_runSys ();
	asciiResult _ascii_flipSys ();
	asciiResult _ascii_setTimeoutSys (asciiTimerID id);
	void _ascii_changedEvent (_ascii_Event ev);
#else
	#error [ASCIILIB]: No backend specified
#endif //BACKEND SWITCH
