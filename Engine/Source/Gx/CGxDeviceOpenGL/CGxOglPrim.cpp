#include "CGxDeviceOpenGl.h"
#include "GlExtSupport.h"

#include <Tempest/c34matrix.h>

#include <gl/gl.h>

#include <string.h>

static unsigned int s_primitiveConversion[GxPrims_Last] = {GL_POINTS, GL_LINES, GL_LINE_STRIP, GL_TRIANGLES, GL_TRIANGLE_STRIP, GL_TRIANGLE_FAN};

static unsigned int               s_vertexCount;
static const NTempest::C3Vector  *s_pos;
static unsigned int               s_posStride;
static const NTempest::C3Vector  *s_normal;
static unsigned int               s_normalStride;
static const NTempest::CImVector *s_color;
static unsigned int               s_colorStride;
static const unsigned char       *s_bone;
static unsigned int               s_boneStride;
static const NTempest::C2Vector  *s_tex[4];
static unsigned int               s_texStride[4];

static NTempest::C3Vector  s_genericNormal(0.0f, 1.0f, 0.0f);
static NTempest::C2Vector  s_genericTexCoord(0.0f, 0.0f);
static NTempest::CImVector s_genericColor(0xFFFFFFFF);

class CGxBufOgl : public CGxBuf {
  friend class CGxDeviceOpenGl;

  CGxMemBuffer   *m_vb;
  CGxMemBuffer   *m_ib;
  void           *vertexPtr[GxVertexMembers_Last];
  unsigned short *indexPtr;

 public:
  CGxBufOgl();
  int  LockVB();
  int  LockIB();
  void UnlockVB();
  void UnlockIB();

  void SetVB(CGxMemBuffer *vb) {
    m_vb = vb;
  }

  void SetIB(CGxMemBuffer *ib) {
    m_ib = ib;
  }

  CGxMemBuffer *GetVB() {
    return m_vb;
  }

  CGxMemBuffer *GetIB() {
    return m_ib;
  }
};

CGxBufOgl::CGxBufOgl() : m_vb(0), m_ib(0) {
}

int CGxBufOgl::LockVB() {
  if (!m_vb) {
    memset(vertexPtr, 0, sizeof(vertexPtr));
    return 0;
  }

  void *mem = 0;
  if (m_vertexStatus == S_INVALID_RELOAD) {
    m_vb->Lock(mem, m_numVertices * GxVertexSize(m_vbFormat), m_vertexBase);
  } else if (m_vertexStatus == S_INVALID_DISCARD) {
    m_vb->Lock(mem, m_numVertices * GxVertexSize(m_vbFormat), BASE_NONE);
    m_vertexBase = m_vb->m_base;
  } else {
    FATALASSERT(0);
  }

  for (unsigned int member = 0; member < GxVertexMembers_Last; ++member) {
    int offset = GxVertexMemberOffset(m_vbFormat, static_cast<EGxVertexMember>(member));
    vertexPtr[member] = offset == -1 ? 0 : static_cast<unsigned char *>(mem) + offset;
  }
  return 1;
}

int CGxBufOgl::LockIB() {
  if (m_ib) {
    FATALASSERT(0);
  } else {
    indexPtr = 0;
  }
  return 0;
}

void CGxBufOgl::UnlockVB() {
  if (m_vb) {
    m_vb->Unlock();
  }
}

void CGxBufOgl::UnlockIB() {
  if (m_ib) {
    m_ib->Unlock();
  }
}

CGxBuf *CGxDeviceOpenGl::BufCreate(
    EGxBufWriteFreq       writeFreq,
    EGxVertexBufferFormat format,
    unsigned int          numVertices,
    unsigned int          numIndices,
    void(*userCallback)(CGxBufCommand &, CGxBuf *),
    void *userArg
) {
  CGxDevice::BufCreate(GxBWF_Dynamic, format, numVertices, numIndices, userCallback, userArg);

  CGxBufOgl *buf = NEW(CGxBufOgl);
  buf->m_userCallback = userCallback;
  buf->m_userArg = userArg;
  buf->m_vbFormat = format;
  buf->m_writeFreq = GxBWF_Dynamic;
  buf->m_numVertices = numVertices;
  buf->m_numIndices = numIndices;
  return buf;
}

void CGxDeviceOpenGl::IBufSetBuffers(CGxBufOgl *buf) {
  AllocBuffers();
  if (buf->GetVB() && buf->GetIB()) {
    return;
  }

  EGxBufWriteFreq frequency = buf->m_writeFreq;
  switch (frequency) {
    case GxBWF_Static:
      FATALASSERT(0);
      break;
    case GxBWF_Low:
    case GxBWF_Medium:
      if (!buf->GetVB()) {
        buf->SetVB(m_vertexBuffer[frequency]);
      }
      if (!buf->GetIB()) {
        buf->SetIB(m_indexBuffer[frequency]);
      }
      break;
    case GxBWF_Dynamic:
      break;
    default:
      FATALASSERT(0);
      break;
  }

  if (!buf->GetVB()) {
    buf->SetVB(m_vertexBuffer[GxBWF_Dynamic]);
  }
  if (!buf->GetIB()) {
    buf->SetIB(m_indexBuffer[GxBWF_Dynamic]);
  }
}

