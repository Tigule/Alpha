#pragma once

#include "../CGxDevice.h"

#include <windows.h>

#include <d3dx9.h>


class CGxVertexBuffer_D3d : public CGxVertexBuffer {
 public:
  CGxVertexBuffer_D3d(EGxVertexBufferFormat format, IDirect3DVertexBuffer9 *vb, unsigned int numVertices);
  virtual ~CGxVertexBuffer_D3d();

  virtual void Lock(void *&mem, unsigned int numVertices, unsigned int base);
  virtual void Unlock();

  void Discard();

 private:
  friend class CGxDeviceD3d;

  IDirect3DVertexBuffer9 *m_d3dvb;
  EGxVertexBufferFormat   m_vbFormat;
};

class CGxIndexBuffer_D3d : public CGxIndexBuffer {
 public:
  CGxIndexBuffer_D3d(IDirect3DIndexBuffer9 *ib, unsigned int numIndices);
  virtual ~CGxIndexBuffer_D3d();

  virtual void Lock(void *&mem, unsigned int numIndices, unsigned int base);
  virtual void Unlock();

 private:
  friend class CGxDeviceD3d;

  IDirect3DIndexBuffer9 *m_d3dib;
};

class CVertexBufferList {
 public:
  CVertexBufferList();
  void                 Create(EGxVertexBufferFormat format, unsigned int numVerts);
  CGxVertexBuffer_D3d *Lock(void *&mem, unsigned int numVertices, unsigned int base);
  unsigned int         GetBase();
  void                 Release();

 private:
  friend class CGxDeviceD3d;
  friend class CGxBufD3d;

  unsigned int                           m_maxContiguousVertices;
  unsigned int                           m_numVerts;
  unsigned int                           m_currentVB;
  TSGrowableArray<CGxVertexBuffer_D3d *> m_vbList;
};

class CGxBufD3d : public CGxBuf {
 public:
  CGxBufD3d();
  virtual ~CGxBufD3d();

  void SetVBL(CVertexBufferList *vbl);
  void SetVB(CGxVertexBuffer_D3d *vb);
  void SetIB(CGxIndexBuffer_D3d *ib);
  void UnsetVB();
  void UnsetIB();
  void LockVB(void *&mem);
  void LockIB(void *&mem);
  int  VBLValid();
  int  IBValid();
  void Release();

 private:
  friend class CGxDeviceD3d;

  CVertexBufferList   *m_vbl;
  CGxVertexBuffer_D3d *m_vb;
  CGxIndexBuffer_D3d  *m_ib;
};

class CGxDeviceD3d : public CGxDevice {
 public:
  struct StateD3dLight {
    unsigned long which;
    _D3DLIGHT9    val;
    int           enabled;
    unsigned int  chkSum;
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

  CGxDeviceD3d();
  virtual ~CGxDeviceD3d();

  static CGxDeviceD3d *GetDevice() {
    return m_thisDevice;
  }

