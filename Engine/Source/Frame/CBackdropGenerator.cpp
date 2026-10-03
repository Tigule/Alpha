#include <Base/Base.h>

#include "Frame/CBackdropGenerator.h"

#include "Base/Status.h"
#include "Frame/CSimpleFrame.h"
#include "Frame/CSimpleRender.h"
#include "FrameXML/LoadXML.h"
#include "FrameXML/XMLTree.h"

#include <storm.h>

static const float DEFAULT_CORNER_SIZE = 0.025f;

inline void CBackdropGenerator::SetCornerSize(float size) {
  m_cornerSize = size;
}

inline void CBackdropGenerator::SetBackgroundSize(float size) {
  m_backgroundSize = size;
}

inline void CBackdropGenerator::SetBackgroundInsets(float right, float left, float top, float bottom) {
  m_leftInset = left;
  m_rightInset = right;
  m_topInset = top;
  m_bottomInset = bottom;
}

CBackdropGenerator::CBackdropGenerator()
    : m_backgroundTexture(0),
      m_leftTexture(0),
      m_rightTexture(0),
      m_topTexture(0),
      m_bottomTexture(0),
      m_topLeftTexture(0),
      m_topRightTexture(0),
      m_bottomLeftTexture(0),
      m_bottomRightTexture(0),
      m_pieces(BACKDROPONLY),
      m_tileBackground(0),
      m_cornerSize(DEFAULT_CORNER_SIZE),
      m_backgroundSize(0.0f),
      m_topInset(0.0f),
      m_bottomInset(0.0f),
      m_leftInset(0.0f),
      m_rightInset(0.0f),
      m_color(0ul),
      m_borderColor(0ul) {
  m_color.Set(0xFFFFFFFF);
  m_borderColor.Set(0xFFFFFFFF);
}

void CBackdropGenerator::LoadXML(const XMLNode *node, CStatus *status) {
  LPCSTR bgFile = node->GetAttributeByName("bgFile");
  LPCSTR edgeFile = node->GetAttributeByName("edgeFile");
  LPCSTR tileString = node->GetAttributeByName("tile");
  int    tile = 0;

  if (tileString && tileString[0]) {
    tile = StringToBOOL(tileString);
  }

  m_background = bgFile;
  m_pieces = THEWORKS;
  m_tileBackground = tile;
  m_border = edgeFile;

  for (const XMLNode *child = node->GetChild(); child; child = child->GetSibling()) {
    if (!SStrCmpI(child->GetName(), "TileSize", 0x7FFFFFFF)) {
      float val;

      if (LoadXML_Value(child, val, status)) {
        SetBackgroundSize(val);
      }
    } else if (!SStrCmpI(child->GetName(), "EdgeSize", 0x7FFFFFFF)) {
      float val;

      if (LoadXML_Value(child, val, status)) {
        SetCornerSize(val);
      }
    } else if (!SStrCmpI(child->GetName(), "BackgroundInsets", 0x7FFFFFFF)) {
      float l;
      float r;
      float t;
      float b;

      if (LoadXML_Insets(child, l, r, t, b, status)) {
        SetBackgroundInsets(r, l, t, b);
      }
    } else {
      status->Add(STATUS_WARNING, "Unknown child node in %s element: %s", node->GetName(), child->GetName());
    }
  }
}