void CGxDeviceOpenGl::BufLock(CGxBuf *b) {
  CGxDevice::BufLock(b);

  CGxBufOgl *buf = static_cast<CGxBufOgl *>(b);
  IBufSetBuffers(buf);

  CGxBufCommand cmd;
  cmd.vertex.op = GxBufOp_Nop;
  cmd.index.op = GxBufOp_Nop;

  if (glNVVertexArrayRange) {
    DsSet(Ds_NVVAR, m_nvvarMem != 0, 0);
  }

  if (buf->m_vertexStatus != CGxBuf::S_VALID) {
    cmd.vertex.op = static_cast<EGxBufOp>(2 - buf->LockVB());
    for (unsigned int member = 0; member < GxVertexMembers_Last; ++member) {
      cmd.vertex.mem[member] = &buf->vertexPtr[member];
      cmd.vertex.stride[member] = GxVertexSize(buf->m_vbFormat);
    }
  }

  if (buf->m_indexStatus != CGxBuf::S_VALID) {
    cmd.index.op = static_cast<EGxBufOp>(2 - buf->LockIB());
    cmd.index.mem[GxVM_Indices] = reinterpret_cast<void **>(&buf->indexPtr);
    cmd.index.stride[GxVM_Indices] = sizeof(unsigned short);
  }

  if (cmd.vertex.op != GxBufOp_Nop || cmd.index.op != GxBufOp_Nop) {
    buf->m_userCallback(cmd, b);
  }

  if (buf->m_vertexStatus != CGxBuf::S_VALID) {
    buf->UnlockVB();
    if (buf->m_writeFreq != GxBWF_Dynamic) {
      buf->m_vertexStatus = CGxBuf::S_VALID;
    }
  }
  if (buf->m_indexStatus != CGxBuf::S_VALID) {
    buf->UnlockIB();
    if (buf->m_writeFreq != GxBWF_Dynamic) {
      buf->m_vertexStatus = CGxBuf::S_VALID;
    }
  }

  glVertexPointer(3, GL_FLOAT, cmd.vertex.stride[GxVM_Position], *cmd.vertex.mem[GxVM_Position]);
  IPrimSetupNormal(cmd.vertex.stride[GxVM_Normal], *cmd.vertex.mem[GxVM_Normal]);
  IPrimSetupColor(cmd.vertex.stride[GxVM_Color], *cmd.vertex.mem[GxVM_Color], buf->m_numVertices, 0);
  for (unsigned int tmu = 0; tmu < m_caps.m_numTmus; ++tmu) {
    IPrimSetupTexCoord(tmu, cmd.vertex.stride[GxVM_Texture0 + tmu], *cmd.vertex.mem[GxVM_Texture0 + tmu]);
  }

  if (!glNVVertexArrayRange) {
    LockArrays(buf->m_numVertices);
  } else {
    unsigned int valid;
    glGetBooleanv(GL_VERTEX_ARRAY_RANGE_VALID_NV, reinterpret_cast<GLboolean *>(&valid));
    if (!*reinterpret_cast<GLboolean *>(&valid)) {
      Log("VAR not valid!\n");
    }
  }
}

void CGxDeviceOpenGl::BufRender(const CGxBatch *batches, unsigned int count) {
  CGxDevice::BufRender(batches, count);
  IStateSync();

  CGxBufOgl *buf = static_cast<CGxBufOgl *>(m_bufLocked);
  while (count--) {
    if (batches->m_count) {
      const unsigned short *indices = buf->indexPtr + batches->m_start;
      if (glDrawRangeElementsEXT) {
        unsigned int minIndex = batches->m_minIndex < 0 ? 0 : batches->m_minIndex;
        unsigned int maxIndex = batches->m_maxIndex < 0 ? buf->m_numVertices : batches->m_maxIndex;
        glDrawRangeElementsEXT(s_primitiveConversion[batches->m_primType], minIndex, maxIndex, batches->m_count, GL_UNSIGNED_SHORT, indices);
      } else {
        glDrawElements(s_primitiveConversion[batches->m_primType], batches->m_count, GL_UNSIGNED_SHORT, indices);
      }
    }
    ++batches;
  }
}

void CGxDeviceOpenGl::BufUnlock() {
  CGxDevice::BufUnlock();
  if (!glNVVertexArrayRange && glExtCVA) {
    UnlockArrays();
  }
}

