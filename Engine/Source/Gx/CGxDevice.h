#pragma once

#include "Gx.h"
#include "CGxStateBom.h"
#include <Tempest/cirect.h>
#include <Tempest/crange.h>
#include <stpl.h>

struct CGxBatch;
struct HTEXTURE__;
class CGxDevice;
class CGxDeviceD3d;
class CGxDeviceOpenGl;
class CVertexBufferList;
class CParticleEmitter2;
class SFile;
struct HSLOG__;
typedef struct HSLOG__ *HSLOG;

class CBoundingBox {
 public:
  NTempest::CRange x;
  NTempest::CRange y;
  NTempest::CRange z;
};

namespace NTempest {
  class C34Matrix;
}

class CGxShaderParam {
  friend class CGxShader;
  friend class CGxDeviceD3d;
  friend class CGxDeviceOpenGl;

  static const UINT TypeCountTable[];

 public:
  enum Type {
    Type_Vector4 = 0,
    Type_Matrix34 = 1,
    Type_Matrix44 = 2,
    Type_Force32Bit = -1
  };

  enum {
    NAME_LEN = 0x20
  };

  LPCSTR GetName() const {
    return name;
  }

  Type GetType() const {
    return type;
  }

  CGxShaderParam() : dirty(0) {
    for (UINT i = 0; i < 16; ++i) {
      f[i] = 0.0f;
    }
  }

 protected:
  void Read(SFile *file);
  void Set(const NTempest::C4Vector &v);
  void Set(const NTempest::C34Matrix &m);
  void Set(const NTempest::C44Matrix &m);

  char  name[NAME_LEN];
  Type  type;
  UINT  index;
  int   dirty;
  float f[16];
  LINKDECLEX(CGxShaderParam, lameAssLink);
};

class CGxShader {
  friend class CGxDevice;
  friend class CGxDeviceD3d;
  friend class CGxDeviceOpenGl;

 private:
  UINT refCount;

 protected:
  struct DirEntry {
    UINT start;
    UINT count;
  };

  typedef LISTEX(CGxShaderParam, lameAssLink) ParamList;

  void Read(SFile *file);

  UINT apiSpecific;
  int  valid;
  int  paramsDirty;
  LISTDECLEX(CGxShaderParam, lameAssLink, consts);
  LISTDECLEX(CGxShaderParam, lameAssLink, params);
  TSGrowableArray<BYTE> code;

 public:
  CGxShader() : refCount(0), apiSpecific(0), valid(0), paramsDirty(0) {
  }

  ~CGxShader();

  int Valid() {
    return valid;
  }

  void            SetParam(CGxShaderParam *p, const NTempest::C4Vector &v);
  void            SetParam(CGxShaderParam *p, const NTempest::C34Matrix &m);
  void            SetParam(CGxShaderParam *p, const NTempest::C44Matrix &m);
  CGxShaderParam *GetFirstParam();
  CGxShaderParam *GetNextParam(CGxShaderParam *p);
  CGxShaderParam *GetParam(LPCSTR name);
};

class CGxPixelShader : public CGxShader, public TSHashObject<CGxPixelShader, HASHKEY_STRI> {
 public:
  enum {
    Magic = 0x47585053
  };

  enum {
    Version = 0x10001
  };

  enum Target {
    Target_default = -2,
    Target_gx = -1,
    Target_ps_1_1 = 0,
    Target_ps_1_2 = 1,
    Target_ps_1_3 = 2,
    Target_ps_1_4 = 3,
    Target_ps_2_0 = 4,
    Target_nvrc = 5,
    Target_nvts = 6,
    Target_nvts2 = 7,
    Target_nvts3 = 8,
    Target_atifs = 9,
    Target_arbfp1 = 10,
    Targets_Last = 11
  };
};

class CGxVertexShader : public CGxShader, public TSHashObject<CGxVertexShader, HASHKEY_STRI> {
 public:
  enum {
    Magic = 0x47585653
  };

  enum {
    Version = 0x10001
  };

  enum Target {
    Target_default = -2,
    Target_gx = -1,
    Target_vs_1_1 = 0,
    Target_vs_2_0 = 1,
    Target_arbvp1 = 2,
    Targets_Last = 3
  };
};

