#include "CGxDeviceD3d.h"
#include <Tempest/c34matrix.h>

static UINT                       s_vertexCount;
static LONGLONG                   s_himask = 0xFFFFFFFF00000000i64;
static LONGLONG                   s_lomask = 0x00000000FFFFFFFFi64;
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
static const WORD                *s_indices;
static UINT                       s_indexCount;

static const NTempest::C3Vector  s_genericNormal(0.0f, 1.0f, 0.0f);
static const NTempest::C2Vector  s_genericTexCoord(0.0f, 0.0f);
static const NTempest::CImVector s_genericColor(0xFFFFFFFF);
static NTempest::CImVector       diffuse;

static enum _D3DPRIMITIVETYPE s_primitiveConversion[GxPrims_Last] = {static_cast<enum _D3DPRIMITIVETYPE>(1), static_cast<enum _D3DPRIMITIVETYPE>(2),
                                                                     static_cast<enum _D3DPRIMITIVETYPE>(3), static_cast<enum _D3DPRIMITIVETYPE>(4),
                                                                     static_cast<enum _D3DPRIMITIVETYPE>(5), static_cast<enum _D3DPRIMITIVETYPE>(6)};

#define MinD3dBufVertices 0x100
#define MinD3dBufIndices  0x300

static DWORD s_vtxBufFmtConversion[GxVertexBufferFormats_Last] = {
    D3DFVF_XYZ | D3DFVF_NORMAL,
    D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE,
    D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1,
    D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_TEX1,
    D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX2,
    D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_TEX2,
    D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1,
    D3DFVF_XYZ | D3DFVF_DIFFUSE,
    D3DFVF_XYZ | D3DFVF_TEX2
};

CGxBufD3d::CGxBufD3d() : m_vbl(0), m_vb(0), m_ib(0) {
}

CGxBufD3d::~CGxBufD3d() {
  ASSERT(m_vb == 0);
  ASSERT(m_ib == 0);
}

void CGxBufD3d::SetVBL(CVertexBufferList *vbl) {
  m_vbl = vbl;
}

void CGxBufD3d::SetVB(CGxVertexBuffer_D3d *vb) {
  if (vb) {
    vb->AddBuf(this);
  }
  m_vb = vb;
}

void CGxBufD3d::SetIB(CGxIndexBuffer_D3d *ib) {
  if (ib) {
    ib->AddBuf(this);
  }
  m_ib = ib;
}

void CGxBufD3d::UnsetVB() {
  if (m_vb) {
    m_vb->RemoveBuf(this);
    m_vb = 0;
  }

  Invalidate(S_INVALID_DISCARD, S_VALID);
}

void CGxBufD3d::UnsetIB() {
  if (m_ib) {
    m_ib->RemoveBuf(this);
    m_ib = 0;
  }

  Invalidate(S_VALID, S_INVALID_DISCARD);
}

void CGxBufD3d::LockVB(LPVOID &mem) {
  if (m_vertexStatus == S_INVALID_RELOAD) {
    m_vb->Lock(mem, m_numVertices, m_vertexBase);
  } else {
    ASSERT(m_vertexStatus == S_INVALID_DISCARD);
    UnsetVB();
    SetVB(m_vbl->Lock(mem, m_numVertices, BASE_NONE));
    m_vertexBase = m_vb->m_base;
  }
}

void CGxBufD3d::LockIB(LPVOID &mem) {
  if (m_indexStatus == S_INVALID_RELOAD) {
    m_ib->Lock(mem, m_numIndices, m_indexBase);
  } else {
    ASSERT(m_indexStatus == S_INVALID_DISCARD);
    m_ib->Lock(mem, m_numIndices, BASE_NONE);
    m_indexBase = m_ib->m_base;
  }
}

int CGxBufD3d::VBLValid() {
  return m_vbl && m_vbl->m_maxContiguousVertices >= m_numVertices;
}

int CGxBufD3d::IBValid() {
  return m_ib && m_ib->m_count >= m_numIndices;
}

void CGxBufD3d::Release() {
  UnsetVB();
  UnsetIB();
}

CVertexBufferList::CVertexBufferList() : m_numVerts(0), m_currentVB(0) {
}

