#ifndef __ASCIILIB_H__
#define __ASCIILIB_H__

#include <stdint.h>

#define ASCII_TIMER_INVALID -1
#define ASCII_MAX_TIMER 16 //the system max is 128 (int8)

#define ASCII_SUCESS ((asciiBool)1)
#define ASCII_FAILED ((asciiBool)0)
#define ASCII_TRUE ((asciiBool)1)
#define ASCII_FALSE ((asciiBool)0)
enum {
	//note: not all backends may be available on all platforms
	//the graphic backend also invokes user events (keyboard/mouse/etc.)
	ASCII_GRAPHIC_WIN_CONSOLE=1<<0,
	ASCII_GRAPHIC_UNIX_CONSOLE=1<<1, //not implemented yet
	ASCII_GRAPHIC_EMSCRIPTEN=1<<2,
	ASCII_GRAPHIC_SDL=1<<3, //not implemented yet
	ASCII_GRAPHIC_SDL2=1<<4, //not implemented yet
	ASCII_GRAPHIC_COUNT=5,
	ASCII_GRAPHIC_DEFAULT=ASCII_GRAPHIC_COUNT, //the library chooses a backend

	ASCII_CHARACTER_LOCK=1<<0,
	ASCII_BACKCOLOR_LOCK=1<<1,
	ASCII_FORECOLOR_LOCK=1<<2,
	ASCII_TARGET_BITMAP=1<<3,
	ASCII_OPT_COUNT=4,

	ASCII_COLOR_BLACK=0,
	ASCII_COLOR_RED,
	ASCII_COLOR_GREEN,
	ASCII_COLOR_YELLOW,
	ASCII_COLOR_BLUE,
	ASCII_COLOR_MAGENTA,
	ASCII_COLOR_CYAN,
	ASCII_COLOR_WHITE,
	ASCII_COLOR_COUNT,

	ASCII_KEY_BACKSPACE,
	ASCII_KEY_TAB,
	ASCII_KEY_RETURN,
	ASCII_KEY_ESCAPE,
	ASCII_KEY_SPACE,
	ASCII_KEY_0,
	ASCII_KEY_1,
	ASCII_KEY_2,
	ASCII_KEY_3,
	ASCII_KEY_4,
	ASCII_KEY_5,
	ASCII_KEY_6,
	ASCII_KEY_7,
	ASCII_KEY_8,
	ASCII_KEY_9,
	ASCII_KEY_A,
	ASCII_KEY_B,
	ASCII_KEY_C,
	ASCII_KEY_D,
	ASCII_KEY_E,
	ASCII_KEY_F,
	ASCII_KEY_G,
	ASCII_KEY_H,
	ASCII_KEY_I,
	ASCII_KEY_J,
	ASCII_KEY_K,
	ASCII_KEY_L,
	ASCII_KEY_M,
	ASCII_KEY_N,
	ASCII_KEY_O,
	ASCII_KEY_P,
	ASCII_KEY_Q,
	ASCII_KEY_R,
	ASCII_KEY_S,
	ASCII_KEY_T,
	ASCII_KEY_U,
	ASCII_KEY_V,
	ASCII_KEY_W,
	ASCII_KEY_X,
	ASCII_KEY_Y,
	ASCII_KEY_Z,
	ASCII_KEY_UP,
	ASCII_KEY_DOWN,
	ASCII_KEY_RIGHT,
	ASCII_KEY_LEFT,
	ASCII_KEY_SHIFT,
	ASCII_KEY_CTRL,
	ASCII_KEYCOUNT,
	ASCII_KEYPRESSED=1,
	ASCII_KEYRELEASED=0,
};
typedef uint8_t asciiGraphicBackend;
typedef int8_t asciiTextchar;
typedef const char* asciiString;
typedef uint8_t asciiColor;
typedef int8_t asciiTimerID;
typedef int8_t asciiBool; //maybe set this to int32_t?
typedef struct _asciiEngine asciiEngine;
typedef struct {
	asciiTextchar character;
	asciiColor backColor;
	asciiColor foreColor;
	int8_t reserved; //padding byte (to 32bit)
} asciiChar;
typedef struct {
	int32_t x;
	int32_t y;
} asciiPoint;
typedef struct {
	asciiPoint offset;
	asciiPoint size;
} asciiRect;
typedef struct {
	asciiRect bounds;
	asciiTextchar* address;
	asciiTextchar trans;
	int8_t ownMemory;
	int32_t pitch;
} asciiBitmap;
typedef struct {
	asciiRect bounds;
	asciiChar* address;
	asciiChar trans;
	int8_t ownMemory;
	int32_t pitch;
} asciiColoredBitmap;

typedef void (*asciiKeyEventCallback) (uint8_t key,uint8_t pressed,void* context);
typedef void (*asciiMouseEventCallback) (uint8_t buttonPressed,asciiPoint mousePos,void* context);
typedef void (*asciiQuitCallback) (void* context);
typedef void (*asciiTimeoutCallback) (void* context);
typedef asciiTextchar (*asciiTextInStreamCallback) (void* context); //pass 0 to signal the end

//System functions
asciiGraphicBackend asciiGetGraphicBackends ();
asciiEngine* asciiInit (asciiGraphicBackend g,int32_t width,int32_t height); //the engine is freed automatically on exit (if the app ever closes)
void asciiRun (asciiEngine* e);
void asciiSignalQuit (asciiEngine* e); //should be only called when building a local app
void asciiSetKeyEventCallback (asciiEngine* e,asciiKeyEventCallback callback,void* context);
void asciiSetMouseKeyEventCallback (asciiEngine* e,asciiMouseEventCallback callback,void* context);
void asciiSetMouseMoveEventCallback (asciiEngine* e,asciiMouseEventCallback callback,void* context);
void asciiSetQuitCallback (asciiEngine* e,asciiQuitCallback callback,void* context);

