#include "CGxDeviceD3d.h"

static const float   MaxLightRange = 10000.0f;
static const float   oo255 = 1.0f / 255.0f;
static _D3DLIGHT9    d3dLight;
static _D3DMATERIAL9 mat;

static _D3DBLEND             s_srcBlend[] = {D3DBLEND_ONE,       D3DBLEND_ONE,       D3DBLEND_SRCALPHA,  D3DBLEND_SRCALPHA,
                                             D3DBLEND_DESTCOLOR, D3DBLEND_DESTCOLOR, D3DBLEND_DESTCOLOR, D3DBLEND_INVSRCALPHA};
static _D3DBLEND             s_dstBlend[] = {D3DBLEND_ZERO, D3DBLEND_ZERO,     D3DBLEND_INVSRCALPHA, D3DBLEND_ONE,
                                             D3DBLEND_ZERO, D3DBLEND_SRCCOLOR, D3DBLEND_ONE,         D3DBLEND_ONE};
static _D3DFOGMODE           s_fogStyle[] = {D3DFOG_LINEAR, D3DFOG_EXP, D3DFOG_EXP2};
static _D3DCMPFUNC           s_cmpFunc[] = {D3DCMP_LESSEQUAL, D3DCMP_EQUAL, D3DCMP_GREATEREQUAL};
static _D3DCULL              s_cullmode[] = {D3DCULL_NONE, D3DCULL_CW};
static _D3DTEXTUREFILTERTYPE s_filterModes[][3] = {
    { D3DTEXF_POINT,       D3DTEXF_POINT,   D3DTEXF_NONE},
    {D3DTEXF_LINEAR,      D3DTEXF_LINEAR,   D3DTEXF_NONE},
    {D3DTEXF_LINEAR,      D3DTEXF_LINEAR,  D3DTEXF_POINT},
    {D3DTEXF_LINEAR,      D3DTEXF_LINEAR, D3DTEXF_LINEAR},
    {D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR}
};
static _D3DTEXTUREADDRESS s_wrapModes[] = {D3DTADDRESS_CLAMP, D3DTADDRESS_WRAP};
static _D3DTEXTUREOP      s_texColorOps[] = {D3DTOP_SELECTARG1, D3DTOP_MODULATE, D3DTOP_BLENDTEXTUREALPHA, D3DTOP_ADD, D3DTOP_MODULATE2X};
static _D3DTEXTUREOP      s_texAlphaOps[] = {D3DTOP_SELECTARG1, D3DTOP_MODULATE, static_cast<_D3DTEXTUREOP>(3), D3DTOP_ADD, D3DTOP_MODULATE2X};

static void SetD3dColor(_D3DCOLORVALUE &dst, const NTempest::CImVector &src, float scale);

static void SetD3dColor(_D3DCOLORVALUE &dst, const NTempest::CImVector &src, float scale) {
  dst.r = src.r * scale;
  dst.g = src.g * scale;
  dst.b = src.b * scale;
  dst.a = src.a * scale;
}

