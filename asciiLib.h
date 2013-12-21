#ifndef __ASCIILIB_H__
#define __ASCIILIB_H__

#include <stdint.h>

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
typedef struct {
	int8_t character;
	int8_t backColor;
	int8_t foreColor;
	int8_t reserved;
} asciiChar;
typedef struct {
	uint8_t x;
	uint8_t y;
} asciiPoint;
typedef struct {
	asciiPoint offset;
	asciiPoint size;
} asciiRect;
typedef struct {
	asciiRect bounds;
	int8_t* address;
	int8_t trans;
} asciiBitmap;
typedef struct {
	asciiRect bounds;
	asciiChar* address;
	int8_t trans;
} asciiColoredBitmap;

typedef void (*asciiKeyEventCallback) (uint8_t key,uint8_t pressed,void* context);
typedef void (*asciiQuitCallback) (void* context);
typedef int8_t (*asciiTextInStreamCallback) (void* context); //pass 0 to signal the end

//System functions
int8_t asciiInit (uint8_t width,uint8_t height);//width/height doesn't get set on UNIX backend, this has to be done in a launcher script
void asciiRun ();
void asciiSignalQuit (); //should be only called when building a local app
//Trivial functions
int8_t asciiGetStdBackColor ();
int8_t asciiGetStdForeColor ();
void asciiSetStdBackColor (int8_t backColor);
void asciiSetStdForeColor (int8_t foreColor);
asciiPoint asciiGetSize ();
asciiChar* asciiGetConsoleBuffer ();
void asciiSetKeyEventCallback (asciiKeyEventCallback callback,void* context);
void asciiSetQuitCallback (asciiQuitCallback callback,void* context);
int8_t asciiKeyToAscii (uint8_t key);
void asciiSetBitmapTransparent (asciiBitmap* bitmap,int8_t ch);
void asciiSetColoredBitmapTransparent (asciiColoredBitmap* bitmap,int8_t ch);
void asciiFreeBitmap (asciiBitmap* bm);// only for bitmaps allocated with malloc (this includes bitmaps returned from asciiLoad* )
void asciiFreeColoredBitmap (asciiColoredBitmap* bm);//see ^
//Graphic functions
int8_t asciiFlip ();
void asciiFillRect (asciiChar ch,asciiRect rect);
void asciiClearRect (asciiRect rect);
void asciiDrawChar (int8_t c,asciiPoint offset);
void asciiDrawCharacter (asciiChar c,asciiPoint offset);
void asciiDrawCharColored (int8_t c,asciiPoint offset,int8_t backColor,int8_t foreColor);
void asciiDrawText (asciiString str,asciiPoint offset);
void asciiDrawTextColored (asciiString str,asciiPoint offset,int8_t backColor,int8_t foreColor);
void asciiDrawSizedText (asciiString str,uint32_t len,asciiPoint offset);
void asciiDrawSizedTextColored (asciiString str,uint32_t len,asciiPoint offset,int8_t backColor,int8_t foreColor);
void asciiDrawBitmap (asciiBitmap bitmap,asciiRect rect);
void asciiDrawBitmapColored (asciiBitmap bitmap,asciiRect rect,int8_t backColor,int8_t foreColor);
void asciiDrawColoredBitmap (asciiColoredBitmap bitmap,asciiRect rect);
void asciiScrollScreen (uint8_t amount);
void asciiScrollRect (uint8_t amount,asciiRect rect);
//Format functions
asciiBitmap asciiLoadBitmapFromFile (asciiString fn);
asciiBitmap asciiLoadBitmapFromFilePtr (void* fp); //pass a FILE pointer in
asciiBitmap asciiLoadBitmapFromStream (asciiTextInStreamCallback stream,void* context);
asciiColoredBitmap asciiLoadColoredBitmapFromFile (asciiString fn);
asciiColoredBitmap asciiLoadColoredBitmapFromFilePtr (void* fp); //pass a FILE pointer in
asciiColoredBitmap asciiLoadColoredBitmapFromStream (asciiTextInStreamCallback stream,void* context);

#ifdef _MSC_VER
    //MSVC doesn't allow initalizer lists
    asciiPoint _ascii_make_asciiPoint (uint8_t x,uint8_t y);
    asciiRect _ascii_make_asciiRect (uint8_t x,uint8_t y,uint8_t w,uint8_t h);
    asciiChar _ascii_make_asciiChar (int8_t ch,int8_t bc,int8_t fc);
    #define asciiPoint(x,y) (_ascii_make_asciiPoint(x,y))
    #define asciiRect(x,y,w,h) (_ascii_make_asciiRect(x,y,w,h))
    #define asciiChar(ch,bc,fc) (_ascii_make_asciiChar(ch,bc,fc))
#else
    #define asciiPoint(x,y) ((asciiPoint){x,y})
    #define asciiRect(x,y,w,h) ((asciiRect){{x,y},{w,h}})
    #define asciiChar(ch,bc,fc) ((asciiChar){ch,bc,fc,0})
#endif

#endif //__ASCIILIB_H__