//Graphic management functions
void asciiToggle (asciiEngine* e,uint32_t bit,asciiBool set);
void asciiDisable (asciiEngine* e,uint32_t bit);
void asciiSetClearChar (asciiEngine* e,asciiChar ch);
void asciiSetTargetBitmap (asciiEngine* e,asciiColoredBitmap* target);
asciiChar asciiGetClearChar (asciiEngine* e);
asciiColoredBitmap* asciiGetTargetBitmap (asciiEngine* e);
asciiPoint asciiGetTargetSize (asciiEngine* e);

//bitmap functions
void asciiSetBitmapTransparent (asciiBitmap* bitmap,asciiTextchar ch);
void asciiSetColoredBitmapTransparent (asciiColoredBitmap* bitmap,asciiChar ch);
asciiBitmap asciiCreateBitmap (asciiPoint size);
asciiBitmap asciiCreateBitmapEx (asciiPoint size,asciiColor backColor,asciiColor foreColor);
asciiBitmap asciiCreateFilledBitmap (asciiPoint size,asciiTextchar fillChar);
asciiBitmap asciiCreateFilledBitmapEx (asciiPoint size,asciiChar ch);
asciiColoredBitmap asciiCreateColoredBitmap (asciiPoint size);
asciiColoredBitmap asciiCreateFilledColoredBitmap (asciiPoint size,asciiChar fillChar);
asciiBitmap asciiCreateSubBitmap (asciiBitmap source,asciiRect bounds);
asciiColoredBitmap asciiCreateSubColoredBitmap (asciiColoredBitmap source,asciiRect bounds);
void asciiFreeBitmap (asciiBitmap* bm);// only for bitmaps allocated with malloc (this includes bitmaps returned from ascii* )
void asciiFreeColoredBitmap (asciiColoredBitmap* bm);//see ^
asciiBitmap asciiLoadBitmapFromFile (asciiString fn);
asciiBitmap asciiLoadBitmapFromFilePtr (void* fp); //pass a FILE pointer in
asciiBitmap asciiLoadBitmapFromStream (asciiTextInStreamCallback stream,void* context);
asciiColoredBitmap asciiLoadColoredBitmapFromFile (asciiString fn);
asciiColoredBitmap asciiLoadColoredBitmapFromFilePtr (void* fp); //pass a FILE pointer in
asciiColoredBitmap asciiLoadColoredBitmapFromStream (asciiTextInStreamCallback stream,void* context);

//time functions
asciiTimerID asciiSetTimeout (asciiEngine* e,uint32_t ms,asciiTimeoutCallback callback,void* context);
void asciiCancelTimer (asciiEngine* e,asciiTimerID id);
void asciiCancelAllTimer (asciiEngine* e);

//render functions
asciiBool asciiFlip (asciiEngine* e);
void asciiFillRect (asciiEngine* e,asciiChar ch,asciiRect rect);
void asciiClearRect (asciiEngine* e,asciiRect rect);
void asciiDrawChar (asciiEngine* e,asciiChar c,asciiPoint offset);
void asciiDrawTextchar (asciiEngine* e,asciiTextchar c,asciiPoint offset);
void asciiDrawText (asciiEngine* e,asciiString str,asciiPoint offset);
void asciiDrawTextColored (asciiEngine* e,asciiString str,asciiPoint offset,asciiColor backColor,asciiColor foreColor);
void asciiDrawSizedText (asciiEngine* e,asciiString str,uint32_t len,asciiPoint offset);
void asciiDrawSizedTextColored (asciiEngine* e,asciiString str,uint32_t len,asciiPoint offset,asciiColor backColor,asciiColor foreColor);
void asciiDrawBitmap (asciiEngine* e,asciiBitmap bitmap,asciiRect rect);
void asciiDrawBitmapColored (asciiEngine* e,asciiBitmap bitmap,asciiRect rect,asciiColor backColor,asciiColor foreColor);
void asciiDrawColoredBitmap (asciiEngine* e,asciiColoredBitmap bitmap,asciiRect rect);
void asciiScrollScreen (asciiEngine* e,uint32_t amount); //currently only up scrolling is supported
void asciiScrollRect (asciiEngine* e,uint32_t amount,asciiRect rect);

//misc functions
int8_t asciiKeyToAscii (uint8_t key);
asciiRect asciiClipRect (asciiRect toClip,asciiRect clipRect);

#ifdef _MSC_VER
    //MSVC doesn't support initalizer lists (yet)
    asciiPoint _ascii_make_asciiPoint (int32_t x,int32_t y);
    asciiRect _ascii_make_asciiRect (int32_t x,int32_t y,int32_t w,int32_t h);
    asciiChar _ascii_make_asciiChar (asciiTextchar ch,asciiColor bc,asciiColor fc);
    #define asciiPoint(x,y) (_ascii_make_asciiPoint(x,y))
    #define asciiRect(x,y,w,h) (_ascii_make_asciiRect(x,y,w,h))
    #define asciiChar(ch,bc,fc) (_ascii_make_asciiChar(ch,bc,fc))
#else
    #define asciiPoint(x,y) ((asciiPoint){x,y})
    #define asciiRect(x,y,w,h) ((asciiRect){{x,y},{w,h}})
    #define asciiChar(ch,bc,fc) ((asciiChar){ch,bc,fc,0})
#endif

#endif //__ASCIILIB_H__
