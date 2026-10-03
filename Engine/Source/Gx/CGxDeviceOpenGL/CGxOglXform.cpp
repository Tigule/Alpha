#include "CGxDeviceOpenGl.h"

#include <gl/gl.h>

void CGxDeviceOpenGl::IXformSetProjection(const NTempest::C44Matrix &m) {
  NTempest::C44Matrix glMat(m.a0, m.a1, m.a2, m.a3, m.b0, m.b1, m.b2, m.b3, -m.c0, -m.c1, -m.c2, -m.c3, m.d0, m.d1, m.d2, m.d3);

  if (!(m_appState.m_masterEnables & (1U << GxMasterEnable_NormalProjection)) && m.d3 != 1.0f) {
    NTempest::C44Matrix shrink(0.2f, 0.0f, 0.0f, 0.0f, 0.0f, 0.2f, 0.0f, 0.0f, 0.0f, 0.0f, 0.2f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    glMat = glMat * shrink;
  }

  if ((m_textureTarget[0].m_apiSpecific || m_textureTarget[1].m_apiSpecific) && !GxCaps().m_rttOriginUpperLeft) {
    glMat.b0 *= -1.0f;
    glMat.b1 *= -1.0f;
    glMat.b2 *= -1.0f;
    glMat.b3 *= -1.0f;
  }

  DsSet(Ds_MatrixMode, GL_PROJECTION, 0);
  glLoadMatrixf(&glMat.a0);
}

void CGxDeviceOpenGl::IXformGLModelView(const NTempest::C44Matrix &gxm, NTempest::C44Matrix &oglm) {
  oglm = NTempest::C44Matrix(gxm.a0, gxm.a1, -gxm.a2, gxm.a3, gxm.b0, gxm.b1, -gxm.b2, gxm.b3, gxm.c0, gxm.c1, -gxm.c2, gxm.c3, gxm.d0, gxm.d1, -gxm.d2, gxm.d3);
}

void CGxDeviceOpenGl::IXformSetModelView(const NTempest::C44Matrix &m) {
  NTempest::C44Matrix glMat;
  IXformGLModelView(m, glMat);
  DsSet(Ds_MatrixMode, GL_MODELVIEW, 0);
  glLoadMatrixf(&glMat.a0);
}

void CGxDeviceOpenGl::XformSetViewport(float minX, float maxX, float minY, float maxY, float minZ, float maxZ) {
  CGxDevice::XformSetViewport(minX, maxX, minY, maxY, minZ, maxZ);

  const NTempest::CRect &rect = DeviceCurWindow();
  RECT                   cr;
  cr.left = static_cast<int>(minX * rect.r);
  cr.top = static_cast<int>(minY * rect.b);
  cr.right = static_cast<int>(maxX * rect.r);
  cr.bottom = static_cast<int>(maxY * rect.b);

  glViewport(cr.left, cr.top, cr.right - cr.left, cr.bottom - cr.top);
  glDepthRange(minZ, maxZ);
  glScissor(cr.left, cr.top, cr.right - cr.left, cr.bottom - cr.top);
}

void CGxDeviceOpenGl::XformSetProjection(const NTempest::C44Matrix &matrix) {
  CGxDevice::XformSetProjection(matrix);
  IXformSetProjection(matrix);
}

void CGxDeviceOpenGl::XformSetView(const NTempest::C44Matrix &matrix) {
  CGxDevice::XformSetView(matrix);
  m_worldViewChange = 1;
}
