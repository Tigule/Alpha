#include <Base/Base.h>

#include "Frame/CSimpleRender.h"

#include "Frame/CSimpleFrame.h"

#include <storm.h>

CSimpleRegion::CSimpleRegion(CSimpleFrame *frame, unsigned int drawlayer, int show) : m_GxColor(0), m_frame(0), m_drawlayer(0), m_visible(0) {
  m_color_a = 0xFF;
  m_color.r = 0xFF;
  m_color.g = 0xFF;
  m_color.b = 0xFF;

  if (frame) {
    SetFrame(frame, drawlayer, show);
  }
}

CSimpleRegion::~CSimpleRegion() {
  SetFrame(0, 1, 0);
}

CLayoutFrame *CSimpleRegion::GetLayoutParent() {
  return m_frame;
}

void CSimpleRegion::SetVertexColor(const NTempest::CImVector &color) {
  m_color_a = color.a;
  m_color.r = color.r;
  m_color.g = color.g;
  m_color.b = color.b;
  OnGxColorChanged();
}

void CSimpleRegion::GetVertexColor(NTempest::CImVector &color) const {
  color.a = m_color_a;
  color.r = m_color.r;
  color.g = m_color.g;
  color.b = m_color.b;
}

void CSimpleRegion::OnGxColorChanged() {
  const NTempest::CImVector *oldGxColor = m_GxColor;

  if (m_frame) {
    m_color.a = static_cast<unsigned char>(m_color_a * m_frame->GetAlpha() / 255);

    if (m_color.a >= 0xFE && m_color.r == 0xFF && m_color.g == 0xFF && m_color.b == 0xFF) {
      m_GxColor = 0;
    } else {
      m_GxColor = &m_color;
    }

    if (m_GxColor != oldGxColor) {
      OnRegionChanged();
    }
  }
}

void CSimpleRegion::Show() {
  ASSERT(m_frame);

  if (!m_visible) {
    m_frame->AddFrameRegion(this, m_drawlayer);
    m_visible = 1;
  }
}

void CSimpleRegion::Hide() {
  if (m_visible) {
    m_frame->RemoveFrameRegion(this, m_drawlayer);
    m_visible = 0;
  }
}

void CSimpleRegion::SetFrame(CSimpleFrame *frame, unsigned int drawlayer, int show) {
  ASSERT(drawlayer < NUM_SIMPLEFRAME_DRAWLAYERS);

  if (frame) {
    SetDeferredResize(frame->IsResizeDeferred());
  }

  if (m_frame != frame) {
    if (m_frame) {
      Hide();
      m_frame->UnregisterRegion(this);
    }

    m_frame = frame;
    m_drawlayer = drawlayer;
    if (m_frame) {
      m_frame->RegisterRegion(this);
      OnGxColorChanged();
      if (show) {
        Show();
      }
    }
  } else if (m_drawlayer != drawlayer) {
    if (m_visible) {
      Hide();
    }
    m_drawlayer = drawlayer;
    if (show) {
      Show();
    }
  } else if (show != m_visible) {
    if (show) {
      Show();
    } else {
      Hide();
    }
  }
}

void CSimpleRegion::OnRegionChanged() {
  if (m_visible) {
    m_frame->NotifyDrawLayerChanged(m_drawlayer);
  }
}
