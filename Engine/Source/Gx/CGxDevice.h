#pragma once

#include "Gx.h"
#include "CGxStateBom.h"
#include <Tempest/cirect.h>
#include <stpl.h>

struct CGxBatch;
class CGxDevice;
class CGxDeviceD3d;
class CGxDeviceOpenGl;
class CVertexBufferList;
class CParticleEmitter2;
class SFile;
struct HSLOG__;
typedef struct HSLOG__ *HSLOG;

namespace NTempest {
  class C34Matrix;
}

class CGxShaderParam {
  friend class CGxShader;
  friend class CGxDeviceD3d;
  friend class CGxDeviceOpenGl;

  static const unsigned int TypeCountTable[3];

 public:
  enum Type {
    Type_Vector4 = 0,
    Type_Matrix34 = 1,
    Type_Matrix44 = 2,
    Type_Force32Bit = 0xFF
  };

  enum {
    NAME_LEN = 0x20
  };

  const char *GetName() {
    return name;
  }

  Type GetType() {
    return type;
  }

  CGxShaderParam() : dirty(0) {
    for (unsigned int i = 0; i < 16; ++i) {
      f[i] = 0.0f;
    }
  }

 protected:
  void Read(SFile *file);
  void Set(const NTempest::C4Vector &v);
  void Set(const NTempest::C34Matrix &m);
  void Set(const NTempest::C44Matrix &m);

  char                   name[NAME_LEN];
  Type                   type;
  unsigned int           index;
  int                    dirty;
  float                  f[16];
  TSLink<CGxShaderParam> lameAssLink;
};

class CGxShader {
  friend class CGxDevice;
  friend class CGxDeviceD3d;
  friend class CGxDeviceOpenGl;

 private:
  unsigned int refCount;

 protected:
  struct DirEntry {
    unsigned int start;
    unsigned int count;
  };

  void Read(SFile *file);

  unsigned int                        apiSpecific;
  int                                 valid;
  int                                 paramsDirty;
  TSExplicitList<CGxShaderParam, 108> consts;
  TSExplicitList<CGxShaderParam, 108> params;
  TSGrowableArray<unsigned char>      code;

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
  CGxShaderParam *GetParam(const char *name);
};

class CGxPixelShader : public CGxShader, public TSHashObject<CGxPixelShader, HASHKEY_STRI> {
 public:
  enum {
    Magic = 0x47585053,
    Version = 0x10001
  };

  enum Target {
    Target_default = 0xFE,
    Target_gx = 0xFF,
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
    Magic = 0x47585653,
    Version = 0x10001
  };

  enum Target {
    Target_default = 0xFE,
    Target_gx = 0xFF,
    Target_vs_1_1 = 0,
    Target_vs_2_0 = 1,
    Target_arbvp1 = 2,
    Targets_Last = 3
  };
};

struct CGxBuf {
  enum Status {
    S_VALID = 0,
    S_INVALID_DISCARD = 1,
    S_INVALID_RELOAD = 2
  };

  static const unsigned int BASE_NONE;

 protected:
  friend class CGxDevice;
  friend class CGxIndexBuffer;
  friend class CGxVertexBuffer;
  friend class CParticleEmitter2;

  TSLink<CGxBuf>        linkGx;
  TSLink<CGxBuf>        linkVB;
  TSLink<CGxBuf>        linkIB;
  EGxBufWriteFreq       m_writeFreq;
  EGxVertexBufferFormat m_vbFormat;
  unsigned int          m_numVertices;
  unsigned int          m_numIndices;
  void(__fastcall *m_userCallback)(CGxBufCommand &, CGxBuf *);
  void        *m_userArg;
  unsigned int m_vertexBase;
  unsigned int m_indexBase;
  Status       m_vertexStatus;
  Status       m_indexStatus;

  CGxBuf(const CGxBuf &);
  const CGxBuf &operator=(const CGxBuf &);

  unsigned int writeFrameTag;

