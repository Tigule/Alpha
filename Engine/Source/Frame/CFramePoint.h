#ifndef ENGINE_SOURCE_FRAME_CFRAMEPOINT_H
#define ENGINE_SOURCE_FRAME_CFRAMEPOINT_H

#include "Frame/CLayoutFrame.h"
#include "Tempest/c2vector.h"

class CFramePoint {
 public:
  static const float UNDEFINED;

  virtual ~CFramePoint() {
  }
  virtual float         X(float scale) = 0;
  virtual float         Y(float scale) = 0;
  virtual CLayoutFrame *GetRelative() {
    return 0;
  }
};

class CFramePointAbsolute : public CFramePoint {
 private:
  NTempest::C2Vector m_point;

 public:
  CFramePointAbsolute(float x, float y) : m_point(x, y) {
  }

  virtual float X(float scale) {
    return m_point.x;
  }

  virtual float Y(float scale) {
    return m_point.y;
  }
};

class CFramePointRelative : public CFramePoint {
 private:
  CLayoutFrame      *m_relative;
  FRAMEPOINT         m_framePoint;
  NTempest::C2Vector m_offset;

 public:
  CFramePointRelative(CLayoutFrame *relative, FRAMEPOINT framePoint, float offsetX, float offsetY)
      : m_relative(relative), m_framePoint(framePoint), m_offset(offsetX, offsetY) {
  }

  virtual float         X(float scale);
  virtual float         Y(float scale);
  virtual CLayoutFrame *GetRelative() {
    return m_relative;
  }
};

#endif
