#include "../CGxDevice.h"

#include <Base/Activity.h>
#include <Base/Base.h>
#include <Tempest/c34matrix.h>
#include <Tempest/c4vector.h>
#include <storm.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

CGxDevice *g_theGxDevicePtr;

static unsigned long      vtxBufSize[GxVertexBufferFormats_Last] = {0x18, 0x1C, 0x20, 0x24, 0x28, 0x2C, 0x18, 0x10, 0x1C};
static const unsigned int s_vtxBufOffset[GxVertexBufferFormats_Last][GxVertexMembers_Last] = {
    {0,  12, ~0u, ~0u, ~0u, ~0u, ~0u},
    {0,  12,  24, ~0u, ~0u, ~0u, ~0u},
    {0,  12, ~0u,  24, ~0u, ~0u, ~0u},
    {0,  12,  24,  28, ~0u, ~0u, ~0u},
    {0,  12, ~0u,  24,  32, ~0u, ~0u},
    {0,  12,  24,  28,  36, ~0u, ~0u},
    {0, ~0u,  12,  16, ~0u, ~0u, ~0u},
    {0, ~0u,  12, ~0u, ~0u, ~0u, ~0u},
    {0, ~0u, ~0u,  12,  20, ~0u, ~0u}
};
static EGxTexFormat gxTexTable[BlitFormats_Last] = {GxTex_Unknown, GxTex_Argb8888, GxTex_Argb4444, GxTex_Argb1555,
                                                    GxTex_Rgb565,  GxTex_Dxt1,     GxTex_Dxt3,     GxTex_Dxt5};
static BlitFormat   blitTable[GxTexFormats_Last] = {BlitFormat_Unknown, BlitFormat_Argb8888, BlitFormat_Argb4444, BlitFormat_Argb1555,
                                                    BlitFormat_Rgb565,  BlitFormat_Dxt1,     BlitFormat_Dxt3,     BlitFormat_Dxt5};

static const float Gx_MinTexAspect = 0.125f;
static const float Gx_MaxTexAspect = 8.0f;

static TSGrowableArray<unsigned char> s_vertexMem;
static TSGrowableArray<unsigned char> s_indexMem;
static TSGrowableArray<unsigned char> s_pixelMem;

const unsigned int CGxShaderParam::TypeCountTable[3] = {1, 3, 4};

CGxFormat::CGxFormat() {
}

CGxFormat::CGxFormat(
    bool                       p_window,
    const NTempest::C2iVector &p_size,
    Format                     p_colorFormat,
    Format                     p_depthFormat,
    unsigned int               p_refreshRate,
    bool                       p_vsync,
    bool                       p_hwTnl,
    bool                       p_fixLag
) {
  hwTnL = p_hwTnl;
  fixLag = p_fixLag;
  window = p_window;
  depthFormat = p_depthFormat;
  size = p_size;
  colorFormat = p_colorFormat;
  refreshRate = p_refreshRate;
  vsync = p_vsync;
}

int __fastcall GxAdapterID(unsigned short &vendorID, unsigned short &deviceID, unsigned long &driverVersionHi, unsigned long &driverVersionLow) {
  return CGxDevice::AdapterID(vendorID, deviceID, driverVersionHi, driverVersionLow);
}

int __fastcall GxAdapterInfer(unsigned short &deviceID) {
  return CGxDevice::AdapterInfer(deviceID);
}

int __fastcall GxAdapterMonitorModes(TSGrowableArray<CGxMonitorMode> &modes) {
  return CGxDevice::AdapterMonitorModes(modes);
}

int __fastcall GxAdapterDesktopMode(CGxMonitorMode &mode) {
  return CGxDevice::AdapterDesktopMode(mode);
}

const TSGrowableArray<CGxFormat> *__fastcall GxEnumFormats(EGxApi api) {
  static TSGrowableArray<CGxFormat> s_formats;

  ASSERT(api < GxApis_Last);

  s_formats.SetCount(0);
  s_formats.Reserve(0x100);

  switch (api) {
    case GxApi_OpenGl:
      CGxDevice::OpenGlEnumFormats(s_formats);
      break;

    case GxApi_Direct3d:
      CGxDevice::D3dEnumFormats(s_formats);
      break;

    default:
      ASSERT(0);
      break;
  }

  return s_formats.Count() ? &s_formats : 0;
}

CGxDevice *__fastcall GxDevCreate(EGxApi api, GXWINDOWPROC windowProc, const CGxFormat &format) {
  CGxDevice *device;

  ASSERT(api < GxApis_Last);

  switch (api) {
    case GxApi_OpenGl:
      device = CGxDevice::NewOpenGl();
      g_theGxDevicePtr = device;
      break;

    case GxApi_Direct3d:
      device = CGxDevice::NewD3d();
      g_theGxDevicePtr = device;
      break;

    default:
      ASSERT(0);
      device = g_theGxDevicePtr;
      break;
  }

  if (!device) {
    ASSERT(g_theGxDevicePtr);
    device = g_theGxDevicePtr;
  }

  if (device->DeviceCreate(windowProc, format)) {
    return g_theGxDevicePtr;
  }

  DELIFUSED(g_theGxDevicePtr);

  return 0;
}

