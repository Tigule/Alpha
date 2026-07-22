#pragma once

#include "../CGxDevice.h"

#include <windows.h>

#define D3D_SDK_VERSION                     31
#define D3DCREATE_FPU_PRESERVE              0x00000002L
#define D3DCREATE_SOFTWARE_VERTEXPROCESSING 0x00000020L
#define D3DCREATE_HARDWARE_VERTEXPROCESSING 0x00000040L
#define D3DDEVCAPS_HWTRANSFORMANDLIGHT      0x00010000L
#define D3DFVF_XYZ                          0x0002L
#define D3DFVF_NORMAL                       0x0010L
#define D3DFVF_DIFFUSE                      0x0040L
#define D3DFVF_TEX1                         0x0100L
#define D3DFVF_TEX2                         0x0200L
#define D3DLOCK_READONLY                    0x00000010L
#define D3DLOCK_NOSYSLOCK                   0x00000800L
#define D3DLOCK_NOOVERWRITE                 0x00001000L
#define D3DLOCK_DISCARD                     0x00002000L
#define D3DPRESENTFLAG_LOCKABLE_BACKBUFFER  0x00000001L
#define D3DPRESENT_INTERVAL_ONE             0x00000001L
#define D3DPRESENT_INTERVAL_IMMEDIATE       0x80000000L
#define D3DUSAGE_WRITEONLY                  0x00000008L
#define D3DUSAGE_SOFTWAREPROCESSING         0x00000010L
#define D3DUSAGE_DYNAMIC                    0x00000200L
#define D3DUSAGE_RENDERTARGET               0x00000001L
#define D3DUSAGE_AUTOGENMIPMAP              0x00000400L
#define D3DERR_OUTOFVIDEOMEMORY             _HRESULT_TYPEDEF_(0x8876017CL)
#define D3DERR_DRIVERINTERNALERROR          _HRESULT_TYPEDEF_(0x88760827L)
#define D3DERR_NOTAVAILABLE                 _HRESULT_TYPEDEF_(0x8876086AL)
#define D3DERR_INVALIDCALL                  _HRESULT_TYPEDEF_(0x8876086CL)
#define D3DERR_DEVICENOTRESET               _HRESULT_TYPEDEF_(0x88760869L)
#define D3DERR_WRONGTEXTUREFORMAT           _HRESULT_TYPEDEF_(0x88760818L)
#define D3DERR_UNSUPPORTEDCOLOROPERATION    _HRESULT_TYPEDEF_(0x88760819L)
#define D3DERR_UNSUPPORTEDCOLORARG          _HRESULT_TYPEDEF_(0x8876081AL)
#define D3DERR_UNSUPPORTEDALPHAOPERATION    _HRESULT_TYPEDEF_(0x8876081BL)
#define D3DERR_UNSUPPORTEDALPHAARG          _HRESULT_TYPEDEF_(0x8876081CL)
#define D3DERR_TOOMANYOPERATIONS            _HRESULT_TYPEDEF_(0x8876081DL)
#define D3DERR_CONFLICTINGTEXTUREFILTER     _HRESULT_TYPEDEF_(0x8876081EL)
#define D3DERR_UNSUPPORTEDFACTORVALUE       _HRESULT_TYPEDEF_(0x8876081FL)
#define D3DERR_UNSUPPORTEDTEXTUREFILTER     _HRESULT_TYPEDEF_(0x88760822L)
#define D3DERR_DEVICELOST                   _HRESULT_TYPEDEF_(0x88760868L)
#define D3DPRASTERCAPS_FOGTABLE             0x00000100L
#define D3DPRASTERCAPS_WFOG                 0x00100000L

enum _D3DDEVTYPE {
  D3DDEVTYPE_HAL = 1,
  D3DDEVTYPE_REF = 2,
  D3DDEVTYPE_SW = 3,
  D3DDEVTYPE_FORCE_DWORD = 0x7FFFFFFF
};

enum _D3DRESOURCETYPE {
  D3DRTYPE_SURFACE = 1,
  D3DRTYPE_VOLUME = 2,
  D3DRTYPE_TEXTURE = 3,
  D3DRTYPE_VOLUMETEXTURE = 4,
  D3DRTYPE_CUBETEXTURE = 5,
  D3DRTYPE_VERTEXBUFFER = 6,
  D3DRTYPE_INDEXBUFFER = 7,
  D3DRTYPE_FORCE_DWORD = 0x7FFFFFFF
};

enum _D3DPOOL {
  D3DPOOL_DEFAULT = 0,
  D3DPOOL_MANAGED = 1,
  D3DPOOL_SYSTEMMEM = 2,
  D3DPOOL_SCRATCH = 3,
  D3DPOOL_FORCE_DWORD = 0x7FFFFFFF
};

enum _D3DBACKBUFFER_TYPE {
  D3DBACKBUFFER_TYPE_MONO = 0,
  D3DBACKBUFFER_TYPE_LEFT = 1,
  D3DBACKBUFFER_TYPE_RIGHT = 2,
  D3DBACKBUFFER_TYPE_FORCE_DWORD = 0x7FFFFFFF
};

enum _D3DCUBEMAP_FACES {
  D3DCUBEMAP_FACE_POSITIVE_X = 0,
  D3DCUBEMAP_FACE_NEGATIVE_X = 1,
  D3DCUBEMAP_FACE_POSITIVE_Y = 2,
  D3DCUBEMAP_FACE_NEGATIVE_Y = 3,
  D3DCUBEMAP_FACE_POSITIVE_Z = 4,
  D3DCUBEMAP_FACE_NEGATIVE_Z = 5,
  D3DCUBEMAP_FACE_FORCE_DWORD = 0x7FFFFFFF
};