void CVertexBufferList::Create(EGxVertexBufferFormat format, UINT numVerts) {
  CGxVertexBuffer_D3d    *vb;
  IDirect3DVertexBuffer9 *d3dvb;
  UINT                    verts;
  UINT                    vbVerts;

  if (numVerts && !m_numVerts) {
    m_maxContiguousVertices = 0;
    m_numVerts = 0;

    verts = numVerts;
    while (verts > MinD3dBufVertices) {
      vbVerts = verts;
      if (vbVerts >= 0x10000) {
        vbVerts = 0x10000;
      }

      ASSERT(CGxDeviceD3d::m_thisDevice);
      CGxDeviceD3d::m_thisDevice->ICreateD3dVB(format, vbVerts, d3dvb);

      if (!d3dvb) {
        break;
      }

      vb = NEW(CGxVertexBuffer_D3d)(format, d3dvb, vbVerts);
      m_vbList.Add(1, &vb);

      verts -= vbVerts;
      m_numVerts += vbVerts;
      if (vbVerts > m_maxContiguousVertices) {
        m_maxContiguousVertices = vbVerts;
      }

      if (!verts) {
        break;
      }
    }
  }
}

void CVertexBufferList::Release() {
  for (UINT i = 0; i < m_vbList.Count(); ++i) {
    DEL(m_vbList[i]);
  }

  m_vbList.Clear();
  m_numVerts = 0;
}

UINT CVertexBufferList::GetBase() {
  return m_vbList[m_currentVB]->m_base;
}

CGxVertexBuffer_D3d::CGxVertexBuffer_D3d(EGxVertexBufferFormat format, IDirect3DVertexBuffer9 *vb, UINT numVertices)
    : CGxVertexBuffer(numVertices), m_d3dvb(vb), m_vbFormat(format) {
}

void CGxVertexBuffer_D3d::Discard() {
  CGxMemBuffer::Discard();
  InvalidateBufs(CGxBuf::S_INVALID_DISCARD, CGxBuf::S_VALID);
}

void CGxVertexBuffer_D3d::Lock(LPVOID &mem, UINT numVertices, UINT base) {
  DWORD lockFlags;

  ASSERT(numVertices <= m_count);

  if (base == CGxBuf::BASE_NONE) {
    if (m_discard) {
      base = 0;
      m_base = 0;
      m_next = 0;
      m_discard = 0;
      m_next = numVertices;
      lockFlags = D3DLOCK_DISCARD | D3DLOCK_NOSYSLOCK;
    } else {
      ASSERT(m_next + numVertices <= m_count);
      m_base = m_next;
      base = m_base;
      m_next += numVertices;
      lockFlags = D3DLOCK_NOOVERWRITE | D3DLOCK_NOSYSLOCK;
    }
  } else {
    lockFlags = D3DLOCK_NOSYSLOCK;
  }

  if (m_d3dvb->Lock(base * GxVertexSize(m_vbFormat), numVertices * GxVertexSize(m_vbFormat), &mem, lockFlags) < 0) {
    ASSERT(0);
  }
}

CGxIndexBuffer_D3d::CGxIndexBuffer_D3d(IDirect3DIndexBuffer9 *ib, UINT numIndices) : CGxIndexBuffer(numIndices), m_d3dib(ib) {
}

CGxVertexBuffer_D3d *CVertexBufferList::Lock(LPVOID &mem, UINT numVertices, UINT base) {
  CGxVertexBuffer_D3d *vb = m_vbList[m_currentVB];

  while (vb->m_next + numVertices > vb->m_count) {
    if (vb->m_discard && numVertices <= vb->m_count) {
      break;
    }

    vb->Discard();

    if (++m_currentVB >= m_vbList.Count()) {
      m_currentVB = 0;
    }

    vb = m_vbList[m_currentVB];
  }

  vb->Lock(mem, numVertices, base);
  return vb;
}

CGxVertexBuffer_D3d::~CGxVertexBuffer_D3d() {
  ASSERT(CGxDeviceD3d::m_thisDevice);
  CGxDeviceD3d::m_thisDevice->IReleaseD3dVB(m_d3dvb);
}

void CGxVertexBuffer_D3d::Unlock() {
  m_d3dvb->Unlock();
}

void CGxIndexBuffer_D3d::Lock(LPVOID &mem, UINT numIndices, UINT base) {
  ASSERT(numIndices <= m_count);

  if (base == CGxBuf::BASE_NONE) {
    if (m_next + numIndices > m_count) {
      m_next = 0;
      InvalidateBufs(CGxBuf::S_VALID, CGxBuf::S_INVALID_DISCARD);
    }

    m_base = m_next;
    m_next += numIndices;
    base = m_base;
  }

  if (m_d3dib->Lock(2 * base, 2 * numIndices, &mem, 0) < 0) {
    ASSERT(0);
  }
}

