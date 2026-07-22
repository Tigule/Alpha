#pragma once

class CStatus;
class CSimpleFrame;
class XMLNode;

typedef CSimpleFrame *(__fastcall *FRAMEFACTORY)(CSimpleFrame *parent);
typedef void(__fastcall *FRAMELOADPROGRESSCALLBACK)(int index, int total);

int __fastcall            FrameXML_GetDebugLevel();
void __fastcall           FrameXML_SetDebugLevel(int level);
CSimpleFrame *__fastcall  FrameXML_CreateFrame(const XMLNode *node, CSimpleFrame *parent, CStatus *status);
int __fastcall            FrameXML_CreateFrames(const char *path, CStatus *status);
const XMLNode *__fastcall FrameXML_FindHashNode(const char *name);
int __fastcall            FrameXML_RegisterFactory(const char *type, FRAMEFACTORY factory);
int __fastcall            FrameXML_RegisterDefault();
void __fastcall           FrameXML_RegisterLoadProgressCallback(FRAMELOADPROGRESSCALLBACK callback);
void __fastcall           FrameXML_ClearFactories();
