#include "CGxDeviceD3d.h"

#include <Os/W32/Debugging.h>
#include <unknwn.h>

const EGxTexFormat CGxDeviceD3d::s_tolerableTexFmtMapping[GxTexFormats_Last] = {GxTex_Unknown,  GxTex_Argb4444, GxTex_Argb4444, GxTex_Argb4444,
                                                                                GxTex_Argb4444, GxTex_Dxt1,     GxTex_Dxt3,     GxTex_Dxt5};

const _D3DFORMAT CGxDeviceD3d::s_GxTexFmtToD3dFmt[GxTexFormats_Last] = {D3DFMT_UNKNOWN, D3DFMT_A8R8G8B8, D3DFMT_A4R4G4B4, D3DFMT_A1R5G5B5,
                                                                        D3DFMT_R5G6B5,  D3DFMT_DXT1,     D3DFMT_DXT3,     D3DFMT_DXT5};

const _D3DCUBEMAP_FACES CGxDeviceD3d::s_d3dCubeMapFaces[6] = {D3DCUBEMAP_FACE_POSITIVE_X, D3DCUBEMAP_FACE_NEGATIVE_X, D3DCUBEMAP_FACE_POSITIVE_Y,
                                                              D3DCUBEMAP_FACE_NEGATIVE_Y, D3DCUBEMAP_FACE_POSITIVE_Z, D3DCUBEMAP_FACE_NEGATIVE_Z};

EGxTexFormat CGxDeviceD3d::s_GxTexFmtToUse[GxTexFormats_Last] = {GxTex_Unknown, GxTex_Argb8888, GxTex_Argb4444, GxTex_Argb1555,
                                                                 GxTex_Rgb565,  GxTex_Dxt1,     GxTex_Dxt3,     GxTex_Dxt5};

static NTempest::CiRect emptyRect;
static NTempest::CiRect lockRect;

void CGxDeviceD3d::ITexForceRecreation(int freeTextures) {
  UINT i = m_textures.Count();

  while (i) {
    CGxTex *texture = m_textures[--i];
    if (texture && texture->m_apiSpecificData && (freeTextures || texture->m_flags.m_renderTarget)) {
      static_cast<IUnknown *>(texture->m_apiSpecificData)->Release();
      texture->m_apiSpecificData = 0;
      texture->m_needsCreation = 1;
      TexMarkForUpdate(texture, emptyRect, 0);
    }
  }
}

BOOL CGxDeviceD3d::TexCreate(
    UINT         width,
    UINT         height,
    EGxTexFormat format,
    CGxTexFlags  flags,
    LPVOID       userArg,
    void (*userFunc)(EGxTexCommand, UINT, UINT, UINT, UINT, LPVOID, UINT &, LPCVOID &),
    CGxTex *&texId
) {
  return CGxDevice::TexCreate(width, height, format, flags, userArg, userFunc, texId);
}

BOOL CGxDeviceD3d::TexCreate(
    EGxTexTarget target,
    UINT         width,
    UINT         height,
    UINT         depth,
    EGxTexFormat format,
    EGxTexFormat dataFormat,
    CGxTexFlags  flags,
    LPVOID       userArg,
    void (*userFunc)(EGxTexCommand, UINT, UINT, UINT, UINT, LPVOID, UINT &, LPCVOID &),
    CGxTex *&texId
) {
  return CGxDevice::TexCreate(target, width, height, depth, format, dataFormat, flags, userArg, userFunc, texId);
}

void CGxDeviceD3d::TexDestroy(CGxTex *texId) {
  if (texId->m_apiSpecificData) {
    static_cast<IUnknown *>(texId->m_apiSpecificData)->Release();
  }

  CGxDevice::TexDestroy(texId);
}

void CGxDeviceD3d::ITexCreate(CGxTex *gxTex, UINT w, UINT h, UINT startLevel, UINT endLevel) {
  gxTex->m_format = s_GxTexFmtToUse[gxTex->m_format];

  if (m_force32BitTextures && m_deviceSupports32BitTextures) {
    gxTex->m_format = GxTex_Argb8888;
  }

  DWORD    usage = 0;
  _D3DPOOL pool = D3DPOOL_MANAGED;

  if (gxTex->m_flags.m_renderTarget) {
    usage = D3DUSAGE_RENDERTARGET;
    pool = D3DPOOL_DEFAULT;
  }

  if (gxTex->m_flags.m_generateMipMaps) {
    usage |= D3DUSAGE_AUTOGENMIPMAP;
  }

  ASSERT(ICheckTextureFormat(usage, s_GxTexFmtToD3dFmt[gxTex->m_format]));

  IDirect3DTexture9 *d3dTex;
  long               result = m_d3dDevice->CreateTexture(w, h, endLevel - startLevel, usage, s_GxTexFmtToD3dFmt[gxTex->m_format], pool, &d3dTex, 0);

  if (result < 0) {
    s_GxTexFmtToUse[gxTex->m_format] = s_tolerableTexFmtMapping[gxTex->m_format];
    gxTex->m_format = s_GxTexFmtToUse[gxTex->m_format];

    ASSERT(gxTex->m_format != GxTex_Unknown);
    ASSERT(ICheckTextureFormat(0, s_GxTexFmtToD3dFmt[gxTex->m_format]));

    result = m_d3dDevice->CreateTexture(w, h, endLevel - startLevel, usage, s_GxTexFmtToD3dFmt[gxTex->m_format], pool, &d3dTex, 0);
  }

  if (result >= 0) {
    gxTex->m_apiSpecificData = d3dTex;
  } else {
    DbgPrintf("Gx: (ERROR): Texture creation failure.\n");
  }
}

