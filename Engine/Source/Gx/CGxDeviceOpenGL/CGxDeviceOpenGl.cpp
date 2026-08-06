#include "CGxDeviceOpenGl.h"
#include "GlExtSupport.h"

#include <gl/gl.h>
#include <gl/glu.h>

static EGxBufWriteFreq freqOrder[3] = {GxBWF_Dynamic, GxBWF_Low, GxBWF_Medium};

CGxDevice *CGxDevice::NewOpenGl() {
  return NEW(CGxDeviceOpenGl);
}

CGxDeviceOpenGl::CGxDeviceOpenGl()
    : m_nvvarMem(0),
      m_nvvarBytes(0),
      m_nvvarNext(0),
      m_bufRealloc(1),
      m_hwnd(0),
      m_ownhwnd(0),
      m_hdc(0),
      m_hglrc(0),
      m_hPbuffer(0),
      m_hPbufferDC(0),
      m_hPbufferRC(0),
      m_primType(GxPrims_Last),
      m_primIndexCount(0),
      m_primIndices(0),
      m_worldViewChange(0) {
  m_api = GxApi_OpenGl;
  m_caps.m_colorFormat = GxCF_rgba;

  for (UINT freq = 0; freq < GxBufWriteFreqs_Last; ++freq) {
    m_vertexBuffer[freq] = 0;
    m_indexBuffer[freq] = 0;
  }

  DsInit();
  m_lockedArrays = 0;
  m_colorSource = Cs_Material;
  m_colorSourceDirty = 0;
}

CGxDeviceOpenGl::~CGxDeviceOpenGl() {
  FreeBuffers();
}

void CGxDeviceOpenGl::DeviceReadPixels(NTempest::CiRect &rect, TSGrowableArray<NTempest::CImVector> &pixels) {
  NTempest::CImVector *src1;

  ClampRectToWindow(rect);

  pixels.SetCount((rect.r - rect.l) * (rect.b - rect.t));
  int width = rect.r - rect.l;

  glReadPixels(rect.l, static_cast<int>(DeviceCurWindow().b) - rect.b, width, rect.b - rect.t, GL_BGRA_EXT, GL_UNSIGNED_BYTE, pixels.Ptr());

  src1 = pixels.Ptr() + width * (rect.b - rect.t - 1);
  for (NTempest::CImVector *src2 = pixels.Ptr(); src2 < src1; src2 += width, src1 -= width) {
    for (int x = 0; x < width; ++x) {
      NTempest::CImVector value = src2[x];
      src2[x] = src1[x];
      src1[x] = value;
    }
  }
}

void CGxDeviceOpenGl::DeviceReadDepths(NTempest::CiRect &rect, TSGrowableArray<float> &depths) {
  float *src1;

  ClampRectToWindow(rect);

  depths.SetCount((rect.r - rect.l) * (rect.b - rect.t));

  glReadPixels(rect.l, static_cast<int>(DeviceCurWindow().b) - rect.b, rect.r - rect.l, rect.b - rect.t, GL_DEPTH_COMPONENT, GL_FLOAT, depths.Ptr());

  src1 = depths.Ptr() + (rect.r - rect.l) * (rect.b - rect.t - 1);
  for (float *src2 = depths.Ptr(); src2 < src1; src2 += rect.r - rect.l, src1 -= rect.r - rect.l) {
    for (int x = 0; x < rect.r - rect.l; ++x) {
      float value = src2[x];
      src2[x] = src1[x];
      src1[x] = value;
    }
  }
}

void CGxDeviceOpenGl::DeviceOverride(EGxOverride override, DWORD value) {
  CGxDevice::DeviceOverride(override, value);

  if (override == GxOverride_PixelShader) {
    ASSERT(value >= CGxPixelShader::Target_nvrc && value <= CGxPixelShader::Target_arbfp1);
    m_caps.m_pixelShaderTarget = static_cast<CGxPixelShader::Target>(value);
  }
}

