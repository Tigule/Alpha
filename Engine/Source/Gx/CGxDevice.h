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

class CGxMemBuffer {
  friend class CGxDeviceD3d;
  friend class CGxBufD3d;
  friend class CGxBufOgl;
  friend class CVertexBufferList;

 protected:
  UINT m_count;
  UINT m_base;
  UINT m_next;
  BOOL m_discard;
  LISTEXDYN(CGxBuf) m_bufList;

  void InvalidateBufs(CGxBuf::Status vertexStatus, CGxBuf::Status indexStatus);

 public:
  CGxMemBuffer(UINT count);
  virtual ~CGxMemBuffer();

  void AddBuf(CGxBuf *buf);
  void RemoveBuf(CGxBuf *buf);
  virtual void Lock(LPVOID &mem, UINT count, UINT base) = 0;
  virtual void Unlock() = 0;

  void Discard();

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

class CGxMatrixStack {
 private:
  friend class CGxDevice;
  friend class CGxDeviceD3d;
  friend class CGxDeviceOpenGl;

  UINT                m_level;
  BYTE                m_dirty;
  NTempest::C44Matrix m_mtx[4];
  UINT                m_flags[4];

  UINT Flags();

 public:
  enum EMatrixFlags {
    F_Identity = 1
  };

  CGxMatrixStack();
  void                       Push();
  void                       Pop();
  void                       Identity();
  NTempest::C44Matrix       &Top();
  const NTempest::C44Matrix &TopConst() const {
    return m_mtx[m_level];
  }
  ~CGxMatrixStack();
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
 private:
  friend HTEXTURE__ *TextureAllocImage(EGxTexFormat format, UINT width, UINT height);
  friend class CGxDeviceD3d;
  friend class CGxDeviceOpenGl;

  TSGrowableArray<CGxPushedRenderState> mPushedStates;
  TSGrowableArray<DWORD>                mStackOffsets;
  TSGrowableArray<enum EGxRenderState>  mDirtyStates;
  UINT                                  m_perfCountersLatched[13];
  UINT                                  m_perfCountersAcc[13];
  EGxPrim                               m_primType;
  UINT                                  m_primIndexCount;
  BOOL                                  m_indexLocked;
  BOOL                                  m_vertexLocked;
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

 public:
  enum {
    PrimMask_Vertex = 1,
    PrimMask_TexCoord = 2,
    PrimMask_Normal = 32,
    PrimMask_Color = 64
  };

 private:
  UINT                                  m_primMask;

  CGxDevice(const CGxDevice &);
  const CGxDevice &operator=(const CGxDevice &);
  void             IRsInit();
  void             IRsSet(EGxRenderState which, const CGxStateBom &value);
  void             PerfCountersLatch();

  NTempest::CRect                       m_defWindowRect;
  NTempest::CRect                       m_curWindowRect;

 protected:
  BOOL                                               m_context;

 public:
  enum {
    MinD3dBufVertices = 256,
    MinD3dBufIndices = 768
  };

 protected:
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

  BOOL EnableState(DWORD app, DWORD appDisables, UINT flagPos);
  BOOL NeedsUpdate(DWORD app, DWORD hw, DWORD appDisables, DWORD hwDisables, UINT flagPos, int &enable);

  LISTDECLEX(CGxBuf, linkGx, m_bufList);
  CGxBuf                              *m_bufLocked;
  UINT                                 m_VBReserve[4][9];
  UINT                                 m_IBReserve[4][9];
  CGxBuf                              *m_dynBuf[9];
  TSFixedArray<CGxAppRenderState>      mAppRenderStates;
  TSFixedArray<CGxStateBom>            mHwRenderStates;
  TSGrowableArray<CGxTex *>            m_textures;

  static const UINT s_texFormatBitDepth[];

 public:
  struct TextureTarget {
    CGxTex *m_texture;
    UINT    m_plane;
    LPVOID  m_apiSpecific;
  };

 protected:
  TextureTarget                        m_textureTarget[2];
  int                                  m_scrShotClick;
  UINT                                 m_scrShotWidth;
  UINT                                 m_scrShotHeight;
  TSGrowableArray<NTempest::CImVector> m_scrShotPixels;

