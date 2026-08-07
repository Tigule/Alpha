#pragma once

#include "../CGxDevice.h"

#include <windows.h>

#include <d3dx9.h>

class CGxVertexBuffer_D3d : public CGxVertexBuffer {
 public:
  CGxVertexBuffer_D3d(EGxVertexBufferFormat format, IDirect3DVertexBuffer9 *vb, UINT numVertices);
  virtual ~CGxVertexBuffer_D3d();

  virtual void Lock(LPVOID &mem, UINT numVertices, UINT base);
  virtual void Unlock();

  void                    Discard();
  IDirect3DVertexBuffer9 *GetD3dBuffer() {
    return m_d3dvb;
  }

 private:
  friend class CGxDeviceD3d;

  IDirect3DVertexBuffer9 *m_d3dvb;
  EGxVertexBufferFormat   m_vbFormat;
};

class CGxIndexBuffer_D3d : public CGxIndexBuffer {
 public:
  CGxIndexBuffer_D3d(IDirect3DIndexBuffer9 *ib, UINT numIndices);
  virtual ~CGxIndexBuffer_D3d();

  virtual void Lock(LPVOID &mem, UINT numIndices, UINT base);
  virtual void Unlock();

  IDirect3DIndexBuffer9 *GetD3dBuffer() {
    return m_d3dib;
  }

 private:
  friend class CGxDeviceD3d;

  IDirect3DIndexBuffer9 *m_d3dib;
};

class CVertexBufferList {
 public:
  CVertexBufferList();
  void                 Create(EGxVertexBufferFormat format, UINT numVerts);
  CGxVertexBuffer_D3d *Lock(LPVOID &mem, UINT numVertices, UINT base);
  UINT                 GetBase();
  int                  Valid();
  UINT                 MaxContiguousVertices() {
    return m_maxContiguousVertices;
  }
  void Release();

 private:
  friend class CGxDeviceD3d;
  friend class CGxBufD3d;

  UINT                                   m_maxContiguousVertices;
  UINT                                   m_numVerts;
  UINT                                   m_currentVB;
  TSGrowableArray<CGxVertexBuffer_D3d *> m_vbList;
};

class CGxBufD3d : public CGxBuf {
 public:
  virtual ~CGxBufD3d();

  void                 SetVBL(CVertexBufferList *vbl);
  void                 SetVB(CGxVertexBuffer_D3d *vb);
  void                 SetIB(CGxIndexBuffer_D3d *ib);
  void                 UnsetVB();
  void                 UnsetIB();
  void                 LockVB(LPVOID &mem);
  void                 LockIB(LPVOID &mem);
  void                 UnlockVB();
  void                 UnlockIB();
  CGxVertexBuffer_D3d *GetVB();
  CGxIndexBuffer_D3d  *GetIB();
  BOOL                 VBLValid();
  BOOL                 IBValid();
  void                 Release();

 private:
  friend class CGxDeviceD3d;

  CGxBufD3d();
  CGxBufD3d(const CGxBufD3d &);
  const CGxBufD3d &operator=(const CGxBufD3d &);

  CVertexBufferList   *m_vbl;
  CGxVertexBuffer_D3d *m_vb;
  CGxIndexBuffer_D3d  *m_ib;
};

class CGxDeviceD3d : public CGxDevice {
 public:
  struct StateD3dLight {
    StateD3dLight() {
    }

    int  InUse();
    int  operator!=(const _D3DLIGHT9 &light);
    UINT CalcChkSum(const _D3DLIGHT9 &light);

    DWORD      which;
    _D3DLIGHT9 val;
    int        enabled;
    UINT       chkSum;
  };

