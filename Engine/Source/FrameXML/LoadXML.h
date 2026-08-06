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

int                StringToDrawLayer(LPCSTR string, UINT &drawLayer);
int                StringToBlendMode(LPCSTR string, EGxBlend &blendMode);
int                StringToJustify(LPCSTR string, UINT &justify);
int                StringToBOOL(LPCSTR string);
int                LoadXML_Dimensions(const XMLNode *node, float &width, float &height, CStatus *status);
int                LoadXML_Value(const XMLNode *node, float &value, CStatus *status);
int                LoadXML_Insets(const XMLNode *node, float &left, float &right, float &top, float &bottom, CStatus *status);
int                LoadXML_Color(const XMLNode *node, NTempest::CImVector &color, CStatus *status);
CSimpleTexture    *LoadXML_Texture(const XMLNode *node, CSimpleFrame *frame, CStatus *status);
CSimpleFontString *LoadXML_String(const XMLNode *node, CSimpleFrame *frame, CStatus *status);
int                StringToFramePoint(LPCSTR string, FRAMEPOINT &point);

#endif