CGxDevice *__fastcall GxDevCreate(EGxApi api, unsigned int hwnd, const CGxFormat &format) {
  CGxDevice *device;

  ASSERT(api < GxApis_Last);

  switch (api) {
    case GxApi_OpenGl:
      device = CGxDevice::NewOpenGl();
      g_theGxDevicePtr = device;
      break;

    case GxApi_Direct3d:
      device = CGxDevice::NewD3d();
      g_theGxDevicePtr = device;
      break;

    default:
      ASSERT(0);
      device = g_theGxDevicePtr;
      break;
  }

  if (!device) {
    ASSERT(g_theGxDevicePtr);
    device = g_theGxDevicePtr;
  }

  if (device->DeviceCreate(hwnd, format)) {
    return g_theGxDevicePtr;
  }

  DELIFUSED(g_theGxDevicePtr);

  return 0;
}

void __fastcall GxDevWM(EGxWM wm, long param1, long param2) {
  g_theGxDevicePtr->DeviceWM(wm, param1, param2);
}

void __fastcall GxDevDestroy(CGxDevice *devicePtr) {
  if (!devicePtr) {
    return;
  }

  FATALASSERT(devicePtr == g_theGxDevicePtr);

  g_theGxDevicePtr->DeviceDestroy();
  DELIFUSED(g_theGxDevicePtr);
  g_theGxDevicePtr = 0;
}

int __fastcall GxDevSetFormat(const CGxFormat &format) {
  return g_theGxDevicePtr->DeviceSetFormat(format);
}

void __fastcall GxDevSetBaseMipLevel(unsigned int baseMipLevel) {
  g_theGxDevicePtr->DeviceSetBaseMipLevel(baseMipLevel);
}

void __fastcall GxDevSetGamma(float gamma) {
  g_theGxDevicePtr->DeviceSetGamma(gamma);
}

void __fastcall GxDevSetGamma(const CGxGammaRamp &ramp) {
  g_theGxDevicePtr->DeviceSetGamma(ramp);
}

void __fastcall GxDevSetTextureQuality(int force32Bit) {
  g_theGxDevicePtr->DeviceSetTextureQuality(force32Bit);
}

const CGxFormat &__fastcall GxDevFormat() {
  return g_theGxDevicePtr->DeviceFormat();
}

unsigned int __fastcall GxDevBaseMipLevel() {
  return g_theGxDevicePtr->DeviceBaseMipLevel();
}

void __fastcall GxDevGammaRamp(CGxGammaRamp &ramp) {
  g_theGxDevicePtr->DeviceGamma(ramp);
}

void __fastcall GxDevSystemGammaRamp(CGxGammaRamp &ramp) {
  g_theGxDevicePtr->DeviceSystemGamma(ramp);
}

int __fastcall GxDevTextureQuality() {
  return g_theGxDevicePtr->DeviceTextureQuality();
}

unsigned long __fastcall GxDevWindow() {
  return g_theGxDevicePtr->DeviceWindow();
}

EGxApi __fastcall GxDevApi() {
  return g_theGxDevicePtr->DeviceApi();
}

void __fastcall GxDevTakeScreenShot() {
  g_theGxDevicePtr->DeviceTakeScreenShot();
}

void __fastcall GxDevReadScreenShot(unsigned int &w, unsigned int &h, const NTempest::CImVector *&pixels) {
  g_theGxDevicePtr->DeviceReadScreenShot(w, h, pixels);
}

void __fastcall GxDevClearScreenShot() {
  g_theGxDevicePtr->DeviceClearScreenShot();
}

void __fastcall GxDevReadPixels(NTempest::CiRect &rect, TSGrowableArray<NTempest::CImVector> &pixels) {
  g_theGxDevicePtr->DeviceReadPixels(rect, pixels);
}

void __fastcall GxDevReadDepth(NTempest::CiRect &rect, TSGrowableArray<float> &depths) {
  g_theGxDevicePtr->DeviceReadDepths(rect, depths);
}

void __fastcall GxDevSetRenderTarget(EGxBuffer buffer, CGxTex *texture, unsigned int plane) {
  ASSERT(texture ? texture->m_flags.m_renderTarget : 1);
  g_theGxDevicePtr->DeviceSetRenderTarget(buffer, texture, plane);
}

void __fastcall GxDevOverride(EGxOverride override, unsigned long value) {
  ASSERT(override <= GxOverrides_Last);

  g_theGxDevicePtr->DeviceOverride(override, value);
}

void __fastcall GxLightSet(unsigned int whichLight, const CGxLight &lightInfo, NTempest::C3Vector cameraPos) {
  FATALASSERT(whichLight < Gx_MaxLights);

  g_theGxDevicePtr->LightSet(whichLight, lightInfo, cameraPos);
}

