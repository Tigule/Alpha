#include "../CGxDevice.h"

#include <stpl.h>

#include <storm.h>

#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <new>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>

#include "Os/W32/Debugging.h"
#include "Os/W32/OSSystem.h"
#include "Os/OsTime.h"
#include "Tempest/c34matrix.h"

static LONGLONG times[8];
static LONGLONG start;
static LONGLONG stop;
static UINT     tail;
static float    frequency;

static UINT UpdateFrameRate() {
  LONGLONG total = 0;
  UINT     frameRate;

  stop = CGxDevice::CpuTicks();
  times[tail] = stop - start;

  for (UINT i = 0; i < 8; ++i) {
    total += times[i];
  }

  if (total > 1024) {
    frameRate = static_cast<UINT>(CGxDevice::CpuFrequency() * 8.0f / static_cast<double>(total));
  } else {
    frameRate = 99;
  }

  tail = (tail + 1) & 7;
  start = stop;
  return frameRate;
}

void CGxDevice::ClampRectToWindow(NTempest::CiRect &rect) {
  const NTempest::CRect &window = DeviceCurWindow();
  long                   top = static_cast<long>(window.t);
  long                   left = static_cast<long>(window.l);
  long                   bottom = static_cast<long>(window.b);
  long                   right = static_cast<long>(window.r);
  long                   clippedTop = rect.t;
  long                   clippedRight = rect.r;

  if (clippedRight >= right) {
    clippedRight = right;
  }
  if (rect.b < bottom) {
    bottom = rect.b;
  }
  if (rect.l > left) {
    left = rect.l;
  }
  if (clippedTop <= top) {
    clippedTop = top;
  }

  rect.t = clippedTop;
  rect.l = left;
  rect.b = bottom;
  rect.r = clippedRight;
}

static LPCSTR FmtNames[CGxFormat::Formats_Last] = {"Rgb565", "ArgbX888", "Argb8888", "Argb2101010", "Ds160", "Ds24X", "Ds248", "Ds320"};
static UINT   s_primVtxDiv[GxPrims_Last] = {1, 2, 1, 3, 1, 1};
static UINT   s_primVtxAdjust[GxPrims_Last] = {0, 0, 1, 0, 2, 2};
static UINT   s_alphaRef[GxBlends_Last] = {0, 224, 1, 1, 1, 1, 1, 0};

const UINT CGxDevice::s_texFormatBitDepth[GxTexFormats_Last] = {0, 32, 16, 16, 16, 4, 8, 8};

CGxLight::CGxLight() {
  m_enabled = m_isOmni = 0;
  m_dir = NTempest::C3Vector(0.0f, 0.0f, 1.0f);
  m_ambColor = NTempest::CImVector(0xFF000000);
  m_dirColor = NTempest::CImVector(0xFFFFFFFF);
  m_specColor = NTempest::CImVector(0x00000000ul);
  m_ambIntensity = 1.0f;
  m_dirIntensity = 1.0f;
  m_specIntensity = 0.0f;
  m_quadraticAttenuation = 0.03f;
  m_constantAttenuation = 0.0f;
  m_linearAttenuation = 0.7f;
  m_attenStart = 0.0f;
  m_attenEnd = 0.0f;
}

CGxStateRegister::CGxStateRegister() {
  for (UINT i = 0; i < 8; ++i) {
    m_lightsDirty[i] = 1;
  }

  m_lightLinearFalloff = 0.0f;
  m_lightQuadraticFalloff = 0.0f;
  m_masterEnables = 0;
}

CGxMatrixStack::CGxMatrixStack() : m_level(0), m_dirty(0) {
  memset(m_flags, 0, sizeof(m_flags));
  m_flags[0] = F_Identity;
}

CGxMatrixStack::~CGxMatrixStack() {
  ASSERT(m_level == 0);
}

void CGxMatrixStack::Identity() {
  if (!(m_flags[m_level] & F_Identity)) {
    NTempest::C44Matrix *matrix = &m_mtx[m_level];
    matrix->d3 = 1.0f;
    matrix->c2 = 1.0f;
    matrix->b1 = 1.0f;
    matrix->a0 = 1.0f;
    matrix->d2 = 0.0f;
    matrix->d1 = 0.0f;
    matrix->d0 = 0.0f;
    matrix->c3 = 0.0f;
    matrix->c1 = 0.0f;
    matrix->c0 = 0.0f;
    matrix->b3 = 0.0f;
    matrix->b2 = 0.0f;
    matrix->b0 = 0.0f;
    matrix->a3 = 0.0f;
    matrix->a2 = 0.0f;
    matrix->a1 = 0.0f;
    m_dirty = 1;
    m_flags[m_level] = F_Identity;
  }
}

void CGxMatrixStack::Push() {
  ++m_level;
  ASSERT(m_level < Gx_MaxMatrixStackDepth);

  if (m_level >= Gx_MaxMatrixStackDepth) {
    m_level = Gx_MaxMatrixStackDepth - 1;
  }

  m_mtx[m_level] = m_mtx[m_level - 1];
  m_flags[m_level] = m_flags[m_level - 1];
  m_dirty = 1;
}

void CGxMatrixStack::Pop() {
  ASSERT(m_level > 0);

  if (m_level > 0) {
    --m_level;
  }

  m_dirty = 1;
}

NTempest::C44Matrix &CGxMatrixStack::Top() {
  m_dirty = 1;
  m_flags[m_level] &= ~F_Identity;
  return m_mtx[m_level];
}

CGxBuf::CGxBuf() {
  m_numVertices = 0;
  m_vertexStatus = S_INVALID_DISCARD;
  m_vertexBase = BASE_NONE;
  m_numIndices = 0;
  m_indexStatus = S_INVALID_DISCARD;
  m_indexBase = BASE_NONE;
}

const UINT CGxBuf::BASE_NONE = 0xFFFFFFFF;

void CGxBuf::Invalidate(Status vertexStatus, Status indexStatus) {
  ASSERT(m_writeFreq != GxBWF_Static);

  if (m_vertexStatus == S_VALID) {
    m_vertexStatus = vertexStatus;
  }

  if (m_indexStatus == S_VALID) {
    m_indexStatus = indexStatus;
  }
}

void CGxBuf::CountSet(UINT numVertices, UINT numIndices) {
  ASSERT(m_writeFreq != GxBWF_Static);
  ASSERT(numVertices <= Gx_MaxVertices);
  ASSERT(numIndices <= Gx_MaxIndices);

  m_numIndices = numIndices;
  m_numVertices = numVertices;
  m_vertexStatus = S_INVALID_DISCARD;
  m_indexStatus = S_INVALID_DISCARD;
}

CGxTex::CGxTex(
    UINT         width,
    UINT         height,
    EGxTexFormat format,
    CGxTexFlags  flags,
    LPVOID       userArg,
    void (*userFunc)(EGxTexCommand, UINT, UINT, UINT, UINT, LPVOID, UINT &, LPCVOID &)
)
    : m_updateRect(), m_flags(GxTex_Linear, 0, 0, 0, 0, 0, 1) {
  Init(GxTex_2d, width, height, 0, format, format, flags, userArg, userFunc);
}

CGxTex::CGxTex(
    EGxTexTarget target,
    UINT         width,
    UINT         height,
    UINT         depth,
    EGxTexFormat format,
    EGxTexFormat dataFormat,
    CGxTexFlags  flags,
    LPVOID       userArg,
    void (*userFunc)(EGxTexCommand, UINT, UINT, UINT, UINT, LPVOID, UINT &, LPCVOID &)
)
    : m_updateRect(), m_flags(GxTex_Linear, 0, 0, 0, 0, 0, 1) {
  Init(target, width, height, depth, format, dataFormat, flags, userArg, userFunc);
}

