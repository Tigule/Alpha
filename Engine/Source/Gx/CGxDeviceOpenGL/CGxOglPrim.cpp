#include "CGxDeviceOpenGl.h"
#include "GlExtSupport.h"

#include <Tempest/c34matrix.h>

#include <gl/gl.h>

#include <string.h>

static UINT s_primitiveConversion[GxPrims_Last] = {GL_POINTS, GL_LINES, GL_LINE_STRIP, GL_TRIANGLES, GL_TRIANGLE_STRIP, GL_TRIANGLE_FAN};

static UINT                       s_vertexCount;
static const NTempest::C3Vector  *s_pos;
static UINT                       s_posStride;
static const NTempest::C3Vector  *s_normal;
static UINT                       s_normalStride;
static const NTempest::CImVector *s_color;
static UINT                       s_colorStride;
static const BYTE                *s_bone;
static UINT                       s_boneStride;
static const NTempest::C2Vector  *s_tex[4];
static UINT                       s_texStride[4];

static NTempest::C3Vector  s_genericNormal(0.0f, 1.0f, 0.0f);
static NTempest::C2Vector  s_genericTexCoord(0.0f, 0.0f);
static NTempest::CImVector s_genericColor(0xFFFFFFFF);

class CGxBufOgl : public CGxBuf {
  friend class CGxDeviceOpenGl;

  CGxMemBuffer *m_vb;
  CGxMemBuffer *m_ib;
  LPVOID        vertexPtr[GxVertexMembers_Last];
  WORD         *indexPtr;

 public:
  CGxBufOgl();
  BOOL LockVB();
  BOOL LockIB();
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

BOOL CGxBufOgl::LockVB() {
  if (!m_vb) {
    memset(vertexPtr, 0, sizeof(vertexPtr));
    return 0;
  }

  LPVOID mem = 0;
  if (m_vertexStatus == S_INVALID_RELOAD) {
    m_vb->Lock(mem, m_numVertices * GxVertexSize(m_vbFormat), m_vertexBase);
  } else if (m_vertexStatus == S_INVALID_DISCARD) {
    m_vb->Lock(mem, m_numVertices * GxVertexSize(m_vbFormat), BASE_NONE);
    m_vertexBase = m_vb->m_base;
  } else {
    FATALASSERT(0);
  }

  for (UINT member = 0; member < GxVertexMembers_Last; ++member) {
    int offset = GxVertexMemberOffset(m_vbFormat, static_cast<EGxVertexMember>(member));
    vertexPtr[member] = offset == -1 ? 0 : static_cast<BYTE *>(mem) + offset;
  }
  return 1;
}

BOOL CGxBufOgl::LockIB() {
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
    UINT                  numVertices,
    UINT                  numIndices,
    void (*userCallback)(CGxBufCommand &, CGxBuf *),
    LPVOID userArg
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
    for (UINT member = 0; member < GxVertexMembers_Last; ++member) {
      cmd.vertex.mem[member] = &buf->vertexPtr[member];
      cmd.vertex.stride[member] = GxVertexSize(buf->m_vbFormat);
    }
  }

  if (buf->m_indexStatus != CGxBuf::S_VALID) {
    cmd.index.op = static_cast<EGxBufOp>(2 - buf->LockIB());
    cmd.index.mem[GxVM_Indices] = reinterpret_cast<LPVOID *>(&buf->indexPtr);
    cmd.index.stride[GxVM_Indices] = sizeof(WORD);
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
  for (UINT tmu = 0; tmu < m_caps.m_numTmus; ++tmu) {
    IPrimSetupTexCoord(tmu, cmd.vertex.stride[GxVM_Texture0 + tmu], *cmd.vertex.mem[GxVM_Texture0 + tmu]);
  }

  if (!glNVVertexArrayRange) {
    LockArrays(buf->m_numVertices);
  } else {
    UINT valid;
    glGetBooleanv(GL_VERTEX_ARRAY_RANGE_VALID_NV, reinterpret_cast<GLboolean *>(&valid));
    if (!*reinterpret_cast<GLboolean *>(&valid)) {
      Log("VAR not valid!\n");
    }
  }
}

void CGxDeviceOpenGl::BufRender(const CGxBatch *batches, UINT count) {
  CGxDevice::BufRender(batches, count);
  IStateSync();

  CGxBufOgl *buf = static_cast<CGxBufOgl *>(m_bufLocked);
  while (count--) {
    if (batches->m_count) {
      const WORD *indices = buf->indexPtr + batches->m_start;
      if (glDrawRangeElementsEXT) {
        UINT minIndex = batches->m_minIndex < 0 ? 0 : batches->m_minIndex;
        UINT maxIndex = batches->m_maxIndex < 0 ? buf->m_numVertices : batches->m_maxIndex;
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

void CGxDeviceOpenGl::IPrimSetupNormal(UINT stride, LPCVOID normals) {
  if (normals) {
    DsSet(Ds_NormalArray, 1, 0);
    glNormalPointer(GL_FLOAT, stride, normals);
  } else {
    DsSet(Ds_NormalArray, 0, 0);
  }
}

void CGxDeviceOpenGl::IPrimSetupColor(UINT stride, LPCVOID colors, UINT count, int convert) {
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

  LPCVOID colorData = colors;
  UINT    colorStride = stride;
  if (convert) {
    const BYTE          *src = static_cast<const BYTE *>(colors);
    NTempest::CImVector *dst = m_primColor.Ptr();
    for (UINT i = 0; i < count; ++i) {
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

void CGxDeviceOpenGl::IPrimSetupTexCoord(UINT tmu, UINT stride, LPCVOID texCoord) {
  if (tmu >= m_caps.m_numTmus) {
    FATALASSERT(0);
  }

  DsSet(Ds_ActiveTexture, tmu, 0);
  EDeviceState state = static_cast<EDeviceState>(static_cast<UINT>(Ds_TextureArray0) + tmu);
  if (texCoord) {
    DsSet(state, 1, 0);
    glTexCoordPointer(2, GL_FLOAT, stride, texCoord);
  } else {
    DsSet(state, 0, 0);
  }
}

void CGxDeviceOpenGl::IPrimSetupTexCoord(UINT coord, int enable) {
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

    const BYTE *bone = s_bone;
    const BYTE *position = reinterpret_cast<const BYTE *>(s_pos);
    const BYTE *normal = reinterpret_cast<const BYTE *>(s_normal);
    for (UINT i = 0; i < s_vertexCount; ++i) {
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
    UINT                       vertexCount,
    const NTempest::C3Vector  *position,
    UINT                       positionStride,
    const NTempest::C3Vector  *normal,
    UINT                       normalStride,
    const NTempest::CImVector *color,
    UINT                       colorStride,
    const BYTE                *bone,
    UINT                       boneStride,
    const NTempest::C2Vector  *tex0,
    UINT                       tex0Stride,
    const NTempest::C2Vector  *tex1,
    UINT                       tex1Stride
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

void CGxDeviceOpenGl::PrimLockIndexPtr(EGxPrim primType, UINT indexCount, const WORD *indices) {
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