 public:
  CGxBuf();
  void Invalidate(Status vertexStatus, Status indexStatus);

  unsigned int VertexCount() {
    return m_numVertices;
  }

  unsigned int IndexCount() {
    return m_numIndices;
  }

  void CountSet(unsigned int numVertices, unsigned int numIndices);

  void *UserArg() {
    return m_userArg;
  }

  void UserArgSet(void *userArg) {
    m_userArg = userArg;
  }

  void(__fastcall *UserCallback())(CGxBufCommand &, CGxBuf *) {
    return m_userCallback;
  }

  void UserCallbackSet(void(__fastcall *userCallback)(CGxBufCommand &, CGxBuf *)) {
    m_userCallback = userCallback;
  }

  ~CGxBuf() {
  }
};

class CGxMemBuffer {
  friend class CGxDeviceD3d;
  friend class CGxBufD3d;
  friend class CGxBufOgl;
  friend class CVertexBufferList;

 public:
  CGxMemBuffer(unsigned int count);
  virtual ~CGxMemBuffer();

  virtual void Lock(void *&mem, unsigned int count, unsigned int base) = 0;
  virtual void Unlock() = 0;

  void AddBuf(CGxBuf *buf);
  void Discard();
  void RemoveBuf(CGxBuf *buf);

  unsigned int GetBase() {
    return m_base;
  }

  unsigned int GetCount() {
    return m_count;
  }

  unsigned int GetNext() {
    return m_next;
  }

  int GetDiscard() {
    return m_discard;
  }

 protected:
  void InvalidateBufs(CGxBuf::Status vertexStatus, CGxBuf::Status indexStatus);

  unsigned int                       m_count;
  unsigned int                       m_base;
  unsigned int                       m_next;
  int                                m_discard;
  TSExplicitList<CGxBuf, -572662307> m_bufList;
};

class CGxVertexBuffer : public CGxMemBuffer {
 public:
  CGxVertexBuffer(unsigned int count) : CGxMemBuffer(count) {
    m_bufList.ChangeLinkOffset(offsetof(CGxBuf, linkVB));
  }
};

class CGxIndexBuffer : public CGxMemBuffer {
 public:
  CGxIndexBuffer(unsigned int count) : CGxMemBuffer(count) {
    m_bufList.ChangeLinkOffset(offsetof(CGxBuf, linkIB));
  }
};

class CGxTex {
 private:
  void Init(
      EGxTexTarget target,
      unsigned int width,
      unsigned int height,
      unsigned int depth,
      EGxTexFormat format,
      EGxTexFormat dataFormat,
      CGxTexFlags  flags,
      void        *userArg,
      void(__fastcall *userFunc)(EGxTexCommand, unsigned int, unsigned int, unsigned int, unsigned int, void *, unsigned int &, const void *&)
  );

 public:
  CGxTex(
      EGxTexTarget target,
      unsigned int width,
      unsigned int height,
      unsigned int depth,
      EGxTexFormat format,
      EGxTexFormat dataFormat,
      CGxTexFlags  flags,
      void        *userArg,
      void(__fastcall *userFunc)(EGxTexCommand, unsigned int, unsigned int, unsigned int, unsigned int, void *, unsigned int &, const void *&)
  );
  CGxTex(
      unsigned int width,
      unsigned int height,
      EGxTexFormat format,
      CGxTexFlags  flags,
      void        *userArg,
      void(__fastcall *userFunc)(EGxTexCommand, unsigned int, unsigned int, unsigned int, unsigned int, void *, unsigned int &, const void *&)
  );
  ~CGxTex() {
  }