enum _D3DFORMAT {
  D3DFMT_UNKNOWN = 0,
  D3DFMT_R8G8B8 = 20,
  D3DFMT_A8R8G8B8 = 21,
  D3DFMT_X8R8G8B8 = 22,
  D3DFMT_R5G6B5 = 23,
  D3DFMT_X1R5G5B5 = 24,
  D3DFMT_A1R5G5B5 = 25,
  D3DFMT_A4R4G4B4 = 26,
  D3DFMT_R3G3B2 = 27,
  D3DFMT_A8 = 28,
  D3DFMT_A8R3G3B2 = 29,
  D3DFMT_X4R4G4B4 = 30,
  D3DFMT_A2B10G10R10 = 31,
  D3DFMT_A8B8G8R8 = 32,
  D3DFMT_X8B8G8R8 = 33,
  D3DFMT_G16R16 = 34,
  D3DFMT_A2R10G10B10 = 35,
  D3DFMT_A16B16G16R16 = 36,
  D3DFMT_A8P8 = 40,
  D3DFMT_P8 = 41,
  D3DFMT_L8 = 50,
  D3DFMT_A8L8 = 51,
  D3DFMT_A4L4 = 52,
  D3DFMT_V8U8 = 60,
  D3DFMT_L6V5U5 = 61,
  D3DFMT_X8L8V8U8 = 62,
  D3DFMT_Q8W8V8U8 = 63,
  D3DFMT_V16U16 = 64,
  D3DFMT_A2W10V10U10 = 67,
  D3DFMT_D16_LOCKABLE = 70,
  D3DFMT_D32 = 71,
  D3DFMT_D15S1 = 73,
  D3DFMT_D24S8 = 75,
  D3DFMT_D24X8 = 77,
  D3DFMT_D24X4S4 = 79,
  D3DFMT_D16 = 80,
  D3DFMT_L16 = 81,
  D3DFMT_D32F_LOCKABLE = 82,
  D3DFMT_D24FS8 = 83,
  D3DFMT_VERTEXDATA = 100,
  D3DFMT_INDEX16 = 101,
  D3DFMT_INDEX32 = 102,
  D3DFMT_Q16W16V16U16 = 110,
  D3DFMT_R16F = 111,
  D3DFMT_G16R16F = 112,
  D3DFMT_A16B16G16R16F = 113,
  D3DFMT_R32F = 114,
  D3DFMT_G32R32F = 115,
  D3DFMT_A32B32G32R32F = 116,
  D3DFMT_CxV8U8 = 117,
  D3DFMT_UYVY = 1498831189,
  D3DFMT_R8G8_B8G8 = 1195525970,
  D3DFMT_YUY2 = 844715353,
  D3DFMT_G8R8_G8B8 = 1111970375,
  D3DFMT_DXT1 = 827611204,
  D3DFMT_DXT2 = 844388420,
  D3DFMT_DXT3 = 861165636,
  D3DFMT_DXT4 = 877942852,
  D3DFMT_DXT5 = 894720068,
  D3DFMT_MULTI2_ARGB8 = 827606349,
  D3DFMT_FORCE_DWORD = 0x7FFFFFFF
};

enum _D3DLIGHTTYPE {
  D3DLIGHT_POINT = 1,
  D3DLIGHT_SPOT = 2,
  D3DLIGHT_DIRECTIONAL = 3,
  D3DLIGHT_FORCE_DWORD = 0x7FFFFFFF
};

enum _D3DZBUFFERTYPE {
  D3DZB_TRUE = 1,
  D3DZB_FORCE_DWORD = 0x7FFFFFFF
};

enum _D3DCMPFUNC {
  D3DCMP_NEVER = 1,
  D3DCMP_LESS = 2,
  D3DCMP_EQUAL = 3,
  D3DCMP_LESSEQUAL = 4,
  D3DCMP_GREATER = 5,
  D3DCMP_NOTEQUAL = 6,
  D3DCMP_GREATEREQUAL = 7,
  D3DCMP_ALWAYS = 8,
  D3DCMP_FORCE_DWORD = 0x7FFFFFFF
};

enum _D3DBLEND {
  D3DBLEND_ZERO = 1,
  D3DBLEND_ONE = 2,
  D3DBLEND_SRCCOLOR = 3,
  D3DBLEND_INVSRCCOLOR = 4,
  D3DBLEND_SRCALPHA = 5,
  D3DBLEND_INVSRCALPHA = 6,
  D3DBLEND_DESTALPHA = 7,
  D3DBLEND_INVDESTALPHA = 8,
  D3DBLEND_DESTCOLOR = 9,
  D3DBLEND_INVDESTCOLOR = 10,
  D3DBLEND_SRCALPHASAT = 11,
  D3DBLEND_FORCE_DWORD = 0x7FFFFFFF
};

enum _D3DFOGMODE {
  D3DFOG_NONE = 0,
  D3DFOG_EXP = 1,
  D3DFOG_EXP2 = 2,
  D3DFOG_LINEAR = 3,
  D3DFOG_FORCE_DWORD = 0x7FFFFFFF
};

enum _D3DCULL {
  D3DCULL_NONE = 1,
  D3DCULL_CW = 2,
  D3DCULL_CCW = 3,
  D3DCULL_FORCE_DWORD = 0x7FFFFFFF
};

enum _D3DTEXTUREFILTERTYPE {
  D3DTEXF_NONE = 0,
  D3DTEXF_POINT = 1,
  D3DTEXF_LINEAR = 2,
  D3DTEXF_ANISOTROPIC = 3,
  D3DTEXF_FORCE_DWORD = 0x7FFFFFFF
};

