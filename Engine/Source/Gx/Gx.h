#pragma once

#include <Images/blit.h>
#include <Tempest/c2ivector.h>
#include <Tempest/c2vector.h>
#include <Tempest/c3vector.h>
#include <Tempest/c44matrix.h>
#include <Tempest/cirect.h>
#include <Tempest/cimvector.h>
#include <Tempest/crect.h>

#include <stddef.h>
#include <stpl.h>
#include <string.h>

namespace NTempest {
  class C34Matrix;
  class C4Vector;
  class CAaBox;
}  // namespace NTempest

typedef DWORD CArgb;

enum {
  Gx_MaxTexWidth = 0x200,
  Gx_MaxTexHeight = 0x200
};

enum EGxApi {
  GxApi_OpenGl = 0,
  GxApi_Direct3d = 1,
  GxApis_Last = 2
};

enum EGxOverride {
  GxOverride_PixelShader = 0,
  GxOverrides_Last = 1
};

enum EGxWM {
  GxWM_Size = 0,
  GxWM_DisplayChange = 1,
  GxWM_Destroy = 2,
  GxWM_SetFocus = 3,
  GxWM_KillFocus = 4
};

enum EGxBuffer {
  GxBuffers_Color = 0,
  GxBuffers_Depth = 1,
  GxBuffers_Last = 2
};

enum EGxBufWriteFreq {
  GxBWF_Static = 0,
  GxBWF_Low = 1,
  GxBWF_Medium = 2,
  GxBWF_Dynamic = 3,
  GxBufWriteFreqs_Last = 4
};

enum EGxVertexBufferFormat {
  GxVBF_PN = 0,
  GxVBF_PNC = 1,
  GxVBF_PNT0 = 2,
  GxVBF_PNCT0 = 3,
  GxVBF_PNT0T1 = 4,
  GxVBF_PNCT0T1 = 5,
  GxVBF_PCT0 = 6,
  GxVBF_PC = 7,
  GxVBF_PT0T1 = 8,
  GxVertexBufferFormats_Last = 9
};

enum EGxBufOp {
  GxBufOp_Nop = 0,
  GxBufOp_Fill = 1,
  GxBufOp_Assign = 2,
  GxBufOps_Last = 3
};

enum EGxVertexMember {
  GxVM_Indices = 0,
  GxVM_Vertex = 0,
  GxVM_Position = 0,
  GxVM_Normal = 1,
  GxVM_Color = 2,
  GxVM_Texture0 = 3,
  GxVM_Texture1 = 4,
  GxVM_Texture2 = 5,
  GxVM_Texture3 = 6,
  GxVertexMembers_Last = 7
};

enum {
  Gx_MaxLights = 8,
  Gx_MinTexWidth = 8,
  Gx_MinTexHeight = 8,
  Gx_MaxVertices = 0x4000,
  Gx_MaxIndices = 0xC000
};

struct CGxBufOp {
  EGxBufOp op;
  LPVOID  *mem[GxVertexMembers_Last];
  UINT     stride[GxVertexMembers_Last];

  CGxBufOp() {
  }
  void Set(EGxVertexMember member, LPVOID memory, UINT memberStride) {
    *mem[member] = memory;
    stride[member] = memberStride;
  }
};

struct CGxBufCommand {
  CGxBufOp vertex;
  CGxBufOp index;
};

struct CGxVertexPN {
  NTempest::C3Vector p;
  NTempest::C3Vector n;
};

struct CGxVertexPC {
  NTempest::C3Vector  p;
  NTempest::CImVector c;
};

struct CGxVertexPCT0 {
  NTempest::C3Vector  p;
  NTempest::CImVector c;
  NTempest::C2Vector  tc[1];
};

struct CGxVertexPNT0 {
  NTempest::C3Vector p;
  NTempest::C3Vector n;
  NTempest::C2Vector tc[1];
};

struct CGxVertexPNT0T1 {
  NTempest::C3Vector p;
  NTempest::C3Vector n;
  NTempest::C2Vector tc[2];
};