  enum EDeviceState {
    Ds_SrcBlend = 0,
    Ds_DstBlend = 1,
    Ds_TssMagFilter0 = 2,
    Ds_TssMagFilter1 = 3,
    Ds_TssMagFilter2 = 4,
    Ds_TssMagFilter3 = 5,
    Ds_TssMinFilter0 = 6,
    Ds_TssMinFilter1 = 7,
    Ds_TssMinFilter2 = 8,
    Ds_TssMinFilter3 = 9,
    Ds_TssMipFilter0 = 10,
    Ds_TssMipFilter1 = 11,
    Ds_TssMipFilter2 = 12,
    Ds_TssMipFilter3 = 13,
    Ds_TssWrapU0 = 14,
    Ds_TssWrapU1 = 15,
    Ds_TssWrapU2 = 16,
    Ds_TssWrapU3 = 17,
    Ds_TssWrapV0 = 18,
    Ds_TssWrapV1 = 19,
    Ds_TssWrapV2 = 20,
    Ds_TssWrapV3 = 21,
    Ds_TssTTF0 = 22,
    Ds_TssTTF1 = 23,
    Ds_TssTTF2 = 24,
    Ds_TssTTF3 = 25,
    Ds_TssMaxAnisotropy0 = 26,
    Ds_TssMaxAnisotropy1 = 27,
    Ds_TssMaxAnisotropy2 = 28,
    Ds_TssMaxAnisotropy3 = 29,
    Ds_DiffuseMaterialSource = 30,
    Ds_AmbientMaterialSource = 31,
    Ds_AlphaBlendEnable = 32,
    Ds_AlphaTestEnable = 33,
    DeviceStates_Last = 34
  };

  virtual ~CGxDeviceD3d();

  static CGxDeviceD3d *GetDevice() {
    return m_thisDevice;
  }

