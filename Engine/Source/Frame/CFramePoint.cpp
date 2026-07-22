#include "Frame/CFramePoint.h"

#include "Frame/CLayoutFrame.h"
#include "Tempest/crect.h"
const float PI = 3.14159265358979323846f;
const float TWO_PI = PI + PI;
const float OO_TWO_PI = 1.0f / TWO_PI;

const float CFramePoint::UNDEFINED = INFINITY;

float CFramePointRelative::X(float scale) {
  NTempest::CRect rect;

  if (!m_relative->GetRect(&rect)) {
    return CFramePoint::UNDEFINED;
  }

  if (m_relative->IsAttachmentOrigin()) {
    float offset = -rect.l;
    rect.l += offset;
    rect.r += offset;
  }

  switch (m_framePoint) {
    case FRAMEPOINT_TOPLEFT:
    case FRAMEPOINT_LEFT:
    case FRAMEPOINT_BOTTOMLEFT:
      return rect.l + scale * m_offset.x;

    case FRAMEPOINT_TOP:
    case FRAMEPOINT_CENTER:
    case FRAMEPOINT_BOTTOM:
      return (rect.r + rect.l) * 0.5f + scale * m_offset.x;

    case FRAMEPOINT_TOPRIGHT:
    case FRAMEPOINT_RIGHT:
    case FRAMEPOINT_BOTTOMRIGHT:
      return rect.r + scale * m_offset.x;
  }

  return CFramePoint::UNDEFINED;
}

float CFramePointRelative::Y(float scale) {
  NTempest::CRect rect;

  if (!m_relative->GetRect(&rect)) {
    return CFramePoint::UNDEFINED;
  }

  if (m_relative->IsAttachmentOrigin()) {
    float offset = -rect.t;
    rect.t += offset;
    rect.b += offset;
  }

  switch (m_framePoint) {
    case FRAMEPOINT_TOPLEFT:
    case FRAMEPOINT_TOP:
    case FRAMEPOINT_TOPRIGHT:
      return rect.b + scale * m_offset.y;

    case FRAMEPOINT_LEFT:
    case FRAMEPOINT_CENTER:
    case FRAMEPOINT_RIGHT:
      return (rect.b + rect.t) * 0.5f + scale * m_offset.y;

    case FRAMEPOINT_BOTTOMLEFT:
    case FRAMEPOINT_BOTTOM:
    case FRAMEPOINT_BOTTOMRIGHT:
      return rect.t + scale * m_offset.y;
  }

  return CFramePoint::UNDEFINED;
}
