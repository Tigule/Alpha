#ifndef ENGINE_SOURCE_GX_CGXDEVICEOPENGL_MAC_GLWNDSUPPORTMAC_H
#define ENGINE_SOURCE_GX_CGXDEVICEOPENGL_MAC_GLWNDSUPPORTMAC_H

void *GxMacWindowCreate(int x, int y, int width, int height, int windowed);
void  GxMacWindowDestroy(void *window);
void  GxMacWindowShow(void *window, int windowed, int focus);
void *GxMacWindowContentView(void *window);
void  GxMacWindowContentRectInScreen(void *window, double width, double height, double *l, double *t, double *r, double *b);

void *GxMacContextCreate(void *view, int colorBits, int depthBits, int stencilBits, int vsync);
void  GxMacContextDestroy(void *context);
void  GxMacContextMakeCurrent(void *context);
void  GxMacContextClearCurrent();

#endif
