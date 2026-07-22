#include "CGxDeviceD3d.h"

#include <math.h>

static const float  PI = 3.14159265358979323846f;
static unsigned int t;

void CGxDeviceD3d::ISceneBegin(unsigned int mask) {
  if (m_appState.m_masterEnables & (1U << GxMasterEnable_ClearOnPresent)) {
    float minX;
    float maxX;
    float minY;
    float maxY;
    float minZ;
    float maxZ;

    XformViewport(minX, maxX, minY, maxY, minZ, maxZ);
    XformSetViewport(0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f);
    SceneClear(mask);
    XformSetViewport(minX, maxX, minY, maxY, minZ, maxZ);
  }

  if (m_d3dDevice->BeginScene() == 0) {
    m_inScene = 1;
  }
}

void CGxDeviceD3d::ISceneEnd() {
  if (m_inScene) {
    m_d3dDevice->EndScene();
    m_inScene = 0;
  }
}

void CGxDeviceD3d::SceneClear(unsigned int mask) {
  CGxDevice::SceneClear(mask);

  NTempest::CImVector clearColor = m_clearColor;
  if (!(m_appState.m_masterEnables & (1U << GxMasterEnable_NormalProjection))) {
    const float phase = static_cast<float>(t) * PI * 0.0078125f;
    t = (t + 1) & 0xFF;

    clearColor.Set(
        static_cast<unsigned char>(0xFF), static_cast<unsigned char>((sin(phase * 3.0f) + 1.0f) * 127.5f),
        static_cast<unsigned char>((sin(phase * 5.0f) + 1.0f) * 127.5f), static_cast<unsigned char>((sin(phase * 7.0f) + 1.0f) * 127.5f)
    );
  }

  unsigned long clearMask = 0;
  if (mask & 1) {
    clearMask = 1;
  }
  if (mask & 2) {
    clearMask |= 2;
  }

  m_d3dDevice->Clear(0, 0, clearMask, NTempest::CImVector::MakeARGB(clearColor.a, clearColor.r, clearColor.g, clearColor.b), 1.0f, 0);
}

void CGxDeviceD3d::ScenePresent(unsigned int mask) {
  int screenShot = m_scrShotClick;

  CGxDevice::ScenePresent(mask);
  ISceneEnd();

  if (m_format.fixLag) {
    IDirect3DSurface9 *backBuffer;
    if (m_d3dDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &backBuffer) >= 0) {
      _D3DSURFACE_DESC desc;
      _D3DLOCKED_RECT  lockedRect;

      backBuffer->GetDesc(&desc);
      if (backBuffer->LockRect(&lockedRect, 0, D3DLOCK_READONLY | D3DLOCK_NOSYSLOCK) >= 0) {
        backBuffer->UnlockRect();
      }
      backBuffer->Release();
    }
  }

  if (screenShot) {
    DeviceScreenShot();
  }

  m_d3dDevice->Present(0, 0, 0, 0);
  ISceneBegin(mask);
}