enum _D3DTEXTUREADDRESS {
  D3DTADDRESS_WRAP = 1,
  D3DTADDRESS_CLAMP = 3,
  D3DTADDRESS_FORCE_DWORD = 0x7FFFFFFF
};

enum _D3DTEXTUREOP {
  D3DTOP_DISABLE = 1,
  D3DTOP_SELECTARG1 = 2,
  D3DTOP_MODULATE = 4,
  D3DTOP_MODULATE2X = 5,
  D3DTOP_ADD = 7,
  D3DTOP_BLENDTEXTUREALPHA = 13,
  D3DTOP_FORCE_DWORD = 0x7FFFFFFF
};

enum _D3DTEXTURESTAGESTATETYPE {
  D3DTSS_COLOROP = 1,
  D3DTSS_ALPHAOP = 4,
  D3DTSS_TEXCOORDINDEX = 11,
  D3DTSS_TEXTURETRANSFORMFLAGS = 24,
  D3DTSS_FORCE_DWORD = 0x7FFFFFFF
};

enum _D3DSAMPLERSTATETYPE {
  D3DSAMP_ADDRESSU = 1,
  D3DSAMP_ADDRESSV = 2,
  D3DSAMP_MAGFILTER = 5,
  D3DSAMP_MINFILTER = 6,
  D3DSAMP_MIPFILTER = 7,
  D3DSAMP_MIPMAPLODBIAS = 8,
  D3DSAMP_MAXANISOTROPY = 10,
  D3DSAMP_FORCE_DWORD = 0x7FFFFFFF
};

enum _D3DRENDERSTATETYPE {
  D3DRS_ZENABLE = 7,
  D3DRS_FILLMODE = 8,
  D3DRS_ZWRITEENABLE = 14,
  D3DRS_ALPHATESTENABLE = 15,
  D3DRS_SRCBLEND = 19,
  D3DRS_DESTBLEND = 20,
  D3DRS_CULLMODE = 22,
  D3DRS_ZFUNC = 23,
  D3DRS_ALPHAREF = 24,
  D3DRS_ALPHAFUNC = 25,
  D3DRS_ALPHABLENDENABLE = 27,
  D3DRS_FOGENABLE = 28,
  D3DRS_FOGCOLOR = 34,
  D3DRS_FOGTABLEMODE = 35,
  D3DRS_FOGSTART = 36,
  D3DRS_FOGEND = 37,
  D3DRS_FOGDENSITY = 38,
  D3DRS_LIGHTING = 137,
  D3DRS_AMBIENT = 139,
  D3DRS_FOGVERTEXMODE = 140,
  D3DRS_LOCALVIEWER = 142,
  D3DRS_NORMALIZENORMALS = 143,
  D3DRS_DIFFUSEMATERIALSOURCE = 145,
  D3DRS_AMBIENTMATERIALSOURCE = 147,
  D3DRS_DEPTHBIAS = 195,
  D3DRS_FORCE_DWORD = 0x7FFFFFFF
};

typedef _D3DRENDERSTATETYPE D3DRENDERSTATETYPE;

enum _D3DTRANSFORMSTATETYPE {
  D3DTS_VIEW = 2,
  D3DTS_PROJECTION = 3,
  D3DTS_TEXTURE0 = 16,
  D3DTS_TEXTURE1 = 17,
  D3DTS_TEXTURE2 = 18,
  D3DTS_TEXTURE3 = 19,
  D3DTS_TEXTURE4 = 20,
  D3DTS_TEXTURE5 = 21,
  D3DTS_TEXTURE6 = 22,
  D3DTS_TEXTURE7 = 23,
  D3DTS_WORLD = 256,
  D3DTS_FORCE_DWORD = 0x7FFFFFFF
};

enum _D3DTEXTURETRANSFORMFLAGS {
  D3DTTFF_DISABLE = 0,
  D3DTTFF_COUNT2 = 2,
  D3DTTFF_COUNT3 = 3,
  D3DTTFF_PROJECTED = 256,
  D3DTTFF_FORCE_DWORD = 0x7FFFFFFF
};

enum _D3DMULTISAMPLE_TYPE {
  D3DMULTISAMPLE_NONE = 0,
  D3DMULTISAMPLE_FORCE_DWORD = 0x7FFFFFFF
};

enum _D3DSWAPEFFECT {
  D3DSWAPEFFECT_DISCARD = 1,
  D3DSWAPEFFECT_FORCE_DWORD = 0x7FFFFFFF
};

#pragma pack(push, 4)

struct _D3DVSHADERCAPS2_0 {
  unsigned int Caps;
  int          DynamicFlowControlDepth;
  int          NumTemps;
  int          StaticFlowControlDepth;
};

struct _D3DPSHADERCAPS2_0 {
  unsigned int Caps;
  int          DynamicFlowControlDepth;
  int          NumTemps;
  int          StaticFlowControlDepth;
  int          NumInstructionSlots;
};

