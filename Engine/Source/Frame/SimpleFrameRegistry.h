#ifndef ENGINE_SOURCE_FRAME_SIMPLEFRAMEREGISTRY_H
#define ENGINE_SOURCE_FRAME_SIMPLEFRAMEREGISTRY_H

class CSimpleFontString;
class CSimpleFrame;
class CSimpleTexture;

int __fastcall                SimpleFrameRegistryAddEntry(const char *name, CSimpleFrame *object, unsigned int context);
void __fastcall               SimpleFrameRegistryRemoveEntry(const char *name, unsigned int context);
int __fastcall                SimpleFontStringRegistryAddEntry(const char *name, CSimpleFontString *object, unsigned int context);
void __fastcall               SimpleFontStringRegistryRemoveEntry(const char *name, unsigned int context);
int __fastcall                SimpleTextureRegistryAddEntry(const char *name, CSimpleTexture *object, unsigned int context);
void __fastcall               SimpleTextureRegistryRemoveEntry(const char *name, unsigned int context);
CSimpleFrame *__fastcall      SimpleFrameRegistryGetEntry(const char *name, unsigned int context);
CSimpleTexture *__fastcall    SimpleTextureRegistryGetEntry(const char *name, unsigned int context);
CSimpleFontString *__fastcall SimpleFontStringRegistryGetEntry(const char *name, unsigned int context);
void __fastcall               SimpleFrameRegistryClear();

#endif
