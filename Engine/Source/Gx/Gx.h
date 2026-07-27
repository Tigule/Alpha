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

template <class T>
class TSGrowableArray;

namespace NTempest {
  class C34Matrix;
  class C4Vector;
  class CAaBox;
}  // namespace NTempest

typedef unsigned long CArgb;

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
  EGxBufOp     op;
  void       **mem[GxVertexMembers_Last];
  unsigned int stride[GxVertexMembers_Last];

  CGxBufOp() {
  }
  void Set(EGxVertexMember member, void *memory, unsigned int memberStride) {
    *mem[member] = memory;
    stride[member] = memberStride;
  }
};

struct CGxBufCommand {
  CGxBufOp vertex;
  CGxBufOp index;

  CGxBufCommand() {
  }
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

  CGxVertexPNT0();
  ~CGxVertexPNT0();
};

struct CGxVertexPNT0T1 {
  NTempest::C3Vector p;
  NTempest::C3Vector n;
  NTempest::C2Vector tc[2];

  CGxVertexPNT0T1();
  ~CGxVertexPNT0T1();
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
  EGxPrim      m_primType;
  unsigned int m_count;
  unsigned int m_start;
  int          m_minIndex;
  int          m_maxIndex;

  CGxBatch() {
  }

  CGxBatch(EGxPrim prim, unsigned int count, unsigned int start, int minIndex, int maxIndex)
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
  mutable unsigned int apiSpecificModeID;

 public:
  bool                hwTnL;
  bool                fixLag;
  bool                window;
  Format              depthFormat;
  NTempest::C2iVector size;
  Format              colorFormat;
  unsigned int        refreshRate;
  bool                vsync;
  NTempest::C2iVector pos;

  CGxFormat();
  CGxFormat(
      bool                       p_window,
      const NTempest::C2iVector &p_size,
      Format                     p_colorFormat,
      Format                     p_depthFormat,
      unsigned int               p_refreshRate,
      bool                       p_vsync,
      bool                       p_hwTnl,
      bool                       p_fixLag
  );
  ~CGxFormat();
};

struct CGxMonitorMode {
  NTempest::C2iVector size;
  unsigned int        bpp;
  unsigned int        refreshRate;

  ~CGxMonitorMode() {
  }
};

class CGxCaps {
 public:
  unsigned int   m_numTmus;
  int            m_pixelCenterOnEdge;
  int            m_texelCenterOnEdge;
  unsigned int   m_maxTextureSize;
  int            m_texOpAdd;
  int            m_texOpMod2X;
  EGxColorFormat m_colorFormat;
  int            m_texFmtDxt;
  unsigned int   m_maxIndex;
  int            m_generateMipMaps;
  int            m_rttFormat[8];
  int            m_rttOriginUpperLeft;
  int            m_pixelShaderTarget;
  int            m_vertexShaderTarget;
  int            m_texFilterTrilinear;
  int            m_texFilterAnisotropic;
  unsigned int   m_maxTexAnisotropy;
  int            m_depthBias;
  int            m_mipMapLodBias;
};

struct CGxGammaRamp {
  enum {
    ENTRIES = 256
  };

  void Set(float gamma);

  unsigned short red[ENTRIES];
  unsigned short green[ENTRIES];
  unsigned short blue[ENTRIES];
};

struct CGxTexFlags {
  unsigned int m_filter : 3;
  unsigned int m_wrapU : 1;
  unsigned int m_wrapV : 1;
  unsigned int m_forceMipTracking : 1;
  unsigned int m_generateMipMaps : 1;
  unsigned int m_renderTarget : 1;
  unsigned int m_maxAnisotropy : 5;

  CGxTexFlags(
      EGxTexFilter  filter,
      unsigned long wrapU,
      unsigned long wrapV,
      unsigned long force,
      unsigned long generateMipMaps,
      unsigned long renderTarget,
      unsigned long maxAnisotropy
  );
};

struct CGxTexParms {
  unsigned int width;
  unsigned int height;
  EGxTexFormat format;
  CGxTexFlags  flags;
  void        *userArg;
  void(*userFunc)(EGxTexCommand, unsigned int, unsigned int, unsigned int, unsigned int, void *, unsigned int &, const void *&);
};

struct CGxTexParmsEx {
  CGxTexParmsEx() : flags(GxTex_Linear, 0, 0, 0, 0, 0, 1) {
  }

  EGxTexTarget target;
  unsigned int width;
  unsigned int height;
  unsigned int depth;
  EGxTexFormat format;
  EGxTexFormat dataFormat;
  CGxTexFlags  flags;
  void        *userArg;
  void(*userFunc)(EGxTexCommand, unsigned int, unsigned int, unsigned int, unsigned int, void *, unsigned int &, const void *&);
};