  virtual int           DeviceCreate(GXWINDOWPROC windowProc, const CGxFormat &format);
  virtual int           DeviceCreate(unsigned int hwnd, const CGxFormat &format);
  virtual void          DeviceDestroy();
  virtual int           DeviceSetFormat(const CGxFormat &format);
  virtual void          DeviceSetBaseMipLevel(unsigned int baseMipLevel);
  virtual void          DeviceSetGamma(float gamma);
  virtual void          DeviceSetGamma(const CGxGammaRamp &ramp);
  virtual void          DeviceSetTextureQuality(int force32);
  virtual unsigned long DeviceWindow();
  virtual void          DeviceReadPixels(NTempest::CiRect &rect, TSGrowableArray<NTempest::CImVector> &pixels);
  virtual void          DeviceReadDepths(NTempest::CiRect &rect, TSGrowableArray<float> &depths);
  virtual void          DeviceWM(EGxWM wm, long param1, long param2);
  virtual void          DeviceSetRenderTarget(EGxBuffer buffer, CGxTex *gxTex, unsigned int plane);
  virtual void          DeviceOverride(EGxOverride override, unsigned long value);
  virtual void          CapsWindowSize(NTempest::CRect &dst);
  virtual void          CapsWindowSizeInScreenCoords(NTempest::CRect &dst);
  virtual int           CapsIsWindowVisible();
  virtual void          ScenePresent(unsigned int mask);
  virtual void          SceneClear(unsigned int mask);
  virtual void          XformSetViewport(float minX, float maxX, float minY, float maxY, float minZ, float maxZ);
  virtual void          XformSetProjection(const NTempest::C44Matrix &matrix);
  virtual void          XformSetView(const NTempest::C44Matrix &matrix);
  virtual void          PrimLockAndProcessVertexPtrs(
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
  );
  virtual void    PrimLockIndexPtr(EGxPrim primType, unsigned int indexCount, const unsigned short *indices);
  virtual void    PrimDrawElements();
  virtual void    PrimUnlockIndexPtr();
  virtual void    PrimUnlockVertexPtrs();
  virtual void    IRsSendToHw(EGxRenderState which);
  virtual CGxBuf *BufCreate(
      EGxBufWriteFreq       writeFreq,
      EGxVertexBufferFormat format,
      unsigned int          numVertices,
      unsigned int          numIndices,
      void(__fastcall *userCallback)(CGxBufCommand &, CGxBuf *),
      void *userArg
  );
  virtual void BufLock(CGxBuf *b);
  virtual void BufRender(const CGxBatch *batches, unsigned int count);
  virtual void BufUnlock();
  virtual void BufDestroy(CGxBuf *&b);
  virtual void BufReserve(EGxBufWriteFreq freq, EGxVertexBufferFormat format, unsigned int numVertices, unsigned int numIndices);
  virtual int  TexCreate(
      unsigned int width,
      unsigned int height,
      EGxTexFormat format,
      CGxTexFlags  flags,
      void        *userArg,
      void(__fastcall *userFunc)(EGxTexCommand, unsigned int, unsigned int, unsigned int, unsigned int, void *, unsigned int &, const void *&),
      CGxTex *&texId
  );
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
  virtual void TexDestroy(CGxTex *texId);
  virtual void PixelShaderCreate(CGxPixelShader *&ps, const char *filename);
  virtual void PixelShaderDestroy(CGxPixelShader *&ps);

