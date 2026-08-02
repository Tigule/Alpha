#include "CGxDeviceOpenGl.h"
#include "GlExtSupport.h"

#include <Images/blit.h>

#include <gl/gl.h>

unsigned int CGxDeviceOpenGl::s_convertMinFilterToOgl[5] = {
    GL_NEAREST, GL_LINEAR, GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR_MIPMAP_LINEAR
};
unsigned int CGxDeviceOpenGl::s_convertMagFilterToOgl[5] = {GL_NEAREST, GL_LINEAR, GL_LINEAR, GL_LINEAR, GL_LINEAR};
int          CGxDeviceOpenGl::s_convertTexFmt[8] = {
    0,
    GL_RGBA8_EXT,
    GL_RGBA4_EXT,
    GL_RGB5_A1_EXT,
    GL_RGB5_EXT,
    GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,
    GL_COMPRESSED_RGBA_S3TC_DXT3_EXT,
    GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
};
unsigned int CGxDeviceOpenGl::s_dataFormatSize[8] = {0, 4, 2, 2, 2, 2, 0, 0};
int          CGxDeviceOpenGl::s_convertDataFmt[8] = {0, GL_BGRA_EXT, GL_BGRA_EXT, GL_BGRA_EXT, GL_RGB, 0, 0, 0};
int          CGxDeviceOpenGl::s_convertDataType[8] = {
    0, GL_UNSIGNED_INT_8_8_8_8_REV_EXT, GL_UNSIGNED_SHORT_4_4_4_4_REV_EXT, GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT, GL_UNSIGNED_SHORT_5_6_5_EXT, 0, 0, 0
};

static TSGrowableArray<unsigned char> scratchTexels;
static CGxTex                        *tex;
static NTempest::CiRect               emptyRect;

void CGxDeviceOpenGl::BindTexture(CGxTex *texId, unsigned int tmu) {
  if (tmu == -1) {
    tmu = 0;
    DsSet(Ds_ActiveTexture, 0, 0);
    IRsForceUpdate(GxRs_Texture0);
  }

  ASSERT(DsGet(Ds_ActiveTexture) == tmu);
  glBindTexture(GL_TEXTURE_2D, reinterpret_cast<unsigned int>(texId->m_apiSpecificData));
}

int CGxDeviceOpenGl::TexCreate(
    unsigned int width,
    unsigned int height,
    EGxTexFormat format,
    CGxTexFlags  flags,
    void        *userArg,
    void(*userFunc)(EGxTexCommand, unsigned int, unsigned int, unsigned int, unsigned int, void *, unsigned int &, const void *&),
    CGxTex *&texId
) {
  return CGxDevice::TexCreate(width, height, format, flags, userArg, userFunc, texId);
}

void CGxDeviceOpenGl::TexDestroy(CGxTex *texId) {
  if (texId->m_apiSpecificData) {
    glDeleteTextures(1, reinterpret_cast<GLuint *>(&texId->m_apiSpecificData));
  }

  CGxDevice::TexDestroy(texId);
}

void CGxDeviceOpenGl::ITexSetFlags(CGxTex *texId) {
  FATALASSERT(texId->m_needsFlagUpdate);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, s_convertMagFilterToOgl[texId->m_flags.m_filter]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, s_convertMinFilterToOgl[texId->m_flags.m_filter]);

  if (glExtClampToEdge) {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, texId->m_flags.m_wrapU ? GL_REPEAT : GL_CLAMP_TO_EDGE_EXT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, texId->m_flags.m_wrapV ? GL_REPEAT : GL_CLAMP_TO_EDGE_EXT);
  } else {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, texId->m_flags.m_wrapU ? GL_REPEAT : GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, texId->m_flags.m_wrapV ? GL_REPEAT : GL_CLAMP);
  }

  if (glSGISGenerateMipmap) {
    glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP_SGIS, texId->m_flags.m_generateMipMaps);
  }
  if (glExtTextureFilterAnisotropic) {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, texId->m_flags.m_maxAnisotropy);
  }

  texId->m_needsFlagUpdate = 0;
}