struct _D3DCAPS9 {
  _D3DDEVTYPE        DeviceType;
  unsigned int       AdapterOrdinal;
  unsigned int       Caps;
  unsigned int       Caps2;
  unsigned int       Caps3;
  unsigned int       PresentationIntervals;
  unsigned int       CursorCaps;
  unsigned int       DevCaps;
  unsigned int       PrimitiveMiscCaps;
  unsigned int       RasterCaps;
  unsigned int       ZCmpCaps;
  unsigned int       SrcBlendCaps;
  unsigned int       DestBlendCaps;
  unsigned int       AlphaCmpCaps;
  unsigned int       ShadeCaps;
  unsigned int       TextureCaps;
  unsigned int       TextureFilterCaps;
  unsigned int       CubeTextureFilterCaps;
  unsigned int       VolumeTextureFilterCaps;
  unsigned int       TextureAddressCaps;
  unsigned int       VolumeTextureAddressCaps;
  unsigned int       LineCaps;
  unsigned int       MaxTextureWidth;
  unsigned int       MaxTextureHeight;
  unsigned int       MaxVolumeExtent;
  unsigned int       MaxTextureRepeat;
  unsigned int       MaxTextureAspectRatio;
  unsigned int       MaxAnisotropy;
  float              MaxVertexW;
  float              GuardBandLeft;
  float              GuardBandTop;
  float              GuardBandRight;
  float              GuardBandBottom;
  float              ExtentsAdjust;
  unsigned int       StencilCaps;
  unsigned int       FVFCaps;
  unsigned int       TextureOpCaps;
  unsigned int       MaxTextureBlendStages;
  unsigned int       MaxSimultaneousTextures;
  unsigned int       VertexProcessingCaps;
  unsigned int       MaxActiveLights;
  unsigned int       MaxUserClipPlanes;
  unsigned int       MaxVertexBlendMatrices;
  unsigned int       MaxVertexBlendMatrixIndex;
  float              MaxPointSize;
  unsigned int       MaxPrimitiveCount;
  unsigned int       MaxVertexIndex;
  unsigned int       MaxStreams;
  unsigned int       MaxStreamStride;
  unsigned int       VertexShaderVersion;
  unsigned int       MaxVertexShaderConst;
  unsigned int       PixelShaderVersion;
  float              PixelShader1xMaxValue;
  unsigned int       DevCaps2;
  float              MaxNpatchTessellationLevel;
  unsigned int       Reserved5;
  unsigned int       MasterAdapterOrdinal;
  unsigned int       AdapterOrdinalInGroup;
  unsigned int       NumberOfAdaptersInGroup;
  unsigned int       DeclTypes;
  unsigned int       NumSimultaneousRTs;
  unsigned int       StretchRectFilterCaps;
  _D3DVSHADERCAPS2_0 VS20Caps;
  _D3DPSHADERCAPS2_0 PS20Caps;
  unsigned int       VertexTextureFilterCaps;
  unsigned int       MaxVShaderInstructionsExecuted;
  unsigned int       MaxPShaderInstructionsExecuted;
  unsigned int       MaxVertexShader30InstructionSlots;
  unsigned int       MaxPixelShader30InstructionSlots;
};

typedef _D3DCAPS9 D3DCAPS9;

struct _D3DDISPLAYMODE {
  unsigned int Width;
  unsigned int Height;
  unsigned int RefreshRate;
  _D3DFORMAT   Format;
};

typedef _D3DDISPLAYMODE D3DDISPLAYMODE;

struct _D3DPRESENT_PARAMETERS_ {
  unsigned int         BackBufferWidth;
  unsigned int         BackBufferHeight;
  _D3DFORMAT           BackBufferFormat;
  unsigned int         BackBufferCount;
  _D3DMULTISAMPLE_TYPE MultiSampleType;
  unsigned int         MultiSampleQuality;
  _D3DSWAPEFFECT       SwapEffect;
  HWND                 hDeviceWindow;
  int                  Windowed;
  int                  EnableAutoDepthStencil;
  _D3DFORMAT           AutoDepthStencilFormat;
  unsigned int         Flags;
  unsigned int         FullScreen_RefreshRateInHz;
  unsigned int         PresentationInterval;
};

typedef _D3DPRESENT_PARAMETERS_ D3DPRESENT_PARAMETERS;

struct _D3DSURFACE_DESC {
  _D3DFORMAT           Format;
  _D3DRESOURCETYPE     Type;
  unsigned int         Usage;
  _D3DPOOL             Pool;
  _D3DMULTISAMPLE_TYPE MultiSampleType;
  unsigned int         MultiSampleQuality;
  unsigned int         Width;
  unsigned int         Height;
};

struct _D3DLOCKED_RECT {
  int   Pitch;
  void *pBits;
};

struct _D3DCOLORVALUE {
  float r;
  float g;
  float b;
  float a;
};

struct _D3DMATERIAL9 {
  _D3DCOLORVALUE Diffuse;
  _D3DCOLORVALUE Ambient;
  _D3DCOLORVALUE Specular;
  _D3DCOLORVALUE Emissive;
  float          Power;
};

struct _D3DVECTOR {
  float x;
  float y;
  float z;
};

struct _D3DLIGHT9 {
  _D3DLIGHTTYPE  Type;
  _D3DCOLORVALUE Diffuse;
  _D3DCOLORVALUE Specular;
  _D3DCOLORVALUE Ambient;
  _D3DVECTOR     Position;
  _D3DVECTOR     Direction;
  float          Range;
  float          Falloff;
  float          Attenuation0;
  float          Attenuation1;
  float          Attenuation2;
  float          Theta;
  float          Phi;
};

struct _D3DRECT {
  long x1;
  long y1;
  long x2;
  long y2;
};

typedef _D3DRECT D3DRECT;

struct _D3DMATRIX {
  union {
    struct {
      float _11;
      float _12;
      float _13;
      float _14;
      float _21;
      float _22;
      float _23;
      float _24;
      float _31;
      float _32;
      float _33;
      float _34;
      float _41;
      float _42;
      float _43;
      float _44;
    };
    float m[4][4];
  };
};

typedef _D3DMATRIX D3DMATRIX;

struct _D3DVIEWPORT9 {
  unsigned long X;
  unsigned long Y;
  unsigned long Width;
  unsigned long Height;
  float         MinZ;
  float         MaxZ;
};

typedef _D3DVIEWPORT9 D3DVIEWPORT9;