void CGxDeviceOpenGl::BufDestroy(CGxBuf *&b) {
  CGxDevice::BufDestroy(b);
  CGxBufOgl *buf = static_cast<CGxBufOgl *>(b);
  DEL(buf);
  b = 0;
}

void CGxDeviceOpenGl::IPrimSetupNormal(unsigned int stride, const void *normals) {
  if (normals) {
    DsSet(Ds_NormalArray, 1, 0);
    glNormalPointer(GL_FLOAT, stride, normals);
  } else {
    DsSet(Ds_NormalArray, 0, 0);
  }
}

void CGxDeviceOpenGl::IPrimSetupColor(unsigned int stride, const void *colors, unsigned int count, int convert) {
  if (!colors) {
    DsSet(Ds_ColorArray, 0, 0);
    IStateSetColorSource(Cs_Material);
    return;
  }

  if (!stride) {
    DsSet(Ds_ColorArray, 0, 0);
    IStateSetColorSource(Cs_Constant);
    IStateSetColorSourceColor(Cs_Constant, *static_cast<const NTempest::CImVector *>(colors));
    return;
  }

  const void  *colorData = colors;
  unsigned int colorStride = stride;
  if (convert) {
    const unsigned char *src = static_cast<const unsigned char *>(colors);
    NTempest::CImVector *dst = m_primColor.Ptr();
    for (unsigned int i = 0; i < count; ++i) {
      dst[i].Set(src[3], src[0], src[1], src[2]);
      src += stride;
    }
    colorData = m_primColor.Ptr();
    colorStride = sizeof(NTempest::CImVector);
  }

  glColorPointer(4, GL_UNSIGNED_BYTE, colorStride, colorData);
  DsSet(Ds_ColorArray, 1, 0);
  IStateSetColorSource(Cs_Array);
}

void CGxDeviceOpenGl::IPrimSetupTexCoord(unsigned int tmu, unsigned int stride, const void *texCoord) {
  if (tmu >= m_caps.m_numTmus) {
    FATALASSERT(0);
  }

  DsSet(Ds_ActiveTexture, tmu, 0);
  EDeviceState state = static_cast<EDeviceState>(static_cast<unsigned int>(Ds_TextureArray0) + tmu);
  if (texCoord) {
    DsSet(state, 1, 0);
    glTexCoordPointer(2, GL_FLOAT, stride, texCoord);
  } else {
    DsSet(state, 0, 0);
  }
}

void CGxDeviceOpenGl::IPrimSetupTexCoord(unsigned int coord, int enable) {
  if (enable) {
    IPrimSetupTexCoord(coord, s_texStride[coord], s_tex[coord]);
  } else {
    IPrimSetupTexCoord(coord, 0, 0);
  }
}

void CGxDeviceOpenGl::IPrimSetupPos() {
  if (glNVVertexArrayRange) {
    DsSet(Ds_NVVAR, 0, 0);
  }

  if (m_vertexShader == GxVS_PassThru) {
    glVertexPointer(3, GL_FLOAT, s_posStride, s_pos);
    if (s_normalStride) {
      IPrimSetupNormal(s_normalStride, s_normal);
    } else {
      IPrimSetupNormal(0, 0);
      glNormal3fv(s_normal ? &s_normal->x : &s_genericNormal.x);
    }
  } else if (m_vertexShader == GxVS_Skin) {
    glVertexPointer(3, GL_FLOAT, sizeof(NTempest::C3Vector), m_primPos.Ptr());
    IPrimSetupNormal(sizeof(NTempest::C3Vector), m_primNormal.Ptr());

    const unsigned char *bone = s_bone;
    const unsigned char *position = reinterpret_cast<const unsigned char *>(s_pos);
    const unsigned char *normal = reinterpret_cast<const unsigned char *>(s_normal);
    for (unsigned int i = 0; i < s_vertexCount; ++i) {
      const NTempest::C34Matrix &matrix = m_bones[*bone];
      const NTempest::C3Vector  &srcPos = *reinterpret_cast<const NTempest::C3Vector *>(position);
      m_primPos[i] = srcPos * matrix;

      const NTempest::C3Vector &srcNormal = *reinterpret_cast<const NTempest::C3Vector *>(normal);
      NTempest::C3Vector       &dstNormal = m_primNormal[i];
      dstNormal.x = matrix.a0 * srcNormal.x + matrix.b0 * srcNormal.y + matrix.c0 * srcNormal.z;
      dstNormal.y = matrix.a1 * srcNormal.x + matrix.b1 * srcNormal.y + matrix.c1 * srcNormal.z;
      dstNormal.z = matrix.a2 * srcNormal.x + matrix.b2 * srcNormal.y + matrix.c2 * srcNormal.z;

      position += s_posStride;
      normal += s_normalStride;
      bone += s_boneStride;
    }
  } else {
    FATALASSERT(0);
  }

  switch (m_vertexBufferFormat) {
    case GxVBF_PN:
      IPrimSetupColor(0, 0, s_vertexCount, 1);
      IPrimSetupTexCoord(0, 0);
      IPrimSetupTexCoord(1, 0);
      break;
    case GxVBF_PNC:
    case GxVBF_PC:
      IPrimSetupColor(s_colorStride, s_color, s_vertexCount, 1);
      IPrimSetupTexCoord(0, 0);
      IPrimSetupTexCoord(1, 0);
      break;
    case GxVBF_PNT0:
      IPrimSetupColor(0, 0, s_vertexCount, 1);
      IPrimSetupTexCoord(0, 1);
      IPrimSetupTexCoord(1, 0);
      break;
    case GxVBF_PNCT0:
    case GxVBF_PCT0:
      IPrimSetupColor(s_colorStride, s_color, s_vertexCount, 1);
      IPrimSetupTexCoord(0, 1);
      IPrimSetupTexCoord(1, 0);
      break;
    case GxVBF_PNT0T1:
    case GxVBF_PT0T1:
      IPrimSetupColor(0, 0, s_vertexCount, 1);
      IPrimSetupTexCoord(0, 1);
      IPrimSetupTexCoord(1, 1);
      break;
    case GxVBF_PNCT0T1:
      IPrimSetupColor(s_colorStride, s_color, s_vertexCount, 1);
      IPrimSetupTexCoord(0, 1);
      IPrimSetupTexCoord(1, 1);
      break;
    default:
      FATALASSERT(0);
      break;
  }
}