void CGxDeviceD3d::IStateSync() {
  IStateSyncLights();
  IStateSyncEnables();
  IStateSyncMaterial();
  IRsSync(0);
  IStateSyncTransforms();

  if ((rand() & 0xFFF) != 0 || m_d3dDevice->TestCooperativeLevel() != 0) {
    return;
  }

  unsigned long numPasses;
  long          result = m_d3dDevice->ValidateDevice(&numPasses);
  if (result == 0 && numPasses == 1) {
    return;
  }

  DbgPrintf("Gx: (WARN): Unsupported pipeline config expect graphical goo\n");

  switch (result) {
    case E_FAIL:
      break;
    case D3DERR_WRONGTEXTUREFORMAT:
      DbgPrintf("\tThe pixel format of the texture surface is not valid.\n");
      break;
    case D3DERR_UNSUPPORTEDCOLOROPERATION:
      DbgPrintf("\tThe device does not support a specified texture-blending operation for color values.\n");
      break;
    case D3DERR_UNSUPPORTEDCOLORARG:
      DbgPrintf("\tThe device does not support a specified texture-blending argument for color values.\n");
      break;
    case D3DERR_UNSUPPORTEDALPHAOPERATION:
      DbgPrintf("\tThe device does not support a specified texture-blending operation for the alpha channel.\n");
      break;
    case D3DERR_UNSUPPORTEDALPHAARG:
      DbgPrintf("\tThe device does not support a specified texture-blending argument for the alpha channel.\n");
      break;
    case D3DERR_TOOMANYOPERATIONS:
      DbgPrintf("\tThe application is requesting more texture-filtering operations than the device supports.\n");
      break;
    case D3DERR_CONFLICTINGTEXTUREFILTER:
      DbgPrintf("\tThe current texture filters cannot be used together.\n");
      break;
    case D3DERR_UNSUPPORTEDFACTORVALUE:
      DbgPrintf("\tThe device does not support the specified texture factor value.\n");
      break;
    case D3DERR_UNSUPPORTEDTEXTUREFILTER:
      DbgPrintf("\tThe device does not support the specified texture filter.\n");
      break;
    case D3DERR_DRIVERINTERNALERROR:
      DbgPrintf(
          "\tInternal driver error. Applications should generally shut down when receiving this error. For more information, see Driver Internal "
          "Errors.\n"
      );
      break;
    case D3DERR_DEVICELOST:
      DbgPrintf("\tThe device has been lost but cannot be reset at this time. Therefore, rendering is not possible.\n");
      break;
    default:
      DbgPrintf("\tUnknown\n");
      break;
  }
}

void CGxDeviceD3d::IStateSyncLights() {
  for (unsigned int whichLight = 0; whichLight < 8; ++whichLight) {
    CGxLight &light = m_appState.m_lights[whichLight];

    d3dLight.Type = light.m_isOmni ? D3DLIGHT_POINT : D3DLIGHT_DIRECTIONAL;
    SetD3dColor(d3dLight.Diffuse, light.m_dirColor, oo255 * light.m_dirIntensity);
    SetD3dColor(d3dLight.Ambient, light.m_ambColor, oo255 * light.m_ambIntensity);
    SetD3dColor(d3dLight.Specular, light.m_specColor, oo255 * light.m_specIntensity);

    if (light.m_isOmni) {
      d3dLight.Position.x = light.m_dir.x;
      d3dLight.Position.y = light.m_dir.y;
      d3dLight.Position.z = light.m_dir.z;
    } else {
      NTempest::C3Vector tmp = light.m_dir;
      tmp.Normalize();
      d3dLight.Direction.x = tmp.x;
      d3dLight.Direction.y = tmp.y;
      d3dLight.Direction.z = tmp.z;
    }

    d3dLight.Range = MaxLightRange;
    d3dLight.Falloff = 1.0f;
    d3dLight.Attenuation0 = light.m_constantAttenuation;
    d3dLight.Attenuation1 = light.m_linearAttenuation;
    d3dLight.Attenuation2 = light.m_quadraticAttenuation;

    ISetLight(whichLight, d3dLight, light.m_enabled);
    m_hwState.m_lights[whichLight] = light;
  }
}

void CGxDeviceD3d::IStateSyncEnables() {
  unsigned long app = m_appState.m_masterEnables;
  unsigned long hw = m_hwState.m_masterEnables;

  if (app != hw) {
    int enable;
    if (NeedsUpdate(app, hw, 0, 0, 8, enable)) {
      m_d3dDevice->SetRenderState(D3DRS_FILLMODE, (enable != 0) + 2);
    }

    m_hwState.m_masterEnables = m_appState.m_masterEnables;
  }
}

