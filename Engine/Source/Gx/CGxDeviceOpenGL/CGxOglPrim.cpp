#include "CGxDeviceOpenGl.h"
#include "GlExtSupport.h"

#include <Os/W32/Debugging.h>
#include <Tempest/c34matrix.h>

#include <gl/gl.h>

#include <ctype.h>
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
  BOOL success = 0;

  if (m_vb) {
    LPVOID mem = 0;
    switch (m_vertexStatus) {
      default:
        ASSERT(!("CGxBufOgl::LockVB(): invalid m_vertexStatus\n"));
        break;

      case S_INVALID_RELOAD:
        m_vb->Lock(mem, m_numVertices * GxVertexSize(m_vbFormat), m_vertexBase);
        break;

      case S_INVALID_DISCARD:
        m_vb->Lock(mem, m_numVertices * GxVertexSize(m_vbFormat), BASE_NONE);
        m_vertexBase = m_vb->m_base;
        break;
    }

    for (UINT member = 0; member < GxVertexMembers_Last; ++member) {
      int offset = GxVertexMemberOffset(m_vbFormat, (EGxVertexMember)member);
      if (offset != -1) {
        vertexPtr[member] = (BYTE *)mem + offset;
      } else {
        vertexPtr[member] = 0;
      }
    }
    success = 1;
  } else {
    memset(vertexPtr, 0, sizeof(vertexPtr));
  }

  return success;
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
  buf->m_writeFreq = GxBWF_Dynamic;
  buf->m_numVertices = numVertices;
  buf->m_numIndices = numIndices;
  buf->m_vbFormat = format;
  buf->m_userCallback = userCallback;
  buf->m_userArg = userArg;
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

  CGxBufOgl *buf = (CGxBufOgl *)b;
  IBufSetBuffers(buf);

  if (glNVVertexArrayRange) {
    DsSet(Ds_NVVAR, m_nvvarMem != 0, 0);
  }

  CGxBufCommand cmd;
  cmd.vertex.op = GxBufOp_Nop;
  cmd.index.op = GxBufOp_Nop;

  if (buf->m_vertexStatus != CGxBuf::S_VALID) {
    cmd.vertex.op = buf->LockVB() ? GxBufOp_Fill : GxBufOp_Assign;
    for (UINT member = 0; member < GxVertexMembers_Last; ++member) {
      cmd.vertex.mem[member] = &buf->vertexPtr[member];
      cmd.vertex.stride[member] = GxVertexSize(buf->m_vbFormat);
    }
  }

  if (buf->m_indexStatus != CGxBuf::S_VALID) {
    cmd.index.op = buf->LockIB() ? GxBufOp_Fill : GxBufOp_Assign;
    cmd.index.mem[GxVM_Indices] = (LPVOID *)&buf->indexPtr;
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

  if (glNVVertexArrayRange) {
    BYTE valid;
    glGetBooleanv(GL_VERTEX_ARRAY_RANGE_VALID_NV, &valid);
    if (!valid) {
      OsOutputDebugString("VAR not valid!\n");
    }
  } else {
    LockArrays(buf->m_numVertices);
  }
}