void CGxDeviceD3d::ITexUpload(CGxTex *texId, UINT w, UINT h, UINT startLevel, UINT endLevel) {
  IDirect3DTexture9 *texD3d = static_cast<IDirect3DTexture9 *>(texId->m_apiSpecificData);
  UINT               texelStrideInBytes;
  LPCVOID            texels;

  texId->m_userFunc(GxTex_Lock, texId->m_width, texId->m_height, 0, 0, texId->m_userArg, texelStrideInBytes, texels);

  if (texId->m_format == GxTex_Dxt1 || texId->m_format == GxTex_Dxt3 || texId->m_format == GxTex_Dxt5) {
    ASSERT(w == (texId->m_width >> startLevel) && h == (texId->m_height >> startLevel));
  }

  UINT d3dBase = startLevel;

  while (startLevel != endLevel) {
    texId->m_userFunc(GxTex_Latch, w, h, 0, startLevel, texId->m_userArg, texelStrideInBytes, texels);

    if (!texId->m_flags.m_renderTarget) {
      IDirect3DSurface9 *texMip;
      if (texD3d->GetSurfaceLevel(startLevel - d3dBase, &texMip) < 0) {
        DbgPrintf("Gx: (ERROR): Unable to get mip level.\n");
        break;
      }

      lockRect = texId->m_updateRect;
      lockRect.l >>= startLevel;
      lockRect.t >>= startLevel;
      lockRect.b = (lockRect.b >> startLevel) + 1;
      lockRect.r = (lockRect.r >> startLevel) + 1;

      if (lockRect.b >= static_cast<long>(h)) {
        lockRect.b = h;
      }
      if (lockRect.r >= static_cast<long>(w)) {
        lockRect.r = w;
      }

      RECT winr;
      winr.left = lockRect.l;
      winr.top = lockRect.t;
      winr.right = lockRect.r;
      winr.bottom = lockRect.b;

      _D3DLOCKED_RECT rect;
      if (texMip->LockRect(&rect, &winr, D3DLOCK_NOSYSLOCK) < 0) {
        DbgPrintf("Gx: (ERROR): Texture lock failure.\n");
        texMip->Release();
        break;
      }

      LPVOID corner = reinterpret_cast<BYTE *>(const_cast<LPVOID>(texels)) + ((lockRect.l * s_texFormatBitDepth[texId->m_dataFormat]) >> 3) +
                      texelStrideInBytes * lockRect.t;

      try {
        Blit(
            NTempest::C2iVector(lockRect.Width(), lockRect.Height()), BlitAlpha_0, corner, texelStrideInBytes, GxGetBlitFormat(texId->m_dataFormat),
            rect.pBits, rect.Pitch, GxGetBlitFormat(texId->m_format)
        );
      } catch (...) {
        OsOutputDebugString("Access violation in Gx silently handled. %s : %d\n", __FILE__, __LINE__);
      }

      texMip->UnlockRect();
      texMip->Release();
    }

    w >>= 1;
    h >>= 1;
    if (w < 1) {
      w = 1;
    }
    if (h < 1) {
      h = 1;
    }
    ++startLevel;
  }

  texId->m_userFunc(GxTex_Unlock, texId->m_width, texId->m_height, 0, 0, texId->m_userArg, texelStrideInBytes, texels);
  texId->m_needsCreation = 0;
}

void CGxDeviceD3d::ITexMarkAsUpdated(CGxTex *texId) {
  ASSERT(texId);

  if (!m_context) {
    return;
  }

  if (texId->m_needsUpdate) {
    UINT w = texId->m_width;
    UINT h = texId->m_height;
    UINT startLevel = 0;
    UINT endLevel = 1;

    if ((texId->m_flags.m_filter >= 2 && !texId->m_flags.m_generateMipMaps) || texId->m_flags.m_forceMipTracking) {
      UINT dimension = w > h ? w : h;

      for (endLevel = 1; dimension != 1; ++endLevel) {
        dimension >>= 1;
      }

      startLevel = m_baseMipLevel;
      if (startLevel >= endLevel - 1) {
        startLevel = endLevel - 1;
      }

      w >>= startLevel;
      h >>= startLevel;

      if (texId->m_flags.m_forceMipTracking) {
        endLevel = startLevel + 1;
      }
    }

    if (!texId->m_apiSpecificData) {
      ITexCreate(texId, w, h, startLevel, endLevel);
    }

    if (texId->m_userFunc) {
      ITexUpload(texId, w, h, startLevel, endLevel);
    }
  }

  CGxDevice::ITexMarkAsUpdated(texId);
}