  unsigned char    m_needsUpdate;
  unsigned char    m_needsCreation;
  unsigned char    m_needsFlagUpdate;
  NTempest::CiRect m_updateRect;
  unsigned char    m_updateFaces[6];
  short            m_updatePlaneMin;
  short            m_updatePlaneMax;
  unsigned int     m_frameTag;
  unsigned int     m_width;
  unsigned int     m_height;
  unsigned int     m_depth;
  EGxTexTarget     m_target;
  EGxTexFormat     m_format;
  EGxTexFormat     m_dataFormat;
  CGxTexFlags      m_flags;
  void            *m_userArg;
  void(__fastcall *m_userFunc)(EGxTexCommand, unsigned int, unsigned int, unsigned int, unsigned int, void *, unsigned int &, const void *&);
  void *m_apiSpecificData;
};

class CGxLight {
 public:
  CGxLight();
  ~CGxLight() {
  }
  CGxLight &operator=(const CGxLight &__that) {
    m_enabled = __that.m_enabled;
    m_isOmni = __that.m_isOmni;
    m_dir = __that.m_dir;
    m_ambColor = __that.m_ambColor;
    m_dirColor = __that.m_dirColor;
    m_specColor = __that.m_specColor;
    m_ambIntensity = __that.m_ambIntensity;
    m_dirIntensity = __that.m_dirIntensity;
    m_specIntensity = __that.m_specIntensity;
    m_constantAttenuation = __that.m_constantAttenuation;
    m_linearAttenuation = __that.m_linearAttenuation;
    m_quadraticAttenuation = __that.m_quadraticAttenuation;
    m_attenStart = __that.m_attenStart;
    m_attenEnd = __that.m_attenEnd;
    return *this;
  }

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
  const NTempest::C44Matrix &TopConst() {
    return m_mtx[m_level];
  }
  const NTempest::C44Matrix &Get() const;

  unsigned int        m_level;
  unsigned char       m_dirty;
  NTempest::C44Matrix m_mtx[4];
  unsigned int        m_flags[4];
};

struct CGxStateRegister {
  CGxStateRegister();

  CGxLight     m_lights[8];
  int          m_lightsDirty[8];
  float        m_lightLinearFalloff;
  float        m_lightQuadraticFalloff;
  unsigned int m_masterEnables;
};

class CGxDevice {
 public:
  struct TextureTarget {
    CGxTex      *m_texture;
    unsigned int m_plane;
    void        *m_apiSpecific;
  };

 public:
  static const unsigned int s_texFormatBitDepth[GxTexFormats_Last];

 protected:
  unsigned int ITexComputeByteSize(const CGxTex *texId, unsigned int width, unsigned int height);
  int          EnableState(unsigned long app, unsigned long appDisables, unsigned int flagPos);
  int  NeedsUpdate(unsigned long app, unsigned long hw, unsigned long appDisables, unsigned long hwDisables, unsigned int flagPos, int &enable);
  void ITexBind(CGxTex *texId);
  virtual void           ITexMarkAsUpdated(CGxTex *texId);
  virtual void           IRsSendToHw(EGxRenderState which) = 0;
  virtual void           ISetShaderParamList(TSExplicitList<CGxShaderParam, 108> &params, int forceForBind) = 0;
  void                   ISetShaderParameters(CGxShader *sh, int forceForBind);
  unsigned int           IMatAlphaRef(EGxBlend op);
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