void CGxTex::Init(
    EGxTexTarget target,
    UINT         width,
    UINT         height,
    UINT         depth,
    EGxTexFormat format,
    EGxTexFormat dataFormat,
    CGxTexFlags  flags,
    LPVOID       userArg,
    void (*userFunc)(EGxTexCommand, UINT, UINT, UINT, UINT, LPVOID, UINT &, LPCVOID &)
) {
  m_needsUpdate = 1;
  m_needsFlagUpdate = 1;
  m_needsCreation = 1;
  m_updateRect.t = 0;
  m_updatePlaneMin = -1;
  m_updatePlaneMax = -1;
  m_updateRect.l = 0;
  m_target = target;
  m_updateRect.b = height;
  m_depth = depth;
  m_format = format;
  m_updateRect.r = width;
  m_width = width;
  m_height = height;
  m_dataFormat = dataFormat;
  m_flags = flags;
  m_userArg = userArg;
  m_userFunc = userFunc;
  m_apiSpecificData = 0;
}

CGxMemBuffer::CGxMemBuffer(UINT count) : m_count(count), m_base(0), m_next(0), m_discard(0) {
}

void CGxMemBuffer::InvalidateBufs(CGxBuf::Status vertexStatus, CGxBuf::Status indexStatus) {
  ITERATELIST(CGxBuf, m_bufList, buf) {
    buf->Invalidate(vertexStatus, indexStatus);
  }
}

void CGxMemBuffer::AddBuf(CGxBuf *buf) {
  m_bufList.LinkNode(buf, LIST_LINK_BEFORE, 0);
}

void CGxMemBuffer::Discard() {
  m_discard = 1;
}

void CGxMemBuffer::RemoveBuf(CGxBuf *buf) {
  m_bufList.UnlinkNode(buf);
}

CGxMemBuffer::~CGxMemBuffer() {
}

void CGxGammaRamp::Set(float gamma) {
  for (UINT i = 0; i < ENTRIES; ++i) {
    WORD value = static_cast<WORD>(pow(static_cast<float>(i) / 255.0f, gamma) * 65535.0f);
    red[i] = value;
    green[i] = value;
    blue[i] = value;
  }
}

HSLOG CGxDevice::m_log;

CGxDevice::CGxDevice() {
  IRsInit();

  memset(m_perfCountersLatched, 0, sizeof(m_perfCountersLatched));
  memset(m_perfCountersAcc, 0, sizeof(m_perfCountersAcc));
  m_primType = GxPrims_Last;
  m_primIndexCount = 0;

  m_api = GxApis_Last;
  m_windowProc = 0;
  m_cpuFeatures = OsGetProcessorFeatures();
  memset(&m_caps, 0, sizeof(m_caps));
  m_caps.m_maxTexAnisotropy = 1;
  m_baseMipLevel = 0;
  m_force32BitTextures = 0;
  m_clearColor = NTempest::CImVector(0xFF000000);
  m_viewport.x.Set(0.0f, 1.0f);
  m_viewport.y.Set(0.0f, 1.0f);
  m_viewport.z.Set(0.0f, 1.0f);
  m_bones = 0;
  m_boneCount = 0;
  m_vertexShader = GxVS_PassThru;
  m_vertexBufferFormat = GxVertexBufferFormats_Last;

  UINT i;
  for (i = 0; i < 8; ++i) {
    m_appState.m_lightsDirty[i] = 0;
    m_hwState.m_lightsDirty[i] = 0;
  }
  m_appState.m_masterEnables = (1u << GxMasterEnables_Last) - 1;
  m_hwState.m_masterEnables = (1u << GxMasterEnables_Last) - 1;

  m_scrShotClick = 0;
  m_scrShotWidth = 0;
  m_scrShotHeight = 0;
  m_indexLocked = 0;
  m_vertexLocked = 0;
  m_inBeginEnd = 0;
  m_bufLocked = 0;

  memset(m_VBReserve, 0, sizeof(m_VBReserve));
  memset(m_IBReserve, 0, sizeof(m_IBReserve));
  for (i = 0; i < GxVertexBufferFormats_Last; ++i) {
    m_VBReserve[GxBWF_Dynamic][i] = 0x4000;
    m_IBReserve[GxBWF_Dynamic][i] = 0x4000;
  }
  memset(m_dynBuf, 0, sizeof(m_dynBuf));

  m_gammaRamp.Set(1.0f);
  m_systemGammaRamp.Set(1.0f);
  memset(m_textureTarget, 0, sizeof(m_textureTarget));
  m_context = 0;
}

CGxDevice::~CGxDevice() {
  UINT count = m_textures.Count();
  while (count) {
    if (m_textures[--count]->m_apiSpecificData) {
      ASSERT(0);
    }
  }
  m_textures.SetCount(0);
}

void CGxDevice::DestroyDynamicBufs() {
  for (int i = 0; i < 9; ++i) {
    if (m_dynBuf[i]) {
      BufDestroy(m_dynBuf[i]);
    }
  }
}

void CGxDevice::CreateDynamicBufs() {
  for (UINT format = 0; format < GxVertexBufferFormats_Last; ++format) {
    m_dynBuf[format] = BufCreate(GxBWF_Dynamic, static_cast<EGxVertexBufferFormat>(format), 1, 1, 0, 0);
  }
}

BOOL CGxDevice::DeviceCreate(GXWINDOWPROC windowProc, const CGxFormat &format) {
  m_windowProc = windowProc;
  CreateDynamicBufs();
  return DeviceSetFormat(format);
}

BOOL CGxDevice::DeviceCreate(UINT hwnd, const CGxFormat &format) {
  CreateDynamicBufs();
  m_format = format;
  return 1;
}

void CGxDevice::DeviceDestroy() {
  m_windowProc = 0;
  DestroyDynamicBufs();
}

BOOL CGxDevice::DeviceSetFormat(const CGxFormat &format) {
  m_format = format;
  return 1;
}

void CGxDevice::DeviceSetBaseMipLevel(UINT baseMipLevel) {
  m_baseMipLevel = baseMipLevel;
}

void CGxDevice::DeviceSetGamma(float gamma) {
  m_gammaRamp.Set(gamma);
}

void CGxDevice::DeviceSetGamma(const CGxGammaRamp &ramp) {
  if (&ramp != &m_gammaRamp) {
    m_gammaRamp = ramp;
  }
}

void CGxDevice::DeviceSetTextureQuality(int force32) {
  m_force32BitTextures = force32;
}

const CGxFormat &CGxDevice::DeviceFormat() {
  return m_format;
}

UINT CGxDevice::DeviceBaseMipLevel() {
  return m_baseMipLevel;
}

void CGxDevice::DeviceGamma(CGxGammaRamp &ramp) {
  if (&ramp != &m_gammaRamp) {
    ramp = m_gammaRamp;
  }
}

void CGxDevice::DeviceSystemGamma(CGxGammaRamp &ramp) {
  if (&ramp != &m_systemGammaRamp) {
    ramp = m_systemGammaRamp;
  }
}

int CGxDevice::DeviceTextureQuality() {
  return m_force32BitTextures;
}

EGxApi CGxDevice::DeviceApi() {
  return m_api;
}

void CGxDevice::DeviceTakeScreenShot() {
  m_scrShotClick = 1;
}

void CGxDevice::DeviceScreenShot() {
  const NTempest::CRect &window = DeviceCurWindow();
  m_scrShotWidth = static_cast<UINT>(window.r);
  UINT height = static_cast<UINT>(window.b);
  m_scrShotHeight = height;

  NTempest::CiRect pixRect(0, 0, height, m_scrShotWidth);
  DeviceReadPixels(pixRect, m_scrShotPixels);
}