struct D3DXMATRIX : public D3DMATRIX {
  D3DXMATRIX() {
  }

  D3DXMATRIX(
      float f11,
      float f12,
      float f13,
      float f14,
      float f21,
      float f22,
      float f23,
      float f24,
      float f31,
      float f32,
      float f33,
      float f34,
      float f41,
      float f42,
      float f43,
      float f44
  ) {
    _11 = f11;
    _12 = f12;
    _13 = f13;
    _14 = f14;
    _21 = f21;
    _22 = f22;
    _23 = f23;
    _24 = f24;
    _31 = f31;
    _32 = f32;
    _33 = f33;
    _34 = f34;
    _41 = f41;
    _42 = f42;
    _43 = f43;
    _44 = f44;
  }

  D3DXMATRIX &operator*=(float value) {
    _11 *= value;
    _12 *= value;
    _13 *= value;
    _14 *= value;
    _21 *= value;
    _22 *= value;
    _23 *= value;
    _24 *= value;
    _31 *= value;
    _32 *= value;
    _33 *= value;
    _34 *= value;
    _41 *= value;
    _42 *= value;
    _43 *= value;
    _44 *= value;
    return *this;
  }
};

struct _D3DADAPTER_IDENTIFIER9;
struct D3DDEVICE_CREATION_PARAMETERS;
struct _D3DMATERIAL9;
struct D3DRASTER_STATUS;
struct IDirect3D9;
struct IDirect3DDevice9;
struct IDirect3DBaseTexture9;
struct IDirect3DCubeTexture9;
struct IDirect3DIndexBuffer9;
struct IDirect3DPixelShader9;
struct IDirect3DSurface9;
struct IDirect3DSwapChain9;
struct IDirect3DTexture9;
struct IDirect3DVertexBuffer9;
struct IDirect3DVolumeTexture9;
typedef _D3DMATERIAL9           D3DMATERIAL9;
typedef _D3DADAPTER_IDENTIFIER9 D3DADAPTER_IDENTIFIER9;
typedef CGxGammaRamp            D3DGAMMARAMP;

struct IDirect3D9 {
  virtual long __stdcall          QueryInterface(const GUID &, void **) = 0;
  virtual unsigned long __stdcall AddRef() = 0;
  virtual unsigned long __stdcall Release() = 0;
  virtual long __stdcall          RegisterSoftwareDevice(void *) = 0;
  virtual unsigned int __stdcall  GetAdapterCount() = 0;
  virtual long __stdcall          GetAdapterIdentifier(unsigned int, unsigned int, D3DADAPTER_IDENTIFIER9 *) = 0;
  virtual unsigned int __stdcall  GetAdapterModeCount(unsigned int, unsigned int) = 0;
  virtual long __stdcall          EnumAdapterModes(unsigned int, unsigned int, unsigned int, D3DDISPLAYMODE *) = 0;
  virtual long __stdcall          GetAdapterDisplayMode(unsigned int, D3DDISPLAYMODE *) = 0;
  virtual long __stdcall          CheckDeviceType(unsigned int, unsigned int, unsigned int, unsigned int, int) = 0;
  virtual long __stdcall          CheckDeviceFormat(unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int) = 0;
  virtual long __stdcall          CheckDeviceMultiSampleType(unsigned int, unsigned int, unsigned int, int, unsigned int, unsigned int *) = 0;
  virtual long __stdcall          CheckDepthStencilMatch(unsigned int, unsigned int, unsigned int, unsigned int, unsigned int) = 0;
  virtual long __stdcall          CheckDeviceFormatConversion(unsigned int, unsigned int, unsigned int, unsigned int) = 0;
  virtual long __stdcall          GetDeviceCaps(unsigned int, unsigned int, D3DCAPS9 *) = 0;
  virtual void *__stdcall         GetAdapterMonitor(unsigned int) = 0;
  virtual long __stdcall          CreateDevice(unsigned int, unsigned int, HWND, unsigned int, D3DPRESENT_PARAMETERS *, IDirect3DDevice9 **) = 0;
};

struct IDirect3DPixelShader9 {
  virtual long __stdcall          QueryInterface(const GUID &, void **) = 0;
  virtual unsigned long __stdcall AddRef() = 0;
  virtual unsigned long __stdcall Release() = 0;
};

struct IDirect3DSurface9 {
  virtual long __stdcall             QueryInterface(const GUID &, void **) = 0;
  virtual unsigned long __stdcall    AddRef() = 0;
  virtual unsigned long __stdcall    Release() = 0;
  virtual long __stdcall             GetDevice(IDirect3DDevice9 **) = 0;
  virtual long __stdcall             SetPrivateData(const GUID &, const void *, unsigned long, unsigned long) = 0;
  virtual long __stdcall             GetPrivateData(const GUID &, void *, unsigned long *) = 0;
  virtual long __stdcall             FreePrivateData(const GUID &) = 0;
  virtual unsigned long __stdcall    SetPriority(unsigned long) = 0;
  virtual unsigned long __stdcall    GetPriority() = 0;
  virtual void __stdcall             PreLoad() = 0;
  virtual _D3DRESOURCETYPE __stdcall GetType() = 0;
  virtual long __stdcall             GetContainer(const GUID &, void **) = 0;
  virtual long __stdcall             GetDesc(_D3DSURFACE_DESC *) = 0;
  virtual long __stdcall             LockRect(_D3DLOCKED_RECT *, const RECT *, unsigned long) = 0;
  virtual long __stdcall             UnlockRect() = 0;
};

