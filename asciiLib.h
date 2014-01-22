#ifndef __ASCIILIB_H__
#define __ASCIILIB_H__

#include <stdint.h>
#define ASCII_TIMER_INVALID -1
#define ASCII_MAX_TIMER 16
#define ASCII_SUCESS ((asciiResult)1)
#define ASCII_FAILED ((asciiResult)0)
enum {
	ASCII_CHARACTER_LOCK=1<<0,
	ASCII_BACKCOLOR_LOCK=1<<1,
	ASCII_FORECOLOR_LOCK=1<<2,
	ASCII_TARGET_BITMAP=1<<3,
	ASCII_OPT_COUNT=4
};
enum {
	ASCII_COLOR_BLACK=0,
	ASCII_COLOR_RED,
	ASCII_COLOR_GREEN,
	ASCII_COLOR_YELLOW,
	ASCII_COLOR_BLUE,
	ASCII_COLOR_MAGENTA,
	ASCII_COLOR_CYAN,
	ASCII_COLOR_WHITE,
	ASCII_COLOR_COUNT
};
enum {
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
	ASCII_KEYRELEASED=0
};
typedef const char* asciiString;
typedef int8_t asciiTextchar;
typedef int8_t asciiColor;
typedef int8_t asciiTimerID;
typedef int8_t asciiResult;
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
asciiResult asciiInit (int32_t width,int32_t height);
void asciiRun ();
void asciiSignalQuit (); //should be only called when building a local app
void asciiSetKeyEventCallback (asciiKeyEventCallback callback,void* context);
void asciiSetMouseKeyEventCallback (asciiMouseEventCallback callback,void* context);
void asciiSetMouseMoveEventCallback (asciiMouseEventCallback callback,void* context);
void asciiSetQuitCallback (asciiQuitCallback callback,void* context);

//Graphic management functions
void asciiEnable (uint32_t bit);
void asciiDisable (uint32_t bit);
void asciiSetClearChar (asciiChar ch);
void asciiSetTargetBitmap (asciiColoredBitmap* target);
asciiChar asciiGetClearChar ();
asciiColoredBitmap* asciiGetTargetBitmap ();
asciiPoint asciiGetTargetSize ();

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

//time functions
asciiTimerID asciiSetTimeout (uint32_t ms,asciiTimeoutCallback callback,void* context);
void asciiCancelTimer (asciiTimerID id);
void asciiCancelAllTimer ();

//render functions
asciiResult asciiFlip ();
void asciiFillRect (asciiChar ch,asciiRect rect);
void asciiClearRect (asciiRect rect);
void asciiDrawChar (asciiChar c,asciiPoint offset);
void asciiDrawTextchar (asciiTextchar c,asciiPoint offset);
void asciiDrawText (asciiString str,asciiPoint offset);
void asciiDrawTextColored (asciiString str,asciiPoint offset,asciiColor backColor,asciiColor foreColor);
void asciiDrawSizedText (asciiString str,uint32_t len,asciiPoint offset);
void asciiDrawSizedTextColored (asciiString str,uint32_t len,asciiPoint offset,asciiColor backColor,asciiColor foreColor);
void asciiDrawBitmap (asciiBitmap bitmap,asciiRect rect);
void asciiDrawBitmapColored (asciiBitmap bitmap,asciiRect rect,asciiColor backColor,asciiColor foreColor);
void asciiDrawColoredBitmap (asciiColoredBitmap bitmap,asciiRect rect);
void asciiScrollScreen (uint32_t amount); //currently only up scrolling is supported
void asciiScrollRect (uint32_t amount,asciiRect rect);

//(bitmap) format functions
asciiBitmap asciiLoadBitmapFromFile (asciiString fn);
asciiBitmap asciiLoadBitmapFromFilePtr (void* fp); //pass a FILE pointer in
asciiBitmap asciiLoadBitmapFromStream (asciiTextInStreamCallback stream,void* context);
asciiColoredBitmap asciiLoadColoredBitmapFromFile (asciiString fn);
asciiColoredBitmap asciiLoadColoredBitmapFromFilePtr (void* fp); //pass a FILE pointer in
asciiColoredBitmap asciiLoadColoredBitmapFromStream (asciiTextInStreamCallback stream,void* context);

//misc functions
asciiColor asciiKeyToAscii (uint8_t key);
asciiRect asciiClipRect (asciiRect toClip,asciiRect clipRect);
#ifdef _MSC_VER
    //MSVC doesn't support initalizer lists
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
