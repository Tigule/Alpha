#ifndef ENGINE_SOURCE_FRAME_CBACKDROPGENERATOR_H
#define ENGINE_SOURCE_FRAME_CBACKDROPGENERATOR_H

#include "Base/RCString.h"
#include "Tempest/cimvector.h"

class CStatus;
class CGNamePlateFrame;
class CSimpleFrame;
class CSimpleTexture;
class XMLNode;

namespace NTempest {
  class CRect;
}

class CBackdropGenerator {
 public:
  enum BACKDROPPIECES {
    BACKDROPONLY = 0x00,
    LEFTSIDE = 0x01,
    RIGHTSIDE = 0x02,
    TOPSIDE = 0x04,
    BOTTOMSIDE = 0x08,
    SIDESONLY = 0x0F,
    TOPLEFTCORNER = 0x10,
    TOPRIGHTCORNER = 0x20,
    BOTTOMLEFTCORNER = 0x40,
    BOTTOMRIGHTCORNER = 0x80,
    CORNERSONLY = 0xF0,
    THEWORKS = 0xFF
  };

  CBackdropGenerator();

  void LoadXML(const XMLNode *node, CStatus *status);
  void SetOutput(CSimpleFrame *output);
  void Generate(const NTempest::CRect *rect);
  void SetVertexColor(const NTempest::CImVector &color);
  void GetVertexColor(NTempest::CImVector &color) const;
  void SetBorderVertexColor(const NTempest::CImVector &color);
  void GetBorderVertexColor(NTempest::CImVector &color) const;
  void SetBackdropTextures(const RCStaticString &background, const RCStaticString &border, UINT pieces, int tileBackground);
  void SetBackdropTextures(LPCSTR background, LPCSTR border, UINT pieces, int tileBackground);
  void SetBackgroundInsets(float right, float left, float top, float bottom);
  void SetBackgroundSize(float size);
  void SetCornerSize(float size);

 private:
  friend class CGNamePlateFrame;

  CSimpleTexture     *m_backgroundTexture;
  CSimpleTexture     *m_leftTexture;
  CSimpleTexture     *m_rightTexture;
  CSimpleTexture     *m_topTexture;
  CSimpleTexture     *m_bottomTexture;
  CSimpleTexture     *m_topLeftTexture;
  CSimpleTexture     *m_topRightTexture;
  CSimpleTexture     *m_bottomLeftTexture;
  CSimpleTexture     *m_bottomRightTexture;
  RCStaticString      m_background;
  RCStaticString      m_border;
  UINT                m_pieces;
  int                 m_tileBackground;
  int                 m_blendAll;
  float               m_cornerSize;
  float               m_backgroundSize;
  float               m_topInset;
  float               m_bottomInset;
  float               m_leftInset;
  float               m_rightInset;
  NTempest::CImVector m_color;
  NTempest::CImVector m_borderColor;
};

#endif
