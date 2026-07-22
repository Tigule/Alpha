#include "CGxDeviceD3d.h"

void CGxDeviceD3d::CapsWindowSize(NTempest::CRect &dst) {
  dst = DeviceCurWindow();
}

void CGxDeviceD3d::CapsWindowSizeInScreenCoords(NTempest::CRect &dst) {
  if (IDevIsWindowed()) {
    const NTempest::CRect &windowRect = DeviceCurWindow();
    RECT                   window = {0, 0, static_cast<LONG>(windowRect.r), static_cast<LONG>(windowRect.b)};
    MapWindowPoints(m_hwnd, 0, reinterpret_cast<LPPOINT>(&window), 2);
    dst.t = static_cast<float>(window.top);
    dst.l = static_cast<float>(window.left);
    dst.b = static_cast<float>(window.bottom);
    dst.r = static_cast<float>(window.right);
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
