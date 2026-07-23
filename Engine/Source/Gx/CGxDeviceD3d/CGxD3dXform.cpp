#include "CGxDeviceD3d.h"

#include <math.h>

static int isIdent;

void CGxDeviceD3d::XformSetViewport(float minX, float maxX, float minY, float maxY, float minZ, float maxZ) {
  CGxDevice::XformSetViewport(minX, maxX, minY, maxY, minZ, maxZ);

  const NTempest::CRect &window = DeviceCurWindow();
  D3DVIEWPORT9           viewport;
  viewport.X = static_cast<unsigned long>(minX * window.r);
  viewport.Y = static_cast<unsigned long>((1.0f - maxY) * window.b);
  viewport.Width = static_cast<unsigned long>(maxX * window.r - viewport.X);
  viewport.Height = static_cast<unsigned long>((1.0f - minY) * window.b - viewport.Y);
  viewport.MinZ = minZ;
  viewport.MaxZ = maxZ;

  m_d3dDevice->SetViewport(&viewport);
}

void CGxDeviceD3d::XformSetProjection(const NTempest::C44Matrix &matrix) {
  CGxDevice::XformSetProjection(matrix);

  D3DXMATRIX tmp = *reinterpret_cast<const D3DXMATRIX *>(&matrix);
  if (!(fabsf(tmp._34 - 1.0f) < 0.00000023841858f) && !(fabsf(tmp._34) < 0.00000023841858f)) {
    tmp *= 1.0f / tmp._34;
  }

  if (tmp._44 == 0.0f) {
    float zNear = -(tmp._43 / (tmp._33 + 1.0f));
    float zFar = -(tmp._43 / (tmp._33 - 1.0f));

    tmp._33 = zFar / (zFar - zNear);
    tmp._43 = zNear * zFar / (zNear - zFar);
  } else {
    float oneOverA = 1.0f / tmp._33;
    float zNear = (-1.0f - tmp._43) * oneOverA;
    float zFar = (1.0f - tmp._43) * oneOverA;

    tmp._33 = 1.0f / (zFar - zNear);
    tmp._43 = zNear / (zNear - zFar);
  }

  if (!(m_appState.m_masterEnables & (1U << GxMasterEnable_NormalProjection)) && tmp._44 != 1.0f) {
    D3DXMATRIX matProj = tmp;
    D3DXMATRIX shrink(0.2f, 0.0f, 0.0f, 0.0f, 0.0f, 0.2f, 0.0f, 0.0f, 0.0f, 0.0f, 0.2f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);

    for (unsigned int row = 0; row < 4; ++row) {
      for (unsigned int column = 0; column < 4; ++column) {
        tmp.m[row][column] = matProj.m[row][0] * shrink.m[0][column] + matProj.m[row][1] * shrink.m[1][column] +
                             matProj.m[row][2] * shrink.m[2][column] + matProj.m[row][3] * shrink.m[3][column];
      }
    }
  }

  m_d3dDevice->SetTransform(D3DTS_PROJECTION, &tmp);
}

void CGxDeviceD3d::XformSetView(const NTempest::C44Matrix &matrix) {
  CGxDevice::XformSetView(matrix);

  D3DXMATRIX matView = *reinterpret_cast<const D3DXMATRIX *>(&matrix);
  m_d3dDevice->SetTransform(D3DTS_VIEW, &matView);
}

void CGxDeviceD3d::IXformSetWorld() {
  CGxMatrixStack &world = m_xforms[GxXform_World];
  unsigned int    identity = world.m_flags[world.m_level] & CGxMatrixStack::F_Identity;

  if (!isIdent || !identity) {
    D3DXMATRIX matWorld = *reinterpret_cast<D3DXMATRIX *>(&world.m_mtx[world.m_level]);
    m_d3dDevice->SetTransform(D3DTS_WORLD, &matWorld);
  }

  isIdent = identity;
  world.m_dirty = 0;
}

void CGxDeviceD3d::IXformSetTex(unsigned int tmu) {
  ASSERT(tmu < m_caps.m_numTmus);

  int ts = 0;
  GxRsGet(static_cast<EGxRenderState>(GxRs_TextureShader0 + tmu), ts);

  unsigned int ttfBits = D3DTTFF_DISABLE;
  D3DXMATRIX   matTex;

  switch (ts) {
    case GxTS_PassThru:
      matTex = *reinterpret_cast<D3DXMATRIX *>(&m_texGen[tmu].m_mtx[m_texGen[tmu].m_level]);
      m_d3dDevice->SetTransform(static_cast<D3DTRANSFORMSTATETYPE>(D3DTS_TEXTURE0 + tmu), &matTex);
      if (!(m_texGen[tmu].m_flags[m_texGen[tmu].m_level] & CGxMatrixStack::F_Identity)) {
        ttfBits = D3DTTFF_COUNT2;
      }
      break;

    case GxTS_Affine: {
      NTempest::C44Matrix concatMat = m_texGen[tmu].m_mtx[m_texGen[tmu].m_level] * m_xforms[tmu].m_mtx[m_xforms[tmu].m_level];
      matTex = *reinterpret_cast<D3DXMATRIX *>(&concatMat);

      int texGen = 0;
      RsGet(static_cast<EGxRenderState>(GxRs_TexGen0 + tmu), texGen);
      if (texGen == GxTexGen_Disable) {
        matTex._31 = concatMat.d0;
        matTex._32 = concatMat.d1;
      }

      m_d3dDevice->SetTransform(static_cast<D3DTRANSFORMSTATETYPE>(D3DTS_TEXTURE0 + tmu), &matTex);
      ttfBits = D3DTTFF_COUNT2;
      break;
    }

    case GxTS_Proj: {
      NTempest::C44Matrix concatMat = m_texGen[tmu].m_mtx[m_texGen[tmu].m_level] * m_xforms[tmu].m_mtx[m_xforms[tmu].m_level];
      matTex = *reinterpret_cast<D3DXMATRIX *>(&concatMat);
      m_d3dDevice->SetTransform(static_cast<D3DTRANSFORMSTATETYPE>(D3DTS_TEXTURE0 + tmu), &matTex);
      ttfBits = D3DTTFF_COUNT3 | D3DTTFF_PROJECTED;
      break;
    }
  }

  DsSet(static_cast<EDeviceState>(Ds_TssTTF0 + tmu), ttfBits);
  m_xforms[tmu].m_dirty = 0;
  m_texGen[tmu].m_dirty = 0;
}