struct IDirect3DBaseTexture9 {
  virtual long __stdcall             QueryInterface(const GUID &, void **) = 0;
  virtual unsigned long __stdcall    AddRef() = 0;
  virtual unsigned long __stdcall    Release() = 0;
  virtual long __stdcall             GetDevice(IDirect3DDevice9 **) = 0;
  virtual long __stdcall             SetPrivateData(const GUID &, const void *, unsigned long, unsigned long) = 0;
  virtual long __stdcall             GetPrivateData(const GUID &, void *, unsigned long *) = 0;
  virtual long __stdcall             FreePrivateData(const GUID &) = 0;
  virtual unsigned long __stdcall    SetPriority(unsigned long) = 0;
  virtual unsigned long __stdcall    GetPriority() = 0;
  virtual void __stdcall             PreLoad() = 0;
  virtual _D3DRESOURCETYPE __stdcall GetType() = 0;
  virtual unsigned long __stdcall    SetLOD(unsigned long) = 0;
  virtual unsigned long __stdcall    GetLOD() = 0;
  virtual unsigned long __stdcall    GetLevelCount() = 0;
  virtual long __stdcall             SetAutoGenFilterType(unsigned int) = 0;
  virtual unsigned int __stdcall     GetAutoGenFilterType() = 0;
  virtual void __stdcall             GenerateMipSubLevels() = 0;
};

struct IDirect3DTexture9 : public IDirect3DBaseTexture9 {
  virtual long __stdcall GetLevelDesc(unsigned int, _D3DSURFACE_DESC *) = 0;
  virtual long __stdcall GetSurfaceLevel(unsigned int, IDirect3DSurface9 **) = 0;
  virtual long __stdcall LockRect(unsigned int, _D3DLOCKED_RECT *, const RECT *, unsigned long) = 0;
  virtual long __stdcall UnlockRect(unsigned int) = 0;
  virtual long __stdcall AddDirtyRect(const RECT *) = 0;
};

struct IDirect3DCubeTexture9 : public IDirect3DBaseTexture9 {
  virtual long __stdcall GetLevelDesc(unsigned int, _D3DSURFACE_DESC *) = 0;
  virtual long __stdcall GetCubeMapSurface(_D3DCUBEMAP_FACES, unsigned int, IDirect3DSurface9 **) = 0;
  virtual long __stdcall LockRect(_D3DCUBEMAP_FACES, unsigned int, _D3DLOCKED_RECT *, const RECT *, unsigned long) = 0;
  virtual long __stdcall UnlockRect(_D3DCUBEMAP_FACES, unsigned int) = 0;
  virtual long __stdcall AddDirtyRect(_D3DCUBEMAP_FACES, const RECT *) = 0;
};

struct IDirect3DVertexBuffer9 {
  virtual long __stdcall             QueryInterface(const GUID &, void **) = 0;
  virtual unsigned long __stdcall    AddRef() = 0;
  virtual unsigned long __stdcall    Release() = 0;
  virtual long __stdcall             GetDevice(IDirect3DDevice9 **) = 0;
  virtual long __stdcall             SetPrivateData(const GUID &, const void *, unsigned long, unsigned long) = 0;
  virtual long __stdcall             GetPrivateData(const GUID &, void *, unsigned long *) = 0;
  virtual long __stdcall             FreePrivateData(const GUID &) = 0;
  virtual unsigned long __stdcall    SetPriority(unsigned long) = 0;
  virtual unsigned long __stdcall    GetPriority() = 0;
  virtual void __stdcall             PreLoad() = 0;
  virtual _D3DRESOURCETYPE __stdcall GetType() = 0;
  virtual long __stdcall             Lock(unsigned int, unsigned int, void **, unsigned long) = 0;
  virtual long __stdcall             Unlock() = 0;
};

struct IDirect3DIndexBuffer9 {
  virtual long __stdcall             QueryInterface(const GUID &, void **) = 0;
  virtual unsigned long __stdcall    AddRef() = 0;
  virtual unsigned long __stdcall    Release() = 0;
  virtual long __stdcall             GetDevice(IDirect3DDevice9 **) = 0;
  virtual long __stdcall             SetPrivateData(const GUID &, const void *, unsigned long, unsigned long) = 0;
  virtual long __stdcall             GetPrivateData(const GUID &, void *, unsigned long *) = 0;
  virtual long __stdcall             FreePrivateData(const GUID &) = 0;
  virtual unsigned long __stdcall    SetPriority(unsigned long) = 0;
  virtual unsigned long __stdcall    GetPriority() = 0;
  virtual void __stdcall             PreLoad() = 0;
  virtual _D3DRESOURCETYPE __stdcall GetType() = 0;
  virtual long __stdcall             Lock(unsigned int, unsigned int, void **, unsigned long) = 0;
  virtual long __stdcall             Unlock() = 0;
};

