#ifndef ENGINE_SOURCE_GX_CGXDEVICEOPENGL_MAC_GLWNDSUPPORTMAC_H
#define ENGINE_SOURCE_GX_CGXDEVICEOPENGL_MAC_GLWNDSUPPORTMAC_H

LPVOID GxMacWindowCreate(int x, int y, int width, int height, int windowed);
void   GxMacWindowDestroy(LPVOID window);
void   GxMacWindowShow(LPVOID window, int windowed, int focus);
LPVOID GxMacWindowContentView(LPVOID window);
void   GxMacWindowContentRectInScreen(LPVOID window, double width, double height, double *l, double *t, double *r, double *b);

LPVOID GxMacContextCreate(LPVOID view, int colorBits, int depthBits, int stencilBits, int vsync);
void   GxMacContextDestroy(LPVOID context);
void   GxMacContextMakeCurrent(LPVOID context);
void   GxMacContextClearCurrent();

#endif