void CGxDeviceOpenGl::LockArrays(UINT count) {
  if (glExtCVA) {
    glLockArraysEXT(0, count);
    m_lockedArrays = count;
  }
}

void CGxDeviceOpenGl::UnlockArrays() {
  if (glExtCVA && m_lockedArrays) {
    glUnlockArraysEXT();
    m_lockedArrays = 0;
  }
}

void CGxDeviceOpenGl::GetError() {
  for (UINT i = 0;; ++i) {
    GLenum error = glGetError();
    if (error == GL_NO_ERROR && i != 0x100) {
      break;
    }

    Log("CGxDeviceOpenGl::GetError(): %s", gluErrorString(error));
  }
}

void CGxDeviceOpenGl::IAllocVAR() {
  FATALASSERT(m_nvvarMem == 0);

  UINT freqlp;
  UINT order = 3;
  while (order) {
    UINT bytes = 0;
    for (freqlp = 0; freqlp < order; ++freqlp) {
      EGxBufWriteFreq freq = freqOrder[freqlp];
      if (freq == GxBWF_Dynamic) {
        bytes += GxVertexSize(GxVBF_PNCT0T1) << 15;
      } else {
        for (UINT format = 0; format < GxVertexBufferFormats_Last; ++format) {
          bytes += m_VBReserve[freq][format] * GxVertexSize(static_cast<EGxVertexBufferFormat>(format));
        }
      }
    }

    m_nvvarMem = wglAllocateMemoryNV(bytes, 0.0f, 0.0f, 0.5f);
    if (m_nvvarMem) {
      m_nvvarBytes = bytes;
      m_nvvarNext = 0;
      glVertexArrayRangeNV(bytes, m_nvvarMem);
      return;
    }

    --order;
  }
}

void CGxDeviceOpenGl::IFreeVAR() {
  wglFreeMemoryNV(m_nvvarMem);
  m_nvvarMem = 0;
  glVertexArrayRangeNV(0, 0);
}

void CGxDeviceOpenGl::IAllocVertexBufferVAR(EGxBufWriteFreq freq, UINT bytes) {
  if (!m_nvvarMem) {
    IAllocVAR();
  }

  if (bytes && m_nvvarMem) {
    FATALASSERT(m_nvvarNext + bytes <= m_nvvarBytes);
    m_vertexBuffer[freq] = NEW(CGxMemBuffer_VAR)(bytes, static_cast<BYTE *>(m_nvvarMem) + m_nvvarNext);
    m_nvvarNext += bytes;
  }
}

void CGxDeviceOpenGl::AllocVertexBuffer(EGxBufWriteFreq freq, UINT bytes) {
  FATALASSERT(m_vertexBuffer[freq] == 0);
  if (glNVVertexArrayRange) {
    IAllocVertexBufferVAR(freq, bytes);
  }
}

void CGxDeviceOpenGl::AllocIndexBuffer(EGxBufWriteFreq freq, UINT bytes) {
  FATALASSERT(m_indexBuffer[freq] == 0);
}

void CGxDeviceOpenGl::IAllocBuffers() {
  for (UINT freqlp = 0; freqlp < GxBufWriteFreqs_Last; ++freqlp) {
    UINT maxVertices = 0;
    UINT maxIndices = 0;
    for (UINT format = 0; format < GxVertexBufferFormats_Last; ++format) {
      if (m_VBReserve[freqlp][format] > maxVertices) {
        maxVertices = m_VBReserve[freqlp][format];
      }
      if (m_IBReserve[freqlp][format] > maxIndices) {
        maxIndices = m_IBReserve[freqlp][format];
      }
    }

    AllocVertexBuffer(static_cast<EGxBufWriteFreq>(freqlp), maxVertices * GxVertexSize(GxVBF_PNCT0T1));
    AllocIndexBuffer(static_cast<EGxBufWriteFreq>(freqlp), maxIndices * sizeof(WORD));
  }
}

void CGxDeviceOpenGl::AllocBuffers() {
  if (m_bufRealloc) {
    FreeBuffers();
    IAllocBuffers();
    m_bufRealloc = 0;
  }
}