void CGxDeviceOpenGl::ITexDownload(
    CGxTex      *texId,
    unsigned int w,
    unsigned int h,
    unsigned int startLevel,
    unsigned int oglBase,
    unsigned int texelStrideInBytes,
    const void  *texels
) {
  NTempest::CiRect r;
  EGxTexFormat     gxDataFmt;
  unsigned int     dataFmt;
  unsigned int     intFmt;
  unsigned int     recth;
  unsigned int     dxtw;
  unsigned int     dataType;

  r.l = texId->m_updateRect.l >> startLevel;
  r.t = texId->m_updateRect.t >> startLevel;
  r.r = (texId->m_updateRect.r >> startLevel) + 1;
  r.b = (texId->m_updateRect.b >> startLevel) + 1;
  if (r.r > static_cast<int>(w)) {
    r.r = w;
  }
  if (r.b > static_cast<int>(h)) {
    r.b = h;
  }

  w = r.r - r.l;
  recth = r.b - r.t;
  intFmt = s_convertTexFmt[texId->m_format];
  gxDataFmt = texId->m_dataFormat;

  switch (texId->m_format) {
    case GxTex_Argb8888:
    case GxTex_Argb4444:
    case GxTex_Argb1555:
    case GxTex_Rgb565: {
      const unsigned char *uploadTexels = static_cast<const unsigned char *>(texels);
      if (gxDataFmt == GxTex_Dxt1 || gxDataFmt == GxTex_Dxt3 || gxDataFmt == GxTex_Dxt5) {
        scratchTexels.SetCount((w * 16 * recth) >> 3);

        const unsigned char *src = uploadTexels + ((r.l * s_texFormatBitDepth[gxDataFmt]) >> 3) + texelStrideInBytes * r.t;
        Blit(
            NTempest::C2iVector(w, recth), BlitAlpha_0, src, texelStrideInBytes, GxGetBlitFormat(gxDataFmt), scratchTexels.Ptr(),
            CalcRowStride(GxGetBlitFormat(GxTex_Rgb565), w), GxGetBlitFormat(GxTex_Rgb565)
        );
        uploadTexels = scratchTexels.Ptr();
        gxDataFmt = GxTex_Rgb565;
      }

      dataFmt = s_convertDataFmt[gxDataFmt];
      dataType = s_convertDataType[gxDataFmt];
      glPixelStorei(GL_UNPACK_ROW_LENGTH, texelStrideInBytes / s_dataFormatSize[gxDataFmt]);

      if (texId->m_needsCreation) {
        if (m_force32BitTextures) {
          intFmt = GL_RGBA8_EXT;
        }
        glTexImage2D(GL_TEXTURE_2D, startLevel - oglBase, intFmt, w, recth, 0, dataFmt, dataType, uploadTexels);
      } else {
        glTexSubImage2D(
            GL_TEXTURE_2D, startLevel - oglBase, r.l, r.t, w, recth, dataFmt, dataType,
            uploadTexels + r.l * s_dataFormatSize[gxDataFmt] + texelStrideInBytes * r.t
        );
      }
      break;
    }

    case GxTex_Dxt1:
    case GxTex_Dxt3:
    case GxTex_Dxt5: {
      dxtw = w > 4 ? w : 4;
      h = recth > 4 ? recth : 4;
      if (texId->m_needsCreation) {
        glCompressedTexImage2DARB(
            GL_TEXTURE_2D, startLevel - oglBase, intFmt, w, recth, 0, (dxtw * h * s_texFormatBitDepth[texId->m_format]) >> 3, texels
        );
      } else {
        glCompressedTexSubImage2DARB(
            GL_TEXTURE_2D, startLevel - oglBase, r.l, r.t, w, recth, intFmt, (dxtw * h * s_texFormatBitDepth[texId->m_format]) >> 3, texels
        );
      }
      break;
    }

    default:
      FATALASSERT(0);
  }
}

