#include "LoadXML.h"

#include "Base/Status.h"
#include "Frame/CSimpleRender.h"
#include "XMLTree.h"

#include <storm.h>

int StringToFramePoint(const char *string, FRAMEPOINT &point) {
  struct FRAMEPOINTNAME {
    FRAMEPOINT  point;
    const char *name;
  };

  FRAMEPOINTNAME framePointNames[FRAMEPOINT_NUMPOINTS] = {
      {     FRAMEPOINT_BOTTOM,      "BOTTOM"},
      { FRAMEPOINT_BOTTOMLEFT,  "BOTTOMLEFT"},
      {FRAMEPOINT_BOTTOMRIGHT, "BOTTOMRIGHT"},
      {     FRAMEPOINT_CENTER,      "CENTER"},
      {        FRAMEPOINT_TOP,         "TOP"},
      {   FRAMEPOINT_TOPRIGHT,    "TOPRIGHT"},
      {    FRAMEPOINT_TOPLEFT,     "TOPLEFT"},
      {       FRAMEPOINT_LEFT,        "LEFT"},
      {      FRAMEPOINT_RIGHT,       "RIGHT"}
  };
  unsigned int index;

  for (index = 0; index < FRAMEPOINT_NUMPOINTS; ++index) {
    if (!SStrCmpI(framePointNames[index].name, string, 0x7FFFFFFF)) {
      point = framePointNames[index].point;
      return 1;
    }
  }

  return 0;
}

int StringToDrawLayer(const char *string, unsigned int &drawLayer) {
  struct DRAWLAYERNAME {
    unsigned int drawLayer;
    const char  *name;
  };

  DRAWLAYERNAME drawLayerNames[5] = {
      {0, "BACKGROUND"},
      {1,     "BORDER"},
      {2,    "ARTWORK"},
      {3,    "OVERLAY"},
      {4,  "HIGHLIGHT"}
  };
  unsigned int index;

  for (index = 0; index < 5; ++index) {
    if (!SStrCmpI(drawLayerNames[index].name, string, 0x7FFFFFFF)) {
      drawLayer = drawLayerNames[index].drawLayer;
      return 1;
    }
  }

  return 0;
}

int StringToBlendMode(const char *string, EGxBlend &blendMode) {
  struct BLENDMODENAME {
    EGxBlend    blendMode;
    const char *name;
  };

  BLENDMODENAME blendModeNames[4] = {
      {  GxBlend_Opaque,  "DISABLE"},
      {   GxBlend_Alpha,    "BLEND"},
      {GxBlend_AlphaKey, "ALPHAKEY"},
      {     GxBlend_Add,      "ADD"}
  };
  unsigned int index;

  for (index = 0; index < 4; ++index) {
    if (!SStrCmpI(blendModeNames[index].name, string, 0x7FFFFFFF)) {
      blendMode = blendModeNames[index].blendMode;
      return 1;
    }
  }

  return 0;
}

int StringToJustify(const char *string, unsigned int &justify) {
  struct JUSTIFYNAME {
    unsigned int justify;
    const char  *name;
  };

  JUSTIFYNAME justifyNames[6] = {
      {0x01,   "LEFT"},
      {0x02, "CENTER"},
      {0x04,  "RIGHT"},
      {0x08,    "TOP"},
      {0x10, "MIDDLE"},
      {0x20, "BOTTOM"}
  };
  unsigned int index;

  for (index = 0; index < 6; ++index) {
    if (!SStrCmpI(justifyNames[index].name, string, 0x7FFFFFFF)) {
      justify = justifyNames[index].justify;
      return 1;
    }
  }

  return 0;
}

int StringToBOOL(const char *string) {
  if (string && !SStrCmpI(string, "true", 0x7FFFFFFF)) {
    return 1;
  }

  if (SStrToInt(string)) {
    return 1;
  }

  return 0;
}

int LoadXML_Value(const XMLNode *node, float &value, CStatus *status) {
  const XMLNode *child;
  const char    *attribute;

  value = 0.0f;

  child = node->GetChild();
  if (!child) {
    status->Add(STATUS_WARNING, "No child node in element: %s", node->GetName());
    return 0;
  }

  if (!SStrCmpI(child->GetName(), "AbsValue", 0x7FFFFFFF)) {
    attribute = child->GetAttributeByName("val");
    if (attribute && *attribute) {
      value = SStrToFloat(attribute) * 0.0009765625f * 0.8f;
    }

    return 1;
  }

  if (!SStrCmpI(child->GetName(), "RelValue", 0x7FFFFFFF)) {
    attribute = child->GetAttributeByName("val");
    if (attribute && *attribute) {
      value = SStrToFloat(attribute);
    }

    return 1;
  }

  status->Add(STATUS_WARNING, "Unknown child node in %s element: %s", node->GetName(), child->GetName());
  return 0;
}