  virtual void          ITexMarkAsUpdated(CGxTex *texId);
  UINT                  ITexComputeByteSize(const CGxTex *texId, const UINT width, const UINT height);
  void                  ITexBind(CGxTex *texId);
  UINT                  IMatAlphaRef(EGxBlend op);
  int                   IDevIsWindowed();
  BOOL                  IVbHasColor(EGxVertexBufferFormat format);
  EGxVertexBufferFormat IGiveVbColor(EGxVertexBufferFormat format);
  virtual void          IRsSendToHw(EGxRenderState which) = 0;
  void                  IRsForceUpdate(EGxRenderState ndx_);
  void                  IRsForceUpdate();
  void                  IRsSync(int force);
  void                  DeviceScreenShot();
  void                  ClampRectToWindow(NTempest::CiRect &rect);
  void                  ISetShaderParameters(CGxShader *sh, int forceForBind);
  virtual void          ISetShaderParamList(CGxShader::ParamList &params, int forceForBind) = 0;
  void                  Log(const CGxCaps &caps) const;
  void                  Log(const CGxFormat &format) const;
  void                  PerfAcc(EGxPerfCounter counter, UINT value) {
    m_perfCountersAcc[counter] += value;
  }
  const NTempest::CRect &DeviceCurWindow();
  const NTempest::CRect &DeviceDefWindow();
  void                   DeviceSetDefWindow(const NTempest::CRect &rect);
  void                   DeviceSetCurWindow(const NTempest::CRect &rect);
  void                   CreateDynamicBufs();
  void                   DestroyDynamicBufs();

  static HSLOG m_log;

 public:
  CGxDevice();
  virtual ~CGxDevice();

  static BOOL       OpenGlEnumFormats(TSGrowableArray<CGxFormat> &formats);
  static BOOL       D3dEnumFormats(TSGrowableArray<CGxFormat> &formats);
  static CGxDevice *NewOpenGl();
  static CGxDevice *NewD3d();
  static void       LogOpen();
  static void       LogClose();

  virtual BOOL DeviceCreate(GXWINDOWPROC windowProc, const CGxFormat &format);
  virtual BOOL DeviceCreate(UINT hwnd, const CGxFormat &format);
  virtual void DeviceDestroy();
  virtual BOOL DeviceSetFormat(const CGxFormat &format);
  virtual void DeviceSetBaseMipLevel(UINT baseMipLevel);
  virtual void DeviceSetGamma(float gamma);
  virtual void DeviceSetGamma(const CGxGammaRamp &ramp);
  virtual void DeviceSetTextureQuality(int force32);

