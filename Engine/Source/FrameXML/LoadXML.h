#ifndef ENGINE_SOURCE_FRAMEXML_LOADXML_H
#define ENGINE_SOURCE_FRAMEXML_LOADXML_H

#include "Frame/CLayoutFrame.h"
#include "Gx/Gx.h"
#include "Tempest/cimvector.h"

class CStatus;
class CSimpleFontString;
class CSimpleFrame;
class CSimpleTexture;
class XMLNode;

int __fastcall                StringToDrawLayer(const char *string, unsigned int &drawLayer);
int __fastcall                StringToBlendMode(const char *string, EGxBlend &blendMode);
int __fastcall                StringToJustify(const char *string, unsigned int &justify);
int __fastcall                StringToBOOL(const char *string);
int __fastcall                LoadXML_Dimensions(const XMLNode *node, float &width, float &height, CStatus *status);
int __fastcall                LoadXML_Value(const XMLNode *node, float &value, CStatus *status);
int __fastcall                LoadXML_Insets(const XMLNode *node, float &left, float &right, float &top, float &bottom, CStatus *status);
int __fastcall                LoadXML_Color(const XMLNode *node, NTempest::CImVector &color, CStatus *status);
CSimpleTexture *__fastcall    LoadXML_Texture(const XMLNode *node, CSimpleFrame *frame, CStatus *status);
CSimpleFontString *__fastcall LoadXML_String(const XMLNode *node, CSimpleFrame *frame, CStatus *status);
int __fastcall                StringToFramePoint(const char *string, FRAMEPOINT &point);

#endif