void CGxDeviceD3d::IStateSyncMaterial() {
  switch (m_vertexBufferFormat) {
    case GxVBF_PN:
    case GxVBF_PNT0:
    case GxVBF_PNT0T1:
      DsSet(Ds_DiffuseMaterialSource, 0);
      DsSet(Ds_AmbientMaterialSource, 0);
      break;
    case GxVBF_PNC:
    case GxVBF_PNCT0:
    case GxVBF_PNCT0T1:
    case GxVBF_PCT0:
    case GxVBF_PC:
    case GxVBF_PT0T1:
      DsSet(Ds_DiffuseMaterialSource, 1);
      DsSet(Ds_AmbientMaterialSource, 1);
      break;
    default:
      ASSERT(0);
      break;
  }
}

void CGxDeviceD3d::IStateSyncTransforms() {
  if (m_xforms[GxXform_World].m_dirty) {
    IXformSetWorld();
  }

  for (unsigned int tmu = 0; tmu < m_caps.m_numTmus; ++tmu) {
    int texture = 0;
    RsGet(static_cast<EGxRenderState>(GxRs_Texture0 + tmu), texture);
    if (texture && (m_xforms[tmu].m_dirty || m_texGen[tmu].m_dirty)) {
      IXformSetTex(tmu);
    }
  }
}

void CGxDeviceD3d::IStateSetD3DDefaults() {
  m_d3dDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
  m_d3dDevice->SetRenderState(D3DRS_LOCALVIEWER, TRUE);
  m_d3dDevice->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);

  for (unsigned int whichLight = 0; whichLight < 8; ++whichLight) {
    CGxLight  &light = m_hwState.m_lights[whichLight];
    _D3DLIGHT9 d3dLight;
    memset(&d3dLight, 0, sizeof(d3dLight));

    d3dLight.Type = light.m_isOmni ? D3DLIGHT_POINT : D3DLIGHT_DIRECTIONAL;
    SetD3dColor(d3dLight.Diffuse, light.m_dirColor, oo255 * light.m_dirIntensity);
    SetD3dColor(d3dLight.Ambient, light.m_ambColor, oo255 * light.m_ambIntensity);
    SetD3dColor(d3dLight.Specular, light.m_specColor, oo255 * light.m_specIntensity);

    if (light.m_isOmni) {
      d3dLight.Position.x = light.m_dir.x;
      d3dLight.Position.y = light.m_dir.y;
      d3dLight.Position.z = light.m_dir.z;
    } else {
      NTempest::C3Vector tmp = light.m_dir;
      tmp.Normalize();
      d3dLight.Direction.x = tmp.x;
      d3dLight.Direction.y = tmp.y;
      d3dLight.Direction.z = tmp.z;
    }

    d3dLight.Range = MaxLightRange;
    d3dLight.Falloff = 1.0f;
    d3dLight.Attenuation0 = 1.0f;
    d3dLight.Attenuation1 = m_hwState.m_lightLinearFalloff;
    d3dLight.Attenuation2 = m_hwState.m_lightQuadraticFalloff;

    ISetLight(whichLight, d3dLight, light.m_enabled);
    m_hwState.m_lightsDirty[whichLight] = 1;
  }

  IForceLights();
  IRsForceUpdate();
  IRsSync(0);

  if (SUCCEEDED(m_d3dDevice->GetRenderTarget(0, &m_defColorSurface)) && SUCCEEDED(m_d3dDevice->GetDepthStencilSurface(&m_defDepthSurface))) {
    ISceneBegin(3);
  }
}

void CGxDeviceD3d::ISetLight(unsigned long which, const _D3DLIGHT9 &value, int enabled) {
  StateD3dLight &state = m_d3dStatesLight[which];
  int            force = state.which == static_cast<unsigned long>(-1);
  unsigned int   chkSum = 0;

  if (!force) {
    const unsigned long *data = reinterpret_cast<const unsigned long *>(&value);
    unsigned int         count = sizeof(value) / sizeof(unsigned long);
    while (count) {
      chkSum += (--count ^ *data++);
    }
  }

  if (force || chkSum != state.chkSum) {
    m_d3dDevice->SetLight(which, &value);
    memcpy(&state.val, &value, sizeof(state.val));

    const unsigned long *data = reinterpret_cast<const unsigned long *>(&state.val);
    unsigned int         count = sizeof(state.val) / sizeof(unsigned long);
    state.chkSum = 0;
    while (count) {
      state.chkSum += (--count ^ *data++);
    }
  }

  if (force || state.enabled != enabled) {
    m_d3dDevice->LightEnable(which, enabled);
    state.enabled = enabled;
  }

  state.which = which;
}