void __fastcall GxLight(unsigned int whichLight, CGxLight &lightInfo) {
  FATALASSERT(whichLight < Gx_MaxLights);

  g_theGxDevicePtr->Light(whichLight, lightInfo);
}

void __fastcall GxLightEnable(unsigned int whichLight, int enable) {
  FATALASSERT(whichLight < Gx_MaxLights);

  g_theGxDevicePtr->LightEnable(whichLight, enable);
}

void __fastcall GxRsSet(EGxRenderState which, NTempest::CImVector value) {
  FATALASSERT(which < GxRenderStates_Last);

  switch (which) {
    case GxRs_MatDiffuse:
    case GxRs_MatEmissive:
    case GxRs_MatSpecular:
    case GxRs_SceneAmbient:
    case GxRs_FogColor:
      break;

    default:
      ASSERT(!("GxRsSet(): inappropriate render state set"));
      break;
  }

  g_theGxDevicePtr->RsSet(which, value);
}

void __fastcall GxRsSet(EGxRenderState which, float value) {
  FATALASSERT(which < GxRenderStates_Last);

  switch (which) {
    case GxRs_TexLodBias0:
    case GxRs_TexLodBias1:
    case GxRs_TexLodBias2:
    case GxRs_TexLodBias3:
      ASSERT(value >= -2.0f && value <= 2.0f);
      break;

    case GxRs_PolygonOffset:
      ASSERT(value >= 0.0f && value <= 16.0f);
      break;

    case GxRs_MatSpecularExp:
      ASSERT(value >= 0.0f && value <= 128.0f);
      break;

    case GxRs_FogStart:
    case GxRs_FogEnd:
    case GxRs_FogDensity:
      break;

    default:
      ASSERT(!("GxRsSet(): inappropriate render state set"));
      break;
  }

  g_theGxDevicePtr->RsSet(which, value);
}

void __fastcall GxRsSet(EGxRenderState which, int value) {
  FATALASSERT(which < GxRenderStates_Last);

  switch (which) {
    case GxRs_TexBlend0:
    case GxRs_TexBlend1:
    case GxRs_TexBlend2:
    case GxRs_TexBlend3:
      ASSERT(value >= 0 && value < 5);
      break;

    case GxRs_TexGen0:
    case GxRs_TexGen1:
    case GxRs_TexGen2:
    case GxRs_TexGen3:
      ASSERT(value >= 0 && value < 7);
      break;

    case GxRs_FogStyle:
      ASSERT(value >= 0 && value < 3);
      break;

    case GxRs_Blend:
      ASSERT(value >= 0 && value < 8);
      break;

    case GxRs_AlphaRef:
      ASSERT(value >= 0 && value <= 255);
      break;

    case GxRs_DepthFunc:
      ASSERT(value >= 0 && value < 3);
      break;

    case GxRs_TextureShader0:
    case GxRs_TextureShader1:
    case GxRs_TextureShader2:
    case GxRs_TextureShader3:
      ASSERT(value >= 0 && value <= 2);
      break;

    case GxRs_NormalizeNormals:
    case GxRs_Lighting:
    case GxRs_Fog:
    case GxRs_DepthTest:
    case GxRs_DepthWrite:
    case GxRs_Culling:
    case GxRs_Texture0:
    case GxRs_Texture1:
    case GxRs_Texture2:
    case GxRs_Texture3:
      break;

    default:
      ASSERT(!("GxRsSet(): inappropriate render state set"));
      break;
  }

  g_theGxDevicePtr->RsSet(which, value);
}

void __fastcall GxRsSet(EGxRenderState which, void *value) {
  ASSERT(which < GxRenderStates_Last);

  switch (which) {
    case GxRs_Texture0:
    case GxRs_Texture1:
    case GxRs_Texture2:
    case GxRs_Texture3:
    case GxRs_PixelShader:
      break;

    default:
      ASSERT(!("GxRsSet(): inappropriate render state set"));
      break;
  }

  g_theGxDevicePtr->RsSet(which, value);
}

void __fastcall GxRsGet(EGxRenderState which, NTempest::CImVector &value) {
  FATALASSERT(which < GxRenderStates_Last);

  g_theGxDevicePtr->RsGet(which, value);
}

void __fastcall GxRsGet(EGxRenderState which, float &value) {
  FATALASSERT(which < GxRenderStates_Last);

  g_theGxDevicePtr->RsGet(which, value);
}

void __fastcall GxRsGet(EGxRenderState which, int &value) {
  FATALASSERT(which < GxRenderStates_Last);

  g_theGxDevicePtr->RsGet(which, value);
}

void __fastcall GxRsGet(EGxRenderState which, void *&value) {
  FATALASSERT(which < GxRenderStates_Last);

  g_theGxDevicePtr->RsGet(which, value);
}

void __fastcall GxRsPush() {
  g_theGxDevicePtr->RsPush();
}

void __fastcall GxRsPop() {
  g_theGxDevicePtr->RsPop();
}

void __fastcall GxRsInit() {
  g_theGxDevicePtr->RsInit();
}