int LoadXML_Dimensions(const XMLNode *node, float &width, float &height, CStatus *status) {
  const XMLNode *child;
  const char    *value;

  width = 0.0f;
  height = 0.0f;

  child = node->GetChild();
  if (!child) {
    status->Add(STATUS_WARNING, "No child node in element: %s", node->GetName());
    return 0;
  }

  if (!SStrCmpI(child->GetName(), "AbsDimension", 0x7FFFFFFF)) {
    value = child->GetAttributeByName("x");
    if (value && *value) {
      width = SStrToFloat(value) * 0.0009765625f * 0.8f;
    }

    value = child->GetAttributeByName("y");
    if (value && *value) {
      height = SStrToFloat(value) * 0.0009765625f * 0.8f;
    }

    return 1;
  }

  if (!SStrCmpI(child->GetName(), "RelDimension", 0x7FFFFFFF)) {
    value = child->GetAttributeByName("x");
    if (value && *value) {
      width = SStrToFloat(value);
    }

    value = child->GetAttributeByName("y");
    if (value && *value) {
      height = SStrToFloat(value);
    }

    return 1;
  }

  status->Add(STATUS_WARNING, "Unknown child node in %s element: %s", node->GetName(), child->GetName());
  return 0;
}

int LoadXML_Insets(const XMLNode *node, float &left, float &right, float &top, float &bottom, CStatus *status) {
  const XMLNode *child;
  const char    *attribute;

  left = 0.0f;
  right = 0.0f;
  top = 0.0f;
  bottom = 0.0f;

  child = node->GetChild();
  if (!child) {
    status->Add(STATUS_WARNING, "No child node in element: %s", node->GetName());
    return 0;
  }

  if (!SStrCmpI(child->GetName(), "AbsInset", 0x7FFFFFFF)) {
    attribute = child->GetAttributeByName("right");
    if (attribute && *attribute) {
      right = SStrToFloat(attribute) * 0.0009765625f * 0.8f;
    }

    attribute = child->GetAttributeByName("left");
    if (attribute && *attribute) {
      left = SStrToFloat(attribute) * 0.0009765625f * 0.8f;
    }

    attribute = child->GetAttributeByName("top");
    if (attribute && *attribute) {
      top = SStrToFloat(attribute) * 0.0009765625f * 0.8f;
    }

    attribute = child->GetAttributeByName("bottom");
    if (attribute && *attribute) {
      bottom = SStrToFloat(attribute) * 0.0009765625f * 0.8f;
    }

    return 1;
  }

  if (!SStrCmpI(child->GetName(), "RelInset", 0x7FFFFFFF)) {
    attribute = child->GetAttributeByName("right");
    if (attribute && *attribute) {
      right = SStrToFloat(attribute);
    }

    attribute = child->GetAttributeByName("left");
    if (attribute && *attribute) {
      left = SStrToFloat(attribute);
    }

    attribute = child->GetAttributeByName("top");
    if (attribute && *attribute) {
      top = SStrToFloat(attribute);
    }

    attribute = child->GetAttributeByName("bottom");
    if (attribute && *attribute) {
      bottom = SStrToFloat(attribute);
    }

    return 1;
  }

  status->Add(STATUS_WARNING, "Unknown child node in %s element: %s", node->GetName(), child->GetName());
  return 0;
}

int LoadXML_Color(const XMLNode *node, NTempest::CImVector &color, CStatus *status) {
  const char *attribute;
  float       alpha = 1.0f;
  float       red = 0.0f;
  float       green = 0.0f;
  float       blue = 0.0f;

  attribute = node->GetAttributeByName("r");
  if (attribute && *attribute) {
    red = min(max(SStrToFloat(attribute), 0.0f), 1.0f);
  }

  attribute = node->GetAttributeByName("g");
  if (attribute && *attribute) {
    green = min(max(SStrToFloat(attribute), 0.0f), 1.0f);
  }

  attribute = node->GetAttributeByName("b");
  if (attribute && *attribute) {
    blue = min(max(SStrToFloat(attribute), 0.0f), 1.0f);
  }

  attribute = node->GetAttributeByName("a");
  if (attribute && *attribute) {
    alpha = min(max(SStrToFloat(attribute), 0.0f), 1.0f);
  }

  color.Set(alpha, red, green, blue);
  return 1;
}

CSimpleTexture *LoadXML_Texture(const XMLNode *node, CSimpleFrame *frame, CStatus *status) {
  CSimpleTexture *texture = new (ALLOC(sizeof(CSimpleTexture))) CSimpleTexture(frame, 2, 1);

  texture->PreLoadXML(node, status);
  static_cast<CLayoutFrame *>(texture)->LoadXML(node, status);
  texture->PostLoadXML(node, status);
  return texture;
}

CSimpleFontString *LoadXML_String(const XMLNode *node, CSimpleFrame *frame, CStatus *status) {
  CSimpleFontString *fontString = new (ALLOC(sizeof(CSimpleFontString))) CSimpleFontString(frame, 2, 1);

  fontString->PreLoadXML(node, status);
  static_cast<CLayoutFrame *>(fontString)->LoadXML(node, status);
  fontString->PostLoadXML(node, status);
  return fontString;
}