struct CGxVertexPNCT0 {
  NTempest::C3Vector  p;
  NTempest::C3Vector  n;
  NTempest::CImVector c;
  NTempest::C2Vector  tc[1];
};

struct CGxVertexPNCT0T1 {
  NTempest::C3Vector  p;
  NTempest::C3Vector  n;
  NTempest::CImVector c;
  NTempest::C2Vector  tc[2];
};

enum EGxTexTarget {
  GxTex_2d = 0,
  GxTex_CubeMap = 1,
  GxTexTargets_Last = 2
};

enum EGxTexFormat {
  GxTex_Unknown = 0,
  GxTex_Argb8888 = 1,
  GxTex_Argb4444 = 2,
  GxTex_Argb1555 = 3,
  GxTex_Rgb565 = 4,
  GxTex_Dxt1 = 5,
  GxTex_Dxt3 = 6,
  GxTex_Dxt5 = 7,
  GxTexFormats_Last = 8
};

enum EGxTexFilter {
  GxTex_Nearest = 0,
  GxTex_Linear = 1,
  GxTex_LinearMipNearest = 2,
  GxTex_LinearMipLinear = 3,
  GxTex_Anisotropic = 4,
  GxTexFilters_Last = 5
};

enum EGxTexCommand {
  GxTex_Lock = 0,
  GxTex_Latch = 1,
  GxTex_Unlock = 2,
  GxTexCommands_Last = 3
};

enum EGxRenderState {
  GxRs_PolygonOffset = 0,
  GxRs_MatDiffuse = 1,
  GxRs_MatEmissive = 2,
  GxRs_MatSpecular = 3,
  GxRs_MatSpecularExp = 4,
  GxRs_NormalizeNormals = 5,
  GxRs_SceneAmbient = 6,
  GxRs_Blend = 7,
  GxRs_AlphaRef = 8,
  GxRs_FogStyle = 9,
  GxRs_FogStart = 10,
  GxRs_FogEnd = 11,
  GxRs_FogDensity = 12,
  GxRs_FogColor = 13,
  GxRs_Lighting = 14,
  GxRs_Fog = 15,
  GxRs_DepthTest = 16,
  GxRs_DepthFunc = 17,
  GxRs_DepthWrite = 18,
  GxRs_Culling = 19,
  GxRs_Texture0 = 20,
  GxRs_Texture1 = 21,
  GxRs_Texture2 = 22,
  GxRs_Texture3 = 23,
  GxRs_TexBlend0 = 24,
  GxRs_TexBlend1 = 25,
  GxRs_TexBlend2 = 26,
  GxRs_TexBlend3 = 27,
  GxRs_TexLodBias0 = 28,
  GxRs_TexLodBias1 = 29,
  GxRs_TexLodBias2 = 30,
  GxRs_TexLodBias3 = 31,
  GxRs_TexGen0 = 32,
  GxRs_TexGen1 = 33,
  GxRs_TexGen2 = 34,
  GxRs_TexGen3 = 35,
  GxRs_TextureShader0 = 36,
  GxRs_TextureShader1 = 37,
  GxRs_TextureShader2 = 38,
  GxRs_TextureShader3 = 39,
  GxRs_PixelShader = 40,
  GxRs_VertexShader = 41,
  GxRenderStates_Last = 42
};

enum EGxBlend {
  GxBlend_Opaque = 0,
  GxBlend_AlphaKey = 1,
  GxBlend_Alpha = 2,
  GxBlend_Add = 3,
  GxBlend_Mod = 4,
  GxBlend_Mod2x = 5,
  GxBlend_ModAdd = 6,
  GxBlend_InvSrcAlphaAdd = 7,
  GxBlends_Last = 8
};

enum EGxTexBlend {
  GxTexBlend_Opaque = 0,
  GxTexBlend_Mod = 1,
  GxTexBlend_Decal = 2,
  GxTexBlend_Add = 3,
  GxTexBlend_Mod2x = 4,
  GxTexBlends_Last = 5
};