unsigned int __fastcall GxRsStackOffset() {
  return g_theGxDevicePtr->RsStackOffset();
}

void __fastcall GxVertexShaderSelect(EGxVertexShader shader) {
  FATALASSERT(shader < GxVertexShaders_Last);

  g_theGxDevicePtr->VertexShaderSelect(shader);
}

unsigned int __fastcall GxVertexSize(EGxVertexBufferFormat format) {
  FATALASSERT(format < GxVertexBufferFormats_Last);

  return vtxBufSize[format];
}

unsigned int __fastcall GxVertexMemberOffset(EGxVertexBufferFormat format, EGxVertexMember member) {
  ASSERT(format < GxVertexBufferFormats_Last);
  ASSERT(member < GxVertexMembers_Last);
  return s_vtxBufOffset[format][member];
}

CGxBuf *__fastcall GxBufCreate(
    EGxBufWriteFreq       writeFreq,
    EGxVertexBufferFormat format,
    unsigned int          numVertices,
    unsigned int          numIndices,
    void(__fastcall *userCallback)(CGxBufCommand &, CGxBuf *),
    void *userArg
) {
  CGxBuf *buf;

  ASSERT(writeFreq < GxBufWriteFreqs_Last);
  ASSERT(format < GxVertexBufferFormats_Last);
  ASSERT(numVertices > 0);
  ASSERT(numVertices < Gx_MaxVertices);
  ASSERT(numIndices > 0);
  ASSERT(numIndices < Gx_MaxIndices);

  ActivityBegin(ACTIVITY_RENDER);
  buf = g_theGxDevicePtr->BufCreate(writeFreq, format, numVertices, numIndices, userCallback, userArg);
  ActivityEnd(ACTIVITY_RENDER);

  return buf;
}

void __fastcall GxBufLock(CGxBuf *buf) {
  ActivityBegin(ACTIVITY_RENDER);
  g_theGxDevicePtr->BufLock(buf);
  ActivityEnd(ACTIVITY_RENDER);
}

void __fastcall GxBufUnlock() {
  ActivityBegin(ACTIVITY_RENDER);
  g_theGxDevicePtr->BufUnlock();
  ActivityEnd(ACTIVITY_RENDER);
}

void __fastcall GxBufRender(const CGxBatch *batches, unsigned int count) {
  ActivityBegin(ACTIVITY_RENDER);
  g_theGxDevicePtr->BufRender(batches, count);
  ActivityEnd(ACTIVITY_RENDER);
}

void __fastcall GxBufRender(const CGxBatch &batch) {
  GxBufRender(&batch, 1);
}

void __fastcall GxBufDestroy(CGxBuf *&buf) {
  g_theGxDevicePtr->BufDestroy(buf);
}

void __fastcall GxBufReserve(EGxBufWriteFreq freq, EGxVertexBufferFormat format, unsigned int numVertices, unsigned int numIndices) {
  g_theGxDevicePtr->BufReserve(freq, format, numVertices, numIndices);
}

CGxBuf *__fastcall GxBufGetDynamic(EGxVertexBufferFormat format) {
  return g_theGxDevicePtr->BufGetDynamic(format);
}

unsigned int __fastcall GxPerfCounter(EGxPerfCounter counter) {
  FATALASSERT(counter < GxPerfCounters_Last);

  return g_theGxDevicePtr->PerfCounter(counter);
}

void __fastcall GxMasterEnableSet(EGxMasterEnables state, int enable) {
  FATALASSERT(state < GxMasterEnables_Last);

  ASSERT((enable & ~1) == 0);

  g_theGxDevicePtr->MasterEnableSet(state, enable);
}

int __fastcall GxMasterEnable(EGxMasterEnables state) {
  FATALASSERT(state < GxMasterEnables_Last);

  return g_theGxDevicePtr->MasterEnable(state);
}

const CGxCaps &__fastcall GxCaps() {
  return g_theGxDevicePtr->Caps();
}

void __fastcall GxCapsWindowSize(NTempest::CRect &dst) {
  g_theGxDevicePtr->CapsWindowSize(dst);
}

void __fastcall GxCapsScreenSize(NTempest::CRect &dst) {
  g_theGxDevicePtr->CapsWindowSizeInScreenCoords(dst);
}

int __fastcall GxCapsIsWindowVisible() {
  if (!g_theGxDevicePtr) {
    return 0;
  }

  return g_theGxDevicePtr->CapsIsWindowVisible();
}

void __fastcall GxPrimLockVertexPtrs(
    unsigned int               vertexCount,
    const NTempest::C3Vector  *pos,
    unsigned int               posStride,
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
  ActivityBegin(ACTIVITY_RENDER);

  FATALASSERT(vertexCount > 0);

  FATALASSERT(vertexCount <= Gx_MaxVertices);

  FATALASSERT(pos && posStride);

  if (!normal) {
    normalStride = 0;
  }
  if (!color) {
    colorStride = 0;
  }
  if (!bone) {
    boneStride = 0;
  }
  if (!tex0) {
    tex0Stride = 0;
  }
  if (!tex1) {
    tex1Stride = 0;
  }

  g_theGxDevicePtr->PrimLockAndProcessVertexPtrs(
      vertexCount, pos, posStride, normal, normalStride, color, colorStride, bone, boneStride, tex0, tex0Stride, tex1, tex1Stride
  );
  ActivityEnd(ACTIVITY_RENDER);
}