void CGxDeviceOpenGl::ITexMarkAsUpdated(CGxTex *texId) {
  ITexMarkAsUpdated(texId, -1);
}

void CGxDeviceOpenGl::ITexMarkAsUpdated(CGxTex *texId, unsigned int tmu) {
  unsigned int oglBase;
  unsigned int texelStrideInBytes;
  const void  *texels;
  unsigned int w;
  unsigned int endLevel;

  FATALASSERT(texId);

  if (!m_context) {
    return;
  }

  if (texId->m_needsUpdate) {
    w = texId->m_width;
    unsigned int h = texId->m_height;

    if (texId->m_apiSpecificData) {
      BindTexture(texId, tmu);
    } else {
      glGenTextures(1, reinterpret_cast<GLuint *>(&texId->m_apiSpecificData));
      BindTexture(texId, tmu);
      ITexSetFlags(texId);

      if (glSGISTextureLod && (texId->m_format == GxTex_Dxt1 || texId->m_format == GxTex_Dxt3 || texId->m_format == GxTex_Dxt5)) {
        unsigned int shortEdge = w < h ? w : h;
        FATALASSERT(shortEdge >= 4);
        unsigned int maxLod = 0;
        while (shortEdge > 4) {
          shortEdge >>= 1;
          ++maxLod;
        }
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD_SGIS, maxLod);
      }
    }

    if ((texId->m_flags.m_filter >= GxTex_LinearMipNearest && !texId->m_flags.m_generateMipMaps) || texId->m_flags.m_forceMipTracking) {
      unsigned int maxDimension = w > h ? w : h;
      endLevel = 1;
      while (maxDimension != 1) {
        maxDimension >>= 1;
        ++endLevel;
      }

      oglBase = m_baseMipLevel < endLevel - 1 ? m_baseMipLevel : endLevel - 1;
      w >>= oglBase;
      h >>= oglBase;
      if (texId->m_flags.m_forceMipTracking) {
        endLevel = oglBase + 1;
      }
    } else {
      oglBase = 0;
      endLevel = 1;
    }

    texId->m_userFunc(GxTex_Lock, texId->m_width, texId->m_height, 0, 0, texId->m_userArg, texelStrideInBytes, texels);

    unsigned int level = oglBase;
    unsigned int uploadedLevels = 0;
    while (level != endLevel) {
      texId->m_userFunc(GxTex_Latch, w, h, 0, level, texId->m_userArg, texelStrideInBytes, texels);

      if (!texId->m_flags.m_renderTarget) {
        ITexDownload(texId, w, h, level, oglBase, texelStrideInBytes, texels);
      }

      if (uploadedLevels == 0 && texId->m_flags.m_generateMipMaps) {
        break;
      }

      w >>= 1;
      h >>= 1;
      if (!w) {
        w = 1;
      }
      if (!h) {
        h = 1;
      }
      ++level;
      ++uploadedLevels;
    }

    texId->m_userFunc(GxTex_Unlock, texId->m_width, texId->m_height, 0, 0, texId->m_userArg, texelStrideInBytes, texels);
    if (!texId->m_flags.m_renderTarget) {
      texId->m_needsCreation = 0;
    }
  }

  if (texId->m_needsFlagUpdate) {
    ITexSetFlags(texId);
  }
  CGxDevice::ITexMarkAsUpdated(texId);
}

void CGxDeviceOpenGl::ITexForceRecreation() {
  unsigned int ndx = m_textures.Count();

  while (ndx) {
    tex = m_textures[--ndx];
    if (tex->m_apiSpecificData) {
      glDeleteTextures(1, reinterpret_cast<GLuint *>(&tex->m_apiSpecificData));
      tex->m_apiSpecificData = 0;
      tex->m_needsCreation = 1;
      tex->m_needsFlagUpdate = 1;
      TexMarkForUpdate(tex, emptyRect, 0);
    }
  }
}