class CGxCaps {
 public:
  UINT                    m_numTmus;
  int                     m_pixelCenterOnEdge;
  int                     m_texelCenterOnEdge;
  UINT                    m_maxTextureSize;
  int                     m_texOpAdd;
  int                     m_texOpMod2X;
  EGxColorFormat          m_colorFormat;
  int                     m_texFmtDxt;
  UINT                    m_maxIndex;
  int                     m_generateMipMaps;
  int                     m_rttFormat[8];
  int                     m_rttOriginUpperLeft;
  CGxPixelShader::Target  m_pixelShaderTarget;
  CGxVertexShader::Target m_vertexShaderTarget;
  int                     m_texFilterTrilinear;
  int                     m_texFilterAnisotropic;
  UINT                    m_maxTexAnisotropy;
  int                     m_depthBias;
  int                     m_mipMapLodBias;
};

struct CGxBuf {
  enum Status {
    S_VALID = 0,
    S_INVALID_DISCARD = 1,
    S_INVALID_RELOAD = 2
  };

  static const UINT BASE_NONE;

 protected:
  friend class CGxDevice;
  friend class CGxIndexBuffer;
  friend class CGxVertexBuffer;
  friend class CParticleEmitter2;

  LINKDECLEX(CGxBuf, linkGx);
  LINKDECLEX(CGxBuf, linkVB);
  LINKDECLEX(CGxBuf, linkIB);
  EGxBufWriteFreq       m_writeFreq;
  EGxVertexBufferFormat m_vbFormat;
  UINT                  m_numVertices;
  UINT                  m_numIndices;
  void (*m_userCallback)(CGxBufCommand &, CGxBuf *);
  LPVOID m_userArg;
  UINT   m_vertexBase;
  UINT   m_indexBase;
  Status m_vertexStatus;
  Status m_indexStatus;

  CGxBuf(const CGxBuf &);
  const CGxBuf &operator=(const CGxBuf &);

  UINT writeFrameTag;

 public:
  CGxBuf();
  void Invalidate(Status vertexStatus, Status indexStatus);

  UINT VertexCount() const {
    return m_numVertices;
  }

  UINT IndexCount() const {
    return m_numIndices;
  }

  void CountSet(UINT numVertices, UINT numIndices);

  LPVOID UserArg() const {
    return m_userArg;
  }

  void UserArgSet(LPVOID userArg) {
    m_userArg = userArg;
  }

  void (*UserCallback() const)(CGxBufCommand &, CGxBuf *) {
    return m_userCallback;
  }

  void UserCallbackSet(void (*userCallback)(CGxBufCommand &, CGxBuf *)) {
    m_userCallback = userCallback;
  }
};

class CGxMemBuffer {
  friend class CGxDeviceD3d;
  friend class CGxBufD3d;
  friend class CGxBufOgl;
  friend class CVertexBufferList;

 public:
  CGxMemBuffer(UINT count);
  virtual ~CGxMemBuffer();

  virtual void Lock(LPVOID &mem, UINT count, UINT base) = 0;
  virtual void Unlock() = 0;

  void AddBuf(CGxBuf *buf);
  void Discard();
  void RemoveBuf(CGxBuf *buf);

  UINT GetBase() {
    return m_base;
  }

  UINT GetCount() {
    return m_count;
  }

  UINT GetNext() {
    return m_next;
  }

  int GetDiscard() {
    return m_discard;
  }

 protected:
  void InvalidateBufs(CGxBuf::Status vertexStatus, CGxBuf::Status indexStatus);

  UINT m_count;
  UINT m_base;
  UINT m_next;
  int  m_discard;
  LISTEXDYN(CGxBuf) m_bufList;
};

class CGxVertexBuffer : public CGxMemBuffer {
 public:
  CGxVertexBuffer(UINT count) : CGxMemBuffer(count) {
    LISTEXSETLINK(CGxBuf, m_bufList, linkVB)
  }
};

class CGxIndexBuffer : public CGxMemBuffer {
 public:
  CGxIndexBuffer(UINT count) : CGxMemBuffer(count) {
    LISTEXSETLINK(CGxBuf, m_bufList, linkIB)
  }
};

class CGxTex {
 private:
  void Init(
      EGxTexTarget target,
      UINT         width,
      UINT         height,
      UINT         depth,
      EGxTexFormat format,
      EGxTexFormat dataFormat,
      CGxTexFlags  flags,
      LPVOID       userArg,
      void (*userFunc)(EGxTexCommand, UINT, UINT, UINT, UINT, LPVOID, UINT &, LPCVOID &)
  );