void CGxDevice::DeviceReadScreenShot(UINT &w, UINT &h, const NTempest::CImVector *&pixels) {
  w = m_scrShotWidth;
  h = m_scrShotHeight;
  if (m_scrShotPixels.Count()) {
    pixels = m_scrShotPixels.Ptr();
  } else {
    pixels = 0;
  }
}

void CGxDevice::DeviceClearScreenShot() {
  m_scrShotHeight = 0;
  m_scrShotWidth = 0;
  m_scrShotPixels.TSFixedArray<NTempest::CImVector>::Clear();
}

int CGxDevice::IDevIsWindowed() {
  return m_format.window;
}

void CGxDevice::DeviceSetRenderTarget(EGxBuffer buffer, CGxTex *texture, UINT plane) {
  m_textureTarget[buffer].m_texture = texture;
  m_textureTarget[buffer].m_plane = plane;

  if (!m_textureTarget[GxBuffers_Color].m_texture && !m_textureTarget[GxBuffers_Depth].m_texture) {
    DeviceSetCurWindow(DeviceDefWindow());
  } else {
    DeviceSetCurWindow(NTempest::CRect(0.0f, 0.0f, static_cast<float>(texture->m_height), static_cast<float>(texture->m_width)));
  }
}

const NTempest::CRect &CGxDevice::DeviceCurWindow() {
  return m_curWindowRect;
}

void CGxDevice::DeviceSetDefWindow(const NTempest::CRect &rect) {
  m_defWindowRect = rect;
  DeviceSetCurWindow(rect);
}

const NTempest::CRect &CGxDevice::DeviceDefWindow() {
  return m_defWindowRect;
}

void CGxDevice::DeviceSetCurWindow(const NTempest::CRect &rect) {
  m_curWindowRect = rect;
}

void CGxDevice::DeviceOverride(EGxOverride override, DWORD value) {
  Log("DeviceOverride(): %d set to %d", override, value);
}

const CGxCaps &CGxDevice::Caps() const {
  return m_caps;
}

void CGxDevice::SceneSetClearColor(NTempest::CImVector clearColor) {
  m_clearColor = clearColor;
}

NTempest::CImVector CGxDevice::SceneClearColor() {
  return m_clearColor;
}

void CGxDevice::ScenePresent(UINT mask) {
  ++m_perfCountersAcc[GxPerf_FrameNum];
  m_perfCountersAcc[GxPerf_FrameRate] = UpdateFrameRate();
  m_scrShotClick = 0;
  PerfCountersLatch();
}

void CGxDevice::SceneClear(UINT mask) {
}

void CGxDevice::XformSetViewport(float minX, float maxX, float minY, float maxY, float minZ, float maxZ) {
  m_viewport.x.Set(minX, maxX);
  m_viewport.y.Set(minY, maxY);
  m_viewport.z.Set(minZ, maxZ);
}

void CGxDevice::XformSetProjection(const NTempest::C44Matrix &matrix) {
  m_projection = matrix;
}

void CGxDevice::XformSetView(const NTempest::C44Matrix &matrix) {
  m_xforms[6].Top() = matrix;
}

void CGxDevice::XformSetBones(UINT numBones, const NTempest::C34Matrix *matrices) {
  m_bones = matrices;
  m_boneCount = numBones;
}

void CGxDevice::XformViewport(float &minX, float &maxX, float &minY, float &maxY, float &minZ, float &maxZ) {
  m_viewport.x.Get(minX, maxX);
  m_viewport.y.Get(minY, maxY);
  m_viewport.z.Get(minZ, maxZ);
}

void CGxDevice::XformProjection(NTempest::C44Matrix &matrix) {
  matrix = m_projection;
}

void CGxDevice::XformView(NTempest::C44Matrix &matrix) {
  matrix = m_xforms[6].m_mtx[m_xforms[6].m_level];
}

void CGxDevice::XformBone(UINT ndx, NTempest::C34Matrix &matrix) {
  matrix = m_bones[ndx];
}

void CGxDevice::XformPush(EGxXform xf) {
  m_xforms[xf].Push();
}

void CGxDevice::XformPush(EGxXform xf, const NTempest::C44Matrix &matrix) {
  m_xforms[xf].Push();
  m_xforms[xf].Top() = matrix;
}

void CGxDevice::XformPop(EGxXform xf) {
  m_xforms[xf].Pop();
}

void CGxDevice::XformIdentity(EGxXform xf) {
  m_xforms[xf].Identity();
}

void CGxDevice::XformSet(EGxXform xf, const NTempest::C44Matrix &matrix) {
  ASSERT(m_xforms[xf].m_level > 0);
  ASSERT(m_xforms[xf].m_flags[0] & CGxMatrixStack::F_Identity);

  m_xforms[xf].Top() = matrix;
}

void CGxDevice::XformTranslate(EGxXform xf, const NTempest::C3Vector &t) {
  m_xforms[xf].Top().Translate(t);
}

void CGxDevice::XformScale(EGxXform xf, const NTempest::C3Vector &s) {
  m_xforms[xf].Top().Scale(s);
}

void CGxDevice::XformMult(EGxXform xf, const NTempest::C44Matrix &m) {
  m_xforms[xf].Top() *= m;
}

void CGxDevice::Xform(EGxXform xf, NTempest::C44Matrix &matrix) {
  matrix = m_xforms[xf].m_mtx[m_xforms[xf].m_level];
}

void CGxDevice::VertexShaderSelect(EGxVertexShader shader) {
  m_vertexShader = shader;
}

BOOL CGxDevice::IVbHasColor(EGxVertexBufferFormat format) {
  switch (format) {
    case GxVBF_PNC:
    case GxVBF_PNCT0:
    case GxVBF_PNCT0T1:
    case GxVBF_PCT0:
    case GxVBF_PC:
      return 1;

    case GxVBF_PN:
    case GxVBF_PNT0:
    case GxVBF_PNT0T1:
    case GxVBF_PT0T1:
      return 0;

    default:
      ASSERT(0);
      return 0;
  }
}

EGxVertexBufferFormat CGxDevice::IGiveVbColor(EGxVertexBufferFormat format) {
  switch (format) {
    case GxVBF_PN:
    case GxVBF_PNT0:
    case GxVBF_PNT0T1:
      return static_cast<EGxVertexBufferFormat>(format + 1);

    case GxVBF_PCT0:
    case GxVBF_PC:
      return format;

    default:
      ASSERT(0);
      return format;
  }
}

void CGxDevice::PrimLockAndProcessVertexPtrs(
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
  if (!m_vertexLocked) {
    m_vertexLocked = 1;
  } else {
    ASSERT(!"CGxDevice::PrimLockAndProcessVertexPtrs(): lock mismatch!");
  }

  m_perfCountersAcc[GxPerf_Vertices] += vertexCount;

  if (pos && normal && !color && !tex0 && !tex1) {
    m_vertexBufferFormat = GxVBF_PN;
  } else if (pos && normal && color && !tex0 && !tex1) {
    m_vertexBufferFormat = GxVBF_PNC;
  } else if (pos && normal && !color && tex0 && !tex1) {
    m_vertexBufferFormat = GxVBF_PNT0;
  } else if (pos && normal && color && tex0 && !tex1) {
    m_vertexBufferFormat = GxVBF_PNCT0;
  } else if (pos && normal && !color && tex0 && tex1) {
    m_vertexBufferFormat = GxVBF_PNT0T1;
  } else if (pos && normal && color && tex0 && tex1) {
    m_vertexBufferFormat = GxVBF_PNCT0T1;
  } else if (pos && !normal && color && tex0 && !tex1) {
    m_vertexBufferFormat = GxVBF_PCT0;
  } else if (pos && !normal && color && !tex0 && !tex1) {
    m_vertexBufferFormat = GxVBF_PC;
  } else if (pos && !normal && !color && tex0 && tex1) {
    m_vertexBufferFormat = GxVBF_PT0T1;
  } else {
    ASSERT(!"CGxDevice::PrimLockAndProcessVertexPtrs(): unhandled vertex format");
  }

  m_perfCountersAcc[GxPerf_VertexBytes] += vertexCount * GxVertexSize(m_vertexBufferFormat);
}

