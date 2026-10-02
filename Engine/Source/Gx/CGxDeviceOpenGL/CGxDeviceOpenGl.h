#pragma once

#include "../CGxDevice.h"

#include <windows.h>

typedef struct HPBUFFERARB__ *HPBUFFERARB;
class CGxBufOgl;

class CGxMemBuffer_VAR : public CGxMemBuffer {
 private:
  LPVOID m_mem;
  UINT   m_fence;
  void Fence();
 public:
  virtual void Lock(LPVOID &mem, UINT bytes, UINT base);
  virtual void Unlock() {
  }
  CGxMemBuffer_VAR(UINT count, LPVOID mem);
 private:
  CGxMemBuffer_VAR(const CGxMemBuffer_VAR &);
 public:
  virtual ~CGxMemBuffer_VAR();
};


class CGxDeviceOpenGl : public CGxDevice {
 private:
  friend class CGxDevice;

  static const UINT kNullTmu;

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

  UINT m_deviceState[43];

  void DsSet(EDeviceState which, UINT newVal, int force);
  UINT DsGet(EDeviceState state) {
    ASSERT(state < DeviceStates_Last);
    return m_deviceState[state];
  }
  void DsInit();

  UINT m_lockedArrays;

  void LockArrays(UINT count);
  void UnlockArrays();

  enum EColorSource {
    Cs_Material = 0,
    Cs_Constant = 1,
    Cs_Array = 2,
    ColorSources_Last = 3
  };

  EColorSource m_colorSource;
  BOOL         m_colorSourceDirty;

  struct ColorSourceColor {
    NTempest::CImVector m_color;
    int                 m_dirty;

    ColorSourceColor() : m_color(0xFFFFFFFF), m_dirty(0) {
    }
  };

  ColorSourceColor m_colorSourceColor[3];

  void BindTexture(CGxTex *texId, UINT tmu);
  void GetError();

  CGxDeviceOpenGl(const CGxDeviceOpenGl &);
  const CGxDeviceOpenGl &operator=(const CGxDeviceOpenGl &);

  void IDevSetFocus(int focus, const CGxFormat &format);
  BOOL SetFormatMode(const CGxFormat &format);
  BOOL IDevAttachGlContext(const CGxFormat &format);
  void IDevRemoveGlContext();
  void IPrimSetupPos();
  void IPrimSetupNormal(UINT stride, LPCVOID normals);
  void IPrimSetupColor(UINT stride, LPCVOID colors, UINT count, int convert);
  void IPrimSetupTexCoord(UINT tmu, UINT stride, LPCVOID texCoord);
  void IPrimSetupTexCoord(UINT coord, int enable);
  void IStateSync();
  void IStateSyncLights();
  void IStateSyncEnables();
  void IStateSyncTexTransforms();
  void IStateSyncTexTransform(UINT tmu);
  void IStateSetContextDefaults();
  void IStateSetColorSource(EColorSource source);
  void IStateSetColorSourceColor(EColorSource source, const NTempest::CImVector &color);
  void IStateSyncColorSource();
  void ISetGlCaps();
  void IXformSet(EGxXform xform);
  void IXformSetProjection(const NTempest::C44Matrix &m);
  void IXformGLModelView(const NTempest::C44Matrix &gxm, NTempest::C44Matrix &oglm);
  void IXformSetModelView(const NTempest::C44Matrix &m);
  void ITexForceRecreation();
  void IShaderForceRecreation();
  void ISetTexture(UINT tmu, CGxTex *tex);
  void ISetTexLodBias(UINT tmu, float bias);
  void ISetTexBlend(UINT tmu, EGxTexBlend blend);
  void ISetTexGen(UINT tmu, EGxTexGen texGen);
  void ISceneBegin(UINT mask);
  void AllocBuffers();
  void IAllocBuffers();
  void FreeBuffers();
  void AllocVertexBuffer(EGxBufWriteFreq freq, UINT bytes);
  void IAllocVertexBufferVAR(EGxBufWriteFreq freq, UINT bytes);
  void FreeVertexBuffer(CGxMemBuffer *&b);
  void AllocIndexBuffer(EGxBufWriteFreq freq, UINT bytes);
  void FreeIndexBuffer(CGxMemBuffer *&b);
  void IAllocVAR();
  void IFreeVAR();

  LPVOID m_nvvarMem;
  UINT   m_nvvarBytes;
  UINT   m_nvvarNext;
  BOOL   m_bufRealloc;

  virtual void ISetShaderParamList(LISTEX(CGxShaderParam, lameAssLink) &params, int forceForBind);

 protected:
  static UINT s_convertMinFilterToOgl[5];
  static UINT s_convertMagFilterToOgl[5];
  static int  s_convertTexFmt[8];
  static UINT s_dataFormatSize[8];
  static int  s_convertDataFmt[8];
  static int  s_convertDataType[8];

  HWND  m_hwnd;
  int   m_ownhwnd;
  WORD  m_hwndClass;
  HDC   m_hdc;
  HGLRC m_hglrc;

  template <class T>
  struct PixelFormatAttribute {
    int attribute;
    T   value;

    PixelFormatAttribute() {
    }

    PixelFormatAttribute(int attribute, T value) : attribute(attribute), value(value) {
    }
  };

  typedef PixelFormatAttribute<int>   PixelFormatAttributei;
  typedef PixelFormatAttribute<float> PixelFormatAttributef;

  HPBUFFERARB                       m_hPbuffer;
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

  virtual void ITexMarkAsUpdated(CGxTex *texId);
  void         ITexMarkAsUpdated(CGxTex *texId, UINT tmu);
  void         ITexSetFlags(CGxTex *texId);
  void         ITexDownload(CGxTex *texId, UINT w, UINT h, UINT startLevel, UINT oglBase, UINT texelStrideInBytes, LPCVOID texels);
  void         IBufSetBuffers(CGxBufOgl *buf);
  void         IPixelShaderBind(CGxPixelShader *ps);

 public:
  CGxDeviceOpenGl();
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
  void          DeviceCreatePbuffer();
  void          DeviceQueryPbuffer();
  void          DeviceDestroyPbuffer();
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

  static long CALLBACK WindowProcGl(HWND window, UINT message, UINT wparam, long lparam);
};
