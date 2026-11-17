#include "CGxOglPrim_OSX.h"
#include "../GlExtSupport.h"

#include <Os/W32/Debugging.h>
#include <Tempest/c34matrix.h>

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
static bool                       s_vxrPrim;

static NTempest::C3Vector  s_genericNormal(0.0f, 1.0f, 0.0f);
static NTempest::C2Vector  s_genericTexCoord(0.0f, 0.0f);
static NTempest::CImVector s_genericColor(0xFFFFFFFF);

CGxVaoManager::CGxVaoManager() {
  GLuint names[kVaoCount];

  m_unalignedMem = ALLOC((kPageCount << kPageSizeLog2) + 0x2000);
  memset(m_unalignedMem, 0xBB, (kPageCount << kPageSizeLog2) + 0x2000);
  m_alignedMem = (LPBYTE)(((UINT_PTR)m_unalignedMem + 0xFFF) & ~(UINT_PTR)0xFFF);
  memset(m_pageMap, 0, sizeof(m_pageMap));
  m_pageMapAllocMark = 0;
  memset(m_desc, 0, sizeof(m_desc));
  m_nextSlot = 1;

  glGenVertexArraysAPPLE(kVaoCount, names);
  for (int i = 0; i < kVaoCount; ++i) {
    m_desc[i].m_index = i;
    m_desc[i].m_vao = names[i];
  }
}

CGxVaoManager::~CGxVaoManager() {
  GLuint names[kVaoCount];

  for (int i = 0; i < kVaoCount; ++i) {
    glFinishObjectAPPLE(GL_VERTEX_ARRAY, m_desc[i].m_vao);
    glBindVertexArrayAPPLE(m_desc[i].m_vao);
    glVertexArrayRangeAPPLE(0, 0);
    names[i] = m_desc[i].m_vao;
  }

  glBindVertexArrayAPPLE(0);
  glDeleteVertexArraysAPPLE(kVaoCount, names);
  FREE(m_unalignedMem);
}