void CGxDeviceD3d::IForceLights() {
  for (unsigned int i = 0; i < 8; ++i) {
    StateD3dLight &state = m_d3dStatesLight[i];
    if (state.which != static_cast<unsigned long>(-1)) {
      m_d3dDevice->SetLight(state.which, &state.val);
      m_d3dDevice->LightEnable(state.which, state.enabled);
    }
  }
}

void CGxDeviceD3d::DsSet(EDeviceState state, unsigned long val) {
  ASSERT(state < DeviceStates_Last);
  if (m_deviceState[state] == val) {
    return;
  }

  if (state == Ds_SrcBlend) {
    m_d3dDevice->SetRenderState(D3DRS_SRCBLEND, val);
  } else if (state == Ds_DstBlend) {
    m_d3dDevice->SetRenderState(D3DRS_DESTBLEND, val);
  } else if (state >= Ds_TssMagFilter0 && state <= Ds_TssMagFilter3) {
    m_d3dDevice->SetSamplerState(state - Ds_TssMagFilter0, D3DSAMP_MAGFILTER, val);
  } else if (state >= Ds_TssMinFilter0 && state <= Ds_TssMinFilter3) {
    m_d3dDevice->SetSamplerState(state - Ds_TssMinFilter0, D3DSAMP_MINFILTER, val);
  } else if (state >= Ds_TssMipFilter0 && state <= Ds_TssMipFilter3) {
    m_d3dDevice->SetSamplerState(state - Ds_TssMipFilter0, D3DSAMP_MIPFILTER, val);
  } else if (state >= Ds_TssWrapU0 && state <= Ds_TssWrapU3) {
    m_d3dDevice->SetSamplerState(state - Ds_TssWrapU0, D3DSAMP_ADDRESSU, val);
  } else if (state >= Ds_TssWrapV0 && state <= Ds_TssWrapV3) {
    m_d3dDevice->SetSamplerState(state - Ds_TssWrapV0, D3DSAMP_ADDRESSV, val);
  } else if (state >= Ds_TssTTF0 && state <= Ds_TssTTF3) {
    m_d3dDevice->SetTextureStageState(state - Ds_TssTTF0, D3DTSS_TEXTURETRANSFORMFLAGS, val);
  } else if (state >= Ds_TssMaxAnisotropy0 && state <= Ds_TssMaxAnisotropy3) {
    m_d3dDevice->SetSamplerState(state - Ds_TssMaxAnisotropy0, D3DSAMP_MAXANISOTROPY, val);
  } else if (state == Ds_DiffuseMaterialSource) {
    m_d3dDevice->SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, val);
  } else if (state == Ds_AmbientMaterialSource) {
    m_d3dDevice->SetRenderState(D3DRS_AMBIENTMATERIALSOURCE, val);
  } else if (state == Ds_AlphaBlendEnable) {
    m_d3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, val);
  } else if (state == Ds_AlphaTestEnable) {
    m_d3dDevice->SetRenderState(D3DRS_ALPHATESTENABLE, val);
  } else {
    ASSERT(0);
  }

  m_deviceState[state] = val;
}

