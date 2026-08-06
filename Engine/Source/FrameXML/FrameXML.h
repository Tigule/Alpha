#pragma once

class CStatus;
class CSimpleFrame;
class XMLNode;

typedef CSimpleFrame *(*FRAMEFACTORY)(CSimpleFrame *parent);
typedef void (*FRAMELOADPROGRESSCALLBACK)(int index, int total);

int            FrameXML_GetDebugLevel();
void           FrameXML_SetDebugLevel(int level);
CSimpleFrame  *FrameXML_CreateFrame(const XMLNode *node, CSimpleFrame *parent, CStatus *status);
int            FrameXML_CreateFrames(LPCSTR path, CStatus *status);
const XMLNode *FrameXML_FindHashNode(LPCSTR name);
int            FrameXML_RegisterFactory(LPCSTR type, FRAMEFACTORY factory);
int            FrameXML_RegisterDefault();
void           FrameXML_RegisterLoadProgressCallback(FRAMELOADPROGRESSCALLBACK callback);
void           FrameXML_ClearFactories();
