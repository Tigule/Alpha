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

static enum _D3DPRIMITIVETYPE s_primitiveConversion[GxPrims_Last] = {(enum _D3DPRIMITIVETYPE)1, (enum _D3DPRIMITIVETYPE)2,
                                                                     (enum _D3DPRIMITIVETYPE)3, (enum _D3DPRIMITIVETYPE)4,
                                                                     (enum _D3DPRIMITIVETYPE)5, (enum _D3DPRIMITIVETYPE)6};

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
  switch (m_vertexStatus) {
    case S_INVALID_DISCARD:
      UnsetVB();
      SetVB(m_vbl->Lock(mem, m_numVertices, BASE_NONE));
      m_vertexBase = m_vb->GetBase();
      break;

    case S_INVALID_RELOAD:
      m_vb->Lock(mem, m_numVertices, m_vertexBase);
      break;

    default:
      ASSERT(!("CGxBufD3d::LockVB(): invalid m_vertexStatus value"));
      break;
  }
}

void CGxBufD3d::LockIB(LPVOID &mem) {
  switch (m_indexStatus) {
    case S_INVALID_DISCARD:
      m_ib->Lock(mem, m_numIndices, BASE_NONE);
      m_indexBase = m_ib->GetBase();
      break;

    case S_INVALID_RELOAD:
      m_ib->Lock(mem, m_numIndices, m_indexBase);
      break;

    default:
      ASSERT(!("CGxBufD3d::LockIB(): invalid m_indexStatus value"));
      break;
  }
}

BOOL CGxBufD3d::VBLValid() {
  if (!m_vbl) {
    return 0;
  }

  return m_numVertices <= m_vbl->MaxContiguousVertices();
}