void CGxDeviceD3d::ISetTexture(unsigned int tmu, CGxTex *tex) {
  if (tmu >= m_caps.m_numTmus) {
    return;
  }

  if (tex) {
    ASSERT(!(tex->m_flags.m_renderTarget && tex->m_needsUpdate));
    ITexBind(tex);
    ITexMarkAsUpdated(tex);
    _D3DTEXTUREFILTERTYPE *filter = s_filterModes[tex->m_flags.m_filter];
    long                   result = m_d3dDevice->SetTexture(tmu, reinterpret_cast<IDirect3DBaseTexture9 *>(tex->m_apiSpecificData));
    ASSERT(result == 0);

    DsSet(static_cast<EDeviceState>(Ds_TssMagFilter0 + tmu), filter[0]);
    DsSet(static_cast<EDeviceState>(Ds_TssMinFilter0 + tmu), filter[1]);
    DsSet(static_cast<EDeviceState>(Ds_TssMipFilter0 + tmu), filter[2]);
    DsSet(static_cast<EDeviceState>(Ds_TssWrapU0 + tmu), s_wrapModes[tex->m_flags.m_wrapU]);
    DsSet(static_cast<EDeviceState>(Ds_TssWrapV0 + tmu), s_wrapModes[tex->m_flags.m_wrapV]);
    DsSet(static_cast<EDeviceState>(Ds_TssMaxAnisotropy0 + tmu), tex->m_flags.m_maxAnisotropy);

    if (!m_texEnable[tmu]) {
      m_texEnable[tmu] = 1;
      unsigned int blendState = GxRs_TexBlend0 + tmu;
      ISetTexBlend(tmu, static_cast<EGxTexBlend>(*reinterpret_cast<unsigned int *>(&mAppRenderStates[blendState].mValue)));
      mAppRenderStates[blendState].mDirty = 0;
    }
  } else {
    m_d3dDevice->SetTexture(tmu, 0);
    if (m_texEnable[tmu]) {
      m_texEnable[tmu] = 0;
      m_d3dDevice->SetTextureStageState(tmu, D3DTSS_COLOROP, D3DTOP_DISABLE);
      m_d3dDevice->SetTextureStageState(tmu, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
    }
  }
}

void CGxDeviceD3d::ISetTexGen(unsigned int tmu, EGxTexGen texGen) {
  if (tmu >= m_caps.m_numTmus) {
    return;
  }

  switch (texGen) {
    case GxTexGen_Disable:
      m_d3dDevice->SetTextureStageState(tmu, D3DTSS_TEXCOORDINDEX, tmu);
      m_texGen[tmu].Top() = NTempest::C44Matrix();
      return;
    case GxTexGen_Object:
    case GxTexGen_World:
    case GxTexGen_View:
      m_d3dDevice->SetTextureStageState(tmu, D3DTSS_TEXCOORDINDEX, tmu | 0x20000);
      break;
    case GxTexGen_ViewReflection:
    case GxTexGen_SphereMap:
      m_d3dDevice->SetTextureStageState(tmu, D3DTSS_TEXCOORDINDEX, tmu | 0x30000);
      break;
    case GxTexGen_ViewNormal:
      m_d3dDevice->SetTextureStageState(tmu, D3DTSS_TEXCOORDINDEX, tmu | 0x10000);
      m_texGen[tmu].Top() = NTempest::C44Matrix();
      return;
    default:
      ASSERT(0);
      break;
  }

  if (texGen == GxTexGen_Object || texGen == GxTexGen_World) {
    NTempest::C44Matrix texMat = m_xforms[GxXform_View].Get();
    float               b0 = texMat.b0;
    float               c0 = texMat.c0;
    float               c1 = texMat.c1;
    texMat.b0 = texMat.a1;
    texMat.c0 = texMat.a2;
    texMat.a1 = b0;
    texMat.c1 = texMat.b2;
    texMat.a2 = c0;
    texMat.b2 = c1;
    texMat.d0 = -texMat.d0;
    texMat.d1 = -texMat.d1;
    texMat.d2 = -texMat.d2;
    if (texGen == GxTexGen_Object) {
      const NTempest::C44Matrix &world = m_xforms[GxXform_World].Get();
      NTempest::C44Matrix        mat = world.Inverse(world.Determinant());
      texMat = texMat * mat;
    }
    m_texGen[tmu].Top() = texMat;
  } else if (texGen == GxTexGen_SphereMap) {
    NTempest::C44Matrix mat;
    mat.a0 = 0.5f;
    mat.b1 = 0.5f;
    mat.d0 = 0.5f;
    mat.d1 = 0.5f;
    m_texGen[tmu].Top() = mat;
  } else {
    m_texGen[tmu].Top() = NTempest::C44Matrix();
  }
}

void CGxDeviceD3d::ISetTexLodBias(unsigned int tmu, float bias) {
  if (m_caps.m_mipMapLodBias && tmu < m_caps.m_numTmus) {
    m_d3dDevice->SetSamplerState(tmu, D3DSAMP_MIPMAPLODBIAS, *reinterpret_cast<unsigned int *>(&bias));
  }
}

void CGxDeviceD3d::ISetTexBlend(unsigned int tmu, EGxTexBlend blend) {
  if (tmu < m_caps.m_numTmus && m_texEnable[tmu]) {
    m_d3dDevice->SetTextureStageState(tmu, D3DTSS_COLOROP, s_texColorOps[blend]);
    m_d3dDevice->SetTextureStageState(tmu, D3DTSS_ALPHAOP, s_texAlphaOps[blend]);
  }
}

void CGxDeviceD3d::IRsSendToHw(EGxRenderState which) {
  CGxAppRenderState &state = mAppRenderStates[which];
  unsigned int       value = *reinterpret_cast<unsigned int *>(&state.mValue);

  switch (which) {
    case GxRs_PolygonOffset:
      if (m_caps.m_depthBias) {
        m_d3dDevice->SetRenderState(D3DRS_DEPTHBIAS, static_cast<unsigned int>(*reinterpret_cast<float *>(&state.mValue) * 16.0f));
      }
      break;
    case GxRs_MatDiffuse:
    case GxRs_MatEmissive:
    case GxRs_MatSpecular:
    case GxRs_MatSpecularExp: {
      SetD3dColor(mat.Diffuse, *reinterpret_cast<NTempest::CImVector *>(&mAppRenderStates[GxRs_MatDiffuse].mValue), oo255);
      SetD3dColor(mat.Emissive, *reinterpret_cast<NTempest::CImVector *>(&mAppRenderStates[GxRs_MatEmissive].mValue), oo255);
      SetD3dColor(mat.Specular, *reinterpret_cast<NTempest::CImVector *>(&mAppRenderStates[GxRs_MatSpecular].mValue), oo255);
      mat.Ambient = mat.Diffuse;
      mat.Power = *reinterpret_cast<float *>(&mAppRenderStates[GxRs_MatSpecularExp].mValue);
      m_d3dDevice->SetMaterial(&mat);
      mAppRenderStates[GxRs_MatDiffuse].mDirty = 0;
      mAppRenderStates[GxRs_MatEmissive].mDirty = 0;
      mAppRenderStates[GxRs_MatSpecular].mDirty = 0;
      mAppRenderStates[GxRs_MatSpecularExp].mDirty = 0;
      break;
    }
    case GxRs_NormalizeNormals:
      m_d3dDevice->SetRenderState(D3DRS_NORMALIZENORMALS, value != 0);
      break;
    case GxRs_SceneAmbient:
      m_d3dDevice->SetRenderState(D3DRS_AMBIENT, value);
      break;
    case GxRs_Blend:
      if (value == GxBlend_Opaque || value == GxBlend_AlphaKey) {
        DsSet(Ds_AlphaBlendEnable, 0);
      } else {
        DsSet(Ds_AlphaBlendEnable, 1);
        DsSet(Ds_SrcBlend, s_srcBlend[value]);
        DsSet(Ds_DstBlend, s_dstBlend[value]);
      }
      break;
    case GxRs_AlphaRef:
      if (static_cast<int>(value) <= 0) {
        DsSet(Ds_AlphaTestEnable, 0);
      } else {
        m_d3dDevice->SetRenderState(D3DRS_ALPHAREF, value);
        DsSet(Ds_AlphaTestEnable, 1);
      }
      break;
    case GxRs_FogStyle:
      if ((m_d3dCaps.RasterCaps & D3DPRASTERCAPS_FOGTABLE) && (m_d3dCaps.RasterCaps & D3DPRASTERCAPS_WFOG)) {
        m_d3dDevice->SetRenderState(D3DRS_FOGTABLEMODE, s_fogStyle[value]);
      } else {
        m_d3dDevice->SetRenderState(D3DRS_FOGVERTEXMODE, s_fogStyle[value]);
      }
      break;
    case GxRs_FogStart:
      m_d3dDevice->SetRenderState(D3DRS_FOGSTART, value);
      break;
    case GxRs_FogEnd:
      m_d3dDevice->SetRenderState(D3DRS_FOGEND, value);
      break;
    case GxRs_FogDensity:
      m_d3dDevice->SetRenderState(D3DRS_FOGDENSITY, value);
      break;
    case GxRs_FogColor:
      m_d3dDevice->SetRenderState(D3DRS_FOGCOLOR, value);
      break;
    case GxRs_Lighting:
      m_d3dDevice->SetRenderState(D3DRS_LIGHTING, (m_appState.m_masterEnables & 1) && value != 0);
      break;
    case GxRs_Fog:
      m_d3dDevice->SetRenderState(D3DRS_FOGENABLE, (m_appState.m_masterEnables & 2) && value != 0);
      break;
    case GxRs_DepthTest:
    case GxRs_DepthFunc:
      if ((m_appState.m_masterEnables & 4) && *reinterpret_cast<unsigned int *>(&mAppRenderStates[GxRs_DepthTest].mValue)) {
        m_d3dDevice->SetRenderState(D3DRS_ZFUNC, s_cmpFunc[*reinterpret_cast<unsigned int *>(&mAppRenderStates[GxRs_DepthFunc].mValue)]);
      } else {
        m_d3dDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);
      }
      mAppRenderStates[GxRs_DepthTest].mDirty = 0;
      mAppRenderStates[GxRs_DepthFunc].mDirty = 0;
      break;
    case GxRs_DepthWrite:
      m_d3dDevice->SetRenderState(D3DRS_ZWRITEENABLE, (m_appState.m_masterEnables & 8) && value != 0);
      break;
    case GxRs_Culling:
      m_d3dDevice->SetRenderState(D3DRS_CULLMODE, s_cullmode[(m_appState.m_masterEnables & 0x10) && value != 0]);
      break;
    case GxRs_Texture0:
    case GxRs_Texture1:
    case GxRs_Texture2:
    case GxRs_Texture3:
      ISetTexture(which - GxRs_Texture0, reinterpret_cast<CGxTex *>(value));
      break;
    case GxRs_TexBlend0:
    case GxRs_TexBlend1:
    case GxRs_TexBlend2:
    case GxRs_TexBlend3:
      ISetTexBlend(which - GxRs_TexBlend0, static_cast<EGxTexBlend>(value));
      break;
    case GxRs_TexLodBias0:
    case GxRs_TexLodBias1:
    case GxRs_TexLodBias2:
    case GxRs_TexLodBias3:
      ISetTexLodBias(which - GxRs_TexLodBias0, *reinterpret_cast<float *>(&state.mValue));
      break;
    case GxRs_TexGen0:
    case GxRs_TexGen1:
    case GxRs_TexGen2:
    case GxRs_TexGen3:
      ISetTexGen(which - GxRs_TexGen0, static_cast<EGxTexGen>(value));
      break;
    case GxRs_TextureShader0:
    case GxRs_TextureShader1:
    case GxRs_TextureShader2:
    case GxRs_TextureShader3:
      return;
    case GxRs_PixelShader:
      IBindPixelShader(reinterpret_cast<CGxPixelShader *>(value));
      break;
    case GxRs_VertexShader:
      IBindVertexShader(reinterpret_cast<CGxVertexShader *>(value));
      break;
    default:
      ASSERT(0);
      break;
  }
}