struct IDirect3DDevice9 {
  virtual long __stdcall          QueryInterface(const GUID &, void **) = 0;
  virtual unsigned long __stdcall AddRef() = 0;
  virtual unsigned long __stdcall Release() = 0;
  virtual long __stdcall          TestCooperativeLevel() = 0;
  virtual unsigned int __stdcall  GetAvailableTextureMem() = 0;
  virtual long __stdcall          EvictManagedResources() = 0;
  virtual long __stdcall          GetDirect3D(IDirect3D9 **) = 0;
  virtual long __stdcall          GetDeviceCaps(D3DCAPS9 *) = 0;
  virtual long __stdcall          GetDisplayMode(unsigned int, D3DDISPLAYMODE *) = 0;
  virtual long __stdcall          GetCreationParameters(D3DDEVICE_CREATION_PARAMETERS *) = 0;
  virtual long __stdcall          SetCursorProperties(unsigned int, unsigned int, IDirect3DSurface9 *) = 0;
  virtual void __stdcall          SetCursorPosition(int, int, unsigned long) = 0;
  virtual int __stdcall           ShowCursor(int) = 0;
  virtual long __stdcall          CreateAdditionalSwapChain(D3DPRESENT_PARAMETERS *, IDirect3DSwapChain9 **) = 0;
  virtual long __stdcall          GetSwapChain(unsigned int, IDirect3DSwapChain9 **) = 0;
  virtual unsigned int __stdcall  GetNumberOfSwapChains() = 0;
  virtual long __stdcall          Reset(D3DPRESENT_PARAMETERS *) = 0;
  virtual long __stdcall          Present(const RECT *, const RECT *, HWND, const RGNDATA *) = 0;
  virtual long __stdcall          GetBackBuffer(unsigned int, unsigned int, _D3DBACKBUFFER_TYPE, IDirect3DSurface9 **) = 0;
  virtual long __stdcall          GetRasterStatus(unsigned int, D3DRASTER_STATUS *) = 0;
  virtual long __stdcall          SetDialogBoxMode(int) = 0;
  virtual void __stdcall          SetGammaRamp(unsigned int, unsigned long, const D3DGAMMARAMP *) = 0;
  virtual void __stdcall          GetGammaRamp(unsigned int, D3DGAMMARAMP *) = 0;
  virtual long __stdcall
  CreateTexture(unsigned int, unsigned int, unsigned int, unsigned long, unsigned int, unsigned int, IDirect3DTexture9 **, HANDLE *) = 0;
  virtual long __stdcall CreateVolumeTexture(
      unsigned int,
      unsigned int,
      unsigned int,
      unsigned int,
      unsigned long,
      unsigned int,
      unsigned int,
      IDirect3DVolumeTexture9 **,
      HANDLE *
  ) = 0;
  virtual long __stdcall
  CreateCubeTexture(unsigned int, unsigned int, unsigned long, unsigned int, unsigned int, IDirect3DCubeTexture9 **, HANDLE *) = 0;
  virtual long __stdcall CreateVertexBuffer(unsigned int, unsigned long, unsigned long, _D3DPOOL, IDirect3DVertexBuffer9 **, HANDLE *) = 0;
  virtual long __stdcall CreateIndexBuffer(unsigned int, unsigned long, _D3DFORMAT, _D3DPOOL, IDirect3DIndexBuffer9 **, HANDLE *) = 0;
  virtual long __stdcall
  CreateRenderTarget(unsigned int, unsigned int, unsigned int, unsigned int, unsigned long, int, IDirect3DSurface9 **, HANDLE *) = 0;
  virtual long __stdcall
  CreateDepthStencilSurface(unsigned int, unsigned int, unsigned int, unsigned int, unsigned long, int, IDirect3DSurface9 **, HANDLE *) = 0;
  virtual long __stdcall  UpdateSurface(IDirect3DSurface9 *, const RECT *, IDirect3DSurface9 *, const POINT *) = 0;
  virtual long __stdcall  UpdateTexture(IDirect3DBaseTexture9 *, IDirect3DBaseTexture9 *) = 0;
  virtual long __stdcall  GetRenderTargetData(IDirect3DSurface9 *, IDirect3DSurface9 *) = 0;
  virtual long __stdcall  GetFrontBufferData(unsigned int, IDirect3DSurface9 *) = 0;
  virtual long __stdcall  StretchRect(IDirect3DSurface9 *, const RECT *, IDirect3DSurface9 *, const RECT *, unsigned int) = 0;
  virtual long __stdcall  ColorFill(IDirect3DSurface9 *, const RECT *, unsigned long) = 0;
  virtual long __stdcall  CreateOffscreenPlainSurface(unsigned int, unsigned int, unsigned int, unsigned int, IDirect3DSurface9 **, HANDLE *) = 0;
  virtual long __stdcall  SetRenderTarget(unsigned long, IDirect3DSurface9 *) = 0;
  virtual long __stdcall  GetRenderTarget(unsigned long, IDirect3DSurface9 **) = 0;
  virtual long __stdcall  SetDepthStencilSurface(IDirect3DSurface9 *) = 0;
  virtual long __stdcall  GetDepthStencilSurface(IDirect3DSurface9 **) = 0;
  virtual long __stdcall  BeginScene() = 0;
  virtual long __stdcall  EndScene() = 0;
  virtual long __stdcall  Clear(unsigned long, const D3DRECT *, unsigned long, unsigned long, float, unsigned long) = 0;
  virtual long __stdcall  SetTransform(unsigned int, const D3DMATRIX *) = 0;
  virtual long __stdcall  GetTransform(unsigned int, D3DMATRIX *) = 0;
  virtual long __stdcall  MultiplyTransform(unsigned int, const D3DMATRIX *) = 0;
  virtual long __stdcall  SetViewport(const D3DVIEWPORT9 *) = 0;
  virtual long __stdcall  GetViewport(D3DVIEWPORT9 *) = 0;
  virtual long __stdcall  SetMaterial(const D3DMATERIAL9 *) = 0;
  virtual long __stdcall  GetMaterial(D3DMATERIAL9 *) = 0;
  virtual long __stdcall  SetLight(unsigned long, const _D3DLIGHT9 *) = 0;
  virtual long __stdcall  GetLight(unsigned long, _D3DLIGHT9 *) = 0;
  virtual long __stdcall  LightEnable(unsigned long, int) = 0;
  virtual long __stdcall  GetLightEnable(unsigned long, int *) = 0;
  virtual long __stdcall  SetClipPlane(unsigned long, const float *) = 0;
  virtual long __stdcall  GetClipPlane(unsigned long, float *) = 0;
  virtual long __stdcall  SetRenderState(D3DRENDERSTATETYPE, unsigned long) = 0;
  virtual long __stdcall  GetRenderState(D3DRENDERSTATETYPE, unsigned long *) = 0;
  virtual long __stdcall  CreateStateBlock(unsigned int, void **) = 0;
  virtual long __stdcall  BeginStateBlock() = 0;
  virtual long __stdcall  EndStateBlock(void **) = 0;
  virtual long __stdcall  SetClipStatus(const void *) = 0;
  virtual long __stdcall  GetClipStatus(void *) = 0;
  virtual long __stdcall  GetTexture(unsigned long, IDirect3DBaseTexture9 **) = 0;
  virtual long __stdcall  SetTexture(unsigned long, IDirect3DBaseTexture9 *) = 0;
  virtual long __stdcall  GetTextureStageState(unsigned long, unsigned int, unsigned long *) = 0;
  virtual long __stdcall  SetTextureStageState(unsigned long, unsigned int, unsigned long) = 0;
  virtual long __stdcall  GetSamplerState(unsigned long, unsigned int, unsigned long *) = 0;
  virtual long __stdcall  SetSamplerState(unsigned long, unsigned int, unsigned long) = 0;
  virtual long __stdcall  ValidateDevice(unsigned long *) = 0;
  virtual long __stdcall  SetPaletteEntries(unsigned int, const void *) = 0;
  virtual long __stdcall  GetPaletteEntries(unsigned int, void *) = 0;
  virtual long __stdcall  SetCurrentTexturePalette(unsigned int) = 0;
  virtual long __stdcall  GetCurrentTexturePalette(unsigned int *) = 0;
  virtual long __stdcall  SetScissorRect(const RECT *) = 0;
  virtual long __stdcall  GetScissorRect(RECT *) = 0;
  virtual long __stdcall  SetSoftwareVertexProcessing(int) = 0;
  virtual int __stdcall   GetSoftwareVertexProcessing() = 0;
  virtual long __stdcall  SetNPatchMode(float) = 0;
  virtual float __stdcall GetNPatchMode() = 0;
  virtual long __stdcall  DrawPrimitive(unsigned int, unsigned int, unsigned int) = 0;
  virtual long __stdcall  DrawIndexedPrimitive(unsigned int, int, unsigned int, unsigned int, unsigned int, unsigned int) = 0;
  virtual long __stdcall  DrawPrimitiveUP(unsigned int, unsigned int, const void *, unsigned int) = 0;
  virtual long __stdcall
  DrawIndexedPrimitiveUP(unsigned int, unsigned int, unsigned int, unsigned int, const void *, _D3DFORMAT, const void *, unsigned int) = 0;
  virtual long __stdcall ProcessVertices(unsigned int, unsigned int, unsigned int, IDirect3DVertexBuffer9 *, void *, unsigned long) = 0;
  virtual long __stdcall CreateVertexDeclaration(const void *, void **) = 0;
  virtual long __stdcall SetVertexDeclaration(void *) = 0;
  virtual long __stdcall GetVertexDeclaration(void **) = 0;
  virtual long __stdcall SetFVF(unsigned long) = 0;
  virtual long __stdcall GetFVF(unsigned long *) = 0;
  virtual long __stdcall CreateVertexShader(const unsigned long *, void **) = 0;
  virtual long __stdcall SetVertexShader(void *) = 0;
  virtual long __stdcall GetVertexShader(void **) = 0;
  virtual long __stdcall SetVertexShaderConstantF(unsigned int, const float *, unsigned int) = 0;
  virtual long __stdcall GetVertexShaderConstantF(unsigned int, float *, unsigned int) = 0;
  virtual long __stdcall SetVertexShaderConstantI(unsigned int, const int *, unsigned int) = 0;
  virtual long __stdcall GetVertexShaderConstantI(unsigned int, int *, unsigned int) = 0;
  virtual long __stdcall SetVertexShaderConstantB(unsigned int, const int *, unsigned int) = 0;
  virtual long __stdcall GetVertexShaderConstantB(unsigned int, int *, unsigned int) = 0;
  virtual long __stdcall SetStreamSource(unsigned int, IDirect3DVertexBuffer9 *, unsigned int, unsigned int) = 0;
  virtual long __stdcall GetStreamSource(unsigned int, IDirect3DVertexBuffer9 **, unsigned int *, unsigned int *) = 0;
  virtual long __stdcall SetStreamSourceFreq(unsigned int, unsigned int) = 0;
  virtual long __stdcall GetStreamSourceFreq(unsigned int, unsigned int *) = 0;
  virtual long __stdcall SetIndices(IDirect3DIndexBuffer9 *) = 0;
  virtual long __stdcall GetIndices(IDirect3DIndexBuffer9 **) = 0;
  virtual long __stdcall CreatePixelShader(const unsigned long *, IDirect3DPixelShader9 **) = 0;
  virtual long __stdcall SetPixelShader(IDirect3DPixelShader9 *) = 0;
  virtual long __stdcall GetPixelShader(IDirect3DPixelShader9 **) = 0;
  virtual long __stdcall SetPixelShaderConstantF(unsigned int, const float *, unsigned int) = 0;
  virtual long __stdcall GetPixelShaderConstantF(unsigned int, float *, unsigned int) = 0;
  virtual long __stdcall SetPixelShaderConstantI(unsigned int, const int *, unsigned int) = 0;
  virtual long __stdcall GetPixelShaderConstantI(unsigned int, int *, unsigned int) = 0;
  virtual long __stdcall SetPixelShaderConstantB(unsigned int, const int *, unsigned int) = 0;
  virtual long __stdcall GetPixelShaderConstantB(unsigned int, int *, unsigned int) = 0;
};

#pragma pack(pop)

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