enum EGxTexGen {
  GxTexGen_Disable = 0,
  GxTexGen_Object = 1,
  GxTexGen_World = 2,
  GxTexGen_View = 3,
  GxTexGen_ViewReflection = 4,
  GxTexGen_ViewNormal = 5,
  GxTexGen_SphereMap = 6,
  GxTexGens_Last = 7
};

enum EGxTextureShader {
  GxTS_PassThru = 0,
  GxTS_Affine = 1,
  GxTS_Proj = 2,
  GxTextureShaders_Last = 3
};

enum EGxXform {
  GxXform_Tex0 = 0,
  GxXform_Tex1 = 1,
  GxXform_Tex2 = 2,
  GxXform_Tex3 = 3,
  GxXform_World = 4,
  GxXform_Projection = 5,
  GxXform_View = 6,
  GxXforms_Last = 7
};

enum {
  Gx_MaxMatrixStackDepth = 4,
  Gx_MaxBoneMatrices = 0x100
};

enum EGxVertexShader {
  GxVS_PassThru = 0,
  GxVS_Skin = 1,
  GxVertexShaders_Last = 2
};

enum EGxPrim {
  GxPrim_Points = 0,
  GxPrim_Lines = 1,
  GxPrim_LineStrip = 2,
  GxPrim_Triangles = 3,
  GxPrim_TriangleStrip = 4,
  GxPrim_TriangleFan = 5,
  GxPrims_Last = 6
};

struct CGxBatch {
  EGxPrim m_primType;
  UINT    m_count;
  UINT    m_start;
  int     m_minIndex;
  int     m_maxIndex;

  CGxBatch() {
  }

  CGxBatch(EGxPrim prim, UINT count, UINT start, int minIndex, int maxIndex)
      : m_primType(prim), m_count(count), m_start(start), m_minIndex(minIndex), m_maxIndex(maxIndex) {
  }
};

enum EGxMasterEnables {
  GxMasterEnable_Lighting = 0,
  GxMasterEnable_Fog = 1,
  GxMasterEnable_DepthTest = 2,
  GxMasterEnable_DepthWrite = 3,
  GxMasterEnable_Culling = 4,
  GxMasterEnable_ClearOnPresent = 5,
  GxMasterEnable_DoubleBuffering = 6,
  GxMasterEnable_NormalProjection = 7,
  GxMasterEnable_PolygonFill = 8,
  GxMasterEnables_Last = 9
};

enum EGxPerfCounter {
  GxPerf_FrameRate = 0,
  GxPerf_FrameNum = 1,
  GxPerf_Vertices = 2,
  GxPerf_Primitives = 3,
  GxPerf_Batches = 4,
  GxPerf_Textures = 5,
  GxPerf_TextureBytes = 6,
  GxPerf_TexUploads = 7,
  GxPerf_TexUploadBytes = 8,
  GxPerf_TexBinds = 9,
  GxPerf_TexBindBytes = 10,
  GxPerf_VertexBytes = 11,
  GxPerf_IndexBytes = 12,
  GxPerfCounters_Last = 13
};

enum EGxColorFormat {
  GxCF_argb = 0,
  GxCF_rgba = 1,
  GxColorFormats_Last = 2
};

struct CGxFormat {
  friend class CGxDevice;
  friend class CGxDeviceOpenGl;

  enum Format {
    Fmt_Rgb565 = 0,
    Fmt_ArgbX888 = 1,
    Fmt_Argb8888 = 2,
    Fmt_Argb2101010 = 3,
    Fmt_Ds160 = 4,
    Fmt_Ds24X = 5,
    Fmt_Ds248 = 6,
    Fmt_Ds320 = 7,
    Formats_Last = 8
  };

 private:
  mutable DWORD apiSpecificModeID;

 public:
  bool                hwTnL;
  bool                fixLag;
  bool                window;
  Format              depthFormat;
  NTempest::C2iVector size;
  Format              colorFormat;
  UINT                refreshRate;
  bool                vsync;
  NTempest::C2iVector pos;