void CGxDeviceOpenGl::BufRender(const CGxBatch *batches, UINT count) {
  CGxDevice::BufRender(batches, count);
  IStateSync();

  CGxBufOgl *buf = (CGxBufOgl *)m_bufLocked;
  for (UINT i = 0; i < count; ++i) {
    if (batches[i].m_count) {
      if (glDrawRangeElementsEXT) {
        UINT minIndex = batches[i].m_minIndex < 0 ? 0 : batches[i].m_minIndex;
        UINT maxIndex;
        if (batches[i].m_maxIndex < 0) {
          maxIndex = ((CGxBufOgl *)m_bufLocked)->m_numVertices;
        } else {
          maxIndex = batches[i].m_maxIndex;
        }
        glDrawRangeElementsEXT(
            s_primitiveConversion[batches[i].m_primType], minIndex, maxIndex, batches[i].m_count, GL_UNSIGNED_SHORT, buf->indexPtr + batches[i].m_start
        );
      } else {
        glDrawElements(s_primitiveConversion[batches[i].m_primType], batches[i].m_count, GL_UNSIGNED_SHORT, buf->indexPtr + batches[i].m_start);
      }
    }
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
  CGxBufOgl *buf = (CGxBufOgl *)b;
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
  if (colors) {
    if (stride) {
      if (convert) {
        const BYTE          *src = (const BYTE *)colors;
        NTempest::CImVector *dst = m_primColor.Ptr();
        for (UINT i = 0; i < count; ++i) {
          dst[i].Set(src[3], src[0], src[1], src[2]);
          src += stride;
        }
        glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(NTempest::CImVector), m_primColor.Ptr());
      } else {
        glColorPointer(4, GL_UNSIGNED_BYTE, stride, colors);
      }
      DsSet(Ds_ColorArray, 1, 0);
      IStateSetColorSource(Cs_Array);
    } else {
      DsSet(Ds_ColorArray, 0, 0);
      IStateSetColorSource(Cs_Constant);
      IStateSetColorSourceColor(Cs_Constant, *(const NTempest::CImVector *)colors);
    }
  } else {
    DsSet(Ds_ColorArray, 0, 0);
    IStateSetColorSource(Cs_Material);
  }
}