void __fastcall GxPrimLockIndexPtr(EGxPrim primType, unsigned int indexCount, const unsigned short *indices) {
  ActivityBegin(ACTIVITY_RENDER);

  FATALASSERT(primType < GxPrims_Last);

  FATALASSERT(indexCount > 0);

  FATALASSERT(indexCount <= Gx_MaxIndices);

  FATALASSERT(indices != 0);

  g_theGxDevicePtr->PrimLockIndexPtr(primType, indexCount, indices);
  ActivityEnd(ACTIVITY_RENDER);
}

void __fastcall GxPrimDrawElements() {
  ActivityBegin(ACTIVITY_RENDER);
  g_theGxDevicePtr->PrimDrawElements();
  ActivityEnd(ACTIVITY_RENDER);
}

void __fastcall GxPrimUnlockIndexPtr() {
  ActivityBegin(ACTIVITY_RENDER);
  g_theGxDevicePtr->PrimUnlockIndexPtr();
  ActivityEnd(ACTIVITY_RENDER);
}

void __fastcall GxPrimDrawElements(EGxPrim primType, unsigned int indexCount, const unsigned short *indices) {
  GxPrimLockIndexPtr(primType, indexCount, indices);
  GxPrimDrawElements();
  GxPrimUnlockIndexPtr();
}

void __fastcall GxPrimUnlockVertexPtrs() {
  ActivityBegin(ACTIVITY_RENDER);
  g_theGxDevicePtr->PrimUnlockVertexPtrs();
  ActivityEnd(ACTIVITY_RENDER);
}

void __fastcall GxPrimBegin(EGxPrim primType) {
  g_theGxDevicePtr->PrimBegin(primType);
}

void __fastcall GxPrimEnd() {
  g_theGxDevicePtr->PrimEnd();
}

void __fastcall GxPrimVertex(const NTempest::C3Vector &v) {
  g_theGxDevicePtr->PrimVertex(v);
}

void __fastcall GxPrimTexCoord(unsigned int tmu, const NTempest::C2Vector &t) {
  g_theGxDevicePtr->PrimTexCoord(tmu, t);
}

void __fastcall GxPrimNormal(const NTempest::C3Vector &n) {
  g_theGxDevicePtr->PrimNormal(n);
}

void __fastcall GxPrimColor(const NTempest::CImVector &c) {
  g_theGxDevicePtr->PrimColor(c);
}

void __fastcall GxPrimPointSize(float s) {
  g_theGxDevicePtr->PrimPointSize(s);
}

void __fastcall GxPrimLineWidth(float w) {
  g_theGxDevicePtr->PrimLineWidth(w);
}

void __fastcall GxSceneSetClearColor(NTempest::CImVector clearColor) {
  g_theGxDevicePtr->SceneSetClearColor(clearColor);
}

NTempest::CImVector __fastcall GxSceneClearColor() {
  return g_theGxDevicePtr->SceneClearColor();
}

void __fastcall GxScenePresent(unsigned int mask) {
  ActivityBegin(ACTIVITY_RENDER);
  g_theGxDevicePtr->ScenePresent(mask);
  ActivityEnd(ACTIVITY_RENDER);
}

void __fastcall GxSceneClear(unsigned int mask) {
  ActivityBegin(ACTIVITY_RENDER);
  g_theGxDevicePtr->SceneClear(mask);
  ActivityEnd(ACTIVITY_RENDER);
}

CGxTexFlags::CGxTexFlags(
    EGxTexFilter  filter,
    unsigned long wrapU,
    unsigned long wrapV,
    unsigned long force,
    unsigned long generateMipMaps,
    unsigned long renderTarget,
    unsigned long maxAnisotropy
) {
  m_filter = filter;
  m_wrapU = wrapU;
  m_wrapV = wrapV;
  m_forceMipTracking = force;
  m_generateMipMaps = generateMipMaps;
  m_renderTarget = renderTarget;

  if (maxAnisotropy >= GxCaps().m_maxTexAnisotropy) {
    maxAnisotropy = GxCaps().m_maxTexAnisotropy;
  }

  m_maxAnisotropy = maxAnisotropy;

  ASSERT(filter == GxTex_Anisotropic ? GxCaps().m_texFilterAnisotropic : 1);
}