  const CGxFormat &DeviceFormat();
  UINT             DeviceBaseMipLevel();
  void             DeviceGamma(CGxGammaRamp &ramp);
  void             DeviceSystemGamma(CGxGammaRamp &ramp);
  int              DeviceTextureQuality();
  virtual DWORD    DeviceWindow() = 0;
  EGxApi           DeviceApi();
  virtual void     DeviceTakeScreenShot();
  virtual void     DeviceReadScreenShot(UINT &w, UINT &h, const NTempest::CImVector *&pixels);
  void             DeviceClearScreenShot();
  virtual void     DeviceReadPixels(NTempest::CiRect &rect, TSGrowableArray<NTempest::CImVector> &pixels) = 0;
  virtual void     DeviceReadDepths(NTempest::CiRect &rect, TSGrowableArray<float> &depths) = 0;
  virtual void     DeviceWM(EGxWM wm, long param1, long param2) = 0;
  virtual void     DeviceSetRenderTarget(EGxBuffer buffer, CGxTex *texture, UINT plane);
  virtual void     DeviceOverride(EGxOverride override, DWORD value);
  static BOOL      AdapterID(WORD &vendorID, WORD &deviceID, DWORD &driverVersionHi, DWORD &driverVersionLow);
  static BOOL      AdapterInfer(WORD &deviceID);
  static BOOL      AdapterMonitorModes(TSGrowableArray<CGxMonitorMode> &modes);
  static BOOL      AdapterDesktopMode(CGxMonitorMode &mode);
  const CGxCaps   &Caps() const;
  virtual void     CapsWindowSize(NTempest::CRect &dst) = 0;
  virtual void     CapsWindowSizeInScreenCoords(NTempest::CRect &dst) = 0;
  virtual int      CapsIsWindowVisible() = 0;
  virtual void     SceneSetClearColor(NTempest::CImVector clearColor);
  NTempest::CImVector SceneClearColor();
  virtual void        ScenePresent(UINT mask);
  virtual void        SceneClear(UINT mask);
  virtual void        XformSetViewport(float minX, float maxX, float minY, float maxY, float minZ, float maxZ);
  virtual void        XformSetProjection(const NTempest::C44Matrix &matrix);
  virtual void        XformSetView(const NTempest::C44Matrix &matrix);
  virtual void        XformSetBones(UINT numBones, const NTempest::C34Matrix *matrices);
  void                XformViewport(float &minX, float &maxX, float &minY, float &maxY, float &minZ, float &maxZ);
  void                XformProjection(NTempest::C44Matrix &matrix);
  void                XformView(NTempest::C44Matrix &matrix);
  void                XformBone(UINT ndx, NTempest::C34Matrix &matrix);
  void                XformPush(EGxXform xf);
  void                XformPush(EGxXform xf, const NTempest::C44Matrix &matrix);
  void                XformPop(EGxXform xf);
  void                XformIdentity(EGxXform xf);
  void                XformSet(EGxXform xf, const NTempest::C44Matrix &matrix);
  void                XformTranslate(EGxXform xf, const NTempest::C3Vector &t);
  void                XformScale(EGxXform xf, const NTempest::C3Vector &s);
  void                XformMult(EGxXform xf, const NTempest::C44Matrix &m);
  void                Xform(EGxXform xf, NTempest::C44Matrix &matrix);
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
  UINT         PrimCalcCount(EGxPrim primType, UINT indexCount);
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
  virtual void LightSet(UINT whichLight, const CGxLight &lightInfo, const NTempest::C3Vector &cameraPos);
  void         Light(UINT whichLight, CGxLight &lightInfo);
  virtual void LightEnable(UINT whichLight, int enable);
  virtual void MasterEnableSet(EGxMasterEnables state, int enable);
  BOOL         MasterEnable(EGxMasterEnables state);
  void         RsSet(EGxRenderState which, int value);
  void         RsSet(EGxRenderState which, float value);
  void         RsSet(EGxRenderState which, NTempest::CImVector value);
  void         RsSet(EGxRenderState which, const NTempest::C3Vector &value);
  void         RsSet(EGxRenderState which, LPVOID value);
  void         RsGet(EGxRenderState which, int &value);
  void         RsGet(EGxRenderState which, float &value);
  void         RsGet(EGxRenderState which, NTempest::CImVector &value);
  void         RsGet(EGxRenderState which, NTempest::C3Vector &value);
  void         RsGet(EGxRenderState which, LPVOID &value);
  void         RsPush();
  void         RsPop();
  void         RsInit();
  UINT         RsStackOffset();
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
  virtual BOOL TexCreate(
      UINT         width,
      UINT         height,
      EGxTexFormat format,
      CGxTexFlags  flags,
      LPVOID       userArg,
      void (*userFunc)(EGxTexCommand, UINT, UINT, UINT, UINT, LPVOID, UINT &, LPCVOID &),
      CGxTex *&texId
  );
  virtual BOOL TexCreate(
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
  void         TexMarkForUpdate(CGxTex *texId, const NTempest::CiRect &updateRect, int immediate);
  int          TexNeedsUpdate(CGxTex *texId);
  void         TexSetUserData(CGxTex *texId, void (*userFunc)(EGxTexCommand, UINT, UINT, UINT, UINT, LPVOID, UINT &, LPCVOID &), LPVOID userArg);
  void         TexSetFlags(CGxTex *texId, CGxTexFlags flags);
  LPVOID       TexUserArg(CGxTex *texId);
  void         TexGetDimensions(const CGxTex *texId, UINT *width, UINT *height);
  virtual void TexDestroy(CGxTex *texId);
  void         TexParameters(const CGxTex *texId, CGxTexParms &parms);
  void         TexParameters(const CGxTex *texId, CGxTexParmsEx &parms);
  void         TexFlags(const CGxTex *texId, CGxTexFlags &flags);
  void         TexSetDataFormat(CGxTex *texId, EGxTexFormat dataFormat);
  virtual void PixelShaderCreate(CGxPixelShader *&ps, LPCSTR filename);
  virtual void PixelShaderDestroy(CGxPixelShader *&ps);
  virtual void VertexShaderCreate(CGxVertexShader *&vs, LPCSTR filename);
  virtual void VertexShaderDestroy(CGxVertexShader *&vs);
  UINT         PerfCounter(EGxPerfCounter counter);
  static float CpuFrequency();
  static LONGLONG CpuTicks();
  static void __cdecl DbgPrintf(LPCSTR format, ...);
  static void __cdecl Log(LPCSTR format, ...);
};