void CGxDevice::PrimLockIndexPtr(EGxPrim primType, UINT indexCount, const WORD *indices) {
  ASSERT(!m_indexLocked);
  m_indexLocked = 1;
  m_primType = primType;
  m_primIndexCount = indexCount;
  m_perfCountersAcc[GxPerf_IndexBytes] += 2 * indexCount;
}

void CGxDevice::PrimDrawElements() {
  m_perfCountersAcc[GxPerf_Primitives] += PrimCalcCount(m_primType, m_primIndexCount);
  ++m_perfCountersAcc[GxPerf_Batches];
}

void CGxDevice::PrimUnlockIndexPtr() {
  if (m_indexLocked) {
    m_indexLocked = 0;
  } else {
    DbgPrintf("CGxDevice::PrimUnlockIndexPtr(): lock mismatch!\n");
  }
}

void CGxDevice::PrimUnlockVertexPtrs() {
  if (m_vertexLocked) {
    m_vertexLocked = 0;
  } else {
    DbgPrintf("CGxDevice::PrimUnlockVertexPtrs(): lock mismatch!\n");
  }
}

UINT CGxDevice::PrimCalcCount(EGxPrim primType, UINT indexCount) {
  UINT divisor = s_primVtxDiv[primType];

  if (divisor != 1) {
    indexCount /= divisor;
  }
  return indexCount - s_primVtxAdjust[primType];
}

void CGxDevice::PrimBegin(EGxPrim primType) {
  ASSERT(m_indexLocked == 0 && m_vertexLocked == 0);
  ASSERT(m_inBeginEnd == 0);

  m_primType = primType;
  m_inBeginEnd = 1;
  m_primIndexArray.SetCount(0);
  m_primVertexArray.SetCount(0);
  for (UINT i = 0; i < 4; ++i) {
    m_primTexCoordArray[i].SetCount(0);
  }
  m_primNormalArray.SetCount(0);
  m_primColorArray.SetCount(0);
}

void CGxDevice::PrimVertex(const NTempest::C3Vector &v) {
  m_primVertex = v;
  m_primMask |= 1;

  *m_primIndexArray.New() = static_cast<WORD>(m_primVertexArray.Count());
  *m_primVertexArray.New() = m_primVertex;

  for (UINT i = 0; i < 4; ++i) {
    *m_primTexCoordArray[i].New() = m_primTexCoord[i];
  }

  *m_primNormalArray.New() = m_primNormal;
  *m_primColorArray.New() = m_primColor;

  if (m_primVertexArray.Count() == 0x4000) {
    PrimEnd();
    PrimBegin(m_primType);
  }
}

void CGxDevice::PrimNormal(const NTempest::C3Vector &n) {
  m_primNormal = n;
  m_primMask |= 0x20;
}

void CGxDevice::PrimColor(const NTempest::CImVector &c) {
  m_primColor = c;
  m_primMask |= 0x40;
}

void CGxDevice::PrimTexCoord(UINT tmu, const NTempest::C2Vector &t) {
  m_primTexCoord[tmu] = t;
  m_primMask |= 2 << tmu;
}

void CGxDevice::PrimEnd() {
  ASSERT(m_inBeginEnd == 1);
  m_inBeginEnd = 0;

  if (m_primVertexArray.Count()) {
    const NTempest::C3Vector  *vertexPtr = 0;
    const NTempest::C3Vector  *normalPtr = 0;
    const NTempest::CImVector *color = 0;
    const NTempest::C2Vector  *tex0 = 0;
    const NTempest::C2Vector  *tex1 = 0;

    if (m_primMask & 1) {
      vertexPtr = m_primVertexArray.Ptr();
    }
    if (m_primMask & 0x20) {
      normalPtr = m_primNormalArray.Ptr();
    }
    if (m_primMask & 0x40) {
      color = m_primColorArray.Ptr();
    }
    if (m_primMask & 2) {
      tex0 = m_primTexCoordArray[0].Ptr();
    }
    if (m_primMask & 4) {
      tex1 = m_primTexCoordArray[1].Ptr();
    }

    PrimLockAndProcessVertexPtrs(
        m_primVertexArray.Count(), vertexPtr, sizeof(NTempest::C3Vector), normalPtr, sizeof(NTempest::C3Vector), color, sizeof(NTempest::CImVector),
        0, 0, tex0, sizeof(NTempest::C2Vector), tex1, sizeof(NTempest::C2Vector)
    );
    PrimLockIndexPtr(m_primType, m_primIndexArray.Count(), m_primIndexArray.Ptr());
    PrimDrawElements();
    PrimUnlockIndexPtr();
    PrimUnlockVertexPtrs();
  }
}

void CGxDevice::LightSet(UINT whichLight, const CGxLight &lightInfo, const NTempest::C3Vector &cameraPos) {
  m_appState.m_lights[whichLight] = lightInfo;

  if (m_appState.m_lights[whichLight].m_isOmni) {
    m_appState.m_lights[whichLight].m_dir.x -= cameraPos.x;
    m_appState.m_lights[whichLight].m_dir.y -= cameraPos.y;
    m_appState.m_lights[whichLight].m_dir.z -= cameraPos.z;
  }

  m_hwState.m_lightsDirty[whichLight] = memcmp(&m_hwState.m_lights[whichLight], &lightInfo, sizeof(CGxLight)) != 0;
}

void CGxDevice::Light(UINT whichLight, CGxLight &lightInfo) {
  const CGxLight &source = m_appState.m_lights[whichLight];
  lightInfo.m_enabled = source.m_enabled;
  lightInfo.m_isOmni = source.m_isOmni;
  lightInfo.m_dir = source.m_dir;
  lightInfo.m_ambColor = source.m_ambColor;
  lightInfo.m_dirColor = source.m_dirColor;
  lightInfo.m_specColor = source.m_specColor;
  lightInfo.m_ambIntensity = source.m_ambIntensity;
  lightInfo.m_dirIntensity = source.m_dirIntensity;
  lightInfo.m_specIntensity = source.m_specIntensity;
  lightInfo.m_constantAttenuation = source.m_constantAttenuation;
  lightInfo.m_linearAttenuation = source.m_linearAttenuation;
  lightInfo.m_quadraticAttenuation = source.m_quadraticAttenuation;
  lightInfo.m_attenStart = source.m_attenStart;
  lightInfo.m_attenEnd = source.m_attenEnd;
}

void CGxDevice::LightEnable(UINT whichLight, int enable) {
  m_appState.m_lights[whichLight].m_enabled = enable;

  if (m_hwState.m_lights[whichLight].m_enabled != enable) {
    m_hwState.m_lightsDirty[whichLight] = 1;
  }
}

BOOL CGxDevice::EnableState(DWORD app, DWORD appDisables, UINT flagPos) {
  return (app & ~appDisables & (1UL << flagPos)) != 0;
}