void CGxIndexBuffer_D3d::Unlock() {
  m_d3dib->Unlock();
}

CGxIndexBuffer_D3d::~CGxIndexBuffer_D3d() {
  ASSERT(CGxDeviceD3d::m_thisDevice);
  CGxDeviceD3d::m_thisDevice->IReleaseD3dIB(m_d3dib);
}

void CGxDeviceD3d::BufReserve(EGxBufWriteFreq freq, EGxVertexBufferFormat format, UINT numVertices, UINT numIndices) {
  CGxDevice::BufReserve(freq, format, numVertices, numIndices);

  if (freq == GxBWF_Low || freq == GxBWF_Medium) {
    ITERATELIST(CGxBuf, m_bufList, buf) {
      CGxBufD3d *d3dBuf = static_cast<CGxBufD3d *>(buf);
      if (d3dBuf->m_vbFormat == format && d3dBuf->m_writeFreq == freq) {
        d3dBuf->Release();
      }
    }

    CVertexBufferList   &vbl = m_VBL[freq][format];
    CGxIndexBuffer_D3d *&ib = m_IB[freq][format];
    vbl.Release();
    if (ib) {
      DEL(ib);
      ib = 0;
    }
    ICreateBuffers(format, numVertices, vbl, numIndices, ib);
  }
}

CGxBuf *CGxDeviceD3d::BufCreate(
    EGxBufWriteFreq       writeFreq,
    EGxVertexBufferFormat format,
    UINT                  numVertices,
    UINT                  numIndices,
    void (*userCallback)(CGxBufCommand &, CGxBuf *),
    LPVOID userArg
) {
  CGxDevice::BufCreate(writeFreq, format, numVertices, numIndices, userCallback, userArg);

  CGxBufD3d *buf = NEW(CGxBufD3d);
  buf->m_writeFreq = writeFreq;
  buf->m_numVertices = numVertices;
  buf->m_numIndices = numIndices;
  buf->m_vbFormat = format;
  buf->m_userCallback = userCallback;
  buf->m_userArg = userArg;
  m_bufList.LinkNode(buf, LIST_TAIL, 0);
  return buf;
}

void CGxDeviceD3d::IBufSetBuffers(CGxBufD3d *buf) {
  if (buf->m_vb && buf->m_ib) {
    return;
  }

  EGxBufWriteFreq frequency = buf->m_writeFreq;
  if (frequency == GxBWF_Static) {
    ASSERT(0);
  } else if (frequency <= GxBWF_Medium) {
    if (!buf->VBLValid()) {
      buf->SetVBL(&m_VBL[frequency][buf->m_vbFormat]);
    }
    if (!buf->m_ib) {
      buf->SetIB(m_IB[frequency][buf->m_vbFormat]);
    }
  }

  if (!buf->VBLValid()) {
    buf->SetVBL(&m_VBL[GxBWF_Dynamic][buf->m_vbFormat]);
  }
  ASSERT(buf->VBLValid());

  if (!buf->IBValid()) {
    buf->SetIB(m_IB[GxBWF_Dynamic][0]);
  }
  ASSERT(buf->IBValid());
}

