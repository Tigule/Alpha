#pragma once

#include "../CGxDevice.h"

#include <windows.h>

struct HPBUFFERARB__;
class CGxBufOgl;

class CGxMemBuffer_VAR : public CGxMemBuffer {
 public:
  CGxMemBuffer_VAR(UINT count, LPVOID mem);
  virtual ~CGxMemBuffer_VAR();
  virtual void Lock(LPVOID &mem, UINT bytes, UINT base);
  virtual void Unlock() {
  }

 private:
  CGxMemBuffer_VAR(const CGxMemBuffer_VAR &);

  void Fence();

  LPVOID m_mem;
  UINT   m_fence;
};

class CGxDeviceOpenGl : public CGxDevice {
 public:
  template <class T>
  struct PixelFormatAttribute {
    PixelFormatAttribute() {
    }

    PixelFormatAttribute(int attribute, T value) : attribute(attribute), value(value) {
    }

    int attribute;
    T   value;
  };

  typedef PixelFormatAttribute<int>   PixelFormatAttributei;
  typedef PixelFormatAttribute<float> PixelFormatAttributef;

  enum EDeviceState {
    Ds_DepthMask = 0,
    Ds_ActiveTexture = 1,
    Ds_TexTarget0 = 2,
    Ds_TexTarget1 = 3,
    Ds_TexTarget2 = 4,
    Ds_TexTarget3 = 5,
    Ds_TexGenS0 = 6,
    Ds_TexGenS1 = 7,
    Ds_TexGenS2 = 8,
    Ds_TexGenS3 = 9,
    Ds_TexGenT0 = 10,
    Ds_TexGenT1 = 11,
    Ds_TexGenT2 = 12,
    Ds_TexGenT3 = 13,
    Ds_TexGenR0 = 14,
    Ds_TexGenR1 = 15,
    Ds_TexGenR2 = 16,
    Ds_TexGenR3 = 17,
    Ds_TexGenQ0 = 18,
    Ds_TexGenQ1 = 19,
    Ds_TexGenQ2 = 20,
    Ds_TexGenQ3 = 21,
    Ds_TexEnvMode0 = 22,
    Ds_TexEnvMode1 = 23,
    Ds_TexEnvMode2 = 24,
    Ds_TexEnvMode3 = 25,
    Ds_NormalArray = 26,
    Ds_ColorArray = 27,
    Ds_TextureArray0 = 28,
    Ds_TextureArray1 = 29,
    Ds_TextureArray2 = 30,
    Ds_TextureArray3 = 31,
    Ds_NVVAR = 32,
    Ds_PolygonOffsetEnable = 33,
    Ds_PolygonOffset = 34,
    Ds_BlendEnable = 35,
    Ds_AlphaTestEnable = 36,
    Ds_RegisterCombinersNV = 37,
    Ds_PerStageConstantsNV = 38,
    Ds_TextureShaderNV = 39,
    Ds_FragmentProgramARB = 40,
    Ds_MatrixMode = 41,
    Ds_BlendFunc = 42,
    DeviceStates_Last = 43
  };

  enum EColorSource {
    Cs_Material = 0,
    Cs_Constant = 1,
    Cs_Array = 2,
    ColorSources_Last = 3
  };

  struct ColorSourceColor {
    NTempest::CImVector m_color;
    int                 m_dirty;

    ColorSourceColor() : m_color(0xFFFFFFFF), m_dirty(0) {
    }
  };

