#ifndef ENGINE_SOURCE_GX_CGXDEVICEOPENGL_MAC_GLWNDSUPPORTMAC_H
#define ENGINE_SOURCE_GX_CGXDEVICEOPENGL_MAC_GLWNDSUPPORTMAC_H

void  *GxMacWindowCreate(int x, int y, int width, int height, int windowed);
void   GxMacWindowDestroy(void *window);
void   GxMacWindowShow(void *window, int windowed, int focus);
void  *GxMacWindowContentView(void *window);
double GxMacWindowBackingScale(void *window);
void   GxMacGetWindowBounds(void *window, int structure, int *left, int *top, int *right, int *bottom);
void   GxMacSetWindowResizeLimits(void *window, int minWidth, int minHeight);
void   GxMacSetWindowBounds(void *window, int left, int top, int right, int bottom);
void   GxMacSetBufferRect(void *window, int x, int y, int width, int height);
void   GxMacWindowSetFullScreenStyle(void *window, int fullscreen);
void   GxMacGetScreenBounds(int available, int *left, int *top, int *right, int *bottom);
void   GxMacSetSystemUIMode(int hidden);
void   GxMacHideCursor();
void   GxMacInitCursor();

void  *GxMacContextCreate(void *view);
void   GxMacContextDestroy(void *context);
void   GxMacContextMakeCurrent(void *context);
void   GxMacContextClearCurrent();
void   GxMacContextUpdate(void *context);

#endif