void CGxDeviceD3d::BufLock(CGxBuf *b) {
  CGxDevice::BufLock(b);

  CGxBufD3d *buf = static_cast<CGxBufD3d *>(b);
  IBufSetBuffers(buf);

  LPVOID        vmember[GxVertexMembers_Last];
  CGxBufCommand cmd;
  LPVOID        imem = 0;

  cmd.vertex.op = GxBufOp_Nop;
  cmd.index.op = GxBufOp_Nop;

  if (buf->m_vertexStatus != CGxBuf::S_VALID) {
    buf->LockVB(vmember[0]);
    for (UINT member = 0; member < GxVertexMembers_Last; ++member) {
      vmember[member] = static_cast<BYTE *>(vmember[0]) + GxVertexMemberOffset(buf->m_vbFormat, static_cast<EGxVertexMember>(member));
      cmd.vertex.mem[member] = &vmember[member];
      cmd.vertex.stride[member] = GxVertexSize(buf->m_vbFormat);
    }
    cmd.vertex.op = GxBufOp_Fill;
  }

  if (buf->m_indexStatus != CGxBuf::S_VALID) {
    buf->LockIB(imem);
    cmd.index.mem[GxVM_Indices] = &imem;
    cmd.index.stride[GxVM_Indices] = sizeof(WORD);
    cmd.index.op = GxBufOp_Fill;
  }

  if (cmd.vertex.op != GxBufOp_Nop || cmd.index.op != GxBufOp_Nop) {
    buf->m_userCallback(cmd, b);
    if (cmd.vertex.op != GxBufOp_Nop) {
      m_perfCountersAcc[GxPerf_VertexBytes] += buf->m_numVertices * GxVertexSize(buf->m_vbFormat);
    }
    if (cmd.index.op != GxBufOp_Nop) {
      m_perfCountersAcc[GxPerf_IndexBytes] += buf->m_numIndices * sizeof(WORD);
    }
  }

  if (buf->m_vertexStatus != CGxBuf::S_VALID) {
    buf->m_vb->Unlock();
    if (buf->m_writeFreq != GxBWF_Dynamic) {
      buf->m_vertexStatus = CGxBuf::S_VALID;
    }
  }
  if (buf->m_indexStatus != CGxBuf::S_VALID) {
    buf->m_ib->Unlock();
    if (buf->m_writeFreq != GxBWF_Dynamic) {
      buf->m_indexStatus = CGxBuf::S_VALID;
    }
  }

  ASSERT(m_d3dDevice->SetVertexShader(0) == 0);
  ASSERT(m_d3dDevice->SetFVF(s_vtxBufFmtConversion[buf->m_vbFormat]) == 0);
  ASSERT(m_d3dDevice->SetStreamSource(0, buf->m_vb->m_d3dvb, 0, GxVertexSize(buf->m_vbFormat)) == 0);
  ASSERT(m_d3dDevice->SetIndices(buf->m_ib->m_d3dib) == 0);
}

void CGxDeviceD3d::BufRender(const CGxBatch *batches, UINT count) {
  UINT       minIndex;
  UINT       numVertices;
  CGxBufD3d *buf;

  CGxDevice::BufRender(batches, count);
  IStateSync();

  buf = static_cast<CGxBufD3d *>(m_bufLocked);
  while (count--) {
    if (batches->m_count) {
      minIndex = batches->m_minIndex < 0 ? 0 : static_cast<UINT>(batches->m_minIndex);
      numVertices = batches->m_maxIndex < 0 ? m_bufLocked->VertexCount() : static_cast<UINT>(batches->m_maxIndex - minIndex);
      m_d3dDevice->DrawIndexedPrimitive(
          s_primitiveConversion[batches->m_primType], buf->m_vertexBase, minIndex, numVertices, buf->m_indexBase + batches->m_start,
          PrimCalcCount(batches->m_primType, batches->m_count)
      );
    }

    ++batches;
  }
}

void CGxDeviceD3d::BufUnlock() {
  CGxDevice::BufUnlock();
}

void CGxDeviceD3d::BufDestroy(CGxBuf *&b) {
  CGxDevice::BufDestroy(b);

  CGxBufD3d *d3dBuf = static_cast<CGxBufD3d *>(b);
  d3dBuf->UnsetVB();
  d3dBuf->UnsetIB();
  m_bufList.UnlinkNode(b);
  DEL(d3dBuf);
  b = 0;
}

static void IPrimSetupPos_PNT0(LPVOID __formal) {
  LPBYTE dst = static_cast<LPBYTE>(__formal);

  for (UINT i = 0; i < s_vertexCount; ++i) {
    *reinterpret_cast<NTempest::C3Vector *>(dst) = s_pos[i];
    *reinterpret_cast<NTempest::C3Vector *>(dst + 12) = s_normal[i];
    *reinterpret_cast<NTempest::C2Vector *>(dst + 24) = s_tex[0][i];
    dst += 32;
  }
}

