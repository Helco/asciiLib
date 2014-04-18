#include "asciiLibIntern.h"

const asciiEngine asciiDefaultEngine={
	{0},0, //GBackend
	{0},0,{' ',ASCII_COLOR_BLACK,ASCII_COLOR_WHITE},0, //screen/target/clearChar/optBitmask
	0,0, 0,0, 0,0, 0,0, //callbacks
	{0}, //timers
};
const asciiChar asciiDefaultClearChar={' ',ASCII_COLOR_BLACK,ASCII_COLOR_WHITE};

#ifdef ASCII_ENABLE_BACKEND_WIN_CONSOLE
extern const asciiGBackend asciiGraphicBackendWinConsole;
#	ifndef ASCII_GRAPHIC_DEFAULT_BACKEND
#		define ASCII_GRAPHIC_DEFAULT_BACKEND asciiGraphicBackendWinConsole
#	endif
#else
const asciiGBackend asciiGraphicBackendWinConsole={0};
#endif
#ifdef ASCII_ENABLE_BACKEND_UNIX_CONSOLE
extern const asciiGBackend asciiGraphicBackendUnixConsole;
#	ifndef ASCII_GRAPHIC_DEFAULT_BACKEND
#		define ASCII_GRAPHIC_DEFAULT_BACKEND asciiGraphicBackendUnixConsole
#	endif
#else
const asciiGBackend asciiGraphicBackendUnixConsole={0};
#endif
#ifdef ASCII_ENABLE_BACKEND_EMSCRIPTEN
extern const asciiGBackend asciiGraphicBackendEmscripten;
#	ifndef ASCII_GRAPHIC_DEFAULT_BACKEND
#		define ASCII_GRAPHIC_DEFAULT_BACKEND asciiGraphicBackendEmscripten
#	endif
#else
const asciiGBackend asciiGraphicBackendEmscripten={0};
#endif
#ifdef ASCII_ENABLE_BACKEND_SDL
extern const asciiGBackend asciiGraphicBackendSDL;
#	ifndef ASCII_GRAPHIC_DEFAULT_BACKEND
#		define ASCII_GRAPHIC_DEFAULT_BACKEND asciiGraphicBackendSDL
#	endif
#else
const asciiGBackend asciiGraphicBackendSDL={0};
#endif
#ifdef ASCII_ENABLE_BACKEND_SDL2
extern const asciiGBackend asciiGraphicBackendSDL2;
#	ifndef ASCII_GRAPHIC_DEFAULT_BACKEND
#		define ASCII_GRAPHIC_DEFAULT_BACKEND asciiGraphicBackendSDL2
#	endif
#else
const asciiGBackend asciiGraphicBackendSDL2={0};
#endif
#ifndef ASCII_GRAPHIC_DEFAULT_BACKEND
#error [ASCIILIB]: No graphics backend enabled
#endif