void CBackdropGenerator::SetOutput(CSimpleFrame *output) {
  VALIDATEBEGIN;
  VALIDATE(output);
  VALIDATEENDVOID;

  NTempest::C2Vector texCoords[4];

  ASSERT(m_pieces == BACKDROPONLY || m_border[0]);

  if (m_background[0]) {
    CSimpleTexture *texture = NEW(CSimpleTexture)(output, 0, 1);

    m_backgroundTexture = texture;
    texture->SetPoint(FRAMEPOINT_TOPLEFT, output, FRAMEPOINT_TOPLEFT, m_leftInset, -m_topInset, 1);
    texture->SetPoint(FRAMEPOINT_TOPRIGHT, output, FRAMEPOINT_TOPRIGHT, -m_rightInset, -m_topInset, 1);
    texture->SetPoint(FRAMEPOINT_BOTTOMLEFT, output, FRAMEPOINT_BOTTOMLEFT, m_leftInset, m_bottomInset, 1);
    texture->SetPoint(FRAMEPOINT_BOTTOMRIGHT, output, FRAMEPOINT_BOTTOMRIGHT, -m_rightInset, m_topInset, 1);
    texture->SetTexture(m_background, m_tileBackground);
  }

  if (m_pieces & LEFTSIDE) {
    CSimpleTexture *texture = NEW(CSimpleTexture)(output, 1, 1);

    m_leftTexture = texture;
    texture->SetWidth(m_cornerSize);
    texture->SetPoint(FRAMEPOINT_TOPLEFT, output, FRAMEPOINT_TOPLEFT, 0.0f, -m_cornerSize, 1);
    texture->SetPoint(FRAMEPOINT_BOTTOMLEFT, output, FRAMEPOINT_BOTTOMLEFT, 0.0f, m_cornerSize, 1);
    texture->SetTexture(m_border, 1);
  }

  if (m_pieces & RIGHTSIDE) {
    CSimpleTexture *texture = NEW(CSimpleTexture)(output, 1, 1);

    m_rightTexture = texture;
    texture->SetWidth(m_cornerSize);
    texture->SetPoint(FRAMEPOINT_TOPRIGHT, output, FRAMEPOINT_TOPRIGHT, 0.0f, -m_cornerSize, 1);
    texture->SetPoint(FRAMEPOINT_BOTTOMRIGHT, output, FRAMEPOINT_BOTTOMRIGHT, 0.0f, m_cornerSize, 1);
    texture->SetTexture(m_border, 1);
  }

  if (m_pieces & TOPSIDE) {
    CSimpleTexture *texture = NEW(CSimpleTexture)(output, 1, 1);

    m_topTexture = texture;
    texture->SetHeight(m_cornerSize);
    texture->SetPoint(FRAMEPOINT_TOPLEFT, output, FRAMEPOINT_TOPLEFT, m_cornerSize, 0.0f, 1);
    texture->SetPoint(FRAMEPOINT_TOPRIGHT, output, FRAMEPOINT_TOPRIGHT, -m_cornerSize, 0.0f, 1);
    texture->SetTexture(m_border, 1);
  }

  if (m_pieces & BOTTOMSIDE) {
    CSimpleTexture *texture = NEW(CSimpleTexture)(output, 1, 1);

    m_bottomTexture = texture;
    texture->SetHeight(m_cornerSize);
    texture->SetPoint(FRAMEPOINT_BOTTOMLEFT, output, FRAMEPOINT_BOTTOMLEFT, m_cornerSize, 0.0f, 1);
    texture->SetPoint(FRAMEPOINT_BOTTOMRIGHT, output, FRAMEPOINT_BOTTOMRIGHT, -m_cornerSize, 0.0f, 1);
    texture->SetTexture(m_border, 1);
  }

  if (m_pieces & TOPLEFTCORNER) {
    CSimpleTexture *texture = NEW(CSimpleTexture)(output, 1, 1);

    m_topLeftTexture = texture;
    texture->SetWidth(m_cornerSize);
    texture->SetHeight(m_cornerSize);
    texture->SetPoint(FRAMEPOINT_TOPLEFT, output, FRAMEPOINT_TOPLEFT, 0.0f, 0.0f, 1);
    texCoords[0] = NTempest::C2Vector(0.500f, 0.0f);
    texCoords[1] = NTempest::C2Vector(0.500f, 1.0f);
    texCoords[2] = NTempest::C2Vector(0.625f, 0.0f);
    texCoords[3] = NTempest::C2Vector(0.625f, 1.0f);
    texture->SetTexture(m_border, 0);
    texture->SetTexCoord(texCoords);
  }

  if (m_pieces & TOPRIGHTCORNER) {
    CSimpleTexture *texture = NEW(CSimpleTexture)(output, 1, 1);

    m_topRightTexture = texture;
    texture->SetWidth(m_cornerSize);
    texture->SetHeight(m_cornerSize);
    texture->SetPoint(FRAMEPOINT_TOPRIGHT, output, FRAMEPOINT_TOPRIGHT, 0.0f, 0.0f, 1);
    texCoords[0] = NTempest::C2Vector(0.625f, 0.0f);
    texCoords[1] = NTempest::C2Vector(0.625f, 1.0f);
    texCoords[2] = NTempest::C2Vector(0.750f, 0.0f);
    texCoords[3] = NTempest::C2Vector(0.750f, 1.0f);
    texture->SetTexture(m_border, 0);
    texture->SetTexCoord(texCoords);
  }

  if (m_pieces & BOTTOMLEFTCORNER) {
    CSimpleTexture *texture = NEW(CSimpleTexture)(output, 1, 1);

    m_bottomLeftTexture = texture;
    texture->SetWidth(m_cornerSize);
    texture->SetHeight(m_cornerSize);
    texture->SetPoint(FRAMEPOINT_BOTTOMLEFT, output, FRAMEPOINT_BOTTOMLEFT, 0.0f, 0.0f, 1);
    texCoords[0] = NTempest::C2Vector(0.750f, 0.0f);
    texCoords[1] = NTempest::C2Vector(0.750f, 1.0f);
    texCoords[2] = NTempest::C2Vector(0.875f, 0.0f);
    texCoords[3] = NTempest::C2Vector(0.875f, 1.0f);
    texture->SetTexture(m_border, 0);
    texture->SetTexCoord(texCoords);
  }

  if (m_pieces & BOTTOMRIGHTCORNER) {
    CSimpleTexture *texture = NEW(CSimpleTexture)(output, 1, 1);

    m_bottomRightTexture = texture;
    texture->SetWidth(m_cornerSize);
    texture->SetHeight(m_cornerSize);
    texture->SetPoint(FRAMEPOINT_BOTTOMRIGHT, output, FRAMEPOINT_BOTTOMRIGHT, 0.0f, 0.0f, 1);
    texCoords[0] = NTempest::C2Vector(0.875f, 0.0f);
    texCoords[1] = NTempest::C2Vector(0.875f, 1.0f);
    texCoords[2] = NTempest::C2Vector(1.000f, 0.0f);
    texCoords[3] = NTempest::C2Vector(1.000f, 1.0f);
    texture->SetTexture(m_border, 0);
    texture->SetTexCoord(texCoords);
  }
}