void CGxDeviceOpenGl::IPrimSetupTexCoord(UINT tmu, UINT stride, LPCVOID texCoord) {
  if (tmu >= m_caps.m_numTmus) {
    FATALASSERT(0);
    return;
  }

  DsSet(Ds_ActiveTexture, tmu, 0);
  if (texCoord) {
    DsSet((EDeviceState)(Ds_TextureArray0 + tmu), 1, 0);
    glTexCoordPointer(2, GL_FLOAT, stride, texCoord);
  } else {
    DsSet((EDeviceState)(Ds_TextureArray0 + tmu), 0, 0);
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

  switch (m_vertexShader) {
    case GxVS_PassThru:
      glVertexPointer(3, GL_FLOAT, s_posStride, s_pos);
      if (s_normalStride) {
        IPrimSetupNormal(s_normalStride, s_normal);
      } else {
        IPrimSetupNormal(0, 0);
        glNormal3fv(s_normal ? &s_normal->x : &s_genericNormal.x);
      }
      break;

    case GxVS_Skin: {
      glVertexPointer(3, GL_FLOAT, sizeof(NTempest::C3Vector), m_primPos.Ptr());
      IPrimSetupNormal(sizeof(NTempest::C3Vector), m_primNormal.Ptr());

      for (UINT ndx = 0; ndx != s_vertexCount; ++ndx) {
        UINT index = *s_bone;
        {
          NTempest::C44Matrix bone = m_bones[index];

          m_primPos[ndx].Set(
              bone.a0 * s_pos->x + bone.b0 * s_pos->y + bone.c0 * s_pos->z + bone.d0,
              bone.a1 * s_pos->x + bone.b1 * s_pos->y + bone.c1 * s_pos->z + bone.d1,
              bone.a2 * s_pos->x + bone.b2 * s_pos->y + bone.c2 * s_pos->z + bone.d2
          );
        }
        {
          NTempest::C44Matrix bone = m_bones[index];

          m_primNormal[ndx].Set(
              bone.a0 * s_normal->x + bone.b0 * s_normal->y + bone.c0 * s_normal->z,
              bone.a1 * s_normal->x + bone.b1 * s_normal->y + bone.c1 * s_normal->z,
              bone.a2 * s_normal->x + bone.b2 * s_normal->y + bone.c2 * s_normal->z
          );
        }
        s_pos = (const NTempest::C3Vector *)((const BYTE *)s_pos + s_posStride);
        s_normal = (const NTempest::C3Vector *)((const BYTE *)s_normal + s_normalStride);
        s_bone += s_boneStride;
      }
      break;
    }

    default:
      FATALASSERT(0);
      break;
  }

  switch (m_vertexBufferFormat) {
    case GxVBF_PN:
      IPrimSetupColor(0, 0, s_vertexCount, 1);
      IPrimSetupTexCoord(0, 0);
      IPrimSetupTexCoord(1, 0);
      break;

    case GxVBF_PNC:
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
      IPrimSetupColor(s_colorStride, s_color, s_vertexCount, 1);
      IPrimSetupTexCoord(0, 1);
      IPrimSetupTexCoord(1, 0);
      break;

    case GxVBF_PNT0T1:
      IPrimSetupColor(0, 0, s_vertexCount, 1);
      IPrimSetupTexCoord(0, 1);
      IPrimSetupTexCoord(1, 1);
      break;

    case GxVBF_PNCT0T1:
      IPrimSetupColor(s_colorStride, s_color, s_vertexCount, 1);
      IPrimSetupTexCoord(0, 1);
      IPrimSetupTexCoord(1, 1);
      break;

    case GxVBF_PCT0:
      IPrimSetupColor(s_colorStride, s_color, s_vertexCount, 1);
      IPrimSetupTexCoord(0, 1);
      IPrimSetupTexCoord(1, 0);
      break;

    case GxVBF_PC:
      IPrimSetupColor(s_colorStride, s_color, s_vertexCount, 1);
      IPrimSetupTexCoord(0, 0);
      IPrimSetupTexCoord(1, 0);
      break;

    case GxVBF_PT0T1:
      IPrimSetupColor(0, 0, s_vertexCount, 1);
      IPrimSetupTexCoord(0, 1);
      IPrimSetupTexCoord(1, 1);
      break;

    default:
      SErrDisplayErrorFmt(
          STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1,
          isprint((m_vertexBufferFormat >> 24) & 0xFF) && isprint((m_vertexBufferFormat >> 16) & 0xFF) && isprint((m_vertexBufferFormat >> 8) & 0xFF) &&
                  isprint(m_vertexBufferFormat & 0xFF)
              ? "\"%s\", %s = %ld (0x%08X, '%c%c%c%c')"
              : "\"%s\", %s = %ld (0x%08X)",
          "0", "m_vertexBufferFormat", m_vertexBufferFormat, m_vertexBufferFormat, (m_vertexBufferFormat >> 24) & 0xFF,
          (m_vertexBufferFormat >> 16) & 0xFF, (m_vertexBufferFormat >> 8) & 0xFF, m_vertexBufferFormat & 0xFF
      );
      break;
  }
}

void CGxDeviceOpenGl::PrimLockAndProcessVertexPtrs(
    UINT                       vertexCount,
    const NTempest::C3Vector  *pos,
    UINT                       posStride,
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
  CGxDevice::PrimLockAndProcessVertexPtrs(vertexCount, pos, posStride, normal, normalStride, color, colorStride, bone, boneStride, tex0, tex0Stride, tex1, tex1Stride);

  if (vertexCount > m_primPos.Count()) {
    m_primPos.SetCount(vertexCount);
    m_primNormal.SetCount(vertexCount);
    m_primColor.SetCount(vertexCount);
    m_primT0.SetCount(vertexCount);
    m_primT1.SetCount(vertexCount);
  }

  s_vertexCount = vertexCount;
  s_pos = pos;
  s_posStride = posStride;
  s_normal = normal;
  s_normalStride = normalStride;
  s_color = color;
  s_colorStride = colorStride;
  s_bone = bone;
  s_boneStride = boneStride;
  s_tex[0] = tex0;
  s_texStride[0] = tex0Stride;
  s_tex[1] = tex1;
  s_texStride[1] = tex1Stride;

  if (!s_normal) {
    s_normal = &s_genericNormal;
  }
  if (!s_color) {
    s_color = &s_genericColor;
  }
  if (!s_tex[0]) {
    s_tex[0] = &s_genericTexCoord;
  }
  if (!s_tex[1]) {
    s_tex[1] = &s_genericTexCoord;
  }

  IPrimSetupPos();

  if (m_vertexShader == GxVS_Skin) {
    LockArrays(s_vertexCount);
  } else {
    LockArrays(s_vertexCount);
  }
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