BOOL CGxDevice::NeedsUpdate(DWORD app, DWORD hw, DWORD appDisables, DWORD hwDisables, UINT flagPos, int &enable) {
  enable = EnableState(app, appDisables, flagPos);
  return enable != EnableState(hw, hwDisables, flagPos);
}

void CGxDevice::MasterEnableSet(EGxMasterEnables state, int enable) {
  m_appState.m_masterEnables = ((enable & 1) << state) | (m_appState.m_masterEnables & ~(1U << state));

  switch (state) {
    case GxMasterEnable_Lighting:
      IRsForceUpdate(GxRs_Lighting);
      break;
    case GxMasterEnable_Fog:
      IRsForceUpdate(GxRs_Fog);
      break;
    case GxMasterEnable_DepthTest:
      IRsForceUpdate(GxRs_DepthTest);
      break;
    case GxMasterEnable_DepthWrite:
      IRsForceUpdate(GxRs_DepthWrite);
      break;
    case GxMasterEnable_Culling:
      IRsForceUpdate(GxRs_Culling);
      break;
    default:
      return;
  }
}

BOOL CGxDevice::MasterEnable(EGxMasterEnables state) {
  return ((1U << state) & m_appState.m_masterEnables) != 0;
}

UINT CGxDevice::IMatAlphaRef(EGxBlend op) {
  return s_alphaRef[op];
}

void CGxDevice::RsSet(EGxRenderState which, int value) {
  CGxStateBom tmp_;

  if (value) {
    if (which == GxRs_Texture0) {
      ASSERT(!value || which != GxRs_Texture0);
    } else if (which == GxRs_Texture1) {
      ASSERT(!value || which != GxRs_Texture1);
    } else if (which == GxRs_Texture2) {
      ASSERT(!value || which != GxRs_Texture2);
    } else if (which == GxRs_Texture3) {
      ASSERT(!value || which != GxRs_Texture3);
    }
  }

  if (mAppRenderStates[which].mValue.mData[0] != value) {
    tmp_.mData[0] = value;
    tmp_.mData[1] = value;
    tmp_.mData[2] = value;
    IRsSet(which, tmp_);

    if (which == GxRs_Blend) {
      RsSet(GxRs_AlphaRef, static_cast<int>(IMatAlphaRef(static_cast<EGxBlend>(value))));
    }
  }
}

void CGxDevice::RsSet(EGxRenderState which, float value) {
  CGxStateBom        tmp_;
  CGxAppRenderState &state = mAppRenderStates[which];
  float              current = *reinterpret_cast<float *>(&state.mValue.mData[0]);

  if (current != value) {
    *reinterpret_cast<float *>(&tmp_.mData[0]) = value;
    *reinterpret_cast<float *>(&tmp_.mData[1]) = value;
    *reinterpret_cast<float *>(&tmp_.mData[2]) = value;
    IRsSet(which, tmp_);
  }
}

void CGxDevice::RsSet(EGxRenderState which, NTempest::CImVector value) {
  CGxStateBom tmp_;

  if (mAppRenderStates[which].mValue.mData[0] != *value.IV_()) {
    tmp_.mData[0] = *value.IV_();
    tmp_.mData[1] = *value.IV_();
    tmp_.mData[2] = *value.IV_();
    IRsSet(which, tmp_);
  }
}

void CGxDevice::RsSet(EGxRenderState which, const NTempest::C3Vector &value) {
  CGxStateBom        tmp_;
  CGxAppRenderState &state = mAppRenderStates[which];
  const float       *current = reinterpret_cast<const float *>(&state.mValue.mData[0]);

  if (current[0] != value.x || current[1] != value.y || current[2] != value.z) {
    tmp_.mData[0] = *reinterpret_cast<const int *>(&value.x);
    tmp_.mData[1] = *reinterpret_cast<const int *>(&value.y);
    tmp_.mData[2] = *reinterpret_cast<const int *>(&value.z);
    IRsSet(which, tmp_);
  }
}

void CGxDevice::RsSet(EGxRenderState which, LPVOID value) {
  CGxStateBom tmp_;

  if (mAppRenderStates[which].mValue.mData[0] != reinterpret_cast<int>(value)) {
    tmp_.mData[0] = reinterpret_cast<int>(value);
    tmp_.mData[1] = 0;
    tmp_.mData[2] = 0;
    IRsSet(which, tmp_);
  }

  if (value && which >= GxRs_Texture0 && which <= GxRs_Texture3 && static_cast<CGxTex *>(value)->m_flags.m_renderTarget &&
      static_cast<CGxTex *>(value)->m_needsUpdate)
  {
    ITexMarkAsUpdated(static_cast<CGxTex *>(value));
  }
}

void CGxDevice::RsGet(EGxRenderState which, int &value) {
  value = mAppRenderStates[which].mValue.mData[0];
}

void CGxDevice::RsGet(EGxRenderState which, float &value) {
  value = *reinterpret_cast<float *>(&mAppRenderStates[which].mValue.mData[0]);
}

void CGxDevice::RsGet(EGxRenderState which, NTempest::CImVector &value) {
  *value.IV_() = mAppRenderStates[which].mValue.mData[0];
}

void CGxDevice::RsGet(EGxRenderState which, NTempest::C3Vector &value) {
  CGxAppRenderState &state = mAppRenderStates[which];
  const float       *data = reinterpret_cast<const float *>(&state.mValue.mData[0]);
  value.x = data[0];
  value.y = data[1];
  value.z = data[2];
}

void CGxDevice::RsGet(EGxRenderState which, LPVOID &value) {
  value = reinterpret_cast<LPVOID>(mAppRenderStates[which].mValue.mData[0]);
}

void CGxDevice::RsPush() {
  ASSERT(mStackOffsets.Count() < 32);

  *mStackOffsets.New() = mPushedStates.Count();
}

void CGxDevice::RsPop() {
  UINT topOfStk_;
  UINT ndx_;

  ASSERT(mStackOffsets.Count() > 0);

  topOfStk_ = mStackOffsets[mStackOffsets.Count() - 1];
  ndx_ = mPushedStates.Count() - 1;

  if (topOfStk_ < mPushedStates.Count()) {
    do {
      CGxPushedRenderState &pushedState = mPushedStates[ndx_];
      CGxAppRenderState    &rs_ = mAppRenderStates[pushedState.mWhich];

      if (!rs_.mDirty) {
        *mDirtyStates.New() = pushedState.mWhich;
        rs_.mDirty = 1;
      }

      rs_.mValue = pushedState.mValue;
      rs_.mStackDepth = pushedState.mStackDepth;
    } while (ndx_-- > topOfStk_);
  }

  ASSERT(topOfStk_ == ndx_ + 1);

  mPushedStates.SetCount(topOfStk_);
  mStackOffsets.SetCount(mStackOffsets.Count() - 1);
}

UINT CGxDevice::RsStackOffset() {
  return mStackOffsets.Count();
}

