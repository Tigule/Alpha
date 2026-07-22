#include "CGxDeviceOpenGl.h"

#include <gl/gl.h>

void CGxDeviceOpenGl::IXformSetProjection(const NTempest::C44Matrix &m) {
  NTempest::C44Matrix glMat(m.a0, m.a1, m.a2, m.a3, m.b0, m.b1, m.b2, m.b3, -m.c0, -m.c1, -m.c2, -m.c3, m.d0, m.d1, m.d2, m.d3);

  if (!(m_appState.m_masterEnables & (1U << GxMasterEnable_NormalProjection)) && m.d3 != 1.0f) {
    NTempest::C44Matrix shrink(0.2f, 0.0f, 0.0f, 0.0f, 0.0f, 0.2f, 0.0f, 0.0f, 0.0f, 0.0f, 0.2f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    glMat = glMat * shrink;
  }

  if ((m_textureTarget[0].m_apiSpecific || m_textureTarget[1].m_apiSpecific) && !GxCaps().m_rttOriginUpperLeft) {
    glMat.b0 = -glMat.b0;
    glMat.b1 = -glMat.b1;
    glMat.b2 = -glMat.b2;
    glMat.b3 = -glMat.b3;
  }

  DsSet(Ds_MatrixMode, GL_PROJECTION, 0);
  glLoadMatrixf(&glMat.a0);
}

void CGxDeviceOpenGl::IXformGLModelView(const NTempest::C44Matrix &gxm, NTempest::C44Matrix &oglm) {
  oglm.a0 = gxm.a0;
  oglm.a1 = gxm.a1;
  oglm.a2 = -gxm.a2;
  oglm.a3 = gxm.a3;
  oglm.b0 = gxm.b0;
  oglm.b1 = gxm.b1;
  oglm.b2 = -gxm.b2;
  oglm.b3 = gxm.b3;
  oglm.c0 = gxm.c0;
  oglm.c1 = gxm.c1;
  oglm.c2 = -gxm.c2;
  oglm.c3 = gxm.c3;
  oglm.d0 = gxm.d0;
  oglm.d1 = gxm.d1;
  oglm.d2 = -gxm.d2;
  oglm.d3 = gxm.d3;
}

void CGxDeviceOpenGl::IXformSetModelView(const NTempest::C44Matrix &m) {
  NTempest::C44Matrix glMat;
  IXformGLModelView(m, glMat);
  DsSet(Ds_MatrixMode, GL_MODELVIEW, 0);
  glLoadMatrixf(&glMat.a0);
}

void CGxDeviceOpenGl::XformSetViewport(float minX, float maxX, float minY, float maxY, float minZ, float maxZ) {
  CGxDevice::XformSetViewport(minX, maxX, minY, maxY, minZ, maxZ);

  const NTempest::CRect &cr = DeviceCurWindow();
  int                    x = static_cast<int>(minX * cr.r);
  int                    y = static_cast<int>(minY * cr.b);
  int                    right = static_cast<int>(maxX * cr.r);
  int                    bottom = static_cast<int>(maxY * cr.b);
  int                    width = right - x;
  int                    height = bottom - y;

  glViewport(x, y, width, height);
  glDepthRange(minZ, maxZ);
  glScissor(x, y, width, height);
}

void CGxDeviceOpenGl::XformSetProjection(const NTempest::C44Matrix &matrix) {
  CGxDevice::XformSetProjection(matrix);
  IXformSetProjection(matrix);
}

void CGxDeviceOpenGl::XformSetView(const NTempest::C44Matrix &matrix) {
  CGxDevice::XformSetView(matrix);
  m_worldViewChange = 1;
}