void CGxDeviceD3d::IPrimSetupPos(LPVOID dstBuf) {
  if ((m_cpuFeatures & 0x2) && m_vertexShader == GxVS_PassThru && m_vertexBufferFormat == GxVBF_PNT0 && s_posStride == sizeof(NTempest::C3Vector) &&
      s_normalStride == sizeof(NTempest::C3Vector) && s_texStride[0] == sizeof(NTempest::C2Vector))
  {
    ASSERT((reinterpret_cast<UINT>(dstBuf) & 7) == 0);
    IPrimSetupPos_PNT0(dstBuf);
    return;
  }

  NTempest::C3Vector   tmpNormal;
  NTempest::C2Vector   tmpTex1;
  NTempest::C2Vector   tmpTex0;
  NTempest::CImVector  tmpColor;
  NTempest::C3Vector  *dstP = static_cast<NTempest::C3Vector *>(dstBuf);
  NTempest::C3Vector  *dstN = &tmpNormal;
  NTempest::CImVector *dstC = &tmpColor;
  NTempest::C2Vector  *dstT0 = &tmpTex0;
  NTempest::C2Vector  *dstT1 = &tmpTex1;
  UINT                 dpStride = GxVertexSize(m_vertexBufferFormat);
  UINT                 dnStride = 0;
  UINT                 dcStride = 0;
  UINT                 dt0Stride = 0;
  UINT                 dt1Stride = 0;
  BYTE                *dst = static_cast<BYTE *>(dstBuf);

  switch (m_vertexBufferFormat) {
    case GxVBF_PN:
      dstN = reinterpret_cast<NTempest::C3Vector *>(dst + 12);
      dnStride = GxVertexSize(m_vertexBufferFormat);
      break;
    case GxVBF_PNC:
      dstN = reinterpret_cast<NTempest::C3Vector *>(dst + 12);
      dstC = reinterpret_cast<NTempest::CImVector *>(dst + 24);
      dnStride = dcStride = GxVertexSize(m_vertexBufferFormat);
      break;
    case GxVBF_PNT0:
      dstN = reinterpret_cast<NTempest::C3Vector *>(dst + 12);
      dstT0 = reinterpret_cast<NTempest::C2Vector *>(dst + 24);
      dnStride = dt0Stride = GxVertexSize(m_vertexBufferFormat);
      break;
    case GxVBF_PNCT0:
      dstN = reinterpret_cast<NTempest::C3Vector *>(dst + 12);
      dstC = reinterpret_cast<NTempest::CImVector *>(dst + 24);
      dstT0 = reinterpret_cast<NTempest::C2Vector *>(dst + 28);
      dnStride = dcStride = dt0Stride = GxVertexSize(m_vertexBufferFormat);
      break;
    case GxVBF_PNT0T1:
      dstN = reinterpret_cast<NTempest::C3Vector *>(dst + 12);
      dstT0 = reinterpret_cast<NTempest::C2Vector *>(dst + 24);
      dstT1 = reinterpret_cast<NTempest::C2Vector *>(dst + 32);
      dnStride = dt0Stride = dt1Stride = GxVertexSize(m_vertexBufferFormat);
      break;
    case GxVBF_PNCT0T1:
      dstN = reinterpret_cast<NTempest::C3Vector *>(dst + 12);
      dstC = reinterpret_cast<NTempest::CImVector *>(dst + 24);
      dstT0 = reinterpret_cast<NTempest::C2Vector *>(dst + 28);
      dstT1 = reinterpret_cast<NTempest::C2Vector *>(dst + 36);
      dnStride = dcStride = dt0Stride = dt1Stride = GxVertexSize(m_vertexBufferFormat);
      break;
    case GxVBF_PCT0:
      dstC = reinterpret_cast<NTempest::CImVector *>(dst + 12);
      dstT0 = reinterpret_cast<NTempest::C2Vector *>(dst + 16);
      dcStride = dt0Stride = GxVertexSize(m_vertexBufferFormat);
      break;
    case GxVBF_PC:
      dstC = reinterpret_cast<NTempest::CImVector *>(dst + 12);
      dcStride = GxVertexSize(m_vertexBufferFormat);
      break;
    case GxVBF_PT0T1:
      dstT0 = reinterpret_cast<NTempest::C2Vector *>(dst + 12);
      dstT1 = reinterpret_cast<NTempest::C2Vector *>(dst + 20);
      dt0Stride = dt1Stride = GxVertexSize(m_vertexBufferFormat);
      break;
    default:
      ASSERT(0);
      break;
  }

  ASSERT(m_vertexShader == GxVS_PassThru || m_vertexShader == GxVS_Skin);

  for (UINT ndx = 0; ndx < s_vertexCount; ++ndx) {
    if (m_vertexShader == GxVS_Skin) {
      const NTempest::C34Matrix &bone = m_bones[*s_bone];

      dstP->x = bone.a0 * s_pos->x + bone.b0 * s_pos->y + bone.c0 * s_pos->z + bone.d0;
      dstP->y = bone.a1 * s_pos->x + bone.b1 * s_pos->y + bone.c1 * s_pos->z + bone.d1;
      dstP->z = bone.a2 * s_pos->x + bone.b2 * s_pos->y + bone.c2 * s_pos->z + bone.d2;

      dstN->x = bone.a0 * s_normal->x + bone.b0 * s_normal->y + bone.c0 * s_normal->z;
      dstN->y = bone.a1 * s_normal->x + bone.b1 * s_normal->y + bone.c1 * s_normal->z;
      dstN->z = bone.a2 * s_normal->x + bone.b2 * s_normal->y + bone.c2 * s_normal->z;
    } else {
      *dstP = *s_pos;
      *dstN = *s_normal;
    }

    *dstC = *s_color;
    *dstT0 = *s_tex[0];
    *dstT1 = *s_tex[1];

    s_pos = reinterpret_cast<const NTempest::C3Vector *>(reinterpret_cast<const BYTE *>(s_pos) + s_posStride);
    s_normal = reinterpret_cast<const NTempest::C3Vector *>(reinterpret_cast<const BYTE *>(s_normal) + s_normalStride);
    s_color = reinterpret_cast<const NTempest::CImVector *>(reinterpret_cast<const BYTE *>(s_color) + s_colorStride);
    s_tex[0] = reinterpret_cast<const NTempest::C2Vector *>(reinterpret_cast<const BYTE *>(s_tex[0]) + s_texStride[0]);
    s_tex[1] = reinterpret_cast<const NTempest::C2Vector *>(reinterpret_cast<const BYTE *>(s_tex[1]) + s_texStride[1]);

    if (m_vertexShader == GxVS_Skin) {
      s_bone += s_boneStride;
    }

    dstP = reinterpret_cast<NTempest::C3Vector *>(reinterpret_cast<BYTE *>(dstP) + dpStride);
    dstN = reinterpret_cast<NTempest::C3Vector *>(reinterpret_cast<BYTE *>(dstN) + dnStride);
    dstC = reinterpret_cast<NTempest::CImVector *>(reinterpret_cast<BYTE *>(dstC) + dcStride);
    dstT0 = reinterpret_cast<NTempest::C2Vector *>(reinterpret_cast<BYTE *>(dstT0) + dt0Stride);
    dstT1 = reinterpret_cast<NTempest::C2Vector *>(reinterpret_cast<BYTE *>(dstT1) + dt1Stride);
  }
}