BOOL CGxBufD3d::IBValid() {
  if (!m_ib) {
    return 0;
  }

  return m_numIndices <= m_ib->GetCount();
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

  if (numVerts && !m_numVerts) {
    m_maxContiguousVertices = 0;
    m_numVerts = 0;

    verts = numVerts;
    while (verts) {
      if (verts <= CGxDevice::MinD3dBufVertices) {
        break;
      }

      UINT vbVerts = min(verts, 0x10000);

      CGxDeviceD3d::GetDevice()->ICreateD3dVB(format, vbVerts, d3dvb);

      if (!d3dvb) {
        break;
      }

      vb = NEW(CGxVertexBuffer_D3d)(format, d3dvb, vbVerts);
      *m_vbList.New() = vb;

      verts -= vbVerts;
      m_numVerts += vbVerts;
      m_maxContiguousVertices = max(m_maxContiguousVertices, vbVerts);
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

UINT CVertexBufferList::GetBase() {
  return m_vbList[m_currentVB]->m_base;
}

CGxVertexBuffer_D3d::CGxVertexBuffer_D3d(EGxVertexBufferFormat format, IDirect3DVertexBuffer9 *vb, UINT numVertices)
    : CGxVertexBuffer(numVertices), m_d3dvb(vb), m_vbFormat(format) {
}

void CGxVertexBuffer_D3d::Lock(LPVOID &mem, UINT numVertices, UINT base) {
  DWORD lockFlags;
  UINT  lockBase;

  ASSERT(numVertices <= m_count);

  if (base == CGxBuf::BASE_NONE) {
    if (m_discard) {
      m_next = 0;
      lockFlags = D3DLOCK_DISCARD | D3DLOCK_NOSYSLOCK;
      m_discard = 0;
    } else {
      ASSERT(m_next + numVertices <= m_count);
      lockFlags = D3DLOCK_NOOVERWRITE | D3DLOCK_NOSYSLOCK;
    }

    m_base = m_next;
    m_next += numVertices;
    lockBase = m_base;
  } else {
    lockFlags = D3DLOCK_NOSYSLOCK;
    lockBase = base;
  }

  if (m_d3dvb->Lock(lockBase * GxVertexSize(m_vbFormat), numVertices * GxVertexSize(m_vbFormat), &mem, lockFlags) < 0) {
    ASSERT(0);
  }
}

void CGxVertexBuffer_D3d::Discard() {
  CGxMemBuffer::Discard();
  InvalidateBufs(CGxBuf::S_INVALID_DISCARD, CGxBuf::S_VALID);
}

void CGxVertexBuffer_D3d::Unlock() {
  m_d3dvb->Unlock();
}

CGxVertexBuffer_D3d::~CGxVertexBuffer_D3d() {
  CGxDeviceD3d::GetDevice()->IReleaseD3dVB(m_d3dvb);
}

CGxIndexBuffer_D3d::CGxIndexBuffer_D3d(IDirect3DIndexBuffer9 *ib, UINT numIndices) : CGxIndexBuffer(numIndices), m_d3dib(ib) {
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
  CGxDeviceD3d::GetDevice()->IReleaseD3dIB(m_d3dib);
}

void CGxDeviceD3d::BufReserve(EGxBufWriteFreq freq, EGxVertexBufferFormat format, UINT numVertices, UINT numIndices) {
  CGxDevice::BufReserve(freq, format, numVertices, numIndices);

  if (freq == GxBWF_Low || freq == GxBWF_Medium) {
    ITERATELIST(CGxBuf, m_bufList, buf) {
      CGxBufD3d *d3dBuf = (CGxBufD3d *)buf;
      if (d3dBuf->m_vbFormat == format && d3dBuf->m_writeFreq == freq) {
        d3dBuf->Release();
      }
    }

    m_VBL[freq][format].Release();
    if (m_IB[freq][format]) {
      DEL(m_IB[freq][format]);
      m_IB[freq][format] = 0;
    }
    ICreateBuffers(format, numVertices, m_VBL[freq][format], numIndices, m_IB[freq][format]);
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

  switch (buf->m_writeFreq) {
    case GxBWF_Static:
      ASSERT(0);
      break;

    case GxBWF_Low:
    case GxBWF_Medium:
      if (!buf->VBLValid()) {
        buf->SetVBL(&m_VBL[buf->m_writeFreq][buf->m_vbFormat]);
      }
      if (!buf->m_ib) {
        buf->SetIB(m_IB[buf->m_writeFreq][buf->m_vbFormat]);
      }
      break;
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

  CGxBufD3d *buf = (CGxBufD3d *)b;
  IBufSetBuffers(buf);

  LPVOID        vmember[GxVertexMembers_Last];
  CGxBufCommand cmd;
  LPVOID        imem = 0;
  LPVOID        vmem = 0;

  cmd.vertex.op = GxBufOp_Nop;
  cmd.index.op = GxBufOp_Nop;

  if (buf->m_vertexStatus != CGxBuf::S_VALID) {
    buf->LockVB(vmem);
    for (UINT member = 0; member < GxVertexMembers_Last; ++member) {
      vmember[member] = (BYTE *)vmem + GxVertexMemberOffset(buf->m_vbFormat, (EGxVertexMember)member);
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
    buf->m_userCallback(cmd, buf);
    if (cmd.vertex.op != GxBufOp_Nop) {
      m_perfCountersAcc[GxPerf_VertexBytes] += GxVertexSize(buf->m_vbFormat) * buf->m_numVertices;
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

  HRESULT res = m_d3dDevice->SetVertexShader(0);
  ASSERT(res == ((HRESULT)0x00000000L));
  res = m_d3dDevice->SetFVF(s_vtxBufFmtConversion[buf->m_vbFormat]);
  ASSERT(res == ((HRESULT)0x00000000L));
  res = m_d3dDevice->SetStreamSource(0, buf->m_vb->GetD3dBuffer(), 0, GxVertexSize(buf->m_vbFormat));
  ASSERT(res == ((HRESULT)0x00000000L));
  res = m_d3dDevice->SetIndices(buf->m_ib->GetD3dBuffer());
  ASSERT(res == ((HRESULT)0x00000000L));
}

void CGxDeviceD3d::BufRender(const CGxBatch *batches, UINT count) {
  UINT       minIndex;
  UINT       numVertices;
  CGxBufD3d *buf;

  CGxDevice::BufRender(batches, count);
  IStateSync();

  buf = (CGxBufD3d *)m_bufLocked;
  while (count--) {
    if (batches->m_count) {
      minIndex = batches->m_minIndex < 0 ? 0 : (UINT)batches->m_minIndex;
      numVertices = batches->m_maxIndex < 0 ? m_bufLocked->VertexCount() : (batches->m_maxIndex - minIndex);
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

  CGxBufD3d *d3dBuf = (CGxBufD3d *)b;
  d3dBuf->UnsetVB();
  d3dBuf->UnsetIB();
  m_bufList.UnlinkNode(b);
  DEL(d3dBuf);
  b = 0;
}

__declspec(naked) void IPrimSetupPos_PNT0(LPVOID) {
  __asm {
    push ebx
    push edi
    mov edi, ecx
    mov eax, s_pos
    mov ebx, s_normal
    mov ecx, s_vertexCount
    shr ecx, 1
    mov edx, s_tex
    jz done_dloop
    jmp dloop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
  dloop:
    movq mm0, [eax]
    movq mm1, [eax + 8]
    movq mm3, [eax + 16]
    movq mm4, [ebx]
    movq mm2, [ebx + 8]
    movq mm5, [ebx + 16]
    movq mm6, mm1
    movq mm7, mm4
    psllq mm4, 32
    pand mm1, s_lomask
    por mm1, mm4
    movq mm4, mm2
    psrlq mm7, 32
    psllq mm2, 32
    por mm2, mm7
    movq mm7, mm3
    psrlq mm6, 32
    psllq mm3, 32
    por mm3, mm6
    movq mm6, [edx]
    psrlq mm7, 32
    pand mm4, s_himask
    por mm4, mm7
    movq mm7, [edx + 8]
    movq [edi], mm0
    movq [edi + 8], mm1
    movq [edi + 16], mm2
    movq [edi + 24], mm6
    movq [edi + 32], mm3
    movq [edi + 40], mm4
    movq [edi + 48], mm5
    movq [edi + 56], mm7
    add eax, 24
    add ebx, 24
    add edx, 16
    add edi, 64
    dec ecx
    jnz dloop
  done_dloop:
    mov ecx, s_vertexCount
    and ecx, 1
    jz done_dlast
    movq mm0, [eax]
    movd mm1, [eax + 8]
    movd mm4, [ebx]
    movq mm2, [ebx + 4]
    movq mm3, [edx]
    psllq mm4, 32
    por mm1, mm4
    movq [edi], mm0
    movq [edi + 8], mm1
    movq [edi + 16], mm2
    movq [edi + 24], mm3
  done_dlast:
    pop edi
    pop ebx
    emms
    ret
  }
}

void CGxDeviceD3d::IPrimSetupPos(LPVOID dstBuf) {
  if ((m_cpuFeatures & 0x2) && m_vertexShader == GxVS_PassThru && m_vertexBufferFormat == GxVBF_PNT0 && s_posStride == sizeof(NTempest::C3Vector) &&
      s_normalStride == sizeof(NTempest::C3Vector) && s_texStride[0] == sizeof(NTempest::C2Vector))
  {
    ASSERT((((int)dstBuf) & 7) == 0);
    IPrimSetupPos_PNT0(dstBuf);
    return;
  }

  NTempest::C3Vector  tmpNormal;
  NTempest::C2Vector  tmpTex1;
  NTempest::C2Vector  tmpTex0;
  DWORD               tmpColor;
  NTempest::C3Vector *dstP = (NTempest::C3Vector *)dstBuf;
  NTempest::C3Vector *dstN = &tmpNormal;
  DWORD              *dstC = &tmpColor;
  NTempest::C2Vector *dstT0 = &tmpTex0;
  NTempest::C2Vector *dstT1 = &tmpTex1;
  UINT                dpStride = GxVertexSize(m_vertexBufferFormat);
  UINT                dnStride = 0;
  UINT                dcStride = 0;
  UINT                dt0Stride = 0;
  UINT                dt1Stride = 0;

  switch (m_vertexBufferFormat) {
    case GxVBF_PN:
      dstN = (NTempest::C3Vector *)((BYTE *)dstBuf + 12);
      dnStride = GxVertexSize(GxVBF_PN);
      break;
    case GxVBF_PNC:
      dstN = (NTempest::C3Vector *)((BYTE *)dstBuf + 12);
      dstC = (DWORD *)((BYTE *)dstBuf + 24);
      dnStride = dcStride = GxVertexSize(GxVBF_PNC);
      break;
    case GxVBF_PNT0:
      dstN = (NTempest::C3Vector *)((BYTE *)dstBuf + 12);
      dstT0 = (NTempest::C2Vector *)((BYTE *)dstBuf + 24);
      dnStride = dt0Stride = GxVertexSize(GxVBF_PNT0);
      break;
    case GxVBF_PNCT0:
      dstN = (NTempest::C3Vector *)((BYTE *)dstBuf + 12);
      dstC = (DWORD *)((BYTE *)dstBuf + 24);
      dstT0 = (NTempest::C2Vector *)((BYTE *)dstBuf + 28);
      dnStride = dcStride = dt0Stride = GxVertexSize(GxVBF_PNCT0);
      break;
    case GxVBF_PNT0T1:
      dstN = (NTempest::C3Vector *)((BYTE *)dstBuf + 12);
      dstT0 = (NTempest::C2Vector *)((BYTE *)dstBuf + 24);
      dstT1 = (NTempest::C2Vector *)((BYTE *)dstBuf + 32);
      dnStride = dt0Stride = dt1Stride = GxVertexSize(GxVBF_PNT0T1);
      break;
    case GxVBF_PNCT0T1:
      dstN = (NTempest::C3Vector *)((BYTE *)dstBuf + 12);
      dstC = (DWORD *)((BYTE *)dstBuf + 24);
      dstT0 = (NTempest::C2Vector *)((BYTE *)dstBuf + 28);
      dstT1 = (NTempest::C2Vector *)((BYTE *)dstBuf + 36);
      dnStride = dcStride = dt0Stride = dt1Stride = GxVertexSize(GxVBF_PNCT0T1);
      break;
    case GxVBF_PCT0:
      dstC = (DWORD *)((BYTE *)dstBuf + 12);
      dstT0 = (NTempest::C2Vector *)((BYTE *)dstBuf + 16);
      dcStride = dt0Stride = GxVertexSize(GxVBF_PCT0);
      break;
    case GxVBF_PC:
      dstC = (DWORD *)((BYTE *)dstBuf + 12);
      dcStride = GxVertexSize(GxVBF_PC);
      break;
    case GxVBF_PT0T1:
      dstT0 = (NTempest::C2Vector *)((BYTE *)dstBuf + 12);
      dstT1 = (NTempest::C2Vector *)((BYTE *)dstBuf + 20);
      dt0Stride = dt1Stride = GxVertexSize(GxVBF_PT0T1);
      break;
    default:
      ASSERT(0);
      break;
  }

  switch (m_vertexShader) {
    case GxVS_PassThru: {
      for (UINT ndx = 0; ndx != s_vertexCount; ++ndx) {
        *dstP = *s_pos;
        *dstN = *s_normal;
        *dstC = *(const DWORD *)s_color;
        *dstT0 = *s_tex[0];
        *dstT1 = *s_tex[1];

        s_pos = (const NTempest::C3Vector *)((const BYTE *)s_pos + s_posStride);
        s_normal = (const NTempest::C3Vector *)((const BYTE *)s_normal + s_normalStride);
        s_color = (const NTempest::CImVector *)((const BYTE *)s_color + s_colorStride);
        s_tex[0] = (const NTempest::C2Vector *)((const BYTE *)s_tex[0] + s_texStride[0]);
        s_tex[1] = (const NTempest::C2Vector *)((const BYTE *)s_tex[1] + s_texStride[1]);

        dstP = (NTempest::C3Vector *)((BYTE *)dstP + dpStride);
        dstN = (NTempest::C3Vector *)((BYTE *)dstN + dnStride);
        dstC = (DWORD *)((BYTE *)dstC + dcStride);
        dstT0 = (NTempest::C2Vector *)((BYTE *)dstT0 + dt0Stride);
        dstT1 = (NTempest::C2Vector *)((BYTE *)dstT1 + dt1Stride);
      }
      break;
    }

    case GxVS_Skin: {
      for (UINT ndx = 0; ndx != s_vertexCount; ++ndx) {
        const NTempest::C44Matrix &bone = m_bones[*s_bone];

        dstP->x = bone.a0 * s_pos->x + bone.b0 * s_pos->y + bone.c0 * s_pos->z + bone.d0;
        dstP->y = bone.a1 * s_pos->x + bone.b1 * s_pos->y + bone.c1 * s_pos->z + bone.d1;
        dstP->z = bone.a2 * s_pos->x + bone.b2 * s_pos->y + bone.c2 * s_pos->z + bone.d2;

        dstN->x = bone.a0 * s_normal->x + bone.b0 * s_normal->y + bone.c0 * s_normal->z;
        dstN->y = bone.a1 * s_normal->x + bone.b1 * s_normal->y + bone.c1 * s_normal->z;
        dstN->z = bone.a2 * s_normal->x + bone.b2 * s_normal->y + bone.c2 * s_normal->z;

        *dstC = *(const DWORD *)s_color;
        *dstT0 = *s_tex[0];
        *dstT1 = *s_tex[1];

        s_pos = (const NTempest::C3Vector *)((const BYTE *)s_pos + s_posStride);
        s_normal = (const NTempest::C3Vector *)((const BYTE *)s_normal + s_normalStride);
        s_color = (const NTempest::CImVector *)((const BYTE *)s_color + s_colorStride);
        s_bone += s_boneStride;
        s_tex[0] = (const NTempest::C2Vector *)((const BYTE *)s_tex[0] + s_texStride[0]);
        s_tex[1] = (const NTempest::C2Vector *)((const BYTE *)s_tex[1] + s_texStride[1]);

        dstP = (NTempest::C3Vector *)((BYTE *)dstP + dpStride);
        dstN = (NTempest::C3Vector *)((BYTE *)dstN + dnStride);
        dstC = (DWORD *)((BYTE *)dstC + dcStride);
        dstT0 = (NTempest::C2Vector *)((BYTE *)dstT0 + dt0Stride);
        dstT1 = (NTempest::C2Vector *)((BYTE *)dstT1 + dt1Stride);
      }
      break;
    }

    default:
      ASSERT(0);
      break;
  }
}

void CGxDeviceD3d::ICreateD3dVB(EGxVertexBufferFormat format, UINT &numVertices, IDirect3DVertexBuffer9 *&vb) {
  ASSERT(numVertices > MinD3dBufVertices);

  DWORD bufFlags = m_d3dIsHwDevice ? D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY : D3DUSAGE_DYNAMIC | D3DUSAGE_SOFTWAREPROCESSING;

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

  DWORD bufFlags = m_d3dIsHwDevice ? D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY : D3DUSAGE_DYNAMIC | D3DUSAGE_SOFTWAREPROCESSING;

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

  static NTempest::CImVector diffuse;
  int                        lighting;
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

  HRESULT res = m_d3dDevice->SetFVF(s_vtxBufFmtConversion[m_vertexBufferFormat]);
  ASSERT(res == ((HRESULT)0x00000000L));
  res = m_d3dDevice->SetStreamSource(0, m_vertexBuffer->GetD3dBuffer(), 0, GxVertexSize(m_vertexBufferFormat));
  ASSERT(res == ((HRESULT)0x00000000L));

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
      s_primitiveConversion[m_primType], m_VBL[GxBWF_Dynamic][m_vertexBufferFormat].GetBase(), 0, s_vertexCount,
      m_IB[GxBWF_Dynamic][0]->GetBase(), PrimCalcCount(m_primType, m_primIndexCount)
  );
}

void CGxDeviceD3d::PrimUnlockIndexPtr() {
  CGxDevice::PrimUnlockIndexPtr();
}

void CGxDeviceD3d::PrimUnlockVertexPtrs() {
  CGxDevice::PrimUnlockVertexPtrs();
}