 public:
  CGxTex(
      EGxTexTarget target,
      UINT         width,
      UINT         height,
      UINT         depth,
      EGxTexFormat format,
      EGxTexFormat dataFormat,
      CGxTexFlags  flags,
      LPVOID       userArg,
      void (*userFunc)(EGxTexCommand, UINT, UINT, UINT, UINT, LPVOID, UINT &, LPCVOID &)
  );
  CGxTex(
      UINT         width,
      UINT         height,
      EGxTexFormat format,
      CGxTexFlags  flags,
      LPVOID       userArg,
      void (*userFunc)(EGxTexCommand, UINT, UINT, UINT, UINT, LPVOID, UINT &, LPCVOID &)
  );
  BYTE             m_needsUpdate;
  BYTE             m_needsCreation;
  BYTE             m_needsFlagUpdate;
  NTempest::CiRect m_updateRect;
  BYTE             m_updateFaces[6];
  short            m_updatePlaneMin;
  short            m_updatePlaneMax;
  UINT             m_frameTag;
  UINT             m_width;
  UINT             m_height;
  UINT             m_depth;
  EGxTexTarget     m_target;
  EGxTexFormat     m_format;
  EGxTexFormat     m_dataFormat;
  CGxTexFlags      m_flags;
  LPVOID           m_userArg;
  void (*m_userFunc)(EGxTexCommand, UINT, UINT, UINT, UINT, LPVOID, UINT &, LPCVOID &);
  LPVOID m_apiSpecificData;
};

class CGxLight {
 public:
  CGxLight();

  int                 m_enabled : 1;
  int                 m_isOmni : 1;
  NTempest::C3Vector  m_dir;
  NTempest::CImVector m_ambColor;
  NTempest::CImVector m_dirColor;
  NTempest::CImVector m_specColor;
  float               m_ambIntensity;
  float               m_dirIntensity;
  float               m_specIntensity;
  float               m_constantAttenuation;
  float               m_linearAttenuation;
  float               m_quadraticAttenuation;
  float               m_attenStart;
  float               m_attenEnd;
};

class CGxMatrixStack {
 public:
  enum EMatrixFlags {
    F_Identity = 1
  };

  CGxMatrixStack();
  ~CGxMatrixStack();
  void                       Identity();
  void                       Push();
  void                       Pop();
  NTempest::C44Matrix       &Top();
  const NTempest::C44Matrix &TopConst() const {
    return m_mtx[m_level];
  }

 private:
  friend class CGxDevice;
  friend class CGxDeviceD3d;
  friend class CGxDeviceOpenGl;

  UINT Flags();

  UINT                m_level;
  BYTE                m_dirty;
  NTempest::C44Matrix m_mtx[4];
  UINT                m_flags[4];
};

class CGxStateRegister {
 public:
  CGxStateRegister();

  CGxLight m_lights[8];
  int      m_lightsDirty[8];
  float    m_lightLinearFalloff;
  float    m_lightQuadraticFalloff;
  DWORD    m_masterEnables;
};

class CGxDevice {
 public:
  enum {
    PrimMask_Vertex = 1,
    PrimMask_TexCoord = 2,
    PrimMask_Normal = 32,
    PrimMask_Color = 64
  };

  enum {
    MinD3dBufVertices = 256,
    MinD3dBufIndices = 768
  };

  struct TextureTarget {
    CGxTex *m_texture;
    UINT    m_plane;
    LPVOID  m_apiSpecific;
  };

 protected:
  static const UINT s_texFormatBitDepth[];

  UINT                   ITexComputeByteSize(const CGxTex *texId, const UINT width, const UINT height);
  int                    EnableState(DWORD app, DWORD appDisables, UINT flagPos);
  int                    NeedsUpdate(DWORD app, DWORD hw, DWORD appDisables, DWORD hwDisables, UINT flagPos, int &enable);
  void                   ITexBind(CGxTex *texId);
  virtual void           ITexMarkAsUpdated(CGxTex *texId);
  virtual void           IRsSendToHw(EGxRenderState which) = 0;
  virtual void           ISetShaderParamList(TSExplicitList<CGxShaderParam, 108> &params, int forceForBind) = 0;
  void                   ISetShaderParameters(CGxShader *sh, int forceForBind);
  UINT                   IMatAlphaRef(EGxBlend op);
  int                    IVbHasColor(EGxVertexBufferFormat format);
  EGxVertexBufferFormat  IGiveVbColor(EGxVertexBufferFormat format);
  void                   DeviceScreenShot();
  int                    IDevIsWindowed();
  void                   ClampRectToWindow(NTempest::CiRect &rect);
  const NTempest::CRect &DeviceCurWindow();
  void                   DeviceSetDefWindow(const NTempest::CRect &rect);
  const NTempest::CRect &DeviceDefWindow();
  void                   DeviceSetCurWindow(const NTempest::CRect &rect);
  void                   CreateDynamicBufs();
  void                   DestroyDynamicBufs();
  void                   Log(const CGxCaps &caps) const;
  void                   Log(const CGxFormat &format) const;
  void                   PerfAcc(EGxPerfCounter counter, UINT value) {
    m_perfCountersAcc[counter] += value;
  }