class CGxDevice;
class CGxLight;
struct CGxBuf;
class CGxPixelShader;
class CGxTex;

typedef long(*GXWINDOWPROC)(void *, unsigned int, unsigned int, long);

int GxAdapterID(unsigned short &vendorID, unsigned short &deviceID, unsigned long &driverVersionHi, unsigned long &driverVersionLow);
int GxAdapterInfer(unsigned short &deviceID);
int GxAdapterMonitorModes(TSGrowableArray<CGxMonitorMode> &modes);
int GxAdapterDesktopMode(CGxMonitorMode &mode);
CGxDevice *GxDevCreate(EGxApi api, GXWINDOWPROC windowProc, const CGxFormat &format);
void GxDevDestroy(CGxDevice *devicePtr);
int GxDevSetFormat(const CGxFormat &format);
void GxDevSetBaseMipLevel(unsigned int baseMipLevel);
void GxDevSetGamma(float gamma);
void GxDevSetGamma(const CGxGammaRamp &ramp);
void GxDevGammaRamp(CGxGammaRamp &ramp);
void GxDevSystemGammaRamp(CGxGammaRamp &ramp);
unsigned long GxDevWindow();
EGxApi GxDevApi();
void GxDevOverride(EGxOverride override, unsigned long value);
void GxDevTakeScreenShot();
void GxDevReadScreenShot(unsigned int &w, unsigned int &h, const NTempest::CImVector *&pixels);
void GxDevClearScreenShot();
int GxTexCreate(const CGxTexParmsEx &parms, CGxTex *&texId);
int GxTexCreate(
    EGxTexTarget target,
    unsigned int width,
    unsigned int height,
    unsigned int depth,
    EGxTexFormat format,
    EGxTexFormat dataFormat,
    CGxTexFlags  flags,
    void        *userArg,
    void(*userFunc)(EGxTexCommand, unsigned int, unsigned int, unsigned int, unsigned int, void *, unsigned int &, const void *&),
    CGxTex *&texId
);
int GxTexCreate(
    unsigned int width,
    unsigned int height,
    EGxTexFormat format,
    CGxTexFlags  flags,
    void        *userArg,
    void(*userFunc)(EGxTexCommand, unsigned int, unsigned int, unsigned int, unsigned int, void *, unsigned int &, const void *&),
    CGxTex *&texId
);
void GxTexDestroy(CGxTex *texId);
void GxTexUpdate(CGxTex *texId, int minX, int minY, int maxX, int maxY, int immediate);
void GxTexUpdate(CGxTex *texId, NTempest::CiRect &updateRect, int immediate);
void GxTexParametersEx(const CGxTex *texId, CGxTexParmsEx &parms);
void GxTexSetFlags(CGxTex *texId, CGxTexFlags flags);
void GxTexSetDataFormat(CGxTex *texId, EGxTexFormat dataFormat);
void GxTexSetUserData(
    CGxTex *texId,
    void(*userFunc)(EGxTexCommand, unsigned int, unsigned int, unsigned int, unsigned int, void *, unsigned int &, const void *&),
    void *userArg
);
void GxuUpdateSingleColorTexture(
    EGxTexCommand cmd,
    unsigned int  w,
    unsigned int  h,
    unsigned int  d,
    unsigned int  mipLevel,
    void         *userArg,
    unsigned int &texelStrideInBytes,
    const void  *&texels
);
void GxuXformCreateOrtho(float minX, float maxX, float minY, float maxY, float minZ, float maxZ, NTempest::C44Matrix &dst);
void GxuXformCreateOrtho(const NTempest::CAaBox &bounds, NTempest::C44Matrix &dst);
void GxuXformCreateProjection(float fovyInRadians, float aspect, float minZ, float maxZ, NTempest::C44Matrix &dst);
void
GxuXformCreateLookAtSgCompat(const NTempest::C3Vector &eye, const NTempest::C3Vector &center, const NTempest::C3Vector &up, NTempest::C44Matrix &dst);
void
GxuXformCreateLookAtXXX(const NTempest::C3Vector &eye, const NTempest::C3Vector &center, const NTempest::C3Vector &up, NTempest::C44Matrix &dst);
void GxuXformCalcFrustumCorners(const NTempest::C44Matrix &view, const NTempest::C44Matrix &proj, NTempest::C3Vector *corners);
void GxuXformCalcFrustumPlanes(const NTempest::C44Matrix &viewProj, NTempest::C4Vector *planes);
void GxuXformCalcFrustumBounds(
    const NTempest::C44Matrix &view,
    const NTempest::C44Matrix &proj,
    NTempest::C3Vector        &minBound,
    NTempest::C3Vector        &maxBound
);
void GxuXformCalc2dScreenCoords(unsigned int count, const NTempest::C3Vector *src, NTempest::C3Vector *dst);
void GxuTexScale(
    const void    *srcPixels,
    EGxTexFormat   srcFormat,
    unsigned int   srcW,
    unsigned int   srcH,
    unsigned int   srcStrideInBytes,
    const void    *dstPixels,
    EGxTexFormat   dstFormat,
    unsigned int   dstW,
    unsigned int   dstH,
    unsigned int   dstStrideInBytes
);
int GxuTestRayAndSphere(
    const NTempest::C3Vector &rayStart,
    const NTempest::C3Vector &rayDirection,
    const NTempest::C3Vector &sphereCenter,
    float                     sphereRadius,
    float                    &distance
);
int GxuTestSphereAndFrustumPlanes(const NTempest::C3Vector &center, float radius, const NTempest::C4Vector *planes);
int GxuTestRayAndTriangle(
    const NTempest::C3Vector &rayStart,
    const NTempest::C3Vector &rayDirection,
    const NTempest::C3Vector &v0,
    const NTempest::C3Vector &v1,
    const NTempest::C3Vector &v2,
    float                    &distance
);
int GxuTestRayAndMesh(
    const NTempest::C3Vector  &rayStart,
    const NTempest::C3Vector  &rayDirection,
    const NTempest::C34Matrix *modelToWorldMatrices,
    unsigned int               matrixCount,
    unsigned int               posCount,
    const NTempest::C3Vector  *pos,
    unsigned int               posStride,
    unsigned int               boneCount,
    const unsigned char       *bone,
    unsigned int               boneStride,
    EGxPrim                    primType,
    unsigned int               indexCount,
    const unsigned short      *indices,
    float                     &distance,
    unsigned int              &primIntersected
);
int GxuTestRayAndRigidMeshInModelSpace(
    const NTempest::C3Vector &rayStart,
    const NTempest::C3Vector &rayDirection,
    unsigned int              posCount,
    const NTempest::C3Vector *pos,
    EGxPrim                   primType,
    unsigned int              indexCount,
    const unsigned short     *indices,
    float                    &distance,
    unsigned int             &primIntersected
);
unsigned int GxuClipCalcCode(const NTempest::C44Matrix &viewProj, const NTempest::C3Vector &pos);
void GxuSnapTexelsToPixels(
    const NTempest::C3Vector *pos,
    NTempest::C2Vector       *tex,
    unsigned int              texW,
    unsigned int              texH
);
const CGxCaps &GxCaps();
BlitFormat GxGetBlitFormat(EGxTexFormat texFormat);
unsigned int GxVertexSize(EGxVertexBufferFormat format);
unsigned int GxVertexMemberOffset(EGxVertexBufferFormat format, EGxVertexMember member);
void GxCapsWindowSize(NTempest::CRect &dst);
int GxCapsIsWindowVisible();
unsigned int GxPerfCounter(EGxPerfCounter counter);
void GxMasterEnableSet(EGxMasterEnables state, int enable);
int GxMasterEnable(EGxMasterEnables state);
void GxRsSet(EGxRenderState which, NTempest::CImVector value);
void GxRsSet(EGxRenderState which, float value);
void GxRsSet(EGxRenderState which, int value);
void GxRsSet(EGxRenderState which, void *value);
void GxRsGet(EGxRenderState which, NTempest::CImVector &value);
void GxRsGet(EGxRenderState which, float &value);
void GxRsGet(EGxRenderState which, int &value);
void GxRsPush();
void GxRsPop();
unsigned int GxRsStackOffset();
void GxLightSet(unsigned int whichLight, const CGxLight &lightInfo, NTempest::C3Vector cameraPos);
void GxLightEnable(unsigned int whichLight, int enable);
void GxVertexShaderSelect(EGxVertexShader shader);
void GxPrimLockVertexPtrs(
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
);
void GxPrimDrawElements(EGxPrim primType, unsigned int indexCount, const unsigned short *indices);
void GxPrimLockIndexPtr(EGxPrim primType, unsigned int indexCount, const unsigned short *indices);
void GxPrimDrawElements();
void GxPrimUnlockIndexPtr();
void GxPrimUnlockVertexPtrs();
CGxBuf *GxBufCreate(
    EGxBufWriteFreq       writeFreq,
    EGxVertexBufferFormat format,
    unsigned int          numVertices,
    unsigned int          numIndices,
    void(*userCallback)(CGxBufCommand &, CGxBuf *),
    void *userArg
);
void GxBufLock(CGxBuf *buf);
void GxBufUnlock();
void GxBufRender(const CGxBatch *batches, unsigned int count);
void GxBufRender(const CGxBatch &batch);
void GxBufDestroy(CGxBuf *&buf);
void GxBufReserve(EGxBufWriteFreq freq, EGxVertexBufferFormat format, unsigned int numVertices, unsigned int numIndices);
CGxBuf *GxBufGetDynamic(EGxVertexBufferFormat format);
void *GxAllocVertexMem(unsigned int nBytes);
void *GxAllocIndexMem(unsigned int nBytes);
void *GxAllocPixelMem(unsigned int nBytes);
void GxPixelShaderCreate(CGxPixelShader *&ps, const char *filename);
void GxPixelShaderDestroy(CGxPixelShader *&ps);
void GxScenePresent(unsigned int mask);
void GxSceneClear(unsigned int mask);
void GxXformSetViewport(float minX, float maxX, float minY, float maxY, float minZ, float maxZ);
void GxXformSetProjection(const NTempest::C44Matrix &matrix);
void GxXformSetView(const NTempest::C44Matrix &matrix);
void GxXformSetBones(unsigned int numBones, const NTempest::C34Matrix *matrices);
void GxXformProjection(NTempest::C44Matrix &matrix);
void GxXformView(NTempest::C44Matrix &matrix);
void GxXform(EGxXform xf, NTempest::C44Matrix &matrix);
void GxXformPush(EGxXform xf);
void GxXformPush(EGxXform xf, const NTempest::C44Matrix &matrix);
void GxXformPop(EGxXform xf);
void GxXformSet(EGxXform xf, const NTempest::C44Matrix &matrix);
void GxXformViewport(float &minX, float &maxX, float &minY, float &maxY, float &minZ, float &maxZ);
void GxLogOpen();
void GxLogClose();
void __cdecl       GxLog(const char *format, ...);