  CGxFormat();
  CGxFormat(
      bool                       p_window,
      const NTempest::C2iVector &p_size,
      Format                     p_colorFormat,
      Format                     p_depthFormat,
      UINT                       p_refreshRate,
      bool                       p_vsync,
      bool                       p_hwTnl,
      bool                       p_fixLag
  );
};

struct CGxMonitorMode {
  NTempest::C2iVector size;
  UINT                bpp;
  UINT                refreshRate;
};

class CGxCaps;

struct CGxGammaRamp {
  enum {
    ENTRIES = 256
  };

  CGxGammaRamp() {
  }
  CGxGammaRamp(float gamma) {
    Set(gamma);
  }
  CGxGammaRamp &operator=(const CGxGammaRamp &ramp) {
    memcpy(this, &ramp, sizeof(*this));
    return *this;
  }

  void Set(float gamma);

  WORD red[ENTRIES];
  WORD green[ENTRIES];
  WORD blue[ENTRIES];
};

struct CGxTexFlags {
  DWORD m_filter : 3;
  DWORD m_wrapU : 1;
  DWORD m_wrapV : 1;
  DWORD m_forceMipTracking : 1;
  DWORD m_generateMipMaps : 1;
  DWORD m_renderTarget : 1;
  DWORD m_maxAnisotropy : 5;

  CGxTexFlags(
      EGxTexFilter filter = GxTex_Linear,
      DWORD        wrapU = 0,
      DWORD        wrapV = 0,
      DWORD        force = 0,
      DWORD        generateMipMaps = 0,
      DWORD        renderTarget = 0,
      DWORD        maxAnisotropy = 1
  );

  bool operator==(const CGxTexFlags &flags) const;
  bool operator!=(const CGxTexFlags &flags) const;
};

struct CGxTexParms {
  UINT         width;
  UINT         height;
  EGxTexFormat format;
  CGxTexFlags  flags;
  LPVOID       userArg;
  void (*userFunc)(EGxTexCommand, UINT, UINT, UINT, UINT, LPVOID, UINT &, LPCVOID &);
};

struct CGxTexParmsEx {
  EGxTexTarget target;
  UINT         width;
  UINT         height;
  UINT         depth;
  EGxTexFormat format;
  EGxTexFormat dataFormat;
  CGxTexFlags  flags;
  LPVOID       userArg;
  void (*userFunc)(EGxTexCommand, UINT, UINT, UINT, UINT, LPVOID, UINT &, LPCVOID &);
};

class CGxDevice;
class CGxLight;
struct CGxBuf;
class CGxPixelShader;
class CGxTex;

typedef long (*GXWINDOWPROC)(LPVOID, UINT, UINT, long);