int __fastcall GxTexCreate(
    unsigned int width,
    unsigned int height,
    EGxTexFormat format,
    CGxTexFlags  flags,
    void        *userArg,
    void(__fastcall *userFunc)(EGxTexCommand, unsigned int, unsigned int, unsigned int, unsigned int, void *, unsigned int &, const void *&),
    CGxTex *&texId
) {
  texId = 0;

  FATALASSERT(width <= Gx_MaxTexWidth);

  FATALASSERT(height <= Gx_MaxTexHeight);

  FATALASSERT((width & (width - 1)) == 0);

  FATALASSERT((height & (height - 1)) == 0);

  FATALASSERT(float(width) >= Gx_MinTexAspect * float(height));

  FATALASSERT(float(width) <= Gx_MaxTexAspect * float(height));

  FATALASSERT(format <= GxTexFormats_Last);

  FATALASSERT((format >= GxTex_Dxt1 && format <= GxTex_Dxt5) ? GxCaps().m_texFmtDxt : 1);

  FATALASSERT(flags.m_generateMipMaps ? (GxCaps().m_generateMipMaps && !(format >= GxTex_Dxt1 && format <= GxTex_Dxt5)) : 1);

  FATALASSERT(flags.m_filter == GxTex_Anisotropic ? GxCaps().m_texFilterAnisotropic : 1);

  FATALASSERT(userFunc != 0);

  FATALASSERT(width >= Gx_MinTexWidth);

  FATALASSERT(height >= Gx_MinTexHeight);

  return g_theGxDevicePtr->TexCreate(width, height, format, flags, userArg, userFunc, texId);
}

int __fastcall GxTexCreate(const CGxTexParms &parms, CGxTex *&texId) {
  return GxTexCreate(parms.width, parms.height, parms.format, parms.flags, parms.userArg, parms.userFunc, texId);
}

int __fastcall GxTexCreate(
    EGxTexTarget target,
    unsigned int width,
    unsigned int height,
    unsigned int depth,
    EGxTexFormat format,
    EGxTexFormat dataFormat,
    CGxTexFlags  flags,
    void        *userArg,
    void(__fastcall *userFunc)(EGxTexCommand, unsigned int, unsigned int, unsigned int, unsigned int, void *, unsigned int &, const void *&),
    CGxTex *&texId
) {
  texId = 0;

  FATALASSERT(target <= GxTexTargets_Last);

  FATALASSERT(width <= Gx_MaxTexWidth);

  FATALASSERT(height <= Gx_MaxTexHeight);

  FATALASSERT((width & (width - 1)) == 0);

  FATALASSERT((height & (height - 1)) == 0);

  FATALASSERT(float(width) >= Gx_MinTexAspect * float(height));

  FATALASSERT(float(width) <= Gx_MaxTexAspect * float(height));

  FATALASSERT(format <= GxTexFormats_Last);

  FATALASSERT((format >= GxTex_Dxt1 && format <= GxTex_Dxt5) ? GxCaps().m_texFmtDxt : 1);

  FATALASSERT(flags.m_generateMipMaps ? (GxCaps().m_generateMipMaps && !(format >= GxTex_Dxt1 && format <= GxTex_Dxt5)) : 1);

  FATALASSERT(flags.m_filter == GxTex_Anisotropic ? GxCaps().m_texFilterAnisotropic : 1);

  FATALASSERT(dataFormat <= GxTexFormats_Last);

  FATALASSERT(userFunc != 0);

  FATALASSERT(width >= Gx_MinTexWidth);

  FATALASSERT(height >= Gx_MinTexHeight);

  return g_theGxDevicePtr->TexCreate(target, width, height, depth, format, dataFormat, flags, userArg, userFunc, texId);
}

int __fastcall GxTexCreate(const CGxTexParmsEx &parms, CGxTex *&texId) {
  return GxTexCreate(
      parms.target, parms.width, parms.height, parms.depth, parms.format, parms.dataFormat, parms.flags, parms.userArg, parms.userFunc, texId
  );
}

void __fastcall GxTexUpdate(CGxTex *texId, int minX, int minY, int maxX, int maxY, int immediate) {
  FATALASSERT(texId != 0);

  NTempest::CiRect rect(minY, minX, maxY, maxX);
  GxTexUpdate(texId, rect, immediate);
}

int __fastcall GxTexNeedsUpdate(CGxTex *texId) {
  FATALASSERT(texId != 0);

  return g_theGxDevicePtr->TexNeedsUpdate(texId);
}

void __fastcall GxTexUpdate(CGxTex *texId, NTempest::CiRect &updateRect, int immediate) {
  FATALASSERT(texId != 0);

  g_theGxDevicePtr->TexMarkForUpdate(texId, updateRect, immediate);
}

void __fastcall GxTexSetUserData(
    CGxTex *texId,
    void(__fastcall *userFunc)(EGxTexCommand, unsigned int, unsigned int, unsigned int, unsigned int, void *, unsigned int &, const void *&),
    void *userArg
) {
  FATALASSERT(texId != 0);

  g_theGxDevicePtr->TexSetUserData(texId, userFunc, userArg);
}

void __fastcall GxTexSetFlags(CGxTex *texId, CGxTexFlags flags) {
  FATALASSERT(texId != 0);

  g_theGxDevicePtr->TexSetFlags(texId, flags);
}