/*
	SYSTEM FUNCTIONS
*/
asciiGraphicBackend asciiGetGraphicBackends () {
	asciiGraphicBackend ret = 0
#ifdef ASCII_ENABLE_BACKEND_WIN_CONSOLE
		| ASCII_GRAPHIC_WIN_CONSOLE
#endif
#ifdef ASCII_ENABLE_BACKEND_UNIX_CONSOLE
		| ASCII_GRAPHIC_UNIX_CONSOLE
#endif
#ifdef ASCII_ENABLE_BACKEND_EMSCRIPTEN
		| ASCII_GRAPHIC_EMSCRIPTEN
#endif
#ifdef ASCII_ENABLE_BACKEND_SDL
		| ASCII_GRAPHIC_SDL
#endif
#ifdef ASCII_ENABLE_BACKEND_SDL2
		| ASCII_GRAPHIC_SDL2
#endif
	;
	return ret;
}
asciiEngine* asciiInit (asciiGraphicBackend g,int32_t w,int32_t h) {
	asciiEngine* e;
	asciiGBackend graphic;
	if (w>0 && h>0) {
		switch (g) {
		case(ASCII_GRAPHIC_WIN_CONSOLE):{graphic = asciiGraphicBackendWinConsole;}break;
		case(ASCII_GRAPHIC_UNIX_CONSOLE):{graphic = asciiGraphicBackendUnixConsole;}break;
		case(ASCII_GRAPHIC_EMSCRIPTEN):{graphic = asciiGraphicBackendEmscripten;}break;
		case(ASCII_GRAPHIC_SDL):{graphic = asciiGraphicBackendSDL;}break;
		case(ASCII_GRAPHIC_SDL2):{graphic = asciiGraphicBackendSDL2;}break;
		case(ASCII_GRAPHIC_DEFAULT):{graphic = ASCII_GRAPHIC_DEFAULT_BACKEND;}break;
		default:{return 0;}break;
		}
		if (!graphic.init)
			return 0;

		e = (asciiEngine*)malloc(sizeof(asciiEngine));
		memcpy(e,&asciiDefaultEngine,sizeof(asciiEngine));
		if (!graphic.init (e,w,h)) {
			free(e);
			return 0;
		}
		e->screen = asciiCreateColoredBitmap (asciiPoint(w,h));
		if (!e->screen.address) {
			graphic.quit (e);
			free(e);
			return 0;
		}
		e->graphics = graphic;
		return e;
	}
	return 0;
}
void asciiRun (asciiEngine* e) {
	if (e)
		e->graphics.run (e);
}
void asciiSignalQuit (asciiEngine* e) {
	if (e)
		e->graphics.signalQuit (e); //this function calls asciiQuit which then calls graphics.quit
}
void asciiQuit (asciiEngine* e) {
	if (e) {
		if (e->quitCallback)
			e->quitCallback(e->quitCallbackContext);
		e->graphics.quit (e);
		asciiFreeColoredBitmap(&e->screen);
		free(e);
	}
}
void asciiSetKeyEventCallback (asciiEngine* e,asciiKeyEventCallback callback,void* context) {
	if (e) {
		e->keyEventCallback = callback;
		e->keyEventCallbackContext = context;
		e->graphics.eventChanged (e,ASCII_EVENT_KEY);
	}
}
void asciiSetMouseKeyEventCallback (asciiEngine* e,asciiMouseEventCallback callback,void* context) {
	if (e) {
		e->mouseKeyEventCallback = callback;
		e->mouseKeyEventCallbackContext = context;
		e->graphics.eventChanged (e,ASCII_EVENT_MOUSEKEY);
	}
}
void asciiSetMouseMoveEventCallback (asciiEngine* e,asciiMouseEventCallback callback,void* context) {
	if (e) {
		e->mouseMoveEventCallback = callback;
		e->mouseMoveEventCallbackContext = context;
		e->graphics.eventChanged(e,ASCII_EVENT_MOUSEMOVE);
	}
}
void asciiSetQuitCallback (asciiEngine* e,asciiQuitCallback callback,void* context) {
	if (e) {
		e->quitCallback = callback;
		e->quitCallbackContext = context;
		e->graphics.eventChanged(e,ASCII_EVENT_QUIT);
	}
}

