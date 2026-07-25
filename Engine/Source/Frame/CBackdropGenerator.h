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
    BACKDROPLEFT = 0x01,
    BACKDROPRIGHT = 0x02,
    BACKDROPTOP = 0x04,
    BACKDROPBOTTOM = 0x08,
    BACKDROPTOPLEFT = 0x10,
    BACKDROPTOPRIGHT = 0x20,
    BACKDROPBOTTOMLEFT = 0x40,
    BACKDROPBOTTOMRIGHT = 0x80,
    BACKDROPALL = 0xFF
  };

  CBackdropGenerator();
  ~CBackdropGenerator() {
  }

  void LoadXML(const XMLNode *node, CStatus *status);
  void SetOutput(CSimpleFrame *output);
  void Generate(const NTempest::CRect *rect);
  void SetVertexColor(const NTempest::CImVector &color);
  void GetVertexColor(NTempest::CImVector &color) const;
  void SetBorderVertexColor(const NTempest::CImVector &color);
  void GetBorderVertexColor(NTempest::CImVector &color) const;

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
  unsigned int        m_pieces;
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