int CGxVaoManager::AllocBuf(UINT count, EGxVertexBufferFormat format) {
  glPushAttrib(GL_ALL_ATTRIB_BITS);
  glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);

  int foundSlotIndex = -1;
  for (int tries = kVaoCount; tries != 0 && foundSlotIndex < 0; --tries) {
    if (!m_desc[m_nextSlot].m_active) {
      foundSlotIndex = m_nextSlot;
    }
    m_nextSlot = (m_nextSlot + 1) % kVaoCount;
  }
  ASSERT(foundSlotIndex>0);

  CGxVaoDesc *desc = &m_desc[foundSlotIndex];
  UINT        vtxSize = GxVertexSize(format);
  UINT        vtxBytes = (vtxSize * count + 127) & ~127;
  UINT        vaoPagesNeeded = (vtxBytes + 4095) >> kPageSizeLog2;

  if (m_pageMapAllocMark + vaoPagesNeeded >= kPageCount) {
    m_pageMapAllocMark = 0;
  }
  ASSERT(m_pageMapAllocMark < 16384);

  int pageRange0 = m_pageMapAllocMark;
  int pageRange1 = pageRange0 + vaoPagesNeeded - 1;
  ASSERT(pageRange1 < 16384);

  UINT freePagesInRange = 0;
  for (int i = pageRange0; i <= pageRange1; ++i) {
    ASSERT(i < 16384);
    if (!m_pageMap[i]) {
      ++freePagesInRange;
    }
  }

  for (int tries = 0; freePagesInRange < vaoPagesNeeded && pageRange0 <= pageRange1 && tries < kPageCount; ++tries) {
    if (!m_pageMap[pageRange0]) {
      --freePagesInRange;
    }
    if (!m_pageMap[(pageRange1 + 1) & (kPageCount - 1)]) {
      ++freePagesInRange;
    }
    pageRange0 = (pageRange0 + 1) & (kPageCount - 1);
    pageRange1 = (pageRange1 + 1) & (kPageCount - 1);
  }
  ASSERT(freePagesInRange <= vaoPagesNeeded);

  if (freePagesInRange != vaoPagesNeeded || pageRange0 > pageRange1) {
    ASSERT(0);
    return 0;
  }

  for (int i = pageRange0; i <= pageRange1; ++i) {
    m_pageMap[i] = foundSlotIndex;
  }

  desc->m_active = 1;
  desc->m_pageCount = vaoPagesNeeded;
  desc->m_firstPage = pageRange0;
  desc->m_count = count;
  desc->m_format = format;
  desc->m_vtxBytes = vtxBytes;

  glBindVertexArrayAPPLE(desc->m_vao);
  LPBYTE mem = m_alignedMem + (desc->m_firstPage << kPageSizeLog2);
  glVertexArrayRangeAPPLE(vtxBytes, mem);
  glVertexArrayParameteriAPPLE(GL_VERTEX_ARRAY_STORAGE_HINT_APPLE, GL_STORAGE_CACHED_APPLE);

  for (int member = GxVertexMembers_Last - 1; member >= 0; --member) {
    int offset = GxVertexMemberOffset(format, (EGxVertexMember)member);
    if (offset != -1) {
      LPBYTE ptr = mem + offset;
      desc->m_ptrs[member] = ptr;
      switch (member) {
        case GxVM_Position:
          glEnableClientState(GL_VERTEX_ARRAY);
          glVertexPointer(3, GL_FLOAT, vtxSize, ptr);
          break;
        case GxVM_Normal:
          glEnableClientState(GL_NORMAL_ARRAY);
          glNormalPointer(GL_FLOAT, vtxSize, ptr);
          break;
        case GxVM_Color:
          glEnableClientState(GL_COLOR_ARRAY);
          glColorPointer(4, GL_UNSIGNED_BYTE, vtxSize, ptr);
          break;
        case GxVM_Texture0:
        case GxVM_Texture1:
        case GxVM_Texture2:
        case GxVM_Texture3:
          glActiveTextureARB(GL_TEXTURE0_ARB + member - GxVM_Texture0);
          glClientActiveTextureARB(GL_TEXTURE0_ARB + member - GxVM_Texture0);
          glEnableClientState(GL_TEXTURE_COORD_ARRAY);
          glTexCoordPointer(2, GL_FLOAT, vtxSize, ptr);
          break;
        default:
          ASSERT(0);
          break;
      }
    } else {
      desc->m_ptrs[member] = 0;
      switch (member) {
        case GxVM_Position:
          ASSERT(0);
          glDisableClientState(GL_VERTEX_ARRAY);
          break;
        case GxVM_Normal:
          glDisableClientState(GL_NORMAL_ARRAY);
          break;
        case GxVM_Color:
          glDisableClientState(GL_COLOR_ARRAY);
          break;
        case GxVM_Texture0:
        case GxVM_Texture1:
        case GxVM_Texture2:
        case GxVM_Texture3:
          glActiveTextureARB(GL_TEXTURE0_ARB + member - GxVM_Texture0);
          glClientActiveTextureARB(GL_TEXTURE0_ARB + member - GxVM_Texture0);
          glDisableClientState(GL_TEXTURE_COORD_ARRAY);
          break;
        default:
          ASSERT(0);
          break;
      }
    }
  }

  glBindVertexArrayAPPLE(0);
  glPopAttrib();
  glPopClientAttrib();
  return desc->m_index;
}

void CGxVaoManager::FreeBuf(int index) {
  CGxVaoDesc *desc = &m_desc[index];

  if (index > 0 && desc->m_active) {
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);
    ASSERT(desc->m_active);

    GLuint vao = desc->m_vao;
    glFinishObjectAPPLE(GL_VERTEX_ARRAY, vao);
    glBindVertexArrayAPPLE(vao);
    glVertexArrayRangeAPPLE(0, 0);

    for (UINT i = 0; i < desc->m_pageCount; ++i) {
      m_pageMap[desc->m_firstPage + i] = 0;
    }

    memset(desc, 0, sizeof(CGxVaoDesc));
    desc->m_index = index;
    desc->m_vao = vao;

    glPopAttrib();
    glPopClientAttrib();
  }
}

