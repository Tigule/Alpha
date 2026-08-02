#include "Frame/CBackdropGenerator.h"

#include "Base/Status.h"
#include "Frame/CSimpleFrame.h"
#include "Frame/CSimpleRender.h"
#include "FrameXML/LoadXML.h"
#include "FrameXML/XMLTree.h"

#include <storm.h>

static const float DEFAULT_CORNER_SIZE = 0.025f;

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
  const char *bgFile = node->GetAttributeByName("bgFile");
  const char *edgeFile = node->GetAttributeByName("edgeFile");
  const char *tileString = node->GetAttributeByName("tile");
  int         tile = 0;

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
        m_backgroundSize = val;
      }
    } else if (!SStrCmpI(child->GetName(), "EdgeSize", 0x7FFFFFFF)) {
      float val;

      if (LoadXML_Value(child, val, status)) {
        m_cornerSize = val;
      }
    } else if (!SStrCmpI(child->GetName(), "BackgroundInsets", 0x7FFFFFFF)) {
      float l;
      float r;
      float t;
      float b;

      if (LoadXML_Insets(child, l, r, t, b, status)) {
        m_leftInset = l;
        m_rightInset = r;
        m_topInset = t;
        m_bottomInset = b;
      }
    } else {
      status->Add(STATUS_WARNING, "Unknown child node in %s element: %s", node->GetName(), child->GetName());
    }
  }
}