void CGxDeviceD3d::ICreateD3dVB(EGxVertexBufferFormat format, UINT &numVertices, IDirect3DVertexBuffer9 *&vb) {
  ASSERT(numVertices > MinD3dBufVertices);

  DWORD bufFlags = D3DUSAGE_DYNAMIC | (m_d3dIsHwDevice ? D3DUSAGE_WRITEONLY : D3DUSAGE_SOFTWAREPROCESSING);

  vb = 0;
  while (numVertices > MinD3dBufVertices && !vb) {
    if (m_d3dDevice->CreateVertexBuffer(numVertices * GxVertexSize(format), bufFlags, s_vtxBufFmtConversion[format], D3DPOOL_DEFAULT, &vb, 0) < 0) {
      vb = 0;
      numVertices >>= 1;
    }
  }
}

void CGxDeviceD3d::ICreateD3dIB(UINT &numIndices, IDirect3DIndexBuffer9 *&ib) {
  ASSERT(numIndices > MinD3dBufIndices);

  DWORD bufFlags = D3DUSAGE_DYNAMIC | (m_d3dIsHwDevice ? D3DUSAGE_WRITEONLY : D3DUSAGE_SOFTWAREPROCESSING);

  ib = 0;
  while (numIndices > MinD3dBufIndices && !ib) {
    if (m_d3dDevice->CreateIndexBuffer(2 * numIndices, bufFlags, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &ib, 0) < 0) {
      ib = 0;
      numIndices >>= 1;
    }
  }
}

void CGxDeviceD3d::ICreateBuffers(
    EGxVertexBufferFormat vbFormat,
    UINT                  numVertices,
    CVertexBufferList    &vbl,
    UINT                  numIndices,
    CGxIndexBuffer_D3d  *&ib
) {
  vbl.Create(vbFormat, numVertices);

  if (!ib && numIndices) {
    IDirect3DIndexBuffer9 *d3dib;
    ICreateD3dIB(numIndices, d3dib);
    if (d3dib) {
      ib = NEW(CGxIndexBuffer_D3d)(d3dib, numIndices);
    }
  }
}