void CGxDeviceOpenGl::FreeVertexBuffer(CGxMemBuffer *&b) {
  if (b) {
    delete b;
    b = 0;
  }
}

void CGxDeviceOpenGl::FreeIndexBuffer(CGxMemBuffer *&b) {
  if (b) {
    delete b;
    b = 0;
  }
}

void CGxDeviceOpenGl::FreeBuffers() {
  for (UINT freq = 0; freq < GxBufWriteFreqs_Last; ++freq) {
    FreeVertexBuffer(m_vertexBuffer[freq]);
    FreeIndexBuffer(m_indexBuffer[freq]);
  }

  if (glNVVertexArrayRange) {
    IFreeVAR();
  }
}

void CGxDeviceOpenGl::BufReserve(EGxBufWriteFreq freq, EGxVertexBufferFormat format, UINT numVertices, UINT numIndices) {
  CGxDevice::BufReserve(freq, format, numVertices, numIndices);
  m_bufRealloc = 1;
}

void CGxDeviceOpenGl::ISetGlCaps() {
  m_caps.m_numTmus = static_cast<UINT>(glExtMultiTextureCount) < 4 ? glExtMultiTextureCount : 4;
  m_caps.m_pixelCenterOnEdge = 1;
  m_caps.m_texelCenterOnEdge = 1;
  m_caps.m_maxTextureSize = 0x200;
  m_caps.m_texFmtDxt = glExtTextureCompressionS3tc;
  m_caps.m_generateMipMaps = glSGISGenerateMipmap;

  m_caps.m_pixelShaderTarget = CGxPixelShader::Target_gx;
  if (glARBFragmentProgram) {
    m_caps.m_pixelShaderTarget = CGxPixelShader::Target_arbfp1;
  } else if (glATIFragmentShader) {
    m_caps.m_pixelShaderTarget = CGxPixelShader::Target_atifs;
  } else if (glNVRegisterCombiners2) {
    if (glNVTextureShader3) {
      m_caps.m_pixelShaderTarget = CGxPixelShader::Target_nvts3;
    } else if (glNVTextureShader2) {
      m_caps.m_pixelShaderTarget = CGxPixelShader::Target_nvts2;
    } else if (glNVTextureShader) {
      m_caps.m_pixelShaderTarget = CGxPixelShader::Target_nvts;
    }
  } else if (glNVRegisterCombiners) {
    m_caps.m_pixelShaderTarget = CGxPixelShader::Target_nvrc;
  }

  m_caps.m_texFilterTrilinear = 1;
  if (glExtTextureFilterAnisotropic) {
    glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, reinterpret_cast<GLint *>(&m_caps.m_maxTexAnisotropy));
    if (m_caps.m_maxTexAnisotropy > 1) {
      m_caps.m_texFilterAnisotropic = 1;
    }
  }

  Log(m_caps);
}

CGxMemBuffer_VAR::CGxMemBuffer_VAR(UINT count, LPVOID mem) : CGxMemBuffer(count), m_mem(mem), m_fence(0) {
  glGenFencesNV(1, &m_fence);
}

void CGxMemBuffer_VAR::Fence() {
  glSetFenceNV(m_fence, GL_ALL_COMPLETED_NV);
  if (!glTestFenceNV(m_fence)) {
    glFinishFenceNV(m_fence);
  }
}

CGxMemBuffer_VAR::~CGxMemBuffer_VAR() {
  glDeleteFencesNV(1, &m_fence);
}

void CGxMemBuffer_VAR::Lock(LPVOID &mem, UINT bytes, UINT base) {
  FATALASSERT(bytes <= m_count);

  if (base == CGxBuf::BASE_NONE) {
    if (m_next + bytes > m_count) {
      Fence();
      m_next = 0;
      InvalidateBufs(CGxBuf::S_VALID, CGxBuf::S_INVALID_DISCARD);
    }

    m_base = m_next;
    m_next += bytes;
    base = m_base;
  }

  mem = static_cast<BYTE *>(m_mem) + base;
}