void CBackdropGenerator::Generate(const NTempest::CRect *rect) {
  VALIDATEBEGIN;
  VALIDATE(rect);
  VALIDATEENDVOID;

  NTempest::C2Vector texCoords[4];
  float              ooCornerSize = 1.0f / m_cornerSize;
  float              xSideRepeats = (rect->r - rect->l) * ooCornerSize - 2.0f;
  float              ySideRepeats = (rect->b - rect->t) * ooCornerSize - 2.0f;

  if (xSideRepeats < 0.0f) {
    xSideRepeats = 0.0f;
  }

  if (ySideRepeats < 0.0f) {
    ySideRepeats = 0.0f;
  }

  ASSERT(m_pieces == BACKDROPONLY || m_border[0]);

  if (m_background[0] && m_tileBackground) {
    CSimpleTexture *texture = m_backgroundTexture;

    ASSERT(texture);

    float xRepeats = (rect->r - rect->l) / (m_backgroundSize != 0.0f ? m_backgroundSize : m_cornerSize);
    float yRepeats = (rect->b - rect->t) / (m_backgroundSize != 0.0f ? m_backgroundSize : m_cornerSize);

    texCoords[0] = NTempest::C2Vector(0.0f, 0.0f);
    texCoords[1] = NTempest::C2Vector(0.0f, yRepeats);
    texCoords[2] = NTempest::C2Vector(xRepeats, 0.0f);
    texCoords[3] = NTempest::C2Vector(xRepeats, yRepeats);
    texture->SetTexCoord(texCoords);
  }

  if (m_pieces & LEFTSIDE) {
    CSimpleTexture *texture = m_leftTexture;

    ASSERT(texture);

    texCoords[0] = NTempest::C2Vector(0.0f, 0.0f);
    texCoords[1] = NTempest::C2Vector(0.0f, ySideRepeats);
    texCoords[2] = NTempest::C2Vector(0.125f, 0.0f);
    texCoords[3] = NTempest::C2Vector(0.125f, ySideRepeats);
    texture->SetTexCoord(texCoords);
  }

  if (m_pieces & RIGHTSIDE) {
    CSimpleTexture *texture = m_rightTexture;

    ASSERT(texture);

    texCoords[0] = NTempest::C2Vector(0.125f, 0.0f);
    texCoords[1] = NTempest::C2Vector(0.125f, ySideRepeats);
    texCoords[2] = NTempest::C2Vector(0.250f, 0.0f);
    texCoords[3] = NTempest::C2Vector(0.250f, ySideRepeats);
    texture->SetTexCoord(texCoords);
  }

  if (m_pieces & TOPSIDE) {
    CSimpleTexture *texture = m_topTexture;

    ASSERT(texture);

    texCoords[0] = NTempest::C2Vector(0.250f, xSideRepeats);
    texCoords[1] = NTempest::C2Vector(0.375f, xSideRepeats);
    texCoords[2] = NTempest::C2Vector(0.250f, 0.0f);
    texCoords[3] = NTempest::C2Vector(0.375f, 0.0f);
    texture->SetTexCoord(texCoords);
  }

  if (m_pieces & BOTTOMSIDE) {
    CSimpleTexture *texture = m_bottomTexture;

    ASSERT(texture);

    texCoords[0] = NTempest::C2Vector(0.375f, xSideRepeats);
    texCoords[1] = NTempest::C2Vector(0.500f, xSideRepeats);
    texCoords[2] = NTempest::C2Vector(0.375f, 0.0f);
    texCoords[3] = NTempest::C2Vector(0.500f, 0.0f);
    texture->SetTexCoord(texCoords);
  }

  SetVertexColor(m_color);
  SetBorderVertexColor(m_borderColor);
}