void __fastcall GxTexDestroy(CGxTex *texId) {
  FATALASSERT(texId != 0);

  g_theGxDevicePtr->TexDestroy(texId);
}

void __fastcall GxTexParameters(const CGxTex *texId, CGxTexParms &parms) {
  FATALASSERT(texId != 0);

  g_theGxDevicePtr->TexParameters(texId, parms);
}

void __fastcall GxTexParametersEx(const CGxTex *texId, CGxTexParmsEx &parms) {
  FATALASSERT(texId != 0);

  g_theGxDevicePtr->TexParameters(texId, parms);
}

void __fastcall GxTexSetDataFormat(CGxTex *texId, EGxTexFormat dataFormat) {
  FATALASSERT(texId != 0);

  FATALASSERT(dataFormat <= GxTexFormats_Last);

  g_theGxDevicePtr->TexSetDataFormat(texId, dataFormat);
}

void __fastcall GxTexFlags(const CGxTex *texId, CGxTexFlags &flags) {
  FATALASSERT(texId != 0);

  g_theGxDevicePtr->TexFlags(texId, flags);
}

void __fastcall GxXformSetViewport(float minX, float maxX, float minY, float maxY, float minZ, float maxZ) {
  FATALASSERT(minX < maxX);

  FATALASSERT(minY < maxY);

  FATALASSERT(minZ <= maxZ);

  FATALASSERT(minX >= 0.0f && maxX <= 1.0f);

  FATALASSERT(minY >= 0.0f && maxY <= 1.0f);

  FATALASSERT(minZ >= 0.0f && maxZ <= 1.0f);

  g_theGxDevicePtr->XformSetViewport(minX, maxX, minY, maxY, minZ, maxZ);
}

void __fastcall GxXformSetProjection(const NTempest::C44Matrix &matrix) {
  g_theGxDevicePtr->XformSetProjection(matrix);
}

void __fastcall GxXformSetView(const NTempest::C44Matrix &matrix) {
  g_theGxDevicePtr->XformSetView(matrix);
}

void __fastcall GxXformSetBones(unsigned int numBones, const NTempest::C34Matrix *matrices) {
  FATALASSERT(numBones < Gx_MaxBoneMatrices);

  FATALASSERT(matrices != 0);

  g_theGxDevicePtr->XformSetBones(numBones, matrices);
}

void __fastcall GxXformViewport(float &minX, float &maxX, float &minY, float &maxY, float &minZ, float &maxZ) {
  g_theGxDevicePtr->XformViewport(minX, maxX, minY, maxY, minZ, maxZ);
}

void __fastcall GxXformProjection(NTempest::C44Matrix &matrix) {
  g_theGxDevicePtr->XformProjection(matrix);
}

void __fastcall GxXformView(NTempest::C44Matrix &matrix) {
  g_theGxDevicePtr->XformView(matrix);
}

void __fastcall GxXformBone(unsigned int ndx, NTempest::C34Matrix &matrix) {
  FATALASSERT(ndx < Gx_MaxBoneMatrices);

  g_theGxDevicePtr->XformBone(ndx, matrix);
}

void __fastcall GxXformViewProj(NTempest::C44Matrix &matrix) {
  NTempest::C44Matrix p;
  NTempest::C44Matrix v;

  GxXformView(v);
  GxXformProjection(p);
  matrix = v * p;
}

void __fastcall GxXformPush(EGxXform xf) {
  ASSERT(xf < GxXforms_Last);
  ASSERT(xf <= GxXform_World);

  g_theGxDevicePtr->XformPush(xf);
}

void __fastcall GxXformPush(EGxXform xf, const NTempest::C44Matrix &matrix) {
  ASSERT(xf < GxXforms_Last);
  ASSERT(xf <= GxXform_World);

  g_theGxDevicePtr->XformPush(xf, matrix);
}

void __fastcall GxXformPop(EGxXform xf) {
  ASSERT(xf < GxXforms_Last);
  ASSERT(xf <= GxXform_World);

  g_theGxDevicePtr->XformPop(xf);
}

void __fastcall GxXformIdentity(EGxXform xf) {
  ASSERT(xf < GxXforms_Last);
  ASSERT(xf <= GxXform_World);

  g_theGxDevicePtr->XformIdentity(xf);
}

void __fastcall GxXformSet(EGxXform xf, const NTempest::C44Matrix &matrix) {
  ASSERT(xf < GxXforms_Last);
  ASSERT(xf <= GxXform_World);

  g_theGxDevicePtr->XformSet(xf, matrix);
}

void __fastcall GxXformTranslate(EGxXform xf, const NTempest::C3Vector &t) {
  ASSERT(xf < GxXforms_Last);
  ASSERT(xf <= GxXform_World);

  g_theGxDevicePtr->XformTranslate(xf, t);
}

void __fastcall GxXformScale(EGxXform xf, const NTempest::C3Vector &s) {
  ASSERT(xf < GxXforms_Last);
  ASSERT(xf <= GxXform_World);

  g_theGxDevicePtr->XformScale(xf, s);
}