void CGxDeviceOpenGl::PrimLockAndProcessVertexPtrs(
    unsigned int               vertexCount,
    const NTempest::C3Vector  *position,
    unsigned int               positionStride,
    const NTempest::C3Vector  *normal,
    unsigned int               normalStride,
    const NTempest::CImVector *color,
    unsigned int               colorStride,
    const unsigned char       *bone,
    unsigned int               boneStride,
    const NTempest::C2Vector  *tex0,
    unsigned int               tex0Stride,
    const NTempest::C2Vector  *tex1,
    unsigned int               tex1Stride
) {
  CGxDevice::PrimLockAndProcessVertexPtrs(
      vertexCount, position, positionStride, normal, normalStride, color, colorStride, bone, boneStride, tex0, tex0Stride, tex1, tex1Stride
  );

  if (vertexCount > m_primPos.Count()) {
    m_primPos.SetCount(vertexCount);
    m_primNormal.SetCount(vertexCount);
    m_primColor.SetCount(vertexCount);
    m_primT0.SetCount(vertexCount);
    m_primT1.SetCount(vertexCount);
  }

  s_vertexCount = vertexCount;
  s_pos = position;
  s_posStride = positionStride;
  s_normal = normal ? normal : &s_genericNormal;
  s_normalStride = normalStride;
  s_color = color ? color : &s_genericColor;
  s_colorStride = colorStride;
  s_bone = bone;
  s_boneStride = boneStride;
  s_tex[0] = tex0 ? tex0 : &s_genericTexCoord;
  s_texStride[0] = tex0Stride;
  s_tex[1] = tex1 ? tex1 : &s_genericTexCoord;
  s_texStride[1] = tex1Stride;

  IPrimSetupPos();
  LockArrays(vertexCount);
}

void CGxDeviceOpenGl::PrimLockIndexPtr(EGxPrim primType, unsigned int indexCount, const unsigned short *indices) {
  CGxDevice::PrimLockIndexPtr(primType, indexCount, indices);
  m_primIndices = indices;
  m_primIndexCount = indexCount;
  m_primType = primType;
}

void CGxDeviceOpenGl::PrimDrawElements() {
  CGxDevice::PrimDrawElements();
  IStateSync();
  glDrawElements(s_primitiveConversion[m_primType], m_primIndexCount, GL_UNSIGNED_SHORT, m_primIndices);
  if (!(m_hwState.m_masterEnables & (1U << GxMasterEnable_DoubleBuffering))) {
    glFlush();
  }
}

void CGxDeviceOpenGl::PrimUnlockIndexPtr() {
  CGxDevice::PrimUnlockIndexPtr();
}

void CGxDeviceOpenGl::PrimUnlockVertexPtrs() {
  CGxDevice::PrimUnlockVertexPtrs();
  UnlockArrays();
}

void CGxDeviceOpenGl::PrimPointSize(float s) {
  glPointSize(s);
}

void CGxDeviceOpenGl::PrimLineWidth(float w) {
  glLineWidth(w);
}
