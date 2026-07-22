#include "CGxDeviceD3d.h"

struct ID3DXBuffer {
  virtual long __stdcall          QueryInterface(const GUID &, void **) = 0;
  virtual unsigned long __stdcall AddRef() = 0;
  virtual unsigned long __stdcall Release() = 0;
  virtual void *__stdcall         GetBufferPointer() = 0;
  virtual unsigned long __stdcall GetBufferSize() = 0;
};

typedef long(__stdcall *D3DXAssembleShaderProc)(const char *, unsigned int, const void *, void *, unsigned long, ID3DXBuffer **, ID3DXBuffer **);

void CGxDeviceD3d::IShaderForceRecreation(int freeShaders) {
  if (!freeShaders) {
    return;
  }

  CGxPixelShader *pixelShader = m_pixelShaderList.Head();
  while (pixelShader) {
    if (pixelShader->apiSpecific) {
      reinterpret_cast<IDirect3DPixelShader9 *>(pixelShader->apiSpecific)->Release();
      pixelShader->apiSpecific = 0;
    }
    pixelShader = m_pixelShaderList.Next(pixelShader);
  }

  CGxVertexShader *vertexShader = m_vertexShaderList.Head();
  while (vertexShader) {
    vertexShader = m_vertexShaderList.Next(vertexShader);
  }
}

void CGxDeviceD3d::IPixelShaderCreate(CGxPixelShader *ps) {
  HINSTANCE d3dxLib = LoadLibraryA("d3dx9_24.dll");
  if (!d3dxLib) {
    return;
  }

  D3DXAssembleShaderProc assembleShader = reinterpret_cast<D3DXAssembleShaderProc>(GetProcAddress(d3dxLib, "D3DXAssembleShader"));
  ID3DXBuffer           *buffer = 0;
  IDirect3DPixelShader9 *shader = 0;

  if (assembleShader && assembleShader(reinterpret_cast<const char *>(ps->code.Ptr()), ps->code.Count(), 0, 0, 0, &buffer, 0) >= 0 &&
      m_d3dDevice->CreatePixelShader(static_cast<const unsigned long *>(buffer->GetBufferPointer()), &shader) >= 0)
  {
    ps->apiSpecific = reinterpret_cast<unsigned int>(shader);
    ps->valid = 1;
  }

  if (buffer) {
    buffer->Release();
  }
  FreeLibrary(d3dxLib);
}

void CGxDeviceD3d::PixelShaderCreate(CGxPixelShader *&ps, const char *filename) {
  CGxDevice::PixelShaderCreate(ps, filename);
  IPixelShaderCreate(ps);
}

void CGxDeviceD3d::PixelShaderDestroy(CGxPixelShader *&ps) {
  if (ps->refCount == 1 && ps->apiSpecific) {
    reinterpret_cast<IDirect3DPixelShader9 *>(ps->apiSpecific)->Release();
    ps->apiSpecific = 0;
  }

  CGxDevice::PixelShaderDestroy(ps);
}

void CGxDeviceD3d::ISetShaderParamList(TSExplicitList<CGxShaderParam, 108> &params, int forceForBind) {
  for (CGxShaderParam *param = params.Head(); param; param = params.Next(param)) {
    if (param->dirty || forceForBind) {
      m_d3dDevice->SetPixelShaderConstantF(param->index, param->f, CGxShaderParam::TypeCountTable[param->type]);
      param->dirty = 0;
    }
  }
}

void CGxDeviceD3d::IBindPixelShader(CGxPixelShader *ps) {
  if (ps && ps->valid) {
    if (!ps->apiSpecific) {
      IPixelShaderCreate(ps);
    }

    if (ps->apiSpecific) {
      m_d3dDevice->SetPixelShader(reinterpret_cast<IDirect3DPixelShader9 *>(ps->apiSpecific));
      ISetShaderParameters(ps, 1);
    } else {
      m_d3dDevice->SetPixelShader(0);
    }
  } else {
    m_d3dDevice->SetPixelShader(0);
  }
}

void CGxDeviceD3d::IBindVertexShader(CGxVertexShader *vs) {
}