  static long CALLBACK WindowProcGl(HWND window, UINT message, UINT wparam, long lparam);
  void                 DeviceCreatePbuffer();
  void                 DeviceQueryPbuffer();
  void                 DeviceDestroyPbuffer();
  virtual ~CGxDeviceOpenGl();
  virtual BOOL  DeviceCreate(GXWINDOWPROC windowProc, const CGxFormat &format);
  virtual BOOL  DeviceCreate(UINT clienthwnd, const CGxFormat &format);
  virtual void  DeviceDestroy();
  virtual BOOL  DeviceSetFormat(const CGxFormat &format);
  virtual void  DeviceSetBaseMipLevel(UINT baseMipLevel);
  virtual void  DeviceSetGamma(float gamma);
  virtual void  DeviceSetGamma(const CGxGammaRamp &ramp);
  virtual void  DeviceSetTextureQuality(int force32);
  virtual DWORD DeviceWindow();
  virtual void  DeviceReadPixels(NTempest::CiRect &rect, TSGrowableArray<NTempest::CImVector> &pixels);
  virtual void  DeviceReadDepths(NTempest::CiRect &rect, TSGrowableArray<float> &depths);
  virtual void  DeviceWM(EGxWM wm, long param1, long param2);
  virtual void  DeviceSetRenderTarget(EGxBuffer buffer, CGxTex *gxTex, UINT plane);
  virtual void  DeviceOverride(EGxOverride override, DWORD value);
  virtual void  CapsWindowSize(NTempest::CRect &dst);
  virtual void  CapsWindowSizeInScreenCoords(NTempest::CRect &dst);
  virtual int   CapsIsWindowVisible();
  virtual void  ScenePresent(UINT mask);
  virtual void  SceneClear(UINT mask);
  virtual void  XformSetViewport(float minX, float maxX, float minY, float maxY, float minZ, float maxZ);
  virtual void  XformSetProjection(const NTempest::C44Matrix &matrix);
  virtual void  XformSetView(const NTempest::C44Matrix &matrix);
  virtual void  PrimLockAndProcessVertexPtrs(
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
  virtual void    PrimLockIndexPtr(EGxPrim primType, UINT indexCount, const WORD *indices);
  virtual void    PrimDrawElements();
  virtual void    PrimUnlockIndexPtr();
  virtual void    PrimUnlockVertexPtrs();
  virtual void    PrimPointSize(float s);
  virtual void    PrimLineWidth(float w);
  virtual void    IRsSendToHw(EGxRenderState which);
  virtual CGxBuf *BufCreate(
      EGxBufWriteFreq       writeFreq,
      EGxVertexBufferFormat format,
      UINT                  numVertices,
      UINT                  numIndices,
      void (*userCallback)(CGxBufCommand &, CGxBuf *),
      LPVOID userArg
  );
  virtual void BufLock(CGxBuf *b);
  virtual void BufRender(const CGxBatch *batches, UINT count);
  virtual void BufUnlock();
  virtual void BufDestroy(CGxBuf *&b);
  virtual void BufReserve(EGxBufWriteFreq freq, EGxVertexBufferFormat format, UINT numVertices, UINT numIndices);
  virtual BOOL TexCreate(
      UINT         width,
      UINT         height,
      EGxTexFormat format,
      CGxTexFlags  flags,
      LPVOID       userArg,
      void (*userFunc)(EGxTexCommand, UINT, UINT, UINT, UINT, LPVOID, UINT &, LPCVOID &),
      CGxTex *&texId
  );
  virtual void TexDestroy(CGxTex *texId);
  virtual void PixelShaderCreate(CGxPixelShader *&ps, LPCSTR filename);
  virtual void PixelShaderDestroy(CGxPixelShader *&ps);

 private:
  friend class CGxDevice;

  static const UINT kNullTmu;

  CGxDeviceOpenGl();
  CGxDeviceOpenGl(const CGxDeviceOpenGl &);
  const CGxDeviceOpenGl &operator=(const CGxDeviceOpenGl &);

  void DsInit();
  UINT DsGet(EDeviceState state) {
    ASSERT(state < DeviceStates_Last);
    return m_deviceState[state];
  }
  void DsSet(EDeviceState which, UINT newVal, int force);
  void AllocBuffers();
  void AllocIndexBuffer(EGxBufWriteFreq freq, UINT bytes);
  void AllocVertexBuffer(EGxBufWriteFreq freq, UINT bytes);
  void FreeBuffers();
  void FreeIndexBuffer(CGxMemBuffer *&b);
  void FreeVertexBuffer(CGxMemBuffer *&b);
  void IAllocBuffers();
  void IAllocVAR();
  void IAllocVertexBufferVAR(EGxBufWriteFreq freq, UINT bytes);
  void IFreeVAR();
  void ITexForceRecreation();
  void BindTexture(CGxTex *texId, UINT tmu);
  void GetError();
  BOOL IDevAttachGlContext(const CGxFormat &format);
  void IDevRemoveGlContext();
  void IDevSetFocus(int focus, const CGxFormat &format);
  void ISceneBegin(UINT mask);
  void ISetGlCaps();
  void IShaderForceRecreation();
  void IStateSetContextDefaults();
  void IStateSetColorSource(EColorSource source);
  void IStateSetColorSourceColor(EColorSource source, const NTempest::CImVector &color);
  void IStateSync();
  void IStateSyncColorSource();
  void IStateSyncEnables();
  void IStateSyncLights();
  void IStateSyncTexTransform(UINT tmu);
  void IStateSyncTexTransforms();
  void IPrimSetupColor(UINT stride, LPCVOID colors, UINT count, int convert);
  void IPrimSetupNormal(UINT stride, LPCVOID normals);
  void IPrimSetupPos();
  void IPrimSetupTexCoord(UINT coord, int enable);
  void IPrimSetupTexCoord(UINT tmu, UINT stride, LPCVOID texCoord);
  void ISetTexBlend(UINT tmu, EGxTexBlend blend);
  void ISetTexGen(UINT tmu, EGxTexGen texGen);
  void ISetTexLodBias(UINT tmu, float bias);
  void ISetTexture(UINT tmu, CGxTex *tex);
  void IXformGLModelView(const NTempest::C44Matrix &gxm, NTempest::C44Matrix &oglm);
  void IXformSetModelView(const NTempest::C44Matrix &m);
  void IXformSetProjection(const NTempest::C44Matrix &m);
  void IXformSet(EGxXform xform);
  void LockArrays(UINT count);
  BOOL SetFormatMode(const CGxFormat &format);
  void UnlockArrays();

  virtual void ISetShaderParamList(TSExplicitList<CGxShaderParam, 108> &params, int forceForBind);

  UINT             m_deviceState[43];
  UINT             m_lockedArrays;
  EColorSource     m_colorSource;
  BOOL             m_colorSourceDirty;
  ColorSourceColor m_colorSourceColor[3];
  LPVOID           m_nvvarMem;
  UINT             m_nvvarBytes;
  UINT             m_nvvarNext;
  BOOL             m_bufRealloc;

 protected:
  void IBufSetBuffers(CGxBufOgl *buf);

  static UINT s_convertMinFilterToOgl[5];
  static UINT s_convertMagFilterToOgl[5];
  static int  s_convertTexFmt[8];
  static UINT s_dataFormatSize[8];
  static int  s_convertDataFmt[8];
  static int  s_convertDataType[8];

  void         IPixelShaderBind(CGxPixelShader *ps);
  void         ITexDownload(CGxTex *texId, UINT w, UINT h, UINT startLevel, UINT oglBase, UINT texelStrideInBytes, LPCVOID texels);
  virtual void ITexMarkAsUpdated(CGxTex *texId);
  void         ITexMarkAsUpdated(CGxTex *texId, UINT tmu);
  void         ITexSetFlags(CGxTex *texId);

  HWND                              m_hwnd;
  int                               m_ownhwnd;
  WORD                              m_hwndClass;
  HDC                               m_hdc;
  HGLRC                             m_hglrc;
  HPBUFFERARB__                    *m_hPbuffer;
  HDC                               m_hPbufferDC;
  HGLRC                             m_hPbufferRC;
  TSFixedArray<NTempest::C3Vector>  m_primPos;
  TSFixedArray<NTempest::C3Vector>  m_primNormal;
  TSFixedArray<NTempest::CImVector> m_primColor;
  TSFixedArray<NTempest::C2Vector>  m_primT0;
  TSFixedArray<NTempest::C2Vector>  m_primT1;
  CGxMemBuffer                     *m_vertexBuffer[4];
  CGxMemBuffer                     *m_indexBuffer[4];
  EGxPrim                           m_primType;
  UINT                              m_primIndexCount;
  const WORD                       *m_primIndices;
  BOOL                              m_worldViewChange;
};