 public:
  CGxDevice();
  virtual ~CGxDevice();
  virtual int           DeviceCreate(unsigned int hwnd, const CGxFormat &format);
  virtual int           DeviceCreate(GXWINDOWPROC windowProc, const CGxFormat &format);
  virtual void          DeviceDestroy();
  virtual int           DeviceSetFormat(const CGxFormat &format);
  virtual void          DeviceSetBaseMipLevel(unsigned int baseMipLevel);
  virtual void          DeviceSetGamma(const CGxGammaRamp &ramp);
  virtual void          DeviceSetGamma(float gamma);
  virtual void          DeviceSetTextureQuality(int force32);
  virtual unsigned long DeviceWindow() = 0;
  virtual void          DeviceTakeScreenShot();
  virtual void          DeviceReadScreenShot(unsigned int &w, unsigned int &h, const NTempest::CImVector *&pixels);
  virtual void          DeviceReadPixels(NTempest::CiRect &rect, TSGrowableArray<NTempest::CImVector> &pixels) = 0;
  virtual void          DeviceReadDepths(NTempest::CiRect &rect, TSGrowableArray<float> &depths) = 0;
  virtual void          DeviceWM(EGxWM wm, long param1, long param2) = 0;
  virtual void          DeviceSetRenderTarget(EGxBuffer buffer, CGxTex *texture, unsigned int plane);
  virtual void          DeviceOverride(EGxOverride override, unsigned long value);
  virtual void          CapsWindowSize(NTempest::CRect &dst) = 0;
  virtual void          CapsWindowSizeInScreenCoords(NTempest::CRect &dst) = 0;
  virtual int           CapsIsWindowVisible() = 0;
  virtual void          SceneSetClearColor(NTempest::CImVector clearColor);
  NTempest::CImVector   SceneClearColor();
  virtual void          ScenePresent(unsigned int mask);
  virtual void          SceneClear(unsigned int mask);
  virtual void          XformSetViewport(float minX, float maxX, float minY, float maxY, float minZ, float maxZ);
  virtual void          XformSetProjection(const NTempest::C44Matrix &matrix);
  virtual void          XformSetView(const NTempest::C44Matrix &matrix);
  virtual void          XformSetBones(unsigned int numBones, const NTempest::C34Matrix *matrices);
  virtual void          VertexShaderSelect(EGxVertexShader shader);
  virtual void          PrimLockAndProcessVertexPtrs(
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
  virtual void PrimLockIndexPtr(EGxPrim primType, unsigned int indexCount, const unsigned short *indices);
  virtual void PrimDrawElements();
  virtual void PrimUnlockIndexPtr();
  virtual void PrimUnlockVertexPtrs();
  virtual void PrimBegin(EGxPrim primType);
  virtual void PrimEnd();
  virtual void PrimVertex(const NTempest::C3Vector &v);
  virtual void PrimTexCoord(unsigned int tmu, const NTempest::C2Vector &t);
  virtual void PrimNormal(const NTempest::C3Vector &n);
  virtual void PrimColor(const NTempest::CImVector &c);
  virtual void PrimPointSize(float s) {
  }
  virtual void PrimLineWidth(float w) {
  }
  unsigned int    PrimCalcCount(EGxPrim primType, unsigned int indexCount);
  virtual void    LightSet(unsigned int whichLight, const CGxLight &lightInfo, const NTempest::C3Vector &cameraPos);
  virtual void    LightEnable(unsigned int whichLight, int enable);
  virtual void    MasterEnableSet(EGxMasterEnables state, int enable);
  virtual CGxBuf *BufCreate(
      EGxBufWriteFreq       writeFreq,
      EGxVertexBufferFormat format,
      unsigned int          numVertices,
      unsigned int          numIndices,
      void(__fastcall *userCallback)(CGxBufCommand &, CGxBuf *),
      void *userArg
  );
  virtual void BufLock(CGxBuf *buf);
  virtual void BufRender(const CGxBatch *batches, unsigned int count);
  virtual void BufUnlock();
  virtual void BufDestroy(CGxBuf *&buf);
  virtual void BufReserve(EGxBufWriteFreq freq, EGxVertexBufferFormat format, unsigned int numVertices, unsigned int numIndices);
  CGxBuf      *BufGetDynamic(EGxVertexBufferFormat format);
  virtual int TexCreate(
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
  );
  virtual int TexCreate(
      unsigned int width,
      unsigned int height,
      EGxTexFormat format,
      CGxTexFlags  flags,
      void        *userArg,
      void(__fastcall *userFunc)(EGxTexCommand, unsigned int, unsigned int, unsigned int, unsigned int, void *, unsigned int &, const void *&),
      CGxTex *&texId
  );
  virtual void TexDestroy(CGxTex *texId);
  void         TexMarkForUpdate(CGxTex *texId, const NTempest::CiRect &updateRect, int immediate);
  int          TexNeedsUpdate(CGxTex *texId);
  void         TexSetFlags(CGxTex *texId, CGxTexFlags flags);
  void         TexSetDataFormat(CGxTex *texId, EGxTexFormat dataFormat);
  void         TexSetUserData(
      CGxTex *texId,
      void(__fastcall *userFunc)(EGxTexCommand, unsigned int, unsigned int, unsigned int, unsigned int, void *, unsigned int &, const void *&),
      void *userArg
  );
  void        *TexUserArg(CGxTex *texId);
  void         TexGetDimensions(const CGxTex *texId, unsigned int *width, unsigned int *height);
  void         TexParameters(const CGxTex *texId, CGxTexParms &parms);
  void         TexParameters(const CGxTex *texId, CGxTexParmsEx &parms);
  void         TexFlags(const CGxTex *texId, CGxTexFlags &flags);
  virtual void PixelShaderCreate(CGxPixelShader *&ps, const char *filename);
  virtual void PixelShaderDestroy(CGxPixelShader *&ps);
  virtual void VertexShaderCreate(CGxVertexShader *&vs, const char *filename);
  virtual void VertexShaderDestroy(CGxVertexShader *&vs);

  static CGxDevice *__fastcall NewD3d();
  static CGxDevice *__fastcall NewOpenGl();
  static int __fastcall
  AdapterID(unsigned short &vendorID, unsigned short &deviceID, unsigned long &driverVersionHi, unsigned long &driverVersionLow);
  static int __fastcall AdapterInfer(unsigned short &deviceID);
  static int __fastcall AdapterMonitorModes(TSGrowableArray<CGxMonitorMode> &modes);
  static int __fastcall AdapterDesktopMode(CGxMonitorMode &mode);
  static int __fastcall D3dEnumFormats(TSGrowableArray<CGxFormat> &formats);
  static int __fastcall OpenGlEnumFormats(TSGrowableArray<CGxFormat> &formats);

  const CGxFormat          &DeviceFormat();
  unsigned int              DeviceBaseMipLevel();
  void                      DeviceGamma(CGxGammaRamp &ramp);
  void                      DeviceSystemGamma(CGxGammaRamp &ramp);
  int                       DeviceTextureQuality();
  EGxApi                    DeviceApi();
  const CGxCaps            &Caps() const;
  void                      DeviceClearScreenShot();
  unsigned int              PerfCounter(EGxPerfCounter counter);
  void                      PerfAcc(EGxPerfCounter counter, unsigned int value) {
    m_perfCountersAcc[counter] += value;
  }
  static float __fastcall   CpuFrequency();
  static __int64 __fastcall CpuTicks();
  void                      XformProjection(NTempest::C44Matrix &matrix);
  void                      XformView(NTempest::C44Matrix &matrix);
  void                      XformBone(unsigned int ndx, NTempest::C34Matrix &matrix);
  void                      Xform(EGxXform xf, NTempest::C44Matrix &matrix);
  void                      XformPush(EGxXform xf);
  void                      XformPush(EGxXform xf, const NTempest::C44Matrix &matrix);
  void                      XformPop(EGxXform xf);
  void                      XformIdentity(EGxXform xf);
  void                      XformSet(EGxXform xf, const NTempest::C44Matrix &matrix);
  void                      XformTranslate(EGxXform xf, const NTempest::C3Vector &t);
  void                      XformScale(EGxXform xf, const NTempest::C3Vector &s);
  void                      XformMult(EGxXform xf, const NTempest::C44Matrix &m);
  void                      XformViewport(float &minX, float &maxX, float &minY, float &maxY, float &minZ, float &maxZ);
  void                      RsSet(EGxRenderState which, int value);
  void                      RsSet(EGxRenderState which, float value);
  void                      RsSet(EGxRenderState which, NTempest::CImVector value);
  void                      RsSet(EGxRenderState which, const NTempest::C3Vector &value);
  void                      RsSet(EGxRenderState which, void *value);
  void                      RsGet(EGxRenderState which, int &value);
  void                      RsGet(EGxRenderState which, float &value);
  void                      RsGet(EGxRenderState which, NTempest::CImVector &value);
  void                      RsGet(EGxRenderState which, NTempest::C3Vector &value);
  void                      RsGet(EGxRenderState which, void *&value);
  void                      RsPush();
  void                      RsPop();
  void                      RsInit();
  unsigned int              RsStackOffset();
  int                       MasterEnable(EGxMasterEnables state);
  void                      Light(unsigned int whichLight, CGxLight &lightInfo);

  static void __fastcall LogOpen();
  static void __fastcall LogClose();
  static void __cdecl    Log(const char *format, ...);
  static void __cdecl    DbgPrintf(const char *format, ...);

 private:
  friend class CGxDeviceD3d;
  friend class CGxDeviceOpenGl;

  CGxDevice(const CGxDevice &);
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
  TSGrowableArray<unsigned long>        mStackOffsets;
  TSGrowableArray<EGxRenderState>       mDirtyStates;
  unsigned int                          m_perfCountersLatched[13];
  unsigned int                          m_perfCountersAcc[13];
  EGxPrim                               m_primType;
  unsigned int                          m_primIndexCount;
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
  TSGrowableArray<unsigned short>       m_primIndexArray;
  unsigned int                          m_primMask;
  NTempest::CRect                       m_defWindowRect;
  NTempest::CRect                       m_curWindowRect;

 protected:
  void Log(const CGxCaps &caps) const;
  void Log(const CGxFormat &format) const;

  int                                                m_context;
  EGxApi                                             m_api;
  unsigned long                                      m_cpuFeatures;
  CGxFormat                                          m_format;
  CGxCaps                                            m_caps;
  unsigned int                                       m_baseMipLevel;
  int                                                m_force32BitTextures;
  NTempest::CImVector                                m_clearColor;
  CGxGammaRamp                                       m_gammaRamp;
  CGxGammaRamp                                       m_systemGammaRamp;
  GXWINDOWPROC                                       m_windowProc;
  float                                              m_viewport[6];
  NTempest::C44Matrix                                m_projection;
  const NTempest::C34Matrix                         *m_bones;
  unsigned int                                       m_boneCount;
  CGxMatrixStack                                     m_xforms[7];
  CGxMatrixStack                                     m_texGen[4];
  EGxVertexShader                                    m_vertexShader;
  EGxVertexBufferFormat                              m_vertexBufferFormat;
  CGxPixelShader::Target                             m_pixelShaderPlatform;
  TSHashTableReuse<CGxPixelShader, HASHKEY_STRI, 1>  m_pixelShaderList;
  TSHashTableReuse<CGxVertexShader, HASHKEY_STRI, 1> m_vertexShaderList;
  CGxStateRegister                                   m_appState;
  CGxStateRegister                                   m_hwState;
  TSExplicitList<CGxBuf, 0>                          m_bufList;
  CGxBuf                                            *m_bufLocked;
  unsigned int                                       m_VBReserve[4][9];
  unsigned int                                       m_IBReserve[4][9];
  CGxBuf                                            *m_dynBuf[9];
  TSFixedArray<CGxAppRenderState>                    mAppRenderStates;
  TSFixedArray<CGxStateBom>                          mHwRenderStates;
  TSGrowableArray<CGxTex *>                          m_textures;
  TextureTarget                                      m_textureTarget[2];
  int                                                m_scrShotClick;
  unsigned int                                       m_scrShotWidth;
  unsigned int                                       m_scrShotHeight;
  TSGrowableArray<NTempest::CImVector>               m_scrShotPixels;

  static HSLOG m_log;
};