void CBackdropGenerator::SetOutput(CSimpleFrame *output) {
  FATALASSERT(output);

  NTempest::C2Vector texCoords[4];

  ASSERT(m_pieces == BACKDROPONLY || m_border[0]);

  if (m_background[0]) {
    m_backgroundTexture = NEW(CSimpleTexture)(output, 0, 1);
    m_backgroundTexture->SetPoint(FRAMEPOINT_TOPLEFT, output, FRAMEPOINT_TOPLEFT, m_leftInset, -m_topInset, 1);
    m_backgroundTexture->SetPoint(FRAMEPOINT_TOPRIGHT, output, FRAMEPOINT_TOPRIGHT, -m_rightInset, -m_topInset, 1);
    m_backgroundTexture->SetPoint(FRAMEPOINT_BOTTOMLEFT, output, FRAMEPOINT_BOTTOMLEFT, m_leftInset, m_bottomInset, 1);
    m_backgroundTexture->SetPoint(FRAMEPOINT_BOTTOMRIGHT, output, FRAMEPOINT_BOTTOMRIGHT, -m_rightInset, m_topInset, 1);
    m_backgroundTexture->SetTexture(m_background, m_tileBackground);
  }

  if (m_pieces & LEFTSIDE) {
    m_leftTexture = NEW(CSimpleTexture)(output, 1, 1);
    m_leftTexture->SetWidth(m_cornerSize);
    m_leftTexture->SetPoint(FRAMEPOINT_TOPLEFT, output, FRAMEPOINT_TOPLEFT, 0.0f, -m_cornerSize, 1);
    m_leftTexture->SetPoint(FRAMEPOINT_BOTTOMLEFT, output, FRAMEPOINT_BOTTOMLEFT, 0.0f, m_cornerSize, 1);
    m_leftTexture->SetTexture(m_border, 1);
  }

  if (m_pieces & RIGHTSIDE) {
    m_rightTexture = NEW(CSimpleTexture)(output, 1, 1);
    m_rightTexture->SetWidth(m_cornerSize);
    m_rightTexture->SetPoint(FRAMEPOINT_TOPRIGHT, output, FRAMEPOINT_TOPRIGHT, 0.0f, -m_cornerSize, 1);
    m_rightTexture->SetPoint(FRAMEPOINT_BOTTOMRIGHT, output, FRAMEPOINT_BOTTOMRIGHT, 0.0f, m_cornerSize, 1);
    m_rightTexture->SetTexture(m_border, 1);
  }

  if (m_pieces & TOPSIDE) {
    m_topTexture = NEW(CSimpleTexture)(output, 1, 1);
    m_topTexture->SetHeight(m_cornerSize);
    m_topTexture->SetPoint(FRAMEPOINT_TOPLEFT, output, FRAMEPOINT_TOPLEFT, m_cornerSize, 0.0f, 1);
    m_topTexture->SetPoint(FRAMEPOINT_TOPRIGHT, output, FRAMEPOINT_TOPRIGHT, -m_cornerSize, 0.0f, 1);
    m_topTexture->SetTexture(m_border, 1);
  }

  if (m_pieces & BOTTOMSIDE) {
    m_bottomTexture = NEW(CSimpleTexture)(output, 1, 1);
    m_bottomTexture->SetHeight(m_cornerSize);
    m_bottomTexture->SetPoint(FRAMEPOINT_BOTTOMLEFT, output, FRAMEPOINT_BOTTOMLEFT, m_cornerSize, 0.0f, 1);
    m_bottomTexture->SetPoint(FRAMEPOINT_BOTTOMRIGHT, output, FRAMEPOINT_BOTTOMRIGHT, -m_cornerSize, 0.0f, 1);
    m_bottomTexture->SetTexture(m_border, 1);
  }

  if (m_pieces & TOPLEFTCORNER) {
    m_topLeftTexture = NEW(CSimpleTexture)(output, 1, 1);
    m_topLeftTexture->SetWidth(m_cornerSize);
    m_topLeftTexture->SetHeight(m_cornerSize);
    m_topLeftTexture->SetPoint(FRAMEPOINT_TOPLEFT, output, FRAMEPOINT_TOPLEFT, 0.0f, 0.0f, 1);
    texCoords[0] = NTempest::C2Vector(0.500f, 0.0f);
    texCoords[1] = NTempest::C2Vector(0.500f, 1.0f);
    texCoords[2] = NTempest::C2Vector(0.625f, 0.0f);
    texCoords[3] = NTempest::C2Vector(0.625f, 1.0f);
    m_topLeftTexture->SetTexture(m_border, 0);
    m_topLeftTexture->SetTexCoord(texCoords);
  }

  if (m_pieces & TOPRIGHTCORNER) {
    m_topRightTexture = NEW(CSimpleTexture)(output, 1, 1);
    m_topRightTexture->SetWidth(m_cornerSize);
    m_topRightTexture->SetHeight(m_cornerSize);
    m_topRightTexture->SetPoint(FRAMEPOINT_TOPRIGHT, output, FRAMEPOINT_TOPRIGHT, 0.0f, 0.0f, 1);
    texCoords[0] = NTempest::C2Vector(0.625f, 0.0f);
    texCoords[1] = NTempest::C2Vector(0.625f, 1.0f);
    texCoords[2] = NTempest::C2Vector(0.750f, 0.0f);
    texCoords[3] = NTempest::C2Vector(0.750f, 1.0f);
    m_topRightTexture->SetTexture(m_border, 0);
    m_topRightTexture->SetTexCoord(texCoords);
  }

  if (m_pieces & BOTTOMLEFTCORNER) {
    m_bottomLeftTexture = NEW(CSimpleTexture)(output, 1, 1);
    m_bottomLeftTexture->SetWidth(m_cornerSize);
    m_bottomLeftTexture->SetHeight(m_cornerSize);
    m_bottomLeftTexture->SetPoint(FRAMEPOINT_BOTTOMLEFT, output, FRAMEPOINT_BOTTOMLEFT, 0.0f, 0.0f, 1);
    texCoords[0] = NTempest::C2Vector(0.750f, 0.0f);
    texCoords[1] = NTempest::C2Vector(0.750f, 1.0f);
    texCoords[2] = NTempest::C2Vector(0.875f, 0.0f);
    texCoords[3] = NTempest::C2Vector(0.875f, 1.0f);
    m_bottomLeftTexture->SetTexture(m_border, 0);
    m_bottomLeftTexture->SetTexCoord(texCoords);
  }

  if (m_pieces & BOTTOMRIGHTCORNER) {
    m_bottomRightTexture = NEW(CSimpleTexture)(output, 1, 1);
    m_bottomRightTexture->SetWidth(m_cornerSize);
    m_bottomRightTexture->SetHeight(m_cornerSize);
    m_bottomRightTexture->SetPoint(FRAMEPOINT_BOTTOMRIGHT, output, FRAMEPOINT_BOTTOMRIGHT, 0.0f, 0.0f, 1);
    texCoords[0] = NTempest::C2Vector(0.875f, 0.0f);
    texCoords[1] = NTempest::C2Vector(0.875f, 1.0f);
    texCoords[2] = NTempest::C2Vector(1.000f, 0.0f);
    texCoords[3] = NTempest::C2Vector(1.000f, 1.0f);
    m_bottomRightTexture->SetTexture(m_border, 0);
    m_bottomRightTexture->SetTexCoord(texCoords);
  }
}

void CBackdropGenerator::Generate(const NTempest::CRect *rect) {
  FATALASSERT(rect);

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

    float xRepeats = (rect->r - rect->l) / (m_backgroundSize == 0.0f ? m_cornerSize : m_backgroundSize);
    float yRepeats = (rect->b - rect->t) / (m_backgroundSize == 0.0f ? m_cornerSize : m_backgroundSize);

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
  color = m_color;
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
  color = m_borderColor;
}