void __fastcall GxXformMult(EGxXform xf, const NTempest::C44Matrix &m) {
  ASSERT(xf < GxXforms_Last);
  ASSERT(xf <= GxXform_World);

  g_theGxDevicePtr->XformMult(xf, m);
}

void __fastcall GxXform(EGxXform xf, NTempest::C44Matrix &matrix) {
  ASSERT(xf < GxXforms_Last);
  ASSERT(xf <= GxXform_World);

  g_theGxDevicePtr->Xform(xf, matrix);
}

void __fastcall GxPixelShaderCreate(CGxPixelShader *&ps, const char *filename) {
  ASSERT(filename);
  g_theGxDevicePtr->PixelShaderCreate(ps, filename);
}

void __fastcall GxPixelShaderDestroy(CGxPixelShader *&ps) {
  g_theGxDevicePtr->PixelShaderDestroy(ps);
}

void CGxShaderParam::Set(const NTempest::C4Vector &v) {
  ASSERT(type == Type_Vector4);
  dirty = 1;
  memcpy(f, &v, TypeCountTable[type] * sizeof(NTempest::C4Vector));
}

void CGxShaderParam::Set(const NTempest::C34Matrix &m) {
  ASSERT(type == Type_Matrix34);
  dirty = 1;
  memcpy(f, &m, TypeCountTable[type] * sizeof(NTempest::C4Vector));
}

void CGxShaderParam::Set(const NTempest::C44Matrix &m) {
  ASSERT(type == Type_Matrix44);
  dirty = 1;
  memcpy(f, &m, TypeCountTable[type] * sizeof(NTempest::C4Vector));
}

void CGxShader::SetParam(CGxShaderParam *p, const NTempest::C4Vector &v) {
  ASSERT(p);
  paramsDirty = 1;
  p->Set(v);
}

void CGxShader::SetParam(CGxShaderParam *p, const NTempest::C34Matrix &m) {
  ASSERT(p);
  paramsDirty = 1;
  p->Set(m);
}

void CGxShader::SetParam(CGxShaderParam *p, const NTempest::C44Matrix &m) {
  ASSERT(p);
  paramsDirty = 1;
  p->Set(m);
}

CGxShaderParam *CGxShader::GetFirstParam() {
  return params.Head();
}

CGxShaderParam *CGxShader::GetNextParam(CGxShaderParam *p) {
  return p->lameAssLink.Next();
}

CGxShaderParam *CGxShader::GetParam(const char *name) {
  for (CGxShaderParam *param = params.Head(); param; param = params.Next(param)) {
    if (!SStrCmp(param->GetName(), name, CGxShaderParam::NAME_LEN)) {
      return param;
    }
  }

  return 0;
}

void *__fastcall GxAllocVertexMem(unsigned int nBytes) {
  ASSERT(nBytes < sizeof(CGxVertexPNCT0T1) * Gx_MaxVertices);

  s_vertexMem.SetCount(nBytes);
  return s_vertexMem.Ptr();
}

void __fastcall GxFreeVertexMem() {
  s_vertexMem.Clear();
}

void *__fastcall GxAllocIndexMem(unsigned int nBytes) {
  ASSERT(nBytes < sizeof(uint16) * Gx_MaxIndices);

  s_indexMem.SetCount(nBytes);
  return s_indexMem.Ptr();
}

void __fastcall GxFreeIndexMem() {
  s_indexMem.Clear();
}

void *__fastcall GxAllocPixelMem(unsigned int nBytes) {
  ASSERT(nBytes <= sizeof(CArgb) * Gx_MaxTexWidth * Gx_MaxTexHeight);

  s_pixelMem.SetCount(nBytes);
  return s_pixelMem.Ptr();
}

void __fastcall GxFreePixelMem() {
  s_pixelMem.Clear();
}

EGxTexFormat __fastcall GxGetGxTexFormat(BlitFormat blitFormat) {
  ASSERT(blitFormat < BlitFormats_Last);
  return gxTexTable[blitFormat];
}

BlitFormat __fastcall GxGetBlitFormat(EGxTexFormat texFormat) {
  ASSERT(texFormat < GxTexFormats_Last);
  return blitTable[texFormat];
}

void __fastcall GxLogOpen() {
  CGxDevice::LogOpen();
}

void __fastcall GxLogClose() {
  CGxDevice::LogClose();
}

void __cdecl GxLog(const char *format, ...) {
  char    buffer[0x800];
  va_list arguments;

  va_start(arguments, format);
  _vsnprintf(buffer, sizeof(buffer), format, arguments);
  CGxDevice::Log(buffer);
  va_end(arguments);
}

void __fastcall GxTexGetDimensions(const CGxTex *texId, unsigned int *width, unsigned int *height) {
  FATALASSERT(texId != 0);
  g_theGxDevicePtr->TexGetDimensions(texId, width, height);
}

CGxFormat::~CGxFormat() {
}