/*
	GRAPHIC MANAGEMENT FUNCTIONS
*/
void asciiToggle (asciiEngine* e,uint32_t bit,asciiBool set) {
	//not used bits can be ignored
	if (e) {
		if (set)
			e->optBitmask |= bit;
		else
			e->optBitmask &= ~bit;
	}
}
void asciiSetClearChar (asciiEngine* e,asciiChar ch) {
	if (e)
		e->clearChar=ch;
}
void asciiSetTargetBitmap (asciiEngine* e,asciiColoredBitmap* target) {
	if (e) {
		e->targetBitmap=target;
		if (target==0)
			asciiToggle(e,ASCII_TARGET_BITMAP,ASCII_FALSE);
	}
}
asciiChar asciiGetClearChar (asciiEngine* e) {
	return (e ? e->clearChar : asciiChar(0,0,0));
}
asciiColoredBitmap* asciiGetTargetBitmap (asciiEngine* e) {
	return (e ? e->targetBitmap : 0);
}
asciiPoint asciiGetTargetSize (asciiEngine* e) {
	return ASCII_TARGET(e)->bounds.size;
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
	return asciiCreateFilledBitmapEx(size,asciiDefaultClearChar);
}
asciiBitmap asciiCreateBitmapEx (asciiPoint size,asciiColor backColor,asciiColor foreColor) {
	return asciiCreateFilledBitmapEx(size,asciiChar(asciiDefaultClearChar.character,backColor,foreColor));
}
asciiBitmap asciiCreateFilledBitmap (asciiPoint size,asciiTextchar fillChar) {
	return asciiCreateFilledBitmapEx(size,asciiChar(fillChar,asciiDefaultClearChar.backColor,asciiDefaultClearChar.foreColor));
}
asciiBitmap asciiCreateFilledBitmapEx (asciiPoint size,asciiChar ch) {
	asciiBitmap bitmap;
	uint32_t memlen = ((uint32_t)size.x)*size.y*sizeof(asciiTextchar);
	bitmap.address = (asciiTextchar*)malloc(memlen);
	if (bitmap.address)
		memset(bitmap.address,0,memlen);
	bitmap.bounds = asciiRect(0,0,size.x,size.y);
	bitmap.trans = 0;
	bitmap.ownMemory = 1;
	bitmap.pitch = bitmap.bounds.size.x;
	return bitmap;
}
asciiColoredBitmap asciiCreateColoredBitmap (asciiPoint size) {
	return asciiCreateFilledColoredBitmap (size,asciiDefaultClearChar);
}
asciiColoredBitmap asciiCreateFilledColoredBitmap (asciiPoint size,asciiChar fillChar) {
	asciiColoredBitmap bitmap;
	uint32_t memlen = ((uint32_t)size.x)*size.y*sizeof(asciiChar);
	bitmap.address = (asciiChar*)malloc(memlen);
	if (bitmap.address)
		memset(bitmap.address,0,memlen);
	bitmap.bounds = asciiRect(0,0,size.x,size.y);
	bitmap.trans = asciiChar(0,0,0);
	bitmap.ownMemory = 1;
	bitmap.pitch = bitmap.bounds.size.x;
	return bitmap;
}
asciiBitmap asciiCreateSubBitmap (asciiBitmap source,asciiRect bounds) {
	asciiBitmap bitmap;
	bounds = asciiClipRect(bounds,source.bounds);
	if (source.address!=0 && bounds.size.x>0 && bounds.size.y>0) {
		bitmap.bounds = bounds;
		bitmap.address = source.address;
		bitmap.trans = source.trans;
		bitmap.ownMemory = 0;
		bitmap.pitch = source.bounds.size.x;
	}
	else
		bitmap.address = 0;
	return bitmap;
}
asciiColoredBitmap asciiCreateSubColoredBitmap (asciiColoredBitmap source,asciiRect bounds) {
	asciiColoredBitmap bitmap;
	bounds = asciiClipRect(bounds,source.bounds);
	if (source.address!=0 && bounds.size.x>0 && bounds.size.y>0) {
		bitmap.bounds = bounds;
		bitmap.address = source.address;
		bitmap.trans = source.trans;
		bitmap.ownMemory = 0;
		bitmap.pitch = source.bounds.size.x;
	}
	else
		bitmap.address = 0;
	return bitmap;
}
void asciiFreeBitmap (asciiBitmap* bm) {
	if (bm && bm->address) {
		if (bm->ownMemory==1)
			free(bm->address);
		bm->address = 0;
		bm->trans = 0;
		bm->bounds = asciiRect(0,0,0,0);
	}
}
void asciiFreeColoredBitmap (asciiColoredBitmap* bm) {
	if (bm && bm->address) {
		if (bm->ownMemory==1)
			free(bm->address);
		bm->address = 0;
		bm->trans.character = 0;
		bm->bounds = asciiRect(0,0,0,0);
	}
}