void CGxDeviceD3d::IReleaseD3dVB(IDirect3DVertexBuffer9 *&vb) {
  if (vb) {
    vb->Release();
    vb = 0;
  }
}

void CGxDeviceD3d::IReleaseD3dIB(IDirect3DIndexBuffer9 *&ib) {
  if (ib) {
    ib->Release();
    ib = 0;
  }
}

void CGxDeviceD3d::PrimLockAndProcessVertexPtrs(
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
  CGxDevice::PrimLockAndProcessVertexPtrs(
      vertexCount, pos, posStride, normal, normalStride, color, colorStride, bone, boneStride, tex0, tex0Stride, tex1, tex1Stride
  );

  s_vertexCount = vertexCount;
  s_pos = pos;
  s_posStride = posStride;
  s_normal = normal;
  s_normalStride = normalStride;
  s_color = color;
  s_colorStride = colorStride;
  s_bone = bone;
  s_texStride[1] = tex1Stride;
  s_tex[1] = tex1;
  s_boneStride = boneStride;
  s_tex[0] = tex0;
  s_texStride[0] = tex0Stride;
  m_processedVertexPtrs = 0;
}

void CGxDeviceD3d::IPrimProcessVertexPtrs() {
  if (m_processedVertexPtrs) {
    return;
  }

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

  int lighting;
  RsGet(GxRs_Lighting, lighting);
  RsGet(GxRs_MatDiffuse, diffuse);

  if (!lighting && *diffuse.IV_() != 0xFFFFFFFF && !IVbHasColor(m_vertexBufferFormat)) {
    s_color = &diffuse;
    s_colorStride = 0;
    m_vertexBufferFormat = IGiveVbColor(m_vertexBufferFormat);
  }

  LPVOID dst;
  m_vertexBuffer = m_VBL[GxBWF_Dynamic][m_vertexBufferFormat].Lock(dst, s_vertexCount, CGxBuf::BASE_NONE);
  IPrimSetupPos(dst);
  m_vertexBuffer->Unlock();

  ASSERT(m_d3dDevice->SetFVF(s_vtxBufFmtConversion[m_vertexBufferFormat]) == 0);
  ASSERT(m_d3dDevice->SetStreamSource(0, m_vertexBuffer->m_d3dvb, 0, GxVertexSize(m_vertexBufferFormat)) == 0);

  m_processedVertexPtrs = 1;
}

void CGxDeviceD3d::IPrimProcessIndexPtrs() {
  if (m_processedIndexPtrs) {
    return;
  }

  LPVOID dst;
  m_IB[GxBWF_Dynamic][0]->Lock(dst, s_indexCount, CGxBuf::BASE_NONE);
  memcpy(dst, s_indices, 2 * s_indexCount);
  m_IB[GxBWF_Dynamic][0]->Unlock();
  m_d3dDevice->SetIndices(m_IB[GxBWF_Dynamic][0]->m_d3dib);
  m_processedIndexPtrs = 1;
}

void CGxDeviceD3d::PrimLockIndexPtr(EGxPrim primType, UINT indexCount, const WORD *indices) {
  CGxDevice::PrimLockIndexPtr(primType, indexCount, indices);
  s_indices = indices;
  s_indexCount = indexCount;
  m_primIndexCount = indexCount;
  m_primType = primType;
  m_processedIndexPtrs = 0;
}

void CGxDeviceD3d::PrimDrawElements() {
  CGxDevice::PrimDrawElements();
  IStateSync();
  IPrimProcessVertexPtrs();
  IPrimProcessIndexPtrs();

  m_d3dDevice->DrawIndexedPrimitive(
      s_primitiveConversion[m_primType], m_VBL[GxBWF_Dynamic][m_vertexBufferFormat].GetBase(), 0, s_vertexCount, m_IB[GxBWF_Dynamic][0]->m_base,
      PrimCalcCount(m_primType, m_primIndexCount)
  );
}

void CGxDeviceD3d::PrimUnlockIndexPtr() {
  CGxDevice::PrimUnlockIndexPtr();
}

void CGxDeviceD3d::PrimUnlockVertexPtrs() {
  CGxDevice::PrimUnlockVertexPtrs();
}
