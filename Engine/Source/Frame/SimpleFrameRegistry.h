#ifndef ENGINE_SOURCE_FRAME_SIMPLEFRAMEREGISTRY_H
#define ENGINE_SOURCE_FRAME_SIMPLEFRAMEREGISTRY_H

class CSimpleFontString;
class CSimpleFrame;
class CSimpleTexture;

int SimpleFrameRegistryAddEntry(const char *name, CSimpleFrame *object, unsigned int context);
void SimpleFrameRegistryRemoveEntry(const char *name, unsigned int context);
int SimpleFontStringRegistryAddEntry(const char *name, CSimpleFontString *object, unsigned int context);
void SimpleFontStringRegistryRemoveEntry(const char *name, unsigned int context);
int SimpleTextureRegistryAddEntry(const char *name, CSimpleTexture *object, unsigned int context);
void SimpleTextureRegistryRemoveEntry(const char *name, unsigned int context);
CSimpleFrame *SimpleFrameRegistryGetEntry(const char *name, unsigned int context);
CSimpleTexture *SimpleTextureRegistryGetEntry(const char *name, unsigned int context);
CSimpleFontString *SimpleFontStringRegistryGetEntry(const char *name, unsigned int context);
void SimpleFrameRegistryClear();

#endif