/*
	TIME FUNCTIONS
*/
asciiTimerID asciiSetTimeout (asciiEngine* e,uint32_t ms,asciiTimeoutCallback callback,void* context) {
	asciiTimerID i;
	if (e) {
		for (i=0;i<ASCII_MAX_TIMER;i++) {
			if (e->timers[i].callback==0) {
				e->timers[i].callback = callback;
				e->timers[i].context = context;
				e->timers[i].timeout = ms;
				if (!e->graphics.setTimeout(e,i)) {
					e->timers[i].callback = 0;
					i = ASCII_TIMER_INVALID;
				}
				return i;
			}
		}
	}
	return ASCII_TIMER_INVALID;
}
void asciiCancelTimer (asciiEngine* e,asciiTimerID id) {
	if (e && id>=0 && id<ASCII_MAX_TIMER)
		e->timers[id].callback = 0;
}
void asciiCancelAllTimer (asciiEngine* e) {
	asciiTimerID i;
	if (e) {
		for (i=0;i<ASCII_MAX_TIMER;i++)
			e->timers[i].callback = 0;
	}
}

/*
	RENDER FUNCTIONS
*/
asciiBool asciiFlip (asciiEngine* e) {
	if (e)
		e->graphics.flip (e);
	return ASCII_FAILED;
}
void asciiFillRect (asciiEngine* e,asciiChar ch,asciiRect rect) {
	asciiColoredBitmap* target = ASCII_TARGET(e);
	asciiChar* ptr;
	int32_t x,y;
	if (e &&
		rect.offset.x<target->bounds.size.x && rect.offset.y<target->bounds.size.y && //the rect isn't out of bounds
		rect.size.x>0 && rect.size.y>0) { //the rect has an area>0

		//clip rect to target size
		rect = asciiClipRect (rect,asciiRect(0,0,target->bounds.size.x,target->bounds.size.y));
		//render
		for (y=0;y<rect.size.y;y++) {
			ptr = target->address +
				(target->bounds.offset.y+rect.offset.y+y) *target->pitch+
				target->bounds.offset.x+rect.offset.x;
			for (x=0;x<rect.size.x;x++) {
				asciiCopyChar(e,ptr,ch); //in emscripten this is a macro
				ptr++; //Here was a bug cause by my bad-style. This shall be a warning for all fellow coders
			}
		}
	}
}
void asciiClearRect (asciiEngine* e,asciiRect rect) {
	asciiFillRect(e,e->clearChar,rect);
}
void asciiDrawChar (asciiEngine* e,asciiChar c,asciiPoint offset) {
	asciiColoredBitmap* target = ASCII_TARGET(e);
	asciiChar* targetPtr;
	if (e &&
		offset.x>=0 && offset.y>=0 && //offset is in target bounds
		offset.x<target->bounds.size.x && offset.y<target->bounds.size.y) {
			targetPtr = target->address +
				(target->bounds.offset.y+offset.y)*target->bounds.size.x+
				target->bounds.offset.x+offset.x;
			asciiCopyChar(e,targetPtr,c);
	}
}
void asciiDrawTextchar (asciiEngine* e,asciiTextchar c,asciiPoint offset) {
	asciiDrawChar(e,asciiChar(c,e->clearChar.backColor,e->clearChar.foreColor),offset);
}
void asciiDrawText (asciiEngine* e,asciiString text,asciiPoint offset) {
	asciiDrawSizedTextColored(e,text,strlen(text),offset,e->clearChar.backColor,e->clearChar.foreColor);
}
void asciiDrawTextColored (asciiEngine* e,asciiString text,asciiPoint offset,asciiColor backColor,asciiColor foreColor) {
	asciiDrawSizedTextColored(e,text,strlen(text),offset,backColor,foreColor);
}
void asciiDrawSizedText (asciiEngine* e,asciiString text,uint32_t len,asciiPoint offset) {
	asciiDrawSizedTextColored(e,text,len,offset,e->clearChar.backColor,e->clearChar.foreColor);
}
void asciiDrawSizedTextColored (asciiEngine* e,asciiString text,uint32_t len,asciiPoint offset,asciiColor backColor,asciiColor foreColor) {
	asciiColoredBitmap* target = ASCII_TARGET(e);
	uint32_t i;
	asciiChar* targetPtr;
	asciiString sourcePtr = text;
	if (e && 
		text!=0 && len!=0 && //text and len are valid
		offset.x>=0 && offset.y>=0 && offset.x<target->bounds.size.x && offset.y<target->bounds.size.y) { //offset is in target bounds
			if (offset.x<0) {
				text += -offset.x;
				len += offset.x; //len gets decreased
			}
			targetPtr = target->address +
				(target->bounds.offset.y+offset.y) * target->pitch+
				target->bounds.offset.x+offset.x;
			for (i=0;i<len;i++) {
				asciiCopyChar(e,targetPtr,asciiChar(*sourcePtr,backColor,foreColor));
				targetPtr++;
				sourcePtr++;
			}
	}
}
void asciiDrawBitmap (asciiEngine* e,asciiBitmap bitmap,asciiRect rect) {
	asciiDrawBitmapColored (e,bitmap,rect,e->clearChar.backColor,e->clearChar.foreColor);
}
void asciiDrawBitmapColored (asciiEngine* e,asciiBitmap bitmap,asciiRect rect,asciiColor backColor,asciiColor foreColor) {
	asciiColoredBitmap* target = ASCII_TARGET(e);
	int32_t x,y,sx = 0,sy = 0;
	asciiTextchar* sourcePtr;
	asciiChar* targetPtr;
	//if rect has no size, it gets set to the size of the bitmap
	if (rect.size.x==0)
		rect.size.x = bitmap.bounds.size.x;
	if (rect.size.y==0)
		rect.size.y = bitmap.bounds.size.y;
	//try to render
	if (e && //asciiLib is inited
		bitmap.address!=0 && bitmap.bounds.size.x>0 && bitmap.bounds.size.y && //bitmap is inited and has a size
		rect.offset.x<target->bounds.size.x && rect.offset.y<target->bounds.size.y && //rect is in target bounds
		rect.offset.x+rect.size.x>=0 && rect.offset.y+rect.size.y>=0) {
			//manually clip rect to bounds to gather the render start coordinates
			if (rect.offset.x<0) {
				sx = -rect.offset.x;
				rect.size.x += rect.offset.x;
			}
			if (rect.offset.y<0) {
				sy = -rect.offset.y;
				rect.size.y += rect.offset.y;
			}
			if (rect.offset.x+rect.size.x >= target->bounds.size.x)
				rect.size.x = target->bounds.size.x-rect.offset.x;
			if (rect.offset.y+rect.size.y >= target->bounds.size.y)
				rect.size.y = target->bounds.size.y-rect.offset.y;
			//render
			for (y=sy;y < sy+rect.size.y;y++) {
				targetPtr = target->address +
					(target->bounds.offset.x+rect.offset.y+y) * target->pitch+
					target->bounds.offset.x+rect.offset.x;
				for (x=sx;x<sx+rect.size.x;x++) {
					sourcePtr = bitmap.address +
						((y%bitmap.bounds.size.y)+bitmap.bounds.offset.y) * bitmap.pitch+
						((x%bitmap.bounds.size.x)+bitmap.bounds.offset.x);
					if (*sourcePtr!=bitmap.trans)
						asciiCopyChar(e,targetPtr,asciiChar(*sourcePtr,backColor,foreColor));
					targetPtr++;
				}
			}
	}
}
void asciiDrawColoredBitmap(asciiEngine* e,asciiColoredBitmap bitmap,asciiRect rect) {
	asciiColoredBitmap* target = ASCII_TARGET(e);
	int32_t x,y,sx = 0,sy = 0;
	asciiChar* sourcePtr;
	asciiChar* targetPtr;
	//if rect has no size, it gets set to the size of the bitmap
	if (rect.size.x==0)
		rect.size.x = bitmap.bounds.size.x;
	if (rect.size.y==0)
		rect.size.y = bitmap.bounds.size.y;
	//try to render
	if (e && //asciiLib is inited
		bitmap.address!=0 && bitmap.bounds.size.x>0 && bitmap.bounds.size.y && //bitmap is inited and has a size
		rect.offset.x<target->bounds.size.x && rect.offset.y<target->bounds.size.y && //rect is in target bounds
		rect.offset.x+rect.size.x>=0 && rect.offset.y+rect.size.y>=0) {
			//manually clip rect to bounds to gather the render start coordinates
			if (rect.offset.x<0) {
				sx = -rect.offset.x;
				rect.size.x += rect.offset.x;
			}
			if (rect.offset.y<0) {
				sy = -rect.offset.y;
				rect.size.y += rect.offset.y;
			}
			if (rect.offset.x+rect.size.x >= target->bounds.size.x)
				rect.size.x = target->bounds.size.x-rect.offset.x;
			if (rect.offset.y+rect.size.y >= target->bounds.size.y)
				rect.size.y = target->bounds.size.y-rect.offset.y;
			//render
			for (y=sy;y < sy+rect.size.y;y++) {
				targetPtr = target->address +
					(target->bounds.offset.x+rect.offset.y+y) * target->pitch+
					target->bounds.offset.x+rect.offset.x;
				for (x=sx;x < sx+rect.size.x;x++) {
					sourcePtr = bitmap.address +
						((y%bitmap.bounds.size.y)+bitmap.bounds.offset.y) * bitmap.pitch+
						((x%bitmap.bounds.size.x)+bitmap.bounds.offset.x);
					if (sourcePtr->character!=bitmap.trans.character ||
						sourcePtr->backColor!=bitmap.trans.backColor ||
						sourcePtr->foreColor!=bitmap.trans.foreColor)
						asciiCopyChar(e,targetPtr,*sourcePtr);
					targetPtr++;
				}
			}
	}
}
void asciiScrollScreen (asciiEngine* e,uint32_t amount) {
	asciiPoint size = asciiGetTargetSize(e);
	asciiRect rect = asciiRect(0,0,size.x,size.y);
	asciiScrollRect(e,amount,rect);
}
void asciiScrollRect (asciiEngine* e,uint32_t amount,asciiRect rect) {
	asciiColoredBitmap* target = ASCII_TARGET(e);
	int32_t x,y;
	asciiChar* sourcePtr,
			 * targetPtr;
	if (e && //asciiLib is inited
		rect.offset.x<target->bounds.size.x && rect.offset.y<target->bounds.size.y && //rect is in target bounds
		rect.offset.x+rect.size.x>=0 && rect.offset.y+rect.size.y>=0 &&
		rect.size.x>0 && rect.size.y>0 && amount>0) { //rect has a size and the screen has to be scrolled
			//clip rect to target size
			rect = asciiClipRect(rect,asciiRect(0,0,target->bounds.size.x,target->bounds.size.y));
			//render
			if (rect.size.y<=(int32_t)amount) //if everything is scrolled away anyway we can just clear everything
				asciiClearRect(e,rect);
			else {
				//copy the lines
				for (y=0;y < rect.size.y-(int32_t)amount;y++) {
					sourcePtr = target->address + 
						(target->bounds.offset.y+rect.offset.y+amount+y) * target->bounds.size.x+
						target->bounds.offset.x+rect.offset.x;
					targetPtr = target->address +
						(target->bounds.offset.y+rect.offset.y+y) * target->bounds.size.x+
						target->bounds.offset.x+rect.offset.x;
					for (x=0;x < rect.size.x;x++) {
						asciiCopyChar(e,targetPtr,*sourcePtr);
						targetPtr++;
						sourcePtr++;
					}
				}
				//clear the upscrolled lines
				rect.offset.y = rect.size.y-amount;
				rect.size.y = amount;
				asciiClearRect(e,rect);
			}
	}
}