LPVOID CGxVxrRingBuffer::Alloc(UINT bytes) {
  UINT half = m_size >> 1;

  bytes = (bytes + 31) & ~31;
  if (((m_offset + bytes) ^ m_offset) & half) {
    if (m_offset < half) {
      glFinishFenceAPPLE(m_fence1);
      m_offset = half;
      glSetFenceAPPLE(m_fence0);
    } else {
      glFinishFenceAPPLE(m_fence0);
      m_offset = 0;
      glSetFenceAPPLE(m_fence1);
    }
  }

  LPVOID ptr = (LPVOID)(m_baseAddress + m_offset);
  m_offset += bytes;
  return ptr;
}

CGxVxrManager::CGxVxrManager(SGxVxrBufferDef *defs, int *success) {
  *success = 0;
  m_totalSize = 0;
  m_unalignedBuf = 0;
  m_alignedBuf = 0;
  memset(m_fences, 0, sizeof(m_fences));
  memset(m_bufs, 0, sizeof(m_bufs));
  m_rangeEnabled = 0;

  UINT totalBufs = 0;
  for (int i = 0; defs[i].sizeLog2; ++i) {
    ASSERT(defs[i].flavor < kVxrBufferCount);
    ASSERT(defs[i].sizeLog2 >= 12);
    m_totalSize += 1 << defs[i].sizeLog2;
    ++totalBufs;
  }
  ASSERT(totalBufs <= kVxrBufferCount);

  m_unalignedBuf = ALLOCZERO(m_totalSize + 4096);
  OsOutputDebugString(" CGxVxrManager::CGxVxrManager: unaligned buf @ %8lx (%d bytes) ", (unsigned long)(UINT_PTR)m_unalignedBuf, m_totalSize + 4096);
  m_alignedBuf = (LPBYTE)(((UINT_PTR)m_unalignedBuf + 0xFFF) & ~(UINT_PTR)0xFFF);
  OsOutputDebugString(" CGxVxrManager::CGxVxrManager: aligned buf @ %8lx ", (unsigned long)(UINT_PTR)m_alignedBuf);

  glVertexArrayRangeAPPLE(m_totalSize, m_alignedBuf);
  glGetError();
  glGenFencesAPPLE(kFenceCount, m_fences);
  glGetError();

  for (int i = 0; i < kFenceCount; ++i) {
    glSetFenceAPPLE(m_fences[i]);
    glGetError();
    glFinishFenceAPPLE(m_fences[i]);
    glGetError();
  }

  UINT offset = 0;
  for (int i = 0; defs[i].sizeLog2; ++i) {
    EGxVxrBufferFlavor flavor = defs[i].flavor;
    ASSERT(!m_bufs[flavor]);
    m_bufs[flavor] = new CGxVxrRingBuffer(m_alignedBuf + offset, defs[i].sizeLog2, m_fences[2 * i], m_fences[2 * i + 1]);
    offset += 1 << defs[i].sizeLog2;
  }

  *success = 1;
}

CGxVxrManager::~CGxVxrManager() {
  glDisableClientState(GL_VERTEX_ARRAY_RANGE_APPLE);
  glVertexArrayRangeAPPLE(0, 0);
  if (m_unalignedBuf) {
    FREE(m_unalignedBuf);
  }
  m_unalignedBuf = 0;
  glDeleteFencesAPPLE(kFenceCount, m_fences);
}

LPVOID CGxVxrManager::Alloc(UINT bytes, EGxVxrBufferFlavor flavor) {
  ASSERT(m_bufs[flavor]);
  return m_bufs[flavor]->Alloc(bytes);
}

