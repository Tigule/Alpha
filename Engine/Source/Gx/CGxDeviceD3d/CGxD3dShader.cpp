#include "CGxDeviceD3d.h"

void CGxDeviceD3d::IShaderForceRecreation(int freeShaders) {
  if (!freeShaders) {
    return;
  }

  {
    ITERATELIST(CGxPixelShader, m_pixelShaderList, pixelShader) {
      if (pixelShader->apiSpecific) {
        reinterpret_cast<IDirect3DPixelShader9 *>(pixelShader->apiSpecific)->Release();
        pixelShader->apiSpecific = 0;
      }
    }
  }

  {
    ITERATELIST(CGxVertexShader, m_vertexShaderList, vertexShader) {
    }
  }
}

void CGxDeviceD3d::IPixelShaderCreate(CGxPixelShader *ps) {
  ID3DXBuffer *buffer = 0;

  if (D3DXAssembleShader(reinterpret_cast<const char *>(ps->code.Ptr()), ps->code.Count(), 0, 0, 0, &buffer, 0) == 0) {
    IDirect3DPixelShader9 *shader = 0;
    if (m_d3dDevice->CreatePixelShader(static_cast<const unsigned long *>(buffer->GetBufferPointer()), &shader) == 0) {
      ps->apiSpecific = reinterpret_cast<unsigned int>(shader);
      ps->valid = 1;
    }
  }

  if (buffer) {
    buffer->Release();
  }
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
  ITERATELIST(CGxShaderParam, params, param) {
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

    m_d3dDevice->SetPixelShader(reinterpret_cast<IDirect3DPixelShader9 *>(ps->apiSpecific));
    ISetShaderParameters(ps, 1);
  } else {
    m_d3dDevice->SetPixelShader(0);
  }
}

void CGxDeviceD3d::IBindVertexShader(CGxVertexShader *vs) {
}
