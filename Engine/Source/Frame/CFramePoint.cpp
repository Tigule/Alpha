#include <Base/Base.h>

#include "Frame/CFramePoint.h"

#include "Frame/CLayoutFrame.h"
#include "Frame/CSimpleFrame.h"
#include "Frame/CSimpleTop.h"
#include "Tempest/crect.h"

const float CFramePoint::UNDEFINED = *(const float *)&INFINITY_ENCODING;

float CFramePointRelative::X(float scale) {
  NTempest::CRect rect;

  if (!m_relative->GetRect(&rect)) {
    return CFramePoint::UNDEFINED;
  }

  if (m_relative->IsAttachmentOrigin()) {
    rect.Offset(-rect.l, 0.0f);
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
    rect.Offset(0.0f, -rect.t);
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