void CGxVxrManager::Set_GL_VERTEX_ARRAY_RANGE_APPLE(int enable) {
  if (enable != m_rangeEnabled) {
    if (enable) {
      glEnableClientState(GL_VERTEX_ARRAY_RANGE_APPLE);
    } else {
      glDisableClientState(GL_VERTEX_ARRAY_RANGE_APPLE);
    }
    m_rangeEnabled = enable;
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
  CGxDevice::BufCreate(writeFreq, format, numVertices, numIndices, userCallback, userArg);

  CGxBufOgl *buf = NEW(CGxBufOgl);
  buf->m_writeFreq = writeFreq;
  buf->m_numVertices = numVertices;
  buf->m_numIndices = numIndices;
  buf->m_vbFormat = format;
  buf->m_userCallback = userCallback;
  buf->m_userArg = userArg;
  buf->m_vaoToken = m_vaoManager->AllocBuf(numVertices, format);
  m_bufList.LinkNode(buf, LIST_TAIL, 0);
  return buf;
}

void CGxDeviceOpenGl::BufLock(CGxBuf *b) {
  CGxDevice::BufLock(b);

  CGxBufOgl *buf = (CGxBufOgl *)b;
  s_vxrPrim = 0;
  ASSERT(m_vxrManager != 0);
  ASSERT(m_vaoManager != 0);

  UINT          vtxSize = GxVertexSize(buf->m_vbFormat);
  UINT          vtxBytes = buf->m_numVertices * vtxSize;
  LPBYTE        vtxPtr = 0;
  CGxVaoDesc   *desc = 0;
  CGxBufCommand cmd;
  cmd.vertex.op = GxBufOp_Nop;
  cmd.index.op = GxBufOp_Nop;
  int dynamic = buf->m_writeFreq >= GxBWF_Dynamic;

  if (dynamic) {
    vtxPtr = (LPBYTE)m_vxrManager->Alloc(vtxBytes, kVxrBuffer_Dynamic);
    cmd.vertex.op = GxBufOp_Assign;
    cmd.index.op = GxBufOp_Assign;
    glBindVertexArrayAPPLE(0);
  } else {
    if (buf->m_vaoToken < 0) {
      buf->m_vaoToken = m_vaoManager->AllocBuf(buf->m_numVertices, buf->m_vbFormat);
      ASSERT(buf->m_vaoToken != 0);
    } else {
      ASSERT(buf->m_vaoToken != 0);
      if (vtxBytes > m_vaoManager->m_desc[buf->m_vaoToken].m_vtxBytes) {
        m_vaoManager->FreeBuf(buf->m_vaoToken);
        buf->m_vaoToken = m_vaoManager->AllocBuf(buf->m_numVertices, buf->m_vbFormat);
        ASSERT(buf->m_vaoToken != 0);
        desc = &m_vaoManager->m_desc[buf->m_vaoToken];
        ASSERT(vtxBytes <= desc->m_vtxBytes);
      }
    }
    desc = &m_vaoManager->m_desc[buf->m_vaoToken];
    glBindVertexArrayAPPLE(desc->m_vao);
    vtxPtr = m_vaoManager->m_alignedMem + (desc->m_firstPage << CGxVaoManager::kPageSizeLog2);
  }
  ASSERT(vtxPtr != 0);
  buf->m_vtxBytes = vtxBytes;

  UINT  idxBytes = buf->m_numIndices * sizeof(WORD) + 32;
  WORD *idxPtr = (WORD *)m_vxrManager->Alloc(idxBytes, kVxrBuffer_Index);
  ASSERT(idxPtr != 0);
  buf->m_idxBytes = idxBytes;

  if (buf->m_vertexStatus != CGxBuf::S_VALID || dynamic) {
    cmd.vertex.op = GxBufOp_Fill;
    for (int ilp = GxVertexMembers_Last - 1; ilp >= 0; --ilp) {
      int offset = GxVertexMemberOffset(buf->m_vbFormat, (EGxVertexMember)ilp);
      buf->vertexPtr[ilp] = offset != -1 ? vtxPtr + offset : 0;
      if (!dynamic) {
        ASSERT(buf->vertexPtr[ilp] == desc->m_ptrs[ ilp ]);
      }
      cmd.vertex.mem[ilp] = &buf->vertexPtr[ilp];
      cmd.vertex.stride[ilp] = vtxSize;
    }
  }

  buf->m_vertexBase = 0;

  if (buf->m_indexStatus != CGxBuf::S_VALID) {
    buf->indexPtr = idxPtr;
    cmd.index.op = GxBufOp_Fill;
    cmd.index.mem[GxVM_Indices] = (LPVOID *)&buf->indexPtr;
    cmd.index.stride[GxVM_Indices] = sizeof(WORD);
  } else {
    ASSERT(0);
  }

  if (cmd.vertex.op != GxBufOp_Nop || cmd.index.op != GxBufOp_Nop) {
    if (!dynamic && cmd.vertex.op != GxBufOp_Nop && buf->m_vertexStatus != CGxBuf::S_VALID) {
      glFinishObjectAPPLE(GL_VERTEX_ARRAY, desc->m_vao);
    }

    buf->m_userCallback(cmd, b);

    if (cmd.vertex.op != GxBufOp_Nop) {
      glFlushVertexArrayRangeAPPLE(vtxBytes, vtxPtr);
    }

    if (buf->m_vertexStatus != CGxBuf::S_VALID && buf->m_writeFreq != GxBWF_Dynamic) {
      buf->m_vertexStatus = CGxBuf::S_VALID;
    }
  }

  if (dynamic) {
    glBindVertexArrayAPPLE(0);
    glVertexPointer(3, GL_FLOAT, vtxSize, buf->vertexPtr[GxVM_Position]);
    IPrimSetupNormal(vtxSize, buf->vertexPtr[GxVM_Normal]);
    IPrimSetupColor(vtxSize, buf->vertexPtr[GxVM_Color], buf->m_numVertices, 0);
    for (UINT tmu = 0; tmu < m_caps.m_numTmus; ++tmu) {
      IPrimSetupTexCoord(tmu, vtxSize, buf->vertexPtr[GxVM_Texture0 + tmu]);
    }
  }
#if BUILD_FIXES
  else {
    IStateSetColorSource(desc->m_ptrs[GxVM_Color] ? Cs_Array : Cs_Material);
  }
#endif
}

void CGxDeviceOpenGl::BufRender(const CGxBatch *batches, UINT count) {
  CGxDevice::BufRender(batches, count);
  IStateSync();

  CGxBufOgl *buf = (CGxBufOgl *)m_bufLocked;
  ASSERT(m_vxrManager != 0);
  m_vxrManager->Set_GL_VERTEX_ARRAY_RANGE_APPLE(1);
  glFlushVertexArrayRangeAPPLE(buf->m_numIndices * sizeof(WORD), buf->indexPtr);

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
  glBindVertexArrayAPPLE(0);
}

void CGxDeviceOpenGl::BufDestroy(CGxBuf *&b) {
  CGxDevice::BufDestroy(b);
  CGxBufOgl *buf = (CGxBufOgl *)b;
  m_bufList.UnlinkNode(b);
  if (buf->m_vaoToken != 0) {
    m_vaoManager->FreeBuf(buf->m_vaoToken);
  }
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
      if (s_vxrPrim) {
        if (convert) {
          const BYTE          *src = (const BYTE *)colors;
          NTempest::CImVector *dst = m_vxrColor;
          for (UINT i = 0; i < count; ++i) {
            dst[i].Set(src[3], src[0], src[1], src[2]);
            src += stride;
          }
          glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(NTempest::CImVector), m_vxrColor);
        } else {
          memcpy(m_vxrColor, colors, count * sizeof(NTempest::CImVector));
          glColorPointer(4, GL_UNSIGNED_BYTE, stride, m_vxrColor);
        }
      } else if (convert) {
        ASSERT(0);
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
    ASSERT(0);
    return;
  }

  DsSet(Ds_ActiveTexture, tmu, 0);
  if (texCoord) {
    DsSet((EDeviceState)(Ds_TextureArray0 + tmu), 1, 0);
    if (s_vxrPrim) {
      ASSERT(m_vxrManager != 0);
      if (stride == sizeof(NTempest::C2Vector)) {
        memcpy(m_vxrTex[tmu], texCoord, s_vertexCount * sizeof(NTempest::C2Vector));
      } else {
        const BYTE         *src = (const BYTE *)texCoord;
        NTempest::C2Vector *dst = m_vxrTex[tmu];
        for (UINT i = 0; i < s_vertexCount; ++i) {
          dst[i] = *(const NTempest::C2Vector *)src;
          src += stride;
        }
      }
      glTexCoordPointer(2, GL_FLOAT, sizeof(NTempest::C2Vector), m_vxrTex[tmu]);
    } else {
      glTexCoordPointer(2, GL_FLOAT, stride, texCoord);
    }
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
  ASSERT(m_vxrManager != 0);

  switch (m_vertexShader) {
    case GxVS_PassThru:
      if (s_posStride == sizeof(NTempest::C3Vector)) {
        memcpy(m_vxrPos, s_pos, s_vertexCount * sizeof(NTempest::C3Vector));
      } else {
        const BYTE *src = (const BYTE *)s_pos;
        for (UINT i = 0; i < s_vertexCount; ++i) {
          m_vxrPos[i] = *(const NTempest::C3Vector *)src;
          src += s_posStride;
        }
      }
      glVertexPointer(3, GL_FLOAT, sizeof(NTempest::C3Vector), m_vxrPos);

      if (s_normalStride == sizeof(NTempest::C3Vector)) {
        memcpy(m_vxrNormal, s_normal, s_vertexCount * sizeof(NTempest::C3Vector));
      } else {
        const BYTE *src = (const BYTE *)s_normal;
        for (UINT i = 0; i < s_vertexCount; ++i) {
          m_vxrNormal[i] = *(const NTempest::C3Vector *)src;
          src += s_normalStride;
        }
      }
      DsSet(Ds_NormalArray, 1, 0);
      glNormalPointer(GL_FLOAT, sizeof(NTempest::C3Vector), m_vxrNormal);
      break;

    case GxVS_Skin: {
      glVertexPointer(3, GL_FLOAT, sizeof(NTempest::C3Vector), m_vxrPos);
      glNormalPointer(GL_FLOAT, sizeof(NTempest::C3Vector), m_vxrNormal);
      DsSet(Ds_NormalArray, 1, 0);

      for (UINT ndx = 0; ndx != s_vertexCount; ++ndx) {
        UINT index = *s_bone;
        {
          NTempest::C44Matrix bone = m_bones[index];

          m_vxrPos[ndx].Set(
              bone.a0 * s_pos->x + bone.b0 * s_pos->y + bone.c0 * s_pos->z + bone.d0,
              bone.a1 * s_pos->x + bone.b1 * s_pos->y + bone.c1 * s_pos->z + bone.d1,
              bone.a2 * s_pos->x + bone.b2 * s_pos->y + bone.c2 * s_pos->z + bone.d2
          );
        }
        {
          NTempest::C44Matrix bone = m_bones[index];

          m_vxrNormal[ndx].Set(
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
      ASSERT(0);
      break;
  }

  switch (m_vertexBufferFormat) {
    case GxVBF_PN:
      IPrimSetupColor(0, 0, s_vertexCount, 1);
      IPrimSetupTexCoord(0, 0, 0);
      IPrimSetupTexCoord(1, 0, 0);
      break;

    case GxVBF_PNC:
      IPrimSetupColor(s_colorStride, s_color, s_vertexCount, 1);
      IPrimSetupTexCoord(0, 0, 0);
      IPrimSetupTexCoord(1, 0, 0);
      break;

    case GxVBF_PNT0:
      IPrimSetupColor(0, 0, s_vertexCount, 1);
      IPrimSetupTexCoord(0, s_texStride[0], s_tex[0]);
      IPrimSetupTexCoord(1, 0, 0);
      break;

    case GxVBF_PNCT0:
      IPrimSetupColor(s_colorStride, s_color, s_vertexCount, 1);
      IPrimSetupTexCoord(0, s_texStride[0], s_tex[0]);
      IPrimSetupTexCoord(1, 0, 0);
      break;

    case GxVBF_PNT0T1:
      IPrimSetupColor(0, 0, s_vertexCount, 1);
      IPrimSetupTexCoord(0, s_texStride[0], s_tex[0]);
      IPrimSetupTexCoord(1, s_texStride[1], s_tex[1]);
      break;

    case GxVBF_PNCT0T1:
      IPrimSetupColor(s_colorStride, s_color, s_vertexCount, 1);
      IPrimSetupTexCoord(0, s_texStride[0], s_tex[0]);
      IPrimSetupTexCoord(1, s_texStride[1], s_tex[1]);
      break;

    case GxVBF_PCT0:
      IPrimSetupColor(s_colorStride, s_color, s_vertexCount, 1);
      IPrimSetupTexCoord(0, s_texStride[0], s_tex[0]);
      IPrimSetupTexCoord(1, 0, 0);
      break;

    case GxVBF_PC:
      IPrimSetupColor(s_colorStride, s_color, s_vertexCount, 1);
      IPrimSetupTexCoord(0, 0, 0);
      IPrimSetupTexCoord(1, 0, 0);
      break;

    case GxVBF_PT0T1:
      IPrimSetupColor(0, 0, s_vertexCount, 1);
      IPrimSetupTexCoord(0, s_texStride[0], s_tex[0]);
      IPrimSetupTexCoord(1, s_texStride[1], s_tex[1]);
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

  glFlushVertexArrayRangeAPPLE(m_vxrVertexSectionSize, m_vxrVertexSectionBase);
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

  m_vxrVertexSectionSize = vertexCount * 44 + 96;
  m_vxrVertexSectionBase = (LPBYTE)m_vxrManager->Alloc(m_vxrVertexSectionSize, m_vertexShader != GxVS_PassThru ? kVxrBuffer_Dynamic : kVxrBuffer_Prim);
  ASSERT(m_vxrVertexSectionBase != 0);

  LPBYTE subVtxPtr = m_vxrVertexSectionBase;
  m_vxrPos = (NTempest::C3Vector *)subVtxPtr;
  subVtxPtr += (s_vertexCount * sizeof(NTempest::C3Vector) + 31) & ~31;
  m_vxrNormal = (NTempest::C3Vector *)subVtxPtr;
  subVtxPtr += (s_vertexCount * sizeof(NTempest::C3Vector) + 31) & ~31;
  m_vxrColor = (NTempest::CImVector *)subVtxPtr;
  subVtxPtr += s_vertexCount * sizeof(NTempest::CImVector);
  m_vxrTex[0] = (NTempest::C2Vector *)subVtxPtr;
  subVtxPtr += s_vertexCount * sizeof(NTempest::C2Vector);
  m_vxrTex[1] = (NTempest::C2Vector *)subVtxPtr;
  subVtxPtr += s_vertexCount * sizeof(NTempest::C2Vector);
  ASSERT(subVtxPtr <= (m_vxrVertexSectionBase + m_vxrVertexSectionSize));

  s_vxrPrim = 1;
  IPrimSetupPos();
}

void CGxDeviceOpenGl::PrimLockIndexPtr(EGxPrim primType, UINT indexCount, const WORD *indices) {
  CGxDevice::PrimLockIndexPtr(primType, indexCount, indices);
  m_primType = primType;
  m_primIndexCount = indexCount;
  m_primIndices = indices;

  m_vxrIndexSectionSize = (indexCount * sizeof(WORD) + 31) & ~31;
  m_vxrIndexSectionBase = (LPBYTE)m_vxrManager->Alloc(m_vxrIndexSectionSize, kVxrBuffer_Index);
  ASSERT(m_vxrIndexSectionBase != 0);
  m_vxrIndices = (WORD *)m_vxrIndexSectionBase;
  memcpy(m_vxrIndexSectionBase, indices, indexCount * sizeof(WORD));
  glFlushVertexArrayRangeAPPLE(m_vxrIndexSectionSize, m_vxrIndexSectionBase);
}

void CGxDeviceOpenGl::PrimDrawElements() {
  CGxDevice::PrimDrawElements();
  IStateSync();
  ASSERT(m_vxrManager != 0);
  m_vxrManager->Set_GL_VERTEX_ARRAY_RANGE_APPLE(1);
  glDrawElements(s_primitiveConversion[m_primType], m_primIndexCount, GL_UNSIGNED_SHORT, m_vxrIndices);
  if (!(m_hwState.m_masterEnables & (1U << GxMasterEnable_DoubleBuffering))) {
    glFlush();
  }
}

void CGxDeviceOpenGl::PrimUnlockIndexPtr() {
  CGxDevice::PrimUnlockIndexPtr();
}

void CGxDeviceOpenGl::PrimUnlockVertexPtrs() {
  CGxDevice::PrimUnlockVertexPtrs();
}

void CGxDeviceOpenGl::PrimPointSize(float s) {
  glPointSize(s);
}

void CGxDeviceOpenGl::PrimLineWidth(float w) {
  glLineWidth(w);
}
