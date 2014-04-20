#ifndef ASCII_LIB_INTERN_H
#define ASCII_LIB_INTERN_H
#include "asciiLib.h"

#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/*
 * COMPILE-SWITCH CONFIGURATION
 * You can enable/disable compiling certain backends with these macros
 */
#ifdef _WIN32
#	define ASCII_ENABLE_BACKEND_WIN_CONSOLE
#endif
//#define ASCII_ENABLE_BACKEND_SDL //not implemented yet
//#define ASCII_ENABLE_BACKEND_SDL2 //^^

#ifdef EMSCRIPTEN //with emscripten you should use the emscripten backend, it's most optimized
#define ASCII_ENABLE_BACKEND_EMSCRIPTEN
#undef ASCII_ENABLE_BACKEND_WIN_CONSOLE
#undef ASCII_ENABLE_BACKEND_UNIX_CONSOLE
#undef ASCII_ENABLE_BACKEND_SDL2
#endif

/*
 * BACKEND INTERFACE
 */
enum {
    ASCII_EVENT_KEY=0,
    ASCII_EVENT_MOUSEMOVE,
    ASCII_EVENT_MOUSEKEY,
    ASCII_EVENT_QUIT,
	ASCII_EVENT_COUNT
};
typedef uint8_t asciiEvent;

typedef asciiBool (*asciiGBackendInit) (asciiEngine* e,int32_t w,int32_t h);
typedef void (*asciiGBackendRun) (asciiEngine* e);
typedef void (*asciiGBackendQuit) (asciiEngine* e);
typedef asciiBool (*asciiGBackendFlip) (asciiEngine* e);
typedef asciiBool (*asciiGBackendSetTimeout) (asciiEngine* e,asciiTimerID id);
typedef void (*asciiGBackendSignalQuit) (asciiEngine* e);
typedef void (*asciiGBackendEventChanged) (asciiEngine* e,asciiEvent event);
typedef struct _asciiGBackend {
	asciiGBackendInit init;
	asciiGBackendRun run;
	asciiGBackendQuit quit;
	asciiGBackendFlip flip;
	asciiGBackendSetTimeout setTimeout;
	asciiGBackendSignalQuit signalQuit;
	asciiGBackendEventChanged eventChanged;
} asciiGBackend;

/*
 * HELPER STRUCTURES/DATA/MACROS
 */
#define ASCII_IS_OPT(e,bit)         (((e)->optBitmask&(bit))>0)
#define ASCII_IS_CHARACTER_LOCK(e)   ASCII_IS_OPT((e),ASCII_CHARACTER_LOCK)
#define ASCII_IS_BACKCOLOR_LOCK(e)   ASCII_IS_OPT((e),ASCII_BACKCOLOR_LOCK)
#define ASCII_IS_FORECOLOR_LOCK(e)   ASCII_IS_OPT((e),ASCII_FORECOLOR_LOCK)
#define ASCII_IS_TARGET_BITMAP(e)    ASCII_IS_OPT((e),ASCII_TARGET_BITMAP)
#define ASCII_IS_KEY_REPEAT(e)		 ASCII_IS_OPT((e),ASCII_KEY_REPEAT)
#define ASCII_TARGET(e)              (ASCII_IS_TARGET_BITMAP(e)?(e)->targetBitmap:&(e)->screen)

typedef struct _asciiKeyMap {
	int32_t hardware;
	uint8_t key;
} asciiKeyMap;
typedef struct _asciiTimerData {
	uint32_t timeout;
	asciiTimeoutCallback callback;
	void* context;
} asciiTimerData;

/*
	LIBRARY RUNTIME DATA
*/
struct _asciiEngine{
	asciiGBackend graphics;
	void* graphicsContext;
	//graphic
	asciiColoredBitmap screen;
	asciiColoredBitmap* targetBitmap;
	asciiChar clearChar;
	uint32_t optBitmask;
	//mouse/keyboard state
	asciiBool keyboardState [ASCII_KEYCOUNT];
	asciiMouseKey mouseKeyState;
	asciiPoint mousePosition;
	//callbacks
	asciiKeyEventCallback keyEventCallback;
	void* keyEventCallbackContext;
	asciiMouseKeyEventCallback mouseKeyEventCallback;
	void* mouseKeyEventCallbackContext;
	asciiMouseMoveEventCallback mouseMoveEventCallback;
	void* mouseMoveEventCallbackContext;
	asciiQuitCallback quitCallback;
	void* quitCallbackContext;
	asciiTimerData timers[ASCII_MAX_TIMER];
};
extern const asciiEngine asciiDefaultEngine;

/*
 * INTERN API (called by the backend)
 */
void asciiOnMouseMove (asciiEngine* e,asciiPoint newPos);
void asciiOnMouseDown (asciiEngine* e,asciiMouseKey key);
void asciiOnMouseUp (asciiEngine* e,asciiMouseKey key);
void asciiOnKeyDown (asciiEngine* e,asciiKey key);
void asciiOnKeyUp (asciiEngine* e,asciiKey key);
void asciiQuit (asciiEngine* e);

/*
 * HELPER FUNCTIONS
 */
#if defined _MSC_VER
#define inline __inline
#endif

#ifdef EMSCRIPTEN
#define inline //inline emscripten calls have to be inlined manually :(
#define asciiCopyChar(e,target,source) do { \
	if (!ASCII_IS_CHARACTER_LOCK(e)) \
		(target)->character = (source).character; \
	if (!ASCII_IS_BACKCOLOR_LOCK(e)) \
		(target)->backColor = (source).backColor; \
	if (!ASCII_IS_FORECOLOR_LOCK(e)) \
		(target)->foreColor = (source).foreColor; \
	} while (0)
#else
inline void asciiCopyChar (asciiEngine* e,asciiChar* target,asciiChar source) {
	if (!ASCII_IS_CHARACTER_LOCK(e))
		target->character = source.character;
	if (!ASCII_IS_BACKCOLOR_LOCK(e))
		target->backColor = source.backColor;
	if (!ASCII_IS_FORECOLOR_LOCK(e))
		target->foreColor = source.foreColor;
}

#endif
#endif //ASCII_LIB_INTERN_H