void CGxDevice::IRsInit() {
  mAppRenderStates.SetCount(GxRenderStates_Last);
  mHwRenderStates.SetCount(GxRenderStates_Last);

  mAppRenderStates[GxRs_PolygonOffset].mValue = 0.0f;
  mAppRenderStates[GxRs_MatDiffuse].mValue = -1;
  mAppRenderStates[GxRs_MatEmissive].mValue = 0;
  mAppRenderStates[GxRs_MatSpecular].mValue = 0;
  mAppRenderStates[GxRs_MatSpecularExp].mValue = 0.0f;
  mAppRenderStates[GxRs_NormalizeNormals].mValue = 0;
  mAppRenderStates[GxRs_SceneAmbient].mValue = 0;
  mAppRenderStates[GxRs_Blend].mValue = GxBlend_Opaque;

  mAppRenderStates[GxRs_FogStyle].mValue = 0;
  mAppRenderStates[GxRs_FogStart].mValue = 0.0f;
  mAppRenderStates[GxRs_FogEnd].mValue = 1.0f;
  mAppRenderStates[GxRs_FogDensity].mValue = 0.0f;
  mAppRenderStates[GxRs_FogColor].mValue = -65536;

  mAppRenderStates[GxRs_Lighting].mValue = 1;
  mAppRenderStates[GxRs_Fog].mValue = 1;
  mAppRenderStates[GxRs_DepthTest].mValue = 1;
  mAppRenderStates[GxRs_DepthFunc].mValue = 0;
  mAppRenderStates[GxRs_DepthWrite].mValue = 1;
  mAppRenderStates[GxRs_Culling].mValue = 1;

  mAppRenderStates[GxRs_Texture0].mValue = 0;
  mAppRenderStates[GxRs_TexBlend0].mValue = GxTexBlend_Mod;
  mAppRenderStates[GxRs_TexLodBias0].mValue = 0.0f;
  mAppRenderStates[GxRs_TexGen0].mValue = 0;
  mAppRenderStates[GxRs_TextureShader0].mValue = GxTS_PassThru;

  mAppRenderStates[GxRs_Texture1].mValue = 0;
  mAppRenderStates[GxRs_TexBlend1].mValue = GxTexBlend_Mod;
  mAppRenderStates[GxRs_TexLodBias1].mValue = 0.0f;
  mAppRenderStates[GxRs_TexGen1].mValue = 0;
  mAppRenderStates[GxRs_TextureShader1].mValue = GxTS_PassThru;

  mAppRenderStates[GxRs_Texture2].mValue = 0;
  mAppRenderStates[GxRs_TexBlend2].mValue = GxTexBlend_Mod;
  mAppRenderStates[GxRs_TexLodBias2].mValue = 0.0f;
  mAppRenderStates[GxRs_TexGen2].mValue = 0;
  mAppRenderStates[GxRs_TextureShader2].mValue = GxTS_PassThru;

  mAppRenderStates[GxRs_Texture3].mValue = 0;
  mAppRenderStates[GxRs_TexBlend3].mValue = GxTexBlend_Mod;
  mAppRenderStates[GxRs_TexLodBias3].mValue = 0.0f;
  mAppRenderStates[GxRs_TexGen3].mValue = 0;
  mAppRenderStates[GxRs_TextureShader3].mValue = GxTS_PassThru;

  mAppRenderStates[GxRs_PixelShader].mValue = 0;
  mAppRenderStates[GxRs_VertexShader].mValue = GxVS_PassThru;
}

void CGxDevice::RsInit() {
  RsSet(GxRs_PolygonOffset, 0.0f);
  RsSet(GxRs_MatDiffuse, NTempest::CImVector(0xFFFFFFFF));
  RsSet(GxRs_MatEmissive, 0.0f);
  RsSet(GxRs_MatSpecular, 0.0f);
  RsSet(GxRs_NormalizeNormals, 0);
  RsSet(GxRs_SceneAmbient, NTempest::CImVector(0ul));
  RsSet(GxRs_Blend, 0);
  RsSet(GxRs_FogStyle, 0);
  RsSet(GxRs_FogStart, 0.0f);
  RsSet(GxRs_FogEnd, 1.0f);
  RsSet(GxRs_FogDensity, 0.0f);
  RsSet(GxRs_FogColor, NTempest::CImVector(0xFFFF0000));
  RsSet(GxRs_Lighting, 1);
  RsSet(GxRs_Fog, 1);
  RsSet(GxRs_DepthTest, 1);
  RsSet(GxRs_DepthFunc, 0);
  RsSet(GxRs_Culling, 1);

  for (UINT tmu = 0; tmu < 4; ++tmu) {
    RsSet(static_cast<EGxRenderState>(GxRs_Texture0 + tmu), static_cast<LPVOID>(0));
    RsSet(static_cast<EGxRenderState>(GxRs_TexBlend0 + tmu), 1);
    RsSet(static_cast<EGxRenderState>(GxRs_TexLodBias0 + tmu), 0.0f);
    RsSet(static_cast<EGxRenderState>(GxRs_TexGen0 + tmu), 0);
    RsSet(static_cast<EGxRenderState>(GxRs_TextureShader0 + tmu), 0);
  }

  RsSet(GxRs_PixelShader, static_cast<LPVOID>(0));
  RsSet(GxRs_VertexShader, static_cast<LPVOID>(0));
}

void CGxDevice::IRsSet(EGxRenderState which, const CGxStateBom &value) {
  CGxAppRenderState &rs_ = mAppRenderStates[which];

  if (!rs_.mDirty) {
    *mDirtyStates.New() = which;
    rs_.mDirty = 1;
  }

  if (rs_.mStackDepth != mStackOffsets.Count()) {
    CGxPushedRenderState tmp_;

    tmp_.mWhich = which;
    tmp_.mValue = rs_.mValue;
    tmp_.mStackDepth = rs_.mStackDepth;
    *mPushedStates.New() = tmp_;
    rs_.mStackDepth = mStackOffsets.Count();
  }

  rs_.mValue = value;
}

void CGxDevice::IRsForceUpdate(EGxRenderState ndx_) {
  *mDirtyStates.New() = ndx_;
  mAppRenderStates[ndx_].mDirty = 1;
  CGxAppRenderState &app = mAppRenderStates[ndx_];
  CGxStateBom       &hw = mHwRenderStates[ndx_];

  hw.mData[0] = ~app.mValue.mData[0];
  hw.mData[1] = ~app.mValue.mData[1];
  hw.mData[2] = ~app.mValue.mData[2];
  hw.filler = ~app.mValue.filler;
}

void CGxDevice::IRsForceUpdate() {
  for (UINT which = 0; which < GxRenderStates_Last; ++which) {
    IRsForceUpdate(static_cast<EGxRenderState>(which));
  }
}

void CGxDevice::IRsSync(int force) {
  if (force) {
    IRsForceUpdate();
  }

  UINT ndx = mDirtyStates.Count();
  while (ndx) {
    EGxRenderState     which = mDirtyStates[--ndx];
    CGxAppRenderState &app = mAppRenderStates[which];
    CGxStateBom       &hw = mHwRenderStates[which];

    if (app.mDirty && (app.mValue.mData[0] != hw.mData[0] || app.mValue.mData[1] != hw.mData[1] || app.mValue.mData[2] != hw.mData[2])) {
      IRsSendToHw(which);
    }

    hw = app.mValue;
    app.mDirty = 0;
  }

  mDirtyStates.SetCount(0);

  CGxShader *sh = reinterpret_cast<CGxShader *>(mAppRenderStates[GxRs_PixelShader].mValue.mData[0]);
  if (sh && (force || sh->paramsDirty)) {
    ISetShaderParameters(sh, 0);
  }
}

CGxBuf *CGxDevice::BufCreate(
    EGxBufWriteFreq       writeFreq,
    EGxVertexBufferFormat format,
    UINT                  numVertices,
    UINT                  numIndices,
    void (*userCallback)(CGxBufCommand &, CGxBuf *),
    LPVOID userArg
) {
  ASSERT(numVertices <= 0xFFFF);
  return 0;
}

void CGxDevice::BufDestroy(CGxBuf *&buf) {
}

void CGxDevice::BufLock(CGxBuf *buf) {
  ASSERT(m_bufLocked == 0);
  m_bufLocked = buf;
  m_perfCountersAcc[GxPerf_Vertices] += buf->VertexCount();
  m_vertexBufferFormat = buf->m_vbFormat;
}