const TSGrowableArray<CGxFormat> *GxEnumFormats(EGxApi api);
CGxDevice *GxDevCreate(EGxApi api, unsigned int hwnd, const CGxFormat &format);
void GxDevWM(EGxWM wm, long param1, long param2);
void GxDevSetTextureQuality(int force32Bit);
const CGxFormat &GxDevFormat();
unsigned int GxDevBaseMipLevel();
int GxDevTextureQuality();
void GxDevReadPixels(NTempest::CiRect &rect, TSGrowableArray<NTempest::CImVector> &pixels);
void GxDevReadDepth(NTempest::CiRect &rect, TSGrowableArray<float> &depths);
void GxDevSetRenderTarget(EGxBuffer buffer, CGxTex *texture, unsigned int plane);
void GxLight(unsigned int whichLight, CGxLight &lightInfo);
void GxRsGet(EGxRenderState which, void *&value);
void GxRsInit();
void GxCapsScreenSize(NTempest::CRect &dst);
void GxPrimBegin(EGxPrim primType);
void GxPrimEnd();
void GxPrimVertex(const NTempest::C3Vector &v);
void GxPrimTexCoord(unsigned int tmu, const NTempest::C2Vector &t);
void GxPrimNormal(const NTempest::C3Vector &n);
void GxPrimColor(const NTempest::CImVector &c);
void GxPrimPointSize(float s);
void GxPrimLineWidth(float w);
void GxSceneSetClearColor(NTempest::CImVector clearColor);
NTempest::CImVector GxSceneClearColor();
int GxTexCreate(const CGxTexParms &parms, CGxTex *&texId);
int GxTexNeedsUpdate(CGxTex *texId);
void GxTexParameters(const CGxTex *texId, CGxTexParms &parms);
void GxTexFlags(const CGxTex *texId, CGxTexFlags &flags);
void GxXformBone(unsigned int ndx, NTempest::C34Matrix &matrix);
void GxXformViewProj(NTempest::C44Matrix &matrix);
void GxXformIdentity(EGxXform xf);
void GxXformTranslate(EGxXform xf, const NTempest::C3Vector &t);
void GxXformScale(EGxXform xf, const NTempest::C3Vector &s);
void GxXformMult(EGxXform xf, const NTempest::C44Matrix &m);
void GxFreeVertexMem();
void GxFreeIndexMem();
void GxFreePixelMem();
EGxTexFormat GxGetGxTexFormat(BlitFormat blitFormat);
void GxTexGetDimensions(const CGxTex *texId, unsigned int *width, unsigned int *height);

extern CGxDevice *g_theGxDevicePtr;
