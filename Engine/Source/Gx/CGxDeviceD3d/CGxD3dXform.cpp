#include "CGxDeviceD3d.h"

#include <math.h>

static BOOL isIdent;

inline UINT CGxMatrixStack::Flags() {
  return m_flags[m_level];
}

void CGxDeviceD3d::XformSetViewport(float minX, float maxX, float minY, float maxY, float minZ, float maxZ) {
  CGxDevice::XformSetViewport(minX, maxX, minY, maxY, minZ, maxZ);

  const NTempest::CRect &window = DeviceCurWindow();
  D3DVIEWPORT9           viewport;
  viewport.X = minX * window.r;
  viewport.Y = (1.0f - maxY) * window.b;
  viewport.Width = maxX * window.r - viewport.X;
  viewport.Height = (1.0f - minY) * window.b - viewport.Y;
  viewport.MinZ = minZ;
  viewport.MaxZ = maxZ;

  m_d3dDevice->SetViewport(&viewport);
}

void CGxDeviceD3d::XformSetProjection(const NTempest::C44Matrix &matrix) {
  CGxDevice::XformSetProjection(matrix);

  D3DXMATRIX matProj = *(const D3DXMATRIX *)&matrix;
  if (!(fabsf(matProj._34 - 1.0f) < 0.00000023841858f) && !(fabsf(matProj._34) < 0.00000023841858f)) {
    matProj *= 1.0f / matProj._34;
  }

  if (matProj._44 == 0.0f) {
    float zNear = -(matProj._43 / (matProj._33 + 1.0f));
    float zFar = -(matProj._43 / (matProj._33 - 1.0f));

    matProj._33 = zFar / (zFar - zNear);
    matProj._43 = zNear * zFar / (zNear - zFar);
  } else {
    float oneOverA = 1.0f / matProj._33;
    float zNear = (-1.0f - matProj._43) * oneOverA;
    float zFar = (1.0f - matProj._43) * oneOverA;

    matProj._33 = 1.0f / (zFar - zNear);
    matProj._43 = zNear / (zNear - zFar);
  }

  if (!(m_appState.m_masterEnables & (1U << GxMasterEnable_NormalProjection)) && matProj._44 != 1.0f) {
    D3DXMATRIX tmp = matProj;
    D3DXMATRIX shrink(0.2f, 0.0f, 0.0f, 0.0f, 0.0f, 0.2f, 0.0f, 0.0f, 0.0f, 0.0f, 0.2f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);

    for (UINT row = 0; row < 4; ++row) {
      for (UINT column = 0; column < 4; ++column) {
        matProj.m[row][column] = tmp.m[row][0] * shrink.m[0][column] + tmp.m[row][1] * shrink.m[1][column] +
                                 tmp.m[row][2] * shrink.m[2][column] + tmp.m[row][3] * shrink.m[3][column];
      }
    }
  }

  m_d3dDevice->SetTransform(D3DTS_PROJECTION, &matProj);
}

void CGxDeviceD3d::XformSetView(const NTempest::C44Matrix &matrix) {
  CGxDevice::XformSetView(matrix);

  D3DXMATRIX matView = *(const D3DXMATRIX *)&matrix;
  m_d3dDevice->SetTransform(D3DTS_VIEW, &matView);
}

void CGxDeviceD3d::IXformSetWorld() {
  CGxMatrixStack &world = m_xforms[GxXform_World];

  if (!isIdent || !(world.m_flags[world.m_level] & CGxMatrixStack::F_Identity)) {
    D3DXMATRIX matWorld = *(D3DXMATRIX *)&world.m_mtx[world.m_level];
    m_d3dDevice->SetTransform(D3DTS_WORLD, &matWorld);
  }

  isIdent = world.m_flags[world.m_level] & CGxMatrixStack::F_Identity;
  world.m_dirty = 0;
}

void CGxDeviceD3d::IXformSetTex(UINT tmu) {
  ASSERT(tmu < m_caps.m_numTmus);

  DWORD ttfBits = D3DTTFF_DISABLE;
  int   ts;
  GxRsGet((EGxRenderState)(GxRs_TextureShader0 + tmu), ts);

  switch (ts) {
    case GxTS_PassThru:
      if (m_texGen[tmu].Flags() & CGxMatrixStack::F_Identity) {
        D3DXMATRIX matTex = *(const D3DXMATRIX *)&m_texGen[tmu].TopConst();
        m_d3dDevice->SetTransform((D3DTRANSFORMSTATETYPE)(D3DTS_TEXTURE0 + tmu), &matTex);
      } else {
        D3DXMATRIX matTex = *(const D3DXMATRIX *)&m_texGen[tmu].TopConst();
        m_d3dDevice->SetTransform((D3DTRANSFORMSTATETYPE)(D3DTS_TEXTURE0 + tmu), &matTex);
        ttfBits = D3DTTFF_COUNT2;
      }
      break;

    case GxTS_Affine: {
      NTempest::C44Matrix concatMat = m_texGen[tmu].TopConst() * m_xforms[tmu].TopConst();
      D3DXMATRIX          matTex = *(D3DXMATRIX *)&concatMat;

      int texGen;
      RsGet((EGxRenderState)(GxRs_TexGen0 + tmu), texGen);
      if (texGen == GxTexGen_Disable) {
        matTex._31 = concatMat.d0;
        matTex._32 = concatMat.d1;
      }

      m_d3dDevice->SetTransform((D3DTRANSFORMSTATETYPE)(D3DTS_TEXTURE0 + tmu), &matTex);
      ttfBits = D3DTTFF_COUNT2;
      break;
    }

    case GxTS_Proj: {
      NTempest::C44Matrix concatMat = m_texGen[tmu].TopConst() * m_xforms[tmu].TopConst();
      D3DXMATRIX          matTex = *(D3DXMATRIX *)&concatMat;
      m_d3dDevice->SetTransform((D3DTRANSFORMSTATETYPE)(D3DTS_TEXTURE0 + tmu), &matTex);
      ttfBits = D3DTTFF_COUNT3 | D3DTTFF_PROJECTED;
      break;
    }
  }

  DsSet((EDeviceState)(Ds_TssTTF0 + tmu), ttfBits);
  m_xforms[tmu].m_dirty = 0;
  m_texGen[tmu].m_dirty = 0;
}
