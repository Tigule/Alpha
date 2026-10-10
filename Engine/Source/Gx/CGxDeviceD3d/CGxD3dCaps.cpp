#include "CGxDeviceD3d.h"

inline NTempest::CRect::CRect(const tagRECT &value) {
  t = value.top;
  l = value.left;
  b = value.bottom;
  r = value.right;
}

void CGxDeviceD3d::CapsWindowSize(NTempest::CRect &dst) {
  dst = DeviceCurWindow();
}

void CGxDeviceD3d::CapsWindowSizeInScreenCoords(NTempest::CRect &dst) {
  if (IDevIsWindowed()) {
    const NTempest::CRect &windowRect = DeviceCurWindow();
    RECT                   wrect = {0, 0, windowRect.r, windowRect.b};
    MapWindowPoints(m_hwnd, 0, (LPPOINT)&wrect, 2);
    dst = NTempest::CRect(wrect);
  } else {
    dst = DeviceCurWindow();
  }
}

int CGxDeviceD3d::CapsIsWindowVisible() {
  if (IDevIsWindowed()) {
    if (!m_d3dNeedsReset) {
      return m_windowVisible;
    }

    if (!m_windowVisible) {
      DbgPrintf("%s %d: m_windowVisible\n", __FILE__, __LINE__);
    }
  } else {
    long cooperativeLevel = m_d3dDevice->TestCooperativeLevel();
    if (cooperativeLevel != D3DERR_DEVICENOTRESET) {
      return cooperativeLevel == 0;
    }
  }

  IReleaseD3dResources(0);

  D3DPRESENT_PARAMETERS d3dpp;
  ISetPresentParms(d3dpp, m_format);
  long result = m_d3dDevice->Reset(&d3dpp);
  if (result == 0) {
    IStateSetD3DDefaults();
    IAllocBuffers();
    m_d3dNeedsReset = 0;
    m_context = 1;
  }

  return result == 0;
}