void CGxDevice::BufUnlock() {
  ASSERT(m_bufLocked);
  m_bufLocked = 0;
}

void CGxDevice::BufRender(const CGxBatch *batches, UINT count) {
  ASSERT(m_bufLocked);
  ASSERT(m_bufLocked->m_vbFormat == m_vertexBufferFormat);

  for (UINT i = 0; i < count; ++i) {
    m_perfCountersAcc[GxPerf_Primitives] += PrimCalcCount(batches[i].m_primType, batches[i].m_count);
  }
  m_perfCountersAcc[GxPerf_Batches] += count;
}

void CGxDevice::BufReserve(EGxBufWriteFreq freq, EGxVertexBufferFormat format, UINT numVertices, UINT numIndices) {
  if (freq > GxBWF_Static && freq <= GxBWF_Medium) {
    m_VBReserve[freq][format] = numVertices;
    m_IBReserve[freq][format] = numIndices;
  }
}

CGxBuf *CGxDevice::BufGetDynamic(EGxVertexBufferFormat format) {
  ASSERT(format <= GxVertexBufferFormats_Last);

  return m_dynBuf[format];
}

BOOL CGxDevice::TexCreate(
    UINT         width,
    UINT         height,
    EGxTexFormat format,
    CGxTexFlags  flags,
    LPVOID       userArg,
    void (*userFunc)(EGxTexCommand, UINT, UINT, UINT, UINT, LPVOID, UINT &, LPCVOID &),
    CGxTex *&texId
) {
  CGxTex *tex = NEW(CGxTex)(width, height, format, flags, userArg, userFunc);
  *m_textures.New() = tex;
  texId = tex;
  ++m_perfCountersAcc[GxPerf_Textures];
  m_perfCountersAcc[GxPerf_TextureBytes] += ITexComputeByteSize(texId, UINT_MAX, UINT_MAX);
  return 1;
}

BOOL CGxDevice::TexCreate(
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
  CGxTex *tex = NEW(CGxTex)(target, width, height, depth, format, dataFormat, flags, userArg, userFunc);
  *m_textures.New() = tex;
  texId = tex;
  ++m_perfCountersAcc[GxPerf_Textures];
  m_perfCountersAcc[GxPerf_TextureBytes] += ITexComputeByteSize(texId, UINT_MAX, UINT_MAX);
  return 1;
}

void CGxDevice::TexMarkForUpdate(CGxTex *texId, const NTempest::CiRect &updateRect, int immediate) {
  texId->m_needsUpdate = 1;

  if (updateRect.t < updateRect.b && updateRect.l < updateRect.r) {
    NTempest::CiRect merged(
        texId->m_updateRect.t < updateRect.t ? texId->m_updateRect.t : updateRect.t,
        texId->m_updateRect.l < updateRect.l ? texId->m_updateRect.l : updateRect.l,
        texId->m_updateRect.b > updateRect.b ? texId->m_updateRect.b : updateRect.b,
        texId->m_updateRect.r > updateRect.r ? texId->m_updateRect.r : updateRect.r
    );

    texId->m_updateRect = merged;
  } else {
    texId->m_updateRect.t = 0;
    texId->m_updateRect.l = 0;
    texId->m_updateRect.b = texId->m_height;
    texId->m_updateRect.r = texId->m_width;
  }

  if (immediate) {
    ITexMarkAsUpdated(texId);
  }
}

int CGxDevice::TexNeedsUpdate(CGxTex *texId) {
  return texId->m_needsUpdate;
}

void CGxDevice::ITexMarkAsUpdated(CGxTex *texId) {
  if (texId->m_needsUpdate) {
    ++m_perfCountersAcc[GxPerf_TexUploads];
    m_perfCountersAcc[GxPerf_TexUploadBytes] +=
        ITexComputeByteSize(texId, texId->m_updateRect.r - texId->m_updateRect.l, texId->m_updateRect.b - texId->m_updateRect.t);

    texId->m_updateRect.t = texId->m_height;
    texId->m_needsUpdate = 0;
    texId->m_updateRect.l = texId->m_width;
    texId->m_updateRect.b = 0;
    texId->m_updateRect.r = 0;
  }
}

UINT CGxDevice::ITexComputeByteSize(const CGxTex *texId, const UINT width, const UINT height) {
  UINT texWidth = width;
  UINT texHeight = height;

  if (texWidth == UINT_MAX) {
    texWidth = texId->m_width;
  }
  if (texHeight == UINT_MAX) {
    texHeight = texId->m_height;
  }

  texWidth >>= m_baseMipLevel;
  texHeight >>= m_baseMipLevel;
  UINT bytes = (texWidth * texHeight * s_texFormatBitDepth[texId->m_format]) >> 3;

  if (texId->m_flags.m_filter == GxTex_LinearMipNearest || texId->m_flags.m_filter == GxTex_LinearMipLinear) {
    return bytes * 1.3f;
  }
  return bytes;
}

void CGxDevice::ITexBind(CGxTex *texId) {
  if (texId->m_frameTag != m_perfCountersAcc[GxPerf_FrameNum]) {
    texId->m_frameTag = m_perfCountersAcc[GxPerf_FrameNum];
    ++m_perfCountersAcc[GxPerf_TexBinds];
    m_perfCountersAcc[GxPerf_TexBindBytes] += ITexComputeByteSize(texId, UINT_MAX, UINT_MAX);
  }
}

void CGxDevice::TexSetUserData(CGxTex *texId, void (*userFunc)(EGxTexCommand, UINT, UINT, UINT, UINT, LPVOID, UINT &, LPCVOID &), LPVOID userArg) {
  texId->m_userFunc = userFunc;
  texId->m_userArg = userArg;

  NTempest::CiRect updateRect;
  TexMarkForUpdate(texId, updateRect, 0);
}

void CGxDevice::TexSetFlags(CGxTex *texId, CGxTexFlags flags) {
  if (*reinterpret_cast<UINT *>(&texId->m_flags) != *reinterpret_cast<UINT *>(&flags)) {
    texId->m_flags = flags;
    texId->m_needsFlagUpdate = 1;
  }
}

LPVOID CGxDevice::TexUserArg(CGxTex *texId) {
  return texId->m_userArg;
}

void CGxDevice::TexGetDimensions(const CGxTex *texId, UINT *width, UINT *height) {
  *width = texId->m_width;
  *height = texId->m_height;
}

void CGxDevice::TexDestroy(CGxTex *texId) {
  UINT count = m_textures.Count();
  UINT i = count;

  while (i) {
    --i;
    if (m_textures[i] == texId) {
      --m_perfCountersAcc[GxPerf_Textures];
      m_perfCountersAcc[GxPerf_TextureBytes] -= ITexComputeByteSize(texId, UINT_MAX, UINT_MAX);
      m_textures[i] = m_textures[count - 1];
      m_textures.SetCount(count - 1);
      DEL(texId);
      return;
    }
  }
}

void CGxDevice::TexSetDataFormat(CGxTex *texId, EGxTexFormat dataFormat) {
  texId->m_dataFormat = dataFormat;
}

void CGxDevice::TexParameters(const CGxTex *texId, CGxTexParms &parms) {
  parms.width = texId->m_width;
  parms.height = texId->m_height;
  parms.format = texId->m_format;
  parms.flags = texId->m_flags;
  parms.userArg = texId->m_userArg;
  parms.userFunc = texId->m_userFunc;
}

void CGxDevice::TexParameters(const CGxTex *texId, CGxTexParmsEx &parms) {
  parms.target = texId->m_target;
  parms.width = texId->m_width;
  parms.height = texId->m_height;
  parms.depth = texId->m_depth;
  parms.format = texId->m_format;
  parms.dataFormat = texId->m_dataFormat;
  parms.flags = texId->m_flags;
  parms.userArg = texId->m_userArg;
  parms.userFunc = texId->m_userFunc;
}