 private:
  CGxDevice(const CGxDevice &);

 public:
  CGxDevice();
  virtual ~CGxDevice();
  virtual int         DeviceCreate(UINT hwnd, const CGxFormat &format);
  virtual int         DeviceCreate(GXWINDOWPROC windowProc, const CGxFormat &format);
  virtual void        DeviceDestroy();
  virtual int         DeviceSetFormat(const CGxFormat &format);
  virtual void        DeviceSetBaseMipLevel(UINT baseMipLevel);
  virtual void        DeviceSetGamma(const CGxGammaRamp &ramp);
  virtual void        DeviceSetGamma(float gamma);
  virtual void        DeviceSetTextureQuality(int force32);
  virtual DWORD       DeviceWindow() = 0;
  virtual void        DeviceTakeScreenShot();
  virtual void        DeviceReadScreenShot(UINT &w, UINT &h, const NTempest::CImVector *&pixels);
  virtual void        DeviceReadPixels(NTempest::CiRect &rect, TSGrowableArray<NTempest::CImVector> &pixels) = 0;
  virtual void        DeviceReadDepths(NTempest::CiRect &rect, TSGrowableArray<float> &depths) = 0;
  virtual void        DeviceWM(EGxWM wm, long param1, long param2) = 0;
  virtual void        DeviceSetRenderTarget(EGxBuffer buffer, CGxTex *texture, UINT plane);
  virtual void        DeviceOverride(EGxOverride override, DWORD value);
  virtual void        CapsWindowSize(NTempest::CRect &dst) = 0;
  virtual void        CapsWindowSizeInScreenCoords(NTempest::CRect &dst) = 0;
  virtual int         CapsIsWindowVisible() = 0;
  virtual void        SceneSetClearColor(NTempest::CImVector clearColor);
  NTempest::CImVector SceneClearColor();
  virtual void        ScenePresent(UINT mask);
  virtual void        SceneClear(UINT mask);
  virtual void        XformSetViewport(float minX, float maxX, float minY, float maxY, float minZ, float maxZ);
  virtual void        XformSetProjection(const NTempest::C44Matrix &matrix);
  virtual void        XformSetView(const NTempest::C44Matrix &matrix);
  virtual void        XformSetBones(UINT numBones, const NTempest::C34Matrix *matrices);
  virtual void        VertexShaderSelect(EGxVertexShader shader);
  virtual void        PrimLockAndProcessVertexPtrs(
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
  virtual void PrimLockIndexPtr(EGxPrim primType, UINT indexCount, const WORD *indices);
  virtual void PrimDrawElements();
  virtual void PrimUnlockIndexPtr();
  virtual void PrimUnlockVertexPtrs();
  virtual void PrimBegin(EGxPrim primType);
  virtual void PrimEnd();
  virtual void PrimVertex(const NTempest::C3Vector &v);
  virtual void PrimTexCoord(UINT tmu, const NTempest::C2Vector &t);
  virtual void PrimNormal(const NTempest::C3Vector &n);
  virtual void PrimColor(const NTempest::CImVector &c);
  virtual void PrimPointSize(float s) {
  }
  virtual void PrimLineWidth(float w) {
  }
  UINT            PrimCalcCount(EGxPrim primType, UINT indexCount);
  virtual void    LightSet(UINT whichLight, const CGxLight &lightInfo, const NTempest::C3Vector &cameraPos);
  virtual void    LightEnable(UINT whichLight, int enable);
  virtual void    MasterEnableSet(EGxMasterEnables state, int enable);
  virtual CGxBuf *BufCreate(
      EGxBufWriteFreq       writeFreq,
      EGxVertexBufferFormat format,
      UINT                  numVertices,
      UINT                  numIndices,
      void (*userCallback)(CGxBufCommand &, CGxBuf *),
      LPVOID userArg
  );
  virtual void BufLock(CGxBuf *buf);
  virtual void BufRender(const CGxBatch *batches, UINT count);
  virtual void BufUnlock();
  virtual void BufDestroy(CGxBuf *&buf);
  virtual void BufReserve(EGxBufWriteFreq freq, EGxVertexBufferFormat format, UINT numVertices, UINT numIndices);
  CGxBuf      *BufGetDynamic(EGxVertexBufferFormat format);
  virtual int  TexCreate(
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
  virtual int TexCreate(
      UINT         width,
      UINT         height,
      EGxTexFormat format,
      CGxTexFlags  flags,
      LPVOID       userArg,
      void (*userFunc)(EGxTexCommand, UINT, UINT, UINT, UINT, LPVOID, UINT &, LPCVOID &),
      CGxTex *&texId
  );
  virtual void TexDestroy(CGxTex *texId);
  void         TexMarkForUpdate(CGxTex *texId, const NTempest::CiRect &updateRect, int immediate);
  int          TexNeedsUpdate(CGxTex *texId);
  void         TexSetFlags(CGxTex *texId, CGxTexFlags flags);
  void         TexSetDataFormat(CGxTex *texId, EGxTexFormat dataFormat);
  void         TexSetUserData(CGxTex *texId, void (*userFunc)(EGxTexCommand, UINT, UINT, UINT, UINT, LPVOID, UINT &, LPCVOID &), LPVOID userArg);
  LPVOID       TexUserArg(CGxTex *texId);
  void         TexGetDimensions(const CGxTex *texId, UINT *width, UINT *height);
  void         TexParameters(const CGxTex *texId, CGxTexParms &parms);
  void         TexParameters(const CGxTex *texId, CGxTexParmsEx &parms);
  void         TexFlags(const CGxTex *texId, CGxTexFlags &flags);
  virtual void PixelShaderCreate(CGxPixelShader *&ps, LPCSTR filename);
  virtual void PixelShaderDestroy(CGxPixelShader *&ps);
  virtual void VertexShaderCreate(CGxVertexShader *&vs, LPCSTR filename);
  virtual void VertexShaderDestroy(CGxVertexShader *&vs);

  static CGxDevice *NewD3d();
  static CGxDevice *NewOpenGl();
  static int        AdapterID(WORD &vendorID, WORD &deviceID, DWORD &driverVersionHi, DWORD &driverVersionLow);
  static int        AdapterInfer(WORD &deviceID);
  static int        AdapterMonitorModes(TSGrowableArray<CGxMonitorMode> &modes);
  static int        AdapterDesktopMode(CGxMonitorMode &mode);
  static int        D3dEnumFormats(TSGrowableArray<CGxFormat> &formats);
  static int        OpenGlEnumFormats(TSGrowableArray<CGxFormat> &formats);

  const CGxFormat &DeviceFormat();
  UINT             DeviceBaseMipLevel();
  void             DeviceGamma(CGxGammaRamp &ramp);
  void             DeviceSystemGamma(CGxGammaRamp &ramp);
  int              DeviceTextureQuality();
  EGxApi           DeviceApi();
  const CGxCaps   &Caps() const;
  void             DeviceClearScreenShot();
  UINT             PerfCounter(EGxPerfCounter counter);
  static float     CpuFrequency();
  static LONGLONG  CpuTicks();
  void             XformProjection(NTempest::C44Matrix &matrix);
  void             XformView(NTempest::C44Matrix &matrix);
  void             XformBone(UINT ndx, NTempest::C34Matrix &matrix);
  void             Xform(EGxXform xf, NTempest::C44Matrix &matrix);
  void             XformPush(EGxXform xf);
  void             XformPush(EGxXform xf, const NTempest::C44Matrix &matrix);
  void             XformPop(EGxXform xf);
  void             XformIdentity(EGxXform xf);
  void             XformSet(EGxXform xf, const NTempest::C44Matrix &matrix);
  void             XformTranslate(EGxXform xf, const NTempest::C3Vector &t);
  void             XformScale(EGxXform xf, const NTempest::C3Vector &s);
  void             XformMult(EGxXform xf, const NTempest::C44Matrix &m);
  void             XformViewport(float &minX, float &maxX, float &minY, float &maxY, float &minZ, float &maxZ);
  void             RsSet(EGxRenderState which, int value);
  void             RsSet(EGxRenderState which, float value);
  void             RsSet(EGxRenderState which, NTempest::CImVector value);
  void             RsSet(EGxRenderState which, const NTempest::C3Vector &value);
  void             RsSet(EGxRenderState which, LPVOID value);
  void             RsGet(EGxRenderState which, int &value);
  void             RsGet(EGxRenderState which, float &value);
  void             RsGet(EGxRenderState which, NTempest::CImVector &value);
  void             RsGet(EGxRenderState which, NTempest::C3Vector &value);
  void             RsGet(EGxRenderState which, LPVOID &value);
  void             RsPush();
  void             RsPop();
  void             RsInit();
  UINT             RsStackOffset();
  int              MasterEnable(EGxMasterEnables state);
  void             Light(UINT whichLight, CGxLight &lightInfo);

  static void         LogOpen();
  static void         LogClose();
  static void __cdecl Log(LPCSTR format, ...);
  static void __cdecl DbgPrintf(LPCSTR format, ...);

 private:
  friend HTEXTURE__ *TextureAllocImage(EGxTexFormat format, UINT width, UINT height);
  friend class CGxDeviceD3d;
  friend class CGxDeviceOpenGl;

  const CGxDevice &operator=(const CGxDevice &);
  void             IRsInit();

 protected:
  void IRsForceUpdate(EGxRenderState ndx_);
  void IRsForceUpdate();
  void IRsSync(int force);

 private:
  void IRsSet(EGxRenderState which, const CGxStateBom &value);
  void PerfCountersLatch();

  TSGrowableArray<CGxPushedRenderState> mPushedStates;
  TSGrowableArray<DWORD>                mStackOffsets;
  TSGrowableArray<EGxRenderState>       mDirtyStates;
  UINT                                  m_perfCountersLatched[13];
  UINT                                  m_perfCountersAcc[13];
  EGxPrim                               m_primType;
  UINT                                  m_primIndexCount;
  int                                   m_indexLocked;
  int                                   m_vertexLocked;
  int                                   m_inBeginEnd;
  NTempest::C3Vector                    m_primVertex;
  NTempest::C2Vector                    m_primTexCoord[4];
  NTempest::C3Vector                    m_primNormal;
  NTempest::CImVector                   m_primColor;
  TSGrowableArray<NTempest::C3Vector>   m_primVertexArray;
  TSGrowableArray<NTempest::C2Vector>   m_primTexCoordArray[4];
  TSGrowableArray<NTempest::C3Vector>   m_primNormalArray;
  TSGrowableArray<NTempest::CImVector>  m_primColorArray;
  TSGrowableArray<WORD>                 m_primIndexArray;
  UINT                                  m_primMask;
  NTempest::CRect                       m_defWindowRect;
  NTempest::CRect                       m_curWindowRect;

 protected:
  int                                                m_context;
  EGxApi                                             m_api;
  DWORD                                              m_cpuFeatures;
  CGxFormat                                          m_format;
  CGxCaps                                            m_caps;
  UINT                                               m_baseMipLevel;
  int                                                m_force32BitTextures;
  NTempest::CImVector                                m_clearColor;
  CGxGammaRamp                                       m_gammaRamp;
  CGxGammaRamp                                       m_systemGammaRamp;
  GXWINDOWPROC                                       m_windowProc;
  CBoundingBox                                       m_viewport;
  NTempest::C44Matrix                                m_projection;
  const NTempest::C34Matrix                         *m_bones;
  UINT                                               m_boneCount;
  CGxMatrixStack                                     m_xforms[7];
  CGxMatrixStack                                     m_texGen[4];
  EGxVertexShader                                    m_vertexShader;
  EGxVertexBufferFormat                              m_vertexBufferFormat;
  CGxPixelShader::Target                             m_pixelShaderPlatform;
  TSHashTableReuse<CGxPixelShader, HASHKEY_STRI, 1>  m_pixelShaderList;
  TSHashTableReuse<CGxVertexShader, HASHKEY_STRI, 1> m_vertexShaderList;
  CGxStateRegister                                   m_appState;
  CGxStateRegister                                   m_hwState;
  LISTDECLEX(CGxBuf, linkGx, m_bufList);
  CGxBuf                              *m_bufLocked;
  UINT                                 m_VBReserve[4][9];
  UINT                                 m_IBReserve[4][9];
  CGxBuf                              *m_dynBuf[9];
  TSFixedArray<CGxAppRenderState>      mAppRenderStates;
  TSFixedArray<CGxStateBom>            mHwRenderStates;
  TSGrowableArray<CGxTex *>            m_textures;
  TextureTarget                        m_textureTarget[2];
  int                                  m_scrShotClick;
  UINT                                 m_scrShotWidth;
  UINT                                 m_scrShotHeight;
  TSGrowableArray<NTempest::CImVector> m_scrShotPixels;

  static HSLOG m_log;
};