BOOL       GxAdapterID(WORD &vendorID, WORD &deviceID, DWORD &driverVersionHi, DWORD &driverVersionLow);
BOOL       GxAdapterInfer(WORD &deviceID);
int        GxAdapterMonitorModes(TSGrowableArray<CGxMonitorMode> &modes);
int        GxAdapterDesktopMode(CGxMonitorMode &mode);
CGxDevice *GxDevCreate(EGxApi api, GXWINDOWPROC windowProc, const CGxFormat &format);
void       GxDevDestroy(CGxDevice *devicePtr);
int        GxDevSetFormat(const CGxFormat &format);
void       GxDevSetBaseMipLevel(UINT baseMipLevel);
void       GxDevSetGamma(float gamma);
void       GxDevSetGamma(const CGxGammaRamp &ramp);
void       GxDevGammaRamp(CGxGammaRamp &ramp);
void       GxDevSystemGammaRamp(CGxGammaRamp &ramp);
DWORD      GxDevWindow();
EGxApi     GxDevApi();
void       GxDevOverride(EGxOverride override, DWORD value);
void       GxDevTakeScreenShot();
void       GxDevReadScreenShot(UINT &w, UINT &h, const NTempest::CImVector *&pixels);
void       GxDevClearScreenShot();
BOOL       GxTexCreate(const CGxTexParmsEx &parms, CGxTex *&texId);
BOOL       GxTexCreate(
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
);
BOOL GxTexCreate(
    UINT         width,
    UINT         height,
    EGxTexFormat format,
    CGxTexFlags  flags,
    LPVOID       userArg,
    void (*userFunc)(EGxTexCommand, UINT, UINT, UINT, UINT, LPVOID, UINT &, LPCVOID &),
    CGxTex *&texId
);
void GxTexDestroy(CGxTex *texId);
void GxTexUpdate(CGxTex *texId, int minX, int minY, int maxX, int maxY, int immediate);
void GxTexUpdate(CGxTex *texId, NTempest::CiRect &updateRect, int immediate);
void GxTexParametersEx(const CGxTex *texId, CGxTexParmsEx &parms);
void GxTexSetFlags(CGxTex *texId, CGxTexFlags flags);
void GxTexSetDataFormat(CGxTex *texId, EGxTexFormat dataFormat);
void GxTexSetUserData(CGxTex *texId, void (*userFunc)(EGxTexCommand, UINT, UINT, UINT, UINT, LPVOID, UINT &, LPCVOID &), LPVOID userArg);
void GxuUpdateSingleColorTexture(EGxTexCommand cmd, UINT w, UINT h, UINT d, UINT mipLevel, LPVOID userArg, UINT &texelStrideInBytes, LPCVOID &texels);
void GxuXformCreateOrtho(float minX, float maxX, float minY, float maxY, float minZ, float maxZ, NTempest::C44Matrix &dst);
void GxuXformCreateOrtho(const NTempest::CAaBox &bounds, NTempest::C44Matrix &dst);
void GxuXformCreateProjection(float fovyInRadians, float aspect, float minZ, float maxZ, NTempest::C44Matrix &dst);
void GxuXformCreateLookAtSgCompat(
    const NTempest::C3Vector &eye,
    const NTempest::C3Vector &center,
    const NTempest::C3Vector &up,
    NTempest::C44Matrix      &dst
);
void GxuXformCreateLookAtXXX(const NTempest::C3Vector &eye, const NTempest::C3Vector &center, const NTempest::C3Vector &up, NTempest::C44Matrix &dst);
void GxuXformCalcFrustumCorners(const NTempest::C44Matrix &view, const NTempest::C44Matrix &proj, NTempest::C3Vector *corners);
void GxuXformCalcFrustumPlanes(const NTempest::C44Matrix &viewProj, NTempest::C4Vector *planes);
void GxuXformCalcFrustumBounds(
    const NTempest::C44Matrix &view,
    const NTempest::C44Matrix &proj,
    NTempest::C3Vector        &minBound,
    NTempest::C3Vector        &maxBound
);
void GxuXformCalc2dScreenCoords(UINT count, const NTempest::C3Vector *src, NTempest::C3Vector *dst);
void GxuTexScale(
    LPCVOID      srcPixels,
    EGxTexFormat srcFormat,
    UINT         srcW,
    UINT         srcH,
    UINT         srcStrideInBytes,
    LPCVOID      dstPixels,
    EGxTexFormat dstFormat,
    UINT         dstW,
    UINT         dstH,
    UINT         dstStrideInBytes
);
BOOL GxuTestRayAndSphere(
    const NTempest::C3Vector &rayStart,
    const NTempest::C3Vector &rayDirection,
    const NTempest::C3Vector &sphereCenter,
    float                     sphereRadius,
    float                    &distance
);
BOOL GxuTestSphereAndFrustumPlanes(const NTempest::C3Vector &center, float radius, const NTempest::C4Vector *planes);
BOOL GxuTestRayAndTriangle(
    const NTempest::C3Vector &rayStart,
    const NTempest::C3Vector &rayDirection,
    const NTempest::C3Vector &v0,
    const NTempest::C3Vector &v1,
    const NTempest::C3Vector &v2,
    float                    &distance
);
BOOL GxuTestRayAndMesh(
    const NTempest::C3Vector  &rayStart,
    const NTempest::C3Vector  &rayDirection,
    const NTempest::C34Matrix *modelToWorldMatrices,
    UINT                       matrixCount,
    UINT                       posCount,
    const NTempest::C3Vector  *pos,
    UINT                       posStride,
    UINT                       boneCount,
    const BYTE                *bone,
    UINT                       boneStride,
    EGxPrim                    primType,
    UINT                       indexCount,
    const WORD                *indices,
    float                     &distance,
    UINT                      &primIntersected
);
BOOL GxuTestRayAndRigidMeshInModelSpace(
    const NTempest::C3Vector &rayStart,
    const NTempest::C3Vector &rayDirection,
    UINT                      posCount,
    const NTempest::C3Vector *pos,
    EGxPrim                   primType,
    UINT                      indexCount,
    const WORD               *indices,
    float                    &distance,
    UINT                     &primIntersected
);
UINT           GxuClipCalcCode(const NTempest::C44Matrix &viewProj, const NTempest::C3Vector &pos);
void           GxuSnapTexelsToPixels(const NTempest::C3Vector *pos, NTempest::C2Vector *tex, UINT texW, UINT texH);
const CGxCaps &GxCaps();
BlitFormat     GxGetBlitFormat(EGxTexFormat texFormat);
UINT           GxVertexSize(EGxVertexBufferFormat format);
UINT           GxVertexMemberOffset(EGxVertexBufferFormat format, EGxVertexMember member);
void           GxCapsWindowSize(NTempest::CRect &dst);
int            GxCapsIsWindowVisible();
UINT           GxPerfCounter(EGxPerfCounter counter);
void           GxMasterEnableSet(EGxMasterEnables state, int enable);
int            GxMasterEnable(EGxMasterEnables state);
void           GxRsSet(EGxRenderState which, NTempest::CImVector value);
void           GxRsSet(EGxRenderState which, float value);
void           GxRsSet(EGxRenderState which, int value);
void           GxRsSet(EGxRenderState which, LPVOID value);
void           GxRsGet(EGxRenderState which, NTempest::CImVector &value);
void           GxRsGet(EGxRenderState which, float &value);
void           GxRsGet(EGxRenderState which, int &value);
void           GxRsPush();
void           GxRsPop();
UINT           GxRsStackOffset();
void           GxLightSet(UINT whichLight, const CGxLight &lightInfo, const NTempest::C3Vector cameraPos);
void           GxLightEnable(UINT whichLight, int enable);
void           GxVertexShaderSelect(EGxVertexShader shader);
void           GxPrimLockVertexPtrs(
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
);
void    GxPrimDrawElements(EGxPrim primType, UINT indexCount, const WORD *indices);
void    GxPrimLockIndexPtr(EGxPrim primType, UINT indexCount, const WORD *indices);
void    GxPrimDrawElements();
void    GxPrimUnlockIndexPtr();
void    GxPrimUnlockVertexPtrs();
CGxBuf *GxBufCreate(
    EGxBufWriteFreq       writeFreq,
    EGxVertexBufferFormat format,
    UINT                  numVertices,
    UINT                  numIndices,
    void (*userCallback)(CGxBufCommand &, CGxBuf *),
    LPVOID userArg
);
void         GxBufLock(CGxBuf *buf);
void         GxBufUnlock();
void         GxBufRender(const CGxBatch *batches, UINT count);
void         GxBufRender(const CGxBatch &batch);
void         GxBufDestroy(CGxBuf *&buf);
void         GxBufReserve(EGxBufWriteFreq freq, EGxVertexBufferFormat format, UINT numVertices, UINT numIndices);
CGxBuf      *GxBufGetDynamic(EGxVertexBufferFormat format);
LPVOID       GxAllocVertexMem(UINT nBytes);
LPVOID       GxAllocIndexMem(UINT nBytes);
LPVOID       GxAllocPixelMem(UINT nBytes);
void         GxPixelShaderCreate(CGxPixelShader *&ps, LPCSTR filename);
void         GxPixelShaderDestroy(CGxPixelShader *&ps);
void         GxScenePresent(UINT mask);
void         GxSceneClear(UINT mask);
void         GxXformSetViewport(float minX, float maxX, float minY, float maxY, float minZ, float maxZ);
void         GxXformSetProjection(const NTempest::C44Matrix &matrix);
void         GxXformSetView(const NTempest::C44Matrix &matrix);
void         GxXformSetBones(UINT numBones, const NTempest::C34Matrix *matrices);
void         GxXformProjection(NTempest::C44Matrix &matrix);
void         GxXformView(NTempest::C44Matrix &matrix);
void         GxXform(EGxXform xf, NTempest::C44Matrix &matrix);
void         GxXformPush(EGxXform xf);
void         GxXformPush(EGxXform xf, const NTempest::C44Matrix &matrix);
void         GxXformPop(EGxXform xf);
void         GxXformSet(EGxXform xf, const NTempest::C44Matrix &matrix);
void         GxXformViewport(float &minX, float &maxX, float &minY, float &maxY, float &minZ, float &maxZ);
void         GxLogOpen();
void         GxLogClose();
void __cdecl GxLog(LPCSTR format, ...);