  static int __fastcall   ILoadD3dLib(HINSTANCE &d3dLib, IDirect3D9 *&d3d);
  static void __fastcall  IUnloadD3dLib(HINSTANCE &d3dLib, IDirect3D9 *&d3d);
  static LRESULT CALLBACK WindowProcD3d(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

 private:
  friend class CVertexBufferList;
  friend class CGxDevice;
  friend class CGxIndexBuffer_D3d;
  friend class CGxVertexBuffer_D3d;

  CGxDeviceD3d(const CGxDeviceD3d &);
  const CGxDeviceD3d &operator=(const CGxDeviceD3d &);

  int  ICheckTextureFormat(unsigned long usage, _D3DFORMAT textureFormat);
  int  ICreateD3d();
  void IDestroyD3d();
  int  IAllocBuffers();
  void
  ICreateBuffers(EGxVertexBufferFormat vbFormat, unsigned int numVertices, CVertexBufferList &vbl, unsigned int numIndices, CGxIndexBuffer_D3d *&ib);
  void         ICreateD3dVB(EGxVertexBufferFormat format, unsigned int &numVertices, IDirect3DVertexBuffer9 *&vb);
  void         ICreateD3dIB(unsigned int &numIndices, IDirect3DIndexBuffer9 *&ib);
  void         IBufSetBuffers(CGxBufD3d *buf);
  int          ICreateD3dDevice(const CGxFormat &format);
  void         IDestroyD3dDevice();
  void         ISetPresentParms(D3DPRESENT_PARAMETERS &d3dpp, const CGxFormat &format);
  void         ISetCaps();
  void         IReleaseD3dResources(int freeTextures);
  void         IStateSetD3DDefaults();
  void         ISetLight(unsigned long which, const _D3DLIGHT9 &value, int enabled);
  void         IForceLights();
  void         ISceneBegin(unsigned int mask);
  void         IReleaseD3dVB(IDirect3DVertexBuffer9 *&vb);
  void         IReleaseD3dIB(IDirect3DIndexBuffer9 *&ib);
  void         IStateSync();
  void         IStateSyncLights();
  void         IStateSyncEnables();
  void         IStateSyncMaterial();
  void         IStateSyncTransforms();
  void         IXformSetWorld();
  void         IXformSetTex(unsigned int tmu);
  void         DsSet(EDeviceState state, unsigned long val);
  void         ISetTexture(unsigned int tmu, CGxTex *tex);
  void         ISetTexBlend(unsigned int tmu, EGxTexBlend blend);
  void         ISetTexLodBias(unsigned int tmu, float bias);
  void         ISetTexGen(unsigned int tmu, EGxTexGen texGen);
  void         IBindPixelShader(CGxPixelShader *ps);
  void         IBindVertexShader(CGxVertexShader *vs);
  void         IPixelShaderCreate(CGxPixelShader *ps);
  void         IPrimSetupPos(void *dstBuf);
  void         IPrimProcessVertexPtrs();
  void         IPrimProcessIndexPtrs();
  void         ISceneEnd();
  void         IShaderForceRecreation(int freeShaders);
  void         ITexForceRecreation(int freeTextures);
  void         ITexCreate(CGxTex *gxTex, unsigned int w, unsigned int h, unsigned int startLevel, unsigned int endLevel);
  void         ITexUpload(CGxTex *texId, unsigned int w, unsigned int h, unsigned int startLevel, unsigned int endLevel);
  virtual void ITexMarkAsUpdated(CGxTex *texId);
  virtual void ISetShaderParamList(TSExplicitList<CGxShaderParam, 108> &params, int forceForBind);

  static const EGxTexFormat      s_tolerableTexFmtMapping[GxTexFormats_Last];
  static const _D3DFORMAT        s_GxTexFmtToD3dFmt[GxTexFormats_Last];
  static const _D3DCUBEMAP_FACES s_d3dCubeMapFaces[6];
  static EGxTexFormat            s_GxTexFmtToUse[GxTexFormats_Last];
  static const _D3DFORMAT        s_GxFormatToD3dFormat[CGxFormat::Formats_Last];
  static CGxDeviceD3d           *m_thisDevice;

  HWND                 m_hwnd;
  unsigned short       m_hwndClass;
  int                  m_ownhwnd;
  HINSTANCE            m_d3dLib;
  IDirect3D9          *m_d3d;
  IDirect3DDevice9    *m_d3dDevice;
  _D3DCAPS9            m_d3dCaps;
  int                  m_d3dIsHwDevice;
  int                  m_d3dNeedsReset;
  CVertexBufferList    m_VBL[4][9];
  CGxIndexBuffer_D3d  *m_IB[4][9];
  CGxVertexBuffer_D3d *m_vertexBuffer;
  EGxPrim              m_primType;
  unsigned int         m_primIndexCount;
  int                  m_processedVertexPtrs;
  int                  m_processedIndexPtrs;
  int                  m_windowVisible;
  _D3DDISPLAYMODE      m_desktopDisplayMode;
  int                  m_deviceSupports32BitTextures;
  int                  m_inScene;
  _D3DFORMAT           m_devDepthFormat;
  _D3DFORMAT           m_devAdapterFormat;
  IDirect3DSurface9   *m_rttColorSurface;
  IDirect3DSurface9   *m_rttDepthSurface;
  IDirect3DSurface9   *m_defColorSurface;
  IDirect3DSurface9   *m_defDepthSurface;
  StateD3dLight        m_d3dStatesLight[8];
  unsigned long        m_deviceState[34];
  unsigned int         m_texEnable[4];
};
