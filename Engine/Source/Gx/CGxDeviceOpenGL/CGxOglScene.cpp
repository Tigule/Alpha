#include "CGxDeviceOpenGl.h"

#include <gl/gl.h>

void CGxDeviceOpenGl::ISceneBegin(unsigned int mask) {
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

void CGxDeviceOpenGl::SceneClear(unsigned int mask) {
  NTempest::CImVector clearColor = m_clearColor;
  if (!(m_appState.m_masterEnables & (1U << GxMasterEnable_NormalProjection))) {
    clearColor.Set(0xFFFF0000);
  }

  const float byteToFloat = 1.0f / 255.0f;
  glClearColor(clearColor.r * byteToFloat, clearColor.g * byteToFloat, clearColor.b * byteToFloat, clearColor.a * byteToFloat);

  unsigned int oldDepthMask = m_deviceState[Ds_DepthMask];
  DsSet(Ds_DepthMask, 1, 0);

  unsigned int glMask = 0;
  if (mask & 1) {
    glMask = GL_COLOR_BUFFER_BIT;
  }
  if (mask & 2) {
    glMask |= GL_DEPTH_BUFFER_BIT;
  }
  glClear(glMask);

  DsSet(Ds_DepthMask, oldDepthMask, 0);
}

void CGxDeviceOpenGl::ScenePresent(unsigned int mask) {
  int screenShot = m_scrShotClick;
  CGxDevice::ScenePresent(mask);

  if (m_hwState.m_masterEnables & (1U << GxMasterEnable_DoubleBuffering)) {
    wglSwapLayerBuffers(m_hdc, WGL_SWAP_MAIN_PLANE);
  } else {
    glFinish();
  }

  GetError();
  if (screenShot) {
    DeviceScreenShot();
  }
  ISceneBegin(mask);
}