const TSGrowableArray<CGxFormat> *GxEnumFormats(EGxApi api);
CGxDevice                        *GxDevCreate(EGxApi api, UINT hwnd, const CGxFormat &format);
void                              GxDevWM(EGxWM wm, long param1, long param2);
void                              GxDevSetTextureQuality(int force32Bit);
const CGxFormat                  &GxDevFormat();
UINT                              GxDevBaseMipLevel();
int                               GxDevTextureQuality();
void                              GxDevReadPixels(NTempest::CiRect &rect, TSGrowableArray<NTempest::CImVector> &pixels);
void                              GxDevReadDepth(NTempest::CiRect &rect, TSGrowableArray<float> &depths);
void                              GxDevSetRenderTarget(EGxBuffer buffer, CGxTex *texture, UINT plane);
void                              GxLight(UINT whichLight, CGxLight &lightInfo);
void                              GxRsGet(EGxRenderState which, LPVOID &value);
void                              GxRsInit();
void                              GxCapsScreenSize(NTempest::CRect &dst);
void                              GxPrimBegin(EGxPrim primType);
void                              GxPrimEnd();
void                              GxPrimVertex(const NTempest::C3Vector &v);
void                              GxPrimTexCoord(UINT tmu, const NTempest::C2Vector &t);
void                              GxPrimNormal(const NTempest::C3Vector &n);
void                              GxPrimColor(const NTempest::CImVector &c);
void                              GxPrimPointSize(float s);
void                              GxPrimLineWidth(float w);
void                              GxSceneSetClearColor(NTempest::CImVector clearColor);
NTempest::CImVector               GxSceneClearColor();
BOOL                              GxTexCreate(const CGxTexParms &parms, CGxTex *&texId);
int                               GxTexNeedsUpdate(CGxTex *texId);
void                              GxTexParameters(const CGxTex *texId, CGxTexParms &parms);
void                              GxTexFlags(const CGxTex *texId, CGxTexFlags &flags);
void                              GxXformBone(UINT ndx, NTempest::C34Matrix &matrix);
void                              GxXformViewProj(NTempest::C44Matrix &matrix);
void                              GxXformIdentity(EGxXform xf);
void                              GxXformTranslate(EGxXform xf, const NTempest::C3Vector &t);
void                              GxXformScale(EGxXform xf, const NTempest::C3Vector &s);
void                              GxXformMult(EGxXform xf, const NTempest::C44Matrix &m);
void                              GxFreeVertexMem();
void                              GxFreeIndexMem();
void                              GxFreePixelMem();
EGxTexFormat                      GxGetGxTexFormat(BlitFormat blitFormat);
void                              GxTexGetDimensions(const CGxTex *texId, UINT *width, UINT *height);

extern CGxDevice *g_theGxDevicePtr;