  virtual BOOL  DeviceCreate(GXWINDOWPROC windowProc, const CGxFormat &format);
  virtual BOOL  DeviceCreate(UINT hwnd, const CGxFormat &format);
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
  );
  virtual void    PrimLockIndexPtr(EGxPrim primType, UINT indexCount, const WORD *indices);
  virtual void    PrimDrawElements();
  virtual void    PrimUnlockIndexPtr();
  virtual void    PrimUnlockVertexPtrs();
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
  virtual void TexDestroy(CGxTex *texId);
  virtual void PixelShaderCreate(CGxPixelShader *&ps, LPCSTR filename);
  virtual void PixelShaderDestroy(CGxPixelShader *&ps);

  static BOOL             ILoadD3dLib(HINSTANCE &d3dLib, IDirect3D9 *&d3d);
  static void             IUnloadD3dLib(HINSTANCE &d3dLib, IDirect3D9 *&d3d);
  static LRESULT CALLBACK WindowProcD3d(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

 private:
  friend class CVertexBufferList;
  friend class CGxDevice;
  friend class CGxIndexBuffer_D3d;
  friend class CGxVertexBuffer_D3d;

  CGxDeviceD3d();
  CGxDeviceD3d(const CGxDeviceD3d &);
  const CGxDeviceD3d &operator=(const CGxDeviceD3d &);

  BOOL         ICheckTextureFormat(DWORD usage, _D3DFORMAT textureFormat);
  _D3DFORMAT   IDepthStencilBitsToFormat(UINT depthBits, UINT stencilBits);
  _D3DFORMAT   IColorAlphaBitsToFormat(UINT colorBits, UINT alphaBits);
  DWORD        DsGet(EDeviceState state);
  BOOL         ICreateD3d();
  void         IDestroyD3d();
  BOOL         IAllocBuffers();
  void         ICreateBuffers(EGxVertexBufferFormat vbFormat, UINT numVertices, CVertexBufferList &vbl, UINT numIndices, CGxIndexBuffer_D3d *&ib);
  void         ICreateD3dVB(EGxVertexBufferFormat format, UINT &numVertices, IDirect3DVertexBuffer9 *&vb);
  void         ICreateD3dIB(UINT &numIndices, IDirect3DIndexBuffer9 *&ib);
  void         IBufSetBuffers(CGxBufD3d *buf);
  BOOL         ICreateD3dDevice(const CGxFormat &format);
  void         IDestroyD3dDevice();
  void         ISetPresentParms(D3DPRESENT_PARAMETERS &d3dpp, const CGxFormat &format);
  void         ISetCaps();
  void         IReleaseD3dResources(int freeTextures);
  void         IStateSetD3DDefaults();
  void         ISetLight(DWORD which, const _D3DLIGHT9 &value, int enabled);
  void         IForceLights();
  void         ISceneBegin(UINT mask);
  void         IReleaseD3dVB(IDirect3DVertexBuffer9 *&vb);
  void         IReleaseD3dIB(IDirect3DIndexBuffer9 *&ib);
  void         IStateSync();
  void         IStateSyncLights();
  void         IStateSyncEnables();
  void         IStateSyncMaterial();
  void         IStateSyncTransforms();
  void         IXformSetWorld();
  void         IXformSetTex(UINT tmu);
  void         DsSet(EDeviceState state, DWORD val);
  void         ISetTexture(UINT tmu, CGxTex *tex);
  void         ISetTexBlend(UINT tmu, EGxTexBlend blend);
  void         ISetTexLodBias(UINT tmu, float bias);
  void         ISetTexGen(UINT tmu, EGxTexGen texGen);
  void         IBindPixelShader(CGxPixelShader *ps);
  void         IBindVertexShader(CGxVertexShader *vs);
  void         IPixelShaderCreate(CGxPixelShader *ps);
  void         IPrimSetupPos(LPVOID dstBuf);
  void         IPrimProcessVertexPtrs();
  void         IPrimProcessIndexPtrs();
  void         ISceneEnd();
  void         IShaderForceRecreation(int freeShaders);
  void         ITexForceRecreation(int freeTextures);
  void         ITexCreate(CGxTex *gxTex, UINT w, UINT h, UINT startLevel, UINT endLevel);
  void         ITexUpload(CGxTex *texId, UINT w, UINT h, UINT startLevel, UINT endLevel);
  virtual void ITexMarkAsUpdated(CGxTex *texId);
  virtual void ISetShaderParamList(TSExplicitList<CGxShaderParam, 108> &params, int forceForBind);

  static const EGxTexFormat      s_tolerableTexFmtMapping[GxTexFormats_Last];
  static const _D3DFORMAT        s_GxTexFmtToD3dFmt[GxTexFormats_Last];
  static const _D3DCUBEMAP_FACES s_d3dCubeMapFaces[6];
  static EGxTexFormat            s_GxTexFmtToUse[GxTexFormats_Last];
  static const _D3DFORMAT        s_GxFormatToD3dFormat[CGxFormat::Formats_Last];
  static CGxDeviceD3d           *m_thisDevice;

  HWND                 m_hwnd;
  WORD                 m_hwndClass;
  int                  m_ownhwnd;
  HINSTANCE            m_d3dLib;
  IDirect3D9          *m_d3d;
  IDirect3DDevice9    *m_d3dDevice;
  _D3DCAPS9            m_d3dCaps;
  int                  m_d3dIsHwDevice;
  BOOL                 m_d3dNeedsReset;
  CVertexBufferList    m_VBL[4][9];
  CGxIndexBuffer_D3d  *m_IB[4][9];
  CGxVertexBuffer_D3d *m_vertexBuffer;
  EGxPrim              m_primType;
  UINT                 m_primIndexCount;
  BOOL                 m_processedVertexPtrs;
  BOOL                 m_processedIndexPtrs;
  BOOL                 m_windowVisible;
  _D3DDISPLAYMODE      m_desktopDisplayMode;
  int                  m_deviceSupports32BitTextures;
  BOOL                 m_inScene;
  _D3DFORMAT           m_devDepthFormat;
  _D3DFORMAT           m_devAdapterFormat;
  IDirect3DSurface9   *m_rttColorSurface;
  IDirect3DSurface9   *m_rttDepthSurface;
  IDirect3DSurface9   *m_defColorSurface;
  IDirect3DSurface9   *m_defDepthSurface;
  StateD3dLight        m_d3dStatesLight[8];
  DWORD                m_deviceState[34];
  BYTE                 m_texEnable[4];
};