void CGxDevice::TexFlags(const CGxTex *texId, CGxTexFlags &flags) {
  flags = texId->m_flags;
}

void CGxShaderParam::Read(SFile *file) {
  SFile::Read(file, name, sizeof(name), 0, 0, 0);
  SFile::Read(file, &type, sizeof(type), 0, 0, 0);
  SFile::Read(file, &index, sizeof(index), 0, 0, 0);
  SFile::Read(file, f, sizeof(f), 0, 0, 0);
}

void CGxShader::Read(SFile *file) {
  UINT bytes;
  UINT pcount;
  UINT ccount;

  SFile::Read(file, &ccount, sizeof(ccount), 0, 0, 0);
  while (ccount--) {
    consts.NewNode(LIST_TAIL, 0, 0)->Read(file);
  }

  SFile::Read(file, &pcount, sizeof(pcount), 0, 0, 0);
  while (pcount--) {
    params.NewNode(LIST_TAIL, 0, 0)->Read(file);
  }

  SFile::Read(file, &bytes, sizeof(bytes), 0, 0, 0);
  code.SetCount(bytes);
  SFile::Read(file, code.Ptr(), bytes, 0, 0, 0);
}

CGxShader::~CGxShader() {
}

void CGxDevice::VertexShaderCreate(CGxVertexShader *&vs, LPCSTR filename) {
  vs = m_vertexShaderList.Ptr(filename);
  if (!vs) {
    vs = m_vertexShaderList.New(filename, 0, 0);

    if (m_caps.m_vertexShaderTarget != -1) {
      SFile *file = 0;
      SFile::Open(filename, &file);
      if (file) {
        UINT magic;
        SFile::Read(file, &magic, sizeof(magic), 0, 0, 0);
        if (magic == CGxVertexShader::Magic) {
          UINT version;
          SFile::Read(file, &version, sizeof(version), 0, 0, 0);
          if (version == CGxVertexShader::Version) {
            CGxShader::DirEntry dir[CGxVertexShader::Targets_Last];
            SFile::Read(file, dir, sizeof(dir), 0, 0, 0);
            if (dir[m_caps.m_vertexShaderTarget].count) {
              SFile::SetFilePointer(file, dir[m_caps.m_vertexShaderTarget].start, 0, FILE_BEGIN);
              vs->Read(file);
            }
          }
        }

        SFile::Close(file);
      }
    }
  }

  ++vs->refCount;
}

void CGxDevice::VertexShaderDestroy(CGxVertexShader *&vs) {
  --vs->refCount;
  if (!vs->refCount) {
    m_vertexShaderList.Delete(vs);
  }

  vs = 0;
}

void CGxDevice::PixelShaderCreate(CGxPixelShader *&ps, LPCSTR filename) {
  ps = m_pixelShaderList.Ptr(filename);
  if (!ps) {
    ps = m_pixelShaderList.New(filename, 0, 0);

    if (m_caps.m_pixelShaderTarget != -1) {
      SFile *file = 0;
      SFile::Open(filename, &file);
      if (file) {
        UINT magic;
        SFile::Read(file, &magic, sizeof(magic), 0, 0, 0);
        if (magic == CGxPixelShader::Magic) {
          UINT version;
          SFile::Read(file, &version, sizeof(version), 0, 0, 0);
          if (version == CGxPixelShader::Version) {
            CGxShader::DirEntry dir[CGxPixelShader::Targets_Last];
            SFile::Read(file, dir, sizeof(dir), 0, 0, 0);
            if (dir[m_caps.m_pixelShaderTarget].count) {
              SFile::SetFilePointer(file, dir[m_caps.m_pixelShaderTarget].start, 0, FILE_BEGIN);
              ps->Read(file);
            }
          }
        }

        SFile::Close(file);
      }
    }
  }

  ++ps->refCount;
}

void CGxDevice::PixelShaderDestroy(CGxPixelShader *&ps) {
  --ps->refCount;
  if (!ps->refCount) {
    m_pixelShaderList.Delete(ps);
  }

  ps = 0;
}

void CGxDevice::ISetShaderParameters(CGxShader *sh, int forceForBind) {
  if (sh->paramsDirty || forceForBind) {
    ISetShaderParamList(sh->consts, forceForBind);
    ISetShaderParamList(sh->params, forceForBind);
    sh->paramsDirty = 0;
  }
}

UINT CGxDevice::PerfCounter(EGxPerfCounter counter) {
  return m_perfCountersLatched[counter];
}

void CGxDevice::PerfCountersLatch() {
  for (UINT i = 0; i < GxPerfCounters_Last; ++i) {
    m_perfCountersLatched[i] = m_perfCountersAcc[i];

    if (i != GxPerf_Textures && i != GxPerf_TextureBytes) {
      m_perfCountersAcc[i] = 0;
    }
  }

  m_perfCountersAcc[GxPerf_FrameNum] = m_perfCountersLatched[GxPerf_FrameNum];
}

float CGxDevice::CpuFrequency() {
  LONGLONG start;
  UINT     millisecond;

  if (frequency != 0.0f) {
    return frequency;
  }

  millisecond = GetTickCount();
  for (start = CpuTicks(); GetTickCount() == millisecond; start = CpuTicks()) {
  }

  Sleep(250);
  frequency = static_cast<float>(4 * (CpuTicks() - start));
  return frequency;
}

LONGLONG CGxDevice::CpuTicks() {
  return OsGetAsyncTimeClocks();
}

void __cdecl CGxDevice::DbgPrintf(LPCSTR format, ...) {
  char    buffer[256];
  va_list arguments;

  va_start(arguments, format);
  _vsnprintf(buffer, sizeof(buffer), format, arguments);
  OsOutputDebugString(buffer);
  Log(buffer);
  va_end(arguments);
}

void CGxDevice::LogOpen() {
  if (!m_log) {
    SLogCreate("gx.log", 0, &m_log);
  }
}

void CGxDevice::LogClose() {
  if (m_log) {
    SLogClose(m_log);
    m_log = 0;
  }
}

void __cdecl CGxDevice::Log(LPCSTR format, ...) {
  char    buffer[0x800];
  va_list arguments;

  va_start(arguments, format);
  LogOpen();
  _vsnprintf(buffer, sizeof(buffer), format, arguments);
  SLogWrite(m_log, buffer);
  va_end(arguments);
}

void CGxDevice::Log(const CGxCaps &caps) const {
  Log("Caps:");
  Log("\tnumTmus: %d", caps.m_numTmus);
  Log("\ttexFmtDxt: %d", caps.m_texFmtDxt);
  Log("\ttexFilterAnisotropic: %d, %d", caps.m_texFilterTrilinear, caps.m_maxTexAnisotropy);
  Log("\trttFormat: %d, %d", caps.m_rttFormat[GxTex_Argb8888], caps.m_rttFormat[GxTex_Rgb565]);
  Log("\tpixelShaderTarget: %d", caps.m_pixelShaderTarget);
  Log("\tvertexShaderTarget: %d", caps.m_vertexShaderTarget);
}

void CGxDevice::Log(const CGxFormat &format) const {
  LPCSTR depthFormat = FmtNames[format.depthFormat];

  if (format.window) {
    Log("\tFormat: %d x %d Window, %s", format.size.x, format.size.y, depthFormat);
  } else {
    Log("\tFormat %d x %d @ %d Fullscreen, %s, %s", format.size.x, format.size.y, format.refreshRate, FmtNames[format.colorFormat], depthFormat);
  }
}