/*
	FORMAT FUNCTIONS
*/
int8_t _ascii_filePtrInStream (void* context) {
	FILE* f = (FILE*) context;
	int8_t ret;
	if (feof(f))
		return 0;
	ret = (int8_t)fgetc(f);
	if (feof(f))
		return 0;
	return ret;
}
int8_t _ascii_stream_skip_to_eol (asciiTextInStreamCallback stream,void* context) {
	int8_t eos = 0;
	int8_t c;
	do {
		c = stream(context);
		if (c==0)
			eos = 1;
	} while (c!=0 && c!='\n');
	return eos;
}
int16_t _ascii_stream_read_number (asciiTextInStreamCallback stream,void* context) { //actually reads only 8bit unsigned
	int8_t c;
	int16_t ret = 0;
	uint8_t i = 0;
	c = stream(context);
	while (c>=0 && !isdigit(c)) {
		if (_ascii_stream_skip_to_eol(stream,context))
			return -1;
		c = stream(context);
	}
	if (c<0)
		return -1;
	ret = c-'0';
	for (i=0;i<2;i++) {
		c = stream(context);
		if (c<0)
			return -1;
		else if (isdigit(c))
			ret = ret*10+(c-'0');
		else {
			if (c!='\n' && _ascii_stream_skip_to_eol(stream,context))
				return -1;
			break;
		}
	}
	if (i==2)
		_ascii_stream_skip_to_eol(stream,context);
	return (ret>0xff ? -1 : ret);
}
asciiBitmap asciiLoadBitmapFromFile (asciiString fn) {
	asciiBitmap ret;
#ifdef _MSC_VER
	FILE* fp = 0;
	fopen_s(&fp,fn,"r");
#else
	FILE* fp = fopen((const char*)fn,"r");
#endif
	ret.address = 0;
	if (fp) {
		ret = asciiLoadBitmapFromFilePtr(fp);
		fclose(fp);
	}
	return ret;
}
asciiBitmap asciiLoadBitmapFromFilePtr (void* fp) {
	if (!fp) {
		asciiBitmap bitmap;
		bitmap.address = 0;
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
	bitmap.address = 0;
	bitmap.trans = 0;
	bitmap.bounds = asciiRect(0,0,0,0);
	if (stream==0 || context==0)
		return bitmap;
	//read first number (width)
	i = _ascii_stream_read_number (stream,context);
	if (i<0)
		return bitmap;
	bitmap.bounds.size.x = i;
	bitmap.pitch = bitmap.bounds.size.x;
	//read second number (height)
	i = _ascii_stream_read_number (stream,context);
	if (i<0)
		return bitmap;
	bitmap.bounds.size.y = i;
	//read character data
	bitmap.address = (int8_t*)malloc(bitmap.bounds.size.x*bitmap.bounds.size.y);
	if (bitmap.address) {
		memset(bitmap.address,' ',bitmap.bounds.size.x*bitmap.bounds.size.y);
		for (y=0;y<bitmap.bounds.size.y;y++) {
			lineEnd = 0;
			ptr = bitmap.address+y*bitmap.bounds.size.x;
			for (x=0;x<bitmap.bounds.size.x;x++) {
				c = stream(context);
				if (c=='\n') {
					lineEnd = 1;
					break;
				}
				else if (c<0 || !isprint(c)) {
					free(bitmap.address);
					bitmap.address = 0;
					return bitmap;
				}
				else
					*ptr = c;
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
	FILE* fp = 0;
	fopen_s(&fp,fn,"r");
#else
	FILE* fp = fopen((const char*)fn,"r");
#endif
	ret.address = 0;
	if (fp) {
		ret = asciiLoadColoredBitmapFromFilePtr(fp);
		fclose(fp);
	}
	return ret;
}
asciiColoredBitmap asciiLoadColoredBitmapFromFilePtr (void* fp) {
	if (!fp) {
		asciiColoredBitmap bitmap;
		bitmap.address = 0;
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
	bitmap.address = 0;
	bitmap.trans = asciiChar(0,0,0);
	bitmap.bounds = asciiRect(0,0,0,0);
	if (stream==0 || context==0)
		return bitmap;
	//read first number (width)
	i = _ascii_stream_read_number (stream,context);
	if (i<=0)
		return bitmap;
	bitmap.bounds.size.x = i;
	bitmap.pitch = i;
	//read second number (height)
	i = _ascii_stream_read_number (stream,context);
	if (i<=0)
		return bitmap;
	bitmap.bounds.size.y = (uint8_t)i;
	//read third number (color count)
	i = _ascii_stream_read_number (stream,context);
	if (i<=0 || i>36)
		return bitmap;
	colorCount = (uint8_t)i;
	//read color table
	for (i=0;i<colorCount;i++) {
		c = stream(context);
		if (c<'0' || c>'9')
			return bitmap;
		colorTable[i].backColor = c-'0';
		c = stream(context);
		if (c<'0'||c>'9')
			return bitmap;
		colorTable[i].foreColor = c-'0';
		if (_ascii_stream_skip_to_eol(stream,context))
			return bitmap;
	}
	bitmap.address = (asciiChar*)malloc(bitmap.bounds.size.x*bitmap.bounds.size.y*sizeof(asciiChar));
	if (bitmap.address) {
		//read character data
		memset(bitmap.address,' ',bitmap.bounds.size.x*bitmap.bounds.size.y*sizeof(asciiChar));
		for (y=0;y<bitmap.bounds.size.y;y++) {
			lineEnd = 0;
			ptr = bitmap.address + y*bitmap.bounds.size.x;
			for (x=0;x<bitmap.bounds.size.x;x++) {
				c = stream(context);
				if (c=='\n') {
					lineEnd = 1;
					break;
				}
				else if (c<0 || !isprint(c)) {
					free(bitmap.address);
					bitmap.address = 0;
					return bitmap;
				}
				else {
					ptr->character = c;
					ptr->backColor = 0;
					ptr->foreColor = 0;
				}
				ptr++;
			}
			for (;x<bitmap.bounds.size.x;x++) {
				ptr->backColor = 0;
				ptr->foreColor = 0;
				ptr++;
			}
			if (lineEnd==0)
				_ascii_stream_skip_to_eol (stream,context);
		}
		//read color data
		for (y=0;y<bitmap.bounds.size.y;y++) {
			lineEnd = 0;
			ptr = bitmap.address + y*bitmap.bounds.size.x;
			for (x=0;x<bitmap.bounds.size.x;x++) {
				c = stream(context);
				if (c=='\n') {
					lineEnd = 1;
					break;
				}
				else if (c<0 || (!isdigit(c) && !isalpha(c))) {
					free(bitmap.address);
					bitmap.address = 0;
					return bitmap;
				}
				else {
					if (c<='9')
						c -= '0';
					else
						c = 10+c - (islower(c) ? 'a' : 'A');
					if (c>=colorCount) {
						free(bitmap.address);
						bitmap.address = 0;
						return bitmap;
					}
					ptr->backColor = colorTable[c].backColor;
					ptr->foreColor = colorTable[c].foreColor;
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
 *MISC FUNCTIONS
 */
int8_t asciiKeyToAscii (uint8_t key) {
	if (key>=ASCII_KEYCOUNT)
		return 0;
	return asciiKeyAsciiTable[key];
}
asciiRect asciiClipRect (asciiRect toClip,asciiRect clipRect) {
	if (toClip.offset.x<clipRect.offset.x) {
		toClip.size.x -= clipRect.offset.x-toClip.offset.x;
		toClip.offset.x = clipRect.offset.x;
	}
	if (toClip.offset.y<clipRect.offset.y) {
		toClip.size.y -= clipRect.offset.y-toClip.offset.y;
		toClip.offset.y = clipRect.offset.y;
	}
	if (toClip.offset.x+toClip.size.x > clipRect.offset.x+clipRect.size.x)
		toClip.size.x = clipRect.offset.x+clipRect.size.x-toClip.offset.x;
	if (toClip.offset.y+toClip.size.y > clipRect.offset.y+clipRect.size.y)
		toClip.size.y = clipRect.offset.y+clipRect.size.y-toClip.offset.y;
	return toClip;
}
#ifdef _MSC_VER
	asciiPoint _ascii_make_asciiPoint (int32_t x,int32_t y) {
		asciiPoint p;
		p.x = x;
		p.y = y;
		return p;
	}
	asciiRect _ascii_make_asciiRect (int32_t x,int32_t y,int32_t w,int32_t h) {
		asciiRect r;
		r.offset.x = x;
		r.offset.y = y;
		r.size.x = w;
		r.size.y = h;
		return r;
	}
	asciiChar _ascii_make_asciiChar (asciiTextchar ch,asciiColor bc,asciiColor fc) {
		asciiChar c;
		c.character = ch;
		c.backColor = bc;
		c.foreColor = fc;
		c.reserved = 0;
		return c;
	}
#endif