#ifndef ENGINE_SOURCE_FRAME_SIMPLEFRAMEREGISTRY_H
#define ENGINE_SOURCE_FRAME_SIMPLEFRAMEREGISTRY_H

class CSimpleFontString;
class CSimpleFrame;
class CSimpleTexture;

int                SimpleFrameRegistryAddEntry(LPCSTR name, CSimpleFrame *object, UINT context);
void               SimpleFrameRegistryRemoveEntry(LPCSTR name, UINT context);
int                SimpleFontStringRegistryAddEntry(LPCSTR name, CSimpleFontString *object, UINT context);
void               SimpleFontStringRegistryRemoveEntry(LPCSTR name, UINT context);
int                SimpleTextureRegistryAddEntry(LPCSTR name, CSimpleTexture *object, UINT context);
void               SimpleTextureRegistryRemoveEntry(LPCSTR name, UINT context);
CSimpleFrame      *SimpleFrameRegistryGetEntry(LPCSTR name, UINT context);
CSimpleTexture    *SimpleTextureRegistryGetEntry(LPCSTR name, UINT context);
CSimpleFontString *SimpleFontStringRegistryGetEntry(LPCSTR name, UINT context);
void               SimpleFrameRegistryClear();

#endif