void CBackdropGenerator::SetVertexColor(const NTempest::CImVector &color) {
  m_color.a = color.a;
  m_color.r = color.r;
  m_color.g = color.g;
  m_color.b = color.b;

  if (m_backgroundTexture) {
    m_backgroundTexture->SetVertexColor(color);
  }
}

void CBackdropGenerator::GetVertexColor(NTempest::CImVector &color) const {
  color.a = m_color.a;
  color.r = m_color.r;
  color.g = m_color.g;
  color.b = m_color.b;
}

void CBackdropGenerator::SetBorderVertexColor(const NTempest::CImVector &color) {
  m_borderColor.a = color.a;
  m_borderColor.r = color.r;
  m_borderColor.g = color.g;
  m_borderColor.b = color.b;

  if (m_leftTexture) {
    m_leftTexture->SetVertexColor(color);
  }

  if (m_rightTexture) {
    m_rightTexture->SetVertexColor(color);
  }

  if (m_topTexture) {
    m_topTexture->SetVertexColor(color);
  }

  if (m_bottomTexture) {
    m_bottomTexture->SetVertexColor(color);
  }

  if (m_topLeftTexture) {
    m_topLeftTexture->SetVertexColor(color);
  }

  if (m_topRightTexture) {
    m_topRightTexture->SetVertexColor(color);
  }

  if (m_bottomLeftTexture) {
    m_bottomLeftTexture->SetVertexColor(color);
  }

  if (m_bottomRightTexture) {
    m_bottomRightTexture->SetVertexColor(color);
  }
}

void CBackdropGenerator::GetBorderVertexColor(NTempest::CImVector &color) const {
  color.a = m_borderColor.a;
  color.r = m_borderColor.r;
  color.g = m_borderColor.g;
  color.b = m_borderColor.b;
}
