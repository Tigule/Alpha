#include "CGxDeviceOpenGl.h"
#include "GlExtSupport.h"

#include <storm.h>

#include <gl/gl.h>
#include <Tempest/c4vector.h>
#include <string.h>

static const float oo255 = 1.0f / 255.0f;

static UINT s_glSrcBlend[8] = {GL_ONE, GL_ONE, GL_SRC_ALPHA, GL_SRC_ALPHA, GL_DST_COLOR, GL_DST_COLOR, GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA};
static UINT s_glDstBlend[8] = {GL_ZERO, GL_ZERO, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO, GL_SRC_COLOR, GL_ONE, GL_ONE};
static int  s_texEnv[5] = {GL_REPLACE, GL_MODULATE, GL_DECAL, GL_ADD, -1};
static UINT s_fogStyle[3] = {GL_LINEAR, GL_EXP, GL_EXP2};
static UINT s_cmpFunc[3] = {GL_LEQUAL, GL_EQUAL, GL_GEQUAL};

void CGxDeviceOpenGl::DsInit() {
  memset(m_deviceState, 0, sizeof(m_deviceState));
  m_deviceState[Ds_DepthMask] = 1;
}

void CGxDeviceOpenGl::DsSet(EDeviceState which, UINT newVal, int force) {
  ASSERT(which < DeviceStates_Last);

  UINT oldValue = DsGet(which);
  if (oldValue == newVal && !force) {
    return;
  }

  switch (which) {
    case Ds_DepthMask:
      glDepthMask(newVal ? GL_TRUE : GL_FALSE);
      break;

    case Ds_ActiveTexture:
      ASSERT(newVal <= m_caps.m_numTmus);
      if (m_caps.m_numTmus > 1) {
        glActiveTextureARB(GL_TEXTURE0_ARB + newVal);
        glClientActiveTextureARB(GL_TEXTURE0_ARB + newVal);
      }
      break;

    case Ds_TexTarget0:
    case Ds_TexTarget1:
    case Ds_TexTarget2:
    case Ds_TexTarget3:
      ASSERT(static_cast<UINT>(which - Ds_TexTarget0) == DsGet(Ds_ActiveTexture));
      if (oldValue) {
        glDisable(oldValue);
      }
      if (newVal) {
        glEnable(newVal);
      }
      break;

    case Ds_TexGenS0:
    case Ds_TexGenS1:
    case Ds_TexGenS2:
    case Ds_TexGenS3:
      ASSERT(static_cast<UINT>(which - Ds_TexGenS0) == DsGet(Ds_ActiveTexture));
      if (newVal) {
        glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, newVal);
      } else {
        glDisable(GL_TEXTURE_GEN_S);
      }
      if (!oldValue) {
        glEnable(GL_TEXTURE_GEN_S);
      }
      break;

    case Ds_TexGenT0:
    case Ds_TexGenT1:
    case Ds_TexGenT2:
    case Ds_TexGenT3:
      ASSERT(static_cast<UINT>(which - Ds_TexGenT0) == DsGet(Ds_ActiveTexture));
      if (newVal) {
        glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, newVal);
      } else {
        glDisable(GL_TEXTURE_GEN_T);
      }
      if (!oldValue) {
        glEnable(GL_TEXTURE_GEN_T);
      }
      break;

    case Ds_TexGenR0:
    case Ds_TexGenR1:
    case Ds_TexGenR2:
    case Ds_TexGenR3:
      ASSERT(static_cast<UINT>(which - Ds_TexGenR0) == DsGet(Ds_ActiveTexture));
      if (newVal) {
        glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, newVal);
      } else {
        glDisable(GL_TEXTURE_GEN_R);
      }
      if (!oldValue) {
        glEnable(GL_TEXTURE_GEN_R);
      }
      break;

    case Ds_TexGenQ0:
    case Ds_TexGenQ1:
    case Ds_TexGenQ2:
    case Ds_TexGenQ3:
      ASSERT(static_cast<UINT>(which - Ds_TexGenQ0) == DsGet(Ds_ActiveTexture));
      if (newVal) {
        glTexGeni(GL_Q, GL_TEXTURE_GEN_MODE, newVal);
      } else {
        glDisable(GL_TEXTURE_GEN_Q);
      }
      if (!oldValue) {
        glEnable(GL_TEXTURE_GEN_Q);
      }
      break;

    case Ds_TexEnvMode0:
    case Ds_TexEnvMode1:
    case Ds_TexEnvMode2:
    case Ds_TexEnvMode3:
      ASSERT(static_cast<UINT>(which - Ds_TexEnvMode0) == DsGet(Ds_ActiveTexture));
      glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, newVal);
      break;

    case Ds_NormalArray:
      if (newVal) {
        glEnableClientState(GL_NORMAL_ARRAY);
      } else {
        glDisableClientState(GL_NORMAL_ARRAY);
      }
      break;

    case Ds_ColorArray:
      if (newVal) {
        glEnableClientState(GL_COLOR_ARRAY);
      } else {
        glDisableClientState(GL_COLOR_ARRAY);
      }
      break;

    case Ds_TextureArray0:
    case Ds_TextureArray1:
    case Ds_TextureArray2:
    case Ds_TextureArray3:
      ASSERT(static_cast<UINT>(which - Ds_TextureArray0) == DsGet(Ds_ActiveTexture));
      if (newVal) {
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
      } else {
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
      }
      break;

    case Ds_NVVAR:
      if (newVal) {
        glEnableClientState(GL_VERTEX_ARRAY_RANGE_NV);
      } else {
        glDisableClientState(GL_VERTEX_ARRAY_RANGE_NV);
      }
      break;

    case Ds_PolygonOffsetEnable:
      if (newVal) {
        glEnable(GL_POLYGON_OFFSET_FILL);
      } else {
        glDisable(GL_POLYGON_OFFSET_FILL);
      }
      break;

    case Ds_PolygonOffset:
      glPolygonOffset(*reinterpret_cast<float *>(&newVal), -1.0f);
      break;

    case Ds_BlendEnable:
      if (newVal) {
        glEnable(GL_BLEND);
      } else {
        glDisable(GL_BLEND);
      }
      break;

    case Ds_AlphaTestEnable:
      if (newVal) {
        glEnable(GL_ALPHA_TEST);
      } else {
        glDisable(GL_ALPHA_TEST);
      }
      break;

    case Ds_RegisterCombinersNV:
      if (glNVRegisterCombiners) {
        if (newVal) {
          glEnable(GL_REGISTER_COMBINERS_NV);
        } else {
          glDisable(GL_REGISTER_COMBINERS_NV);
        }
      }
      break;

    case Ds_PerStageConstantsNV:
      if (glNVRegisterCombiners2) {
        if (newVal) {
          glEnable(GL_PER_STAGE_CONSTANTS_NV);
        } else {
          glDisable(GL_PER_STAGE_CONSTANTS_NV);
        }
      }
      break;

    case Ds_TextureShaderNV:
      if (glNVTextureShader) {
        if (newVal) {
          glEnable(GL_TEXTURE_SHADER_NV);
        } else {
          glDisable(GL_TEXTURE_SHADER_NV);
        }
      }
      break;

    case Ds_FragmentProgramARB:
      if (glARBFragmentProgram) {
        if (newVal) {
          glEnable(GL_FRAGMENT_PROGRAM_ARB);
        } else {
          glDisable(GL_FRAGMENT_PROGRAM_ARB);
        }
      }
      break;

    case Ds_MatrixMode:
      glMatrixMode(newVal);
      break;

    case Ds_BlendFunc:
      glBlendFunc(newVal >> 16, newVal & 0xFFFF);
      break;

    default:
      ASSERT(0);
      break;
  }

  m_deviceState[which] = newVal;
}

void CGxDeviceOpenGl::IStateSync() {
  IStateSyncLights();
  IStateSyncEnables();
  IRsSync(0);
  IStateSyncTexTransforms();
  IStateSyncColorSource();
}

void CGxDeviceOpenGl::IStateSetColorSource(EColorSource source) {
  if (m_colorSource != source) {
    m_colorSourceDirty = 1;
    m_colorSource = source;
  }
}

void CGxDeviceOpenGl::IStateSetColorSourceColor(EColorSource source, const NTempest::CImVector &color) {
  FATALASSERT(source != Cs_Array);
  m_colorSourceColor[source].m_color = color;
  m_colorSourceColor[source].m_dirty = 1;
}

void CGxDeviceOpenGl::IStateSyncColorSource() {
  float color4f[4];

  if (m_colorSourceDirty || m_colorSourceColor[m_colorSource].m_dirty) {
    switch (m_colorSource) {
      case Cs_Material:
      case Cs_Constant:
        color4f[0] = m_colorSourceColor[m_colorSource].m_color.r * oo255;
        color4f[1] = m_colorSourceColor[m_colorSource].m_color.g * oo255;
        color4f[2] = m_colorSourceColor[m_colorSource].m_color.b * oo255;
        color4f[3] = m_colorSourceColor[m_colorSource].m_color.a * oo255;
        glColor4fv(color4f);
        break;

      case Cs_Array:
        break;

      default:
        FATALASSERT(0);
    }

    m_colorSourceDirty = 0;
    m_colorSourceColor[m_colorSource].m_dirty = 0;
  }
}

void CGxDeviceOpenGl::IStateSyncLights() {
  NTempest::C44Matrix mwv;
  int                 haveSetView;
  NTempest::C4Vector  glTmp;
  int                 updateNeeded;

  m_worldViewChange |= m_xforms[GxXform_World].m_dirty | m_xforms[GxXform_View].m_dirty;
  haveSetView = 0;

  for (UINT which = 0; which < 8; ++which) {
    CGxLight &app = m_appState.m_lights[which];
    CGxLight &hw = m_hwState.m_lights[which];
    updateNeeded = m_hwState.m_lightsDirty[which];
    UINT light = GL_LIGHT0 + which;
    memset(&glTmp, 0, sizeof(glTmp));

    if (m_worldViewChange ||
        ((app.m_isOmni != hw.m_isOmni || app.m_dir.x != hw.m_dir.x || app.m_dir.y != hw.m_dir.y || app.m_dir.z != hw.m_dir.z) && updateNeeded))
    {
      if (!haveSetView) {
        IXformSetModelView(m_xforms[GxXform_View].TopConst());
        haveSetView = 1;
      }

      if (app.m_enabled) {
        if (app.m_isOmni) {
          glTmp.x = app.m_dir.x;
          glTmp.y = app.m_dir.y;
          glTmp.z = app.m_dir.z;
          glTmp.w = 1.0f;
        } else {
          glTmp.x = -app.m_dir.x;
          glTmp.y = -app.m_dir.y;
          glTmp.z = -app.m_dir.z;
          glTmp.w = 0.0f;
          reinterpret_cast<NTempest::C3Vector *>(&glTmp)->Normalize();
        }
        glLightfv(light, GL_POSITION, &glTmp.x);
      }
    }

    if (!updateNeeded) {
      continue;
    }

    if (app.m_enabled != hw.m_enabled) {
      if (app.m_enabled)
        glEnable(light);
      else
        glDisable(light);
    }

    if (*app.m_ambColor.IV_() != *hw.m_ambColor.IV_() || app.m_ambIntensity != hw.m_ambIntensity) {
      glTmp.x = app.m_ambColor.r * app.m_ambIntensity * oo255;
      glTmp.y = app.m_ambColor.g * app.m_ambIntensity * oo255;
      glTmp.z = app.m_ambColor.b * app.m_ambIntensity * oo255;
      glTmp.w = app.m_ambColor.a * app.m_ambIntensity * oo255;
      glLightfv(light, GL_AMBIENT, &glTmp.x);
    }
    if (*app.m_dirColor.IV_() != *hw.m_dirColor.IV_() || app.m_dirIntensity != hw.m_dirIntensity) {
      glTmp.x = app.m_dirColor.r * app.m_dirIntensity * oo255;
      glTmp.y = app.m_dirColor.g * app.m_dirIntensity * oo255;
      glTmp.z = app.m_dirColor.b * app.m_dirIntensity * oo255;
      glTmp.w = app.m_dirColor.a * app.m_dirIntensity * oo255;
      glLightfv(light, GL_DIFFUSE, &glTmp.x);
    }
    if (*app.m_specColor.IV_() != *hw.m_specColor.IV_() || app.m_specIntensity != hw.m_specIntensity) {
      glTmp.x = app.m_specColor.r * app.m_specIntensity * oo255;
      glTmp.y = app.m_specColor.g * app.m_specIntensity * oo255;
      glTmp.z = app.m_specColor.b * app.m_specIntensity * oo255;
      glTmp.w = app.m_specColor.a * app.m_specIntensity * oo255;
      glLightfv(light, GL_SPECULAR, &glTmp.x);
    }
    if (app.m_constantAttenuation != hw.m_constantAttenuation) {
      glLightf(light, GL_CONSTANT_ATTENUATION, app.m_constantAttenuation);
    }
    if (app.m_linearAttenuation != hw.m_linearAttenuation) {
      glLightf(light, GL_LINEAR_ATTENUATION, app.m_linearAttenuation);
    }
    if (app.m_constantAttenuation != hw.m_quadraticAttenuation) {
      glLightf(light, GL_QUADRATIC_ATTENUATION, app.m_quadraticAttenuation);
    }

    hw = app;
    m_hwState.m_lightsDirty[which] = 0;
  }

  if (m_worldViewChange || haveSetView) {
    mwv = m_xforms[GxXform_World].TopConst() * m_xforms[GxXform_View].TopConst();
    IXformSetModelView(mwv);
    m_worldViewChange = 0;
    m_xforms[GxXform_World].m_dirty = 0;
    m_xforms[GxXform_View].m_dirty = 0;
  }
}

void CGxDeviceOpenGl::IStateSyncEnables() {
  int enable;

  if (m_appState.m_masterEnables != m_hwState.m_masterEnables) {
    enable = 0;
    if (NeedsUpdate(m_appState.m_masterEnables, m_hwState.m_masterEnables, 0, 0, GxMasterEnable_PolygonFill, enable)) {
      glPolygonMode(GL_FRONT_AND_BACK, enable ? GL_FILL : GL_LINE);
    }

    m_hwState.m_masterEnables = m_appState.m_masterEnables;
  }
}

void CGxDeviceOpenGl::IStateSyncTexTransforms() {
  int texture;

  for (UINT tmu = 0; tmu < m_caps.m_numTmus; ++tmu) {
    texture = 0;
    RsGet(static_cast<EGxRenderState>(GxRs_Texture0 + tmu), texture);
    if (texture && (m_xforms[tmu].m_dirty || m_texGen[tmu].m_dirty)) {
      IStateSyncTexTransform(tmu);
    }
  }
}

void CGxDeviceOpenGl::IStateSyncTexTransform(UINT tmu) {
  NTempest::C44Matrix concatMat;
  int                 ts;

  FATALASSERT(tmu < m_caps.m_numTmus);
  DsSet(Ds_ActiveTexture, tmu, 0);
  DsSet(Ds_MatrixMode, GL_TEXTURE, 0);

  ts = 0;
  RsGet(static_cast<EGxRenderState>(GxRs_TextureShader0 + tmu), ts);
  if (ts) {
    if (ts == GxTS_Affine || ts == GxTS_Proj) {
      concatMat = m_texGen[tmu].TopConst() * m_xforms[tmu].TopConst();
      glLoadMatrixf(&concatMat.a0);
    } else {
      FATALASSERT(0);
    }
  } else {
    glLoadIdentity();
  }

  m_xforms[tmu].m_dirty = 0;
  m_texGen[tmu].m_dirty = 0;
  DsSet(Ds_MatrixMode, GL_MODELVIEW, 0);
}

void CGxDeviceOpenGl::IStateSetContextDefaults() {
  NTempest::C44Matrix mwv;
  NTempest::C4Vector  opaqueBlack(0.0f);
  float               rPlane[4] = {0.0f, 0.0f, 1.0f, 0.0f};
  float               qPlane[4] = {0.0f, 0.0f, 0.0f, 1.0f};
  UINT                maxTex;
  NTempest::C4Vector  glTmp;

  DsInit();
  DsSet(Ds_MatrixMode, GL_MODELVIEW, 1);
  glLoadIdentity();

  for (maxTex = 0; maxTex < m_caps.m_numTmus; ++maxTex) {
    DsSet(Ds_ActiveTexture, maxTex, 1);
    glTexGenfv(GL_R, GL_OBJECT_PLANE, rPlane);
    glTexGenfv(GL_Q, GL_OBJECT_PLANE, qPlane);
    glTexGenfv(GL_R, GL_EYE_PLANE, rPlane);
    glTexGenfv(GL_Q, GL_EYE_PLANE, qPlane);
  }

  glLightModelfv(GL_LIGHT_MODEL_AMBIENT, &opaqueBlack.x);
  glLightModeli(0x81F8, 0x81FA);
  glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);
  IXformSetModelView(m_xforms[GxXform_View].TopConst());

  for (UINT which = 0; which < 8; ++which) {
    const CGxLight &light = m_hwState.m_lights[which];
    UINT            glLight = GL_LIGHT0 + which;
    glTmp = NTempest::C4Vector(0.0f);
    glLightfv(glLight, GL_SPECULAR, &opaqueBlack.x);
    if (light.m_isOmni) {
      glTmp.x = light.m_dir.x;
      glTmp.y = light.m_dir.y;
      glTmp.z = light.m_dir.z;
      glTmp.w = 1.0f;
    } else {
      glTmp.x = -light.m_dir.x;
      glTmp.y = -light.m_dir.y;
      glTmp.z = -light.m_dir.z;
      reinterpret_cast<NTempest::C3Vector *>(&glTmp)->Normalize();
    }
    glLightfv(glLight, GL_POSITION, &glTmp.x);
    if (light.m_enabled)
      glEnable(glLight);
    else
      glDisable(glLight);

    glTmp.x = light.m_ambColor.r * light.m_ambIntensity * oo255;
    glTmp.y = light.m_ambColor.g * light.m_ambIntensity * oo255;
    glTmp.z = light.m_ambColor.b * light.m_ambIntensity * oo255;
    glTmp.w = light.m_ambColor.a * light.m_ambIntensity * oo255;
    glLightfv(glLight, GL_AMBIENT, &glTmp.x);
    glTmp.x = light.m_dirColor.r * light.m_dirIntensity * oo255;
    glTmp.y = light.m_dirColor.g * light.m_dirIntensity * oo255;
    glTmp.z = light.m_dirColor.b * light.m_dirIntensity * oo255;
    glTmp.w = light.m_dirColor.a * light.m_dirIntensity * oo255;
    glLightfv(glLight, GL_DIFFUSE, &glTmp.x);
    m_hwState.m_lightsDirty[which] = 1;
    glLightf(glLight, GL_LINEAR_ATTENUATION, m_hwState.m_lightLinearFalloff);
    glLightf(glLight, GL_QUADRATIC_ATTENUATION, m_hwState.m_lightQuadraticFalloff);
  }

  mwv = m_xforms[GxXform_World].TopConst() * m_xforms[GxXform_View].TopConst();
  IXformSetModelView(mwv);
  m_worldViewChange = 0;
  glEnableClientState(GL_VERTEX_ARRAY);
  glEnable(GL_COLOR_MATERIAL);
  glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
  glEnable(GL_SCISSOR_TEST);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
}

void CGxDeviceOpenGl::ISetTexture(UINT tmu, CGxTex *tex) {
  if (tmu >= m_caps.m_numTmus) {
    return;
  }
  DsSet(Ds_ActiveTexture, tmu, 0);
  if (tex) {
    DsSet(static_cast<EDeviceState>(Ds_TexTarget0 + tmu), GL_TEXTURE_2D, 0);
    BindTexture(tex, tmu);
    ITexMarkAsUpdated(tex, tmu);
    ITexBind(tex);
  } else {
    DsSet(static_cast<EDeviceState>(Ds_TexTarget0 + tmu), 0, 0);
  }
}

void CGxDeviceOpenGl::ISetTexBlend(UINT tmu, EGxTexBlend blend) {
  if (tmu < m_caps.m_numTmus) {
    DsSet(Ds_ActiveTexture, tmu, 0);
    DsSet(static_cast<EDeviceState>(Ds_TexEnvMode0 + tmu), s_texEnv[blend], 0);
  }
}

void CGxDeviceOpenGl::ISetTexLodBias(UINT tmu, float bias) {
  if (tmu < m_caps.m_numTmus && glExtTextureLodBias) {
    DsSet(Ds_ActiveTexture, tmu, 0);
    glTexEnvf(GL_TEXTURE_FILTER_CONTROL_EXT, GL_TEXTURE_LOD_BIAS_EXT, bias);
  }
}

void CGxDeviceOpenGl::ISetTexGen(UINT tmu, EGxTexGen texGen) {
  if (tmu >= m_caps.m_numTmus) {
    return;
  }
  DsSet(Ds_ActiveTexture, tmu, 0);
  switch (texGen) {
    case GxTexGen_Disable:
      DsSet(static_cast<EDeviceState>(Ds_TexGenS0 + tmu), 0, 0);
      DsSet(static_cast<EDeviceState>(Ds_TexGenT0 + tmu), 0, 0);
      DsSet(static_cast<EDeviceState>(Ds_TexGenR0 + tmu), 0, 0);
      DsSet(static_cast<EDeviceState>(Ds_TexGenQ0 + tmu), 0, 0);
      break;
    case GxTexGen_Object:
      DsSet(static_cast<EDeviceState>(Ds_TexGenS0 + tmu), GL_OBJECT_LINEAR, 0);
      DsSet(static_cast<EDeviceState>(Ds_TexGenT0 + tmu), GL_OBJECT_LINEAR, 0);
      DsSet(static_cast<EDeviceState>(Ds_TexGenR0 + tmu), GL_OBJECT_LINEAR, 0);
      DsSet(static_cast<EDeviceState>(Ds_TexGenQ0 + tmu), 0, 0);
      break;
    case GxTexGen_World:
    case GxTexGen_View:
      DsSet(static_cast<EDeviceState>(Ds_TexGenS0 + tmu), GL_EYE_LINEAR, 0);
      DsSet(static_cast<EDeviceState>(Ds_TexGenT0 + tmu), GL_EYE_LINEAR, 0);
      DsSet(static_cast<EDeviceState>(Ds_TexGenR0 + tmu), GL_EYE_LINEAR, 0);
      DsSet(static_cast<EDeviceState>(Ds_TexGenQ0 + tmu), 0, 0);
      break;
    case GxTexGen_ViewReflection:
      DsSet(static_cast<EDeviceState>(Ds_TexGenS0 + tmu), GL_REFLECTION_MAP_NV, 0);
      DsSet(static_cast<EDeviceState>(Ds_TexGenT0 + tmu), GL_REFLECTION_MAP_NV, 0);
      DsSet(static_cast<EDeviceState>(Ds_TexGenR0 + tmu), GL_REFLECTION_MAP_NV, 0);
      DsSet(static_cast<EDeviceState>(Ds_TexGenQ0 + tmu), 0, 0);
      break;
    case GxTexGen_ViewNormal:
      DsSet(static_cast<EDeviceState>(Ds_TexGenS0 + tmu), GL_NORMAL_MAP_NV, 0);
      DsSet(static_cast<EDeviceState>(Ds_TexGenT0 + tmu), GL_NORMAL_MAP_NV, 0);
      DsSet(static_cast<EDeviceState>(Ds_TexGenR0 + tmu), GL_NORMAL_MAP_NV, 0);
      DsSet(static_cast<EDeviceState>(Ds_TexGenQ0 + tmu), 0, 0);
      break;
    case GxTexGen_SphereMap:
      DsSet(static_cast<EDeviceState>(Ds_TexGenS0 + tmu), GL_SPHERE_MAP, 0);
      DsSet(static_cast<EDeviceState>(Ds_TexGenT0 + tmu), GL_SPHERE_MAP, 0);
      DsSet(static_cast<EDeviceState>(Ds_TexGenR0 + tmu), 0, 0);
      DsSet(static_cast<EDeviceState>(Ds_TexGenQ0 + tmu), 0, 0);
      break;
    default:
      FATALASSERT(0);
      break;
  }

  if (texGen == GxTexGen_World) {
    NTempest::C44Matrix &texMat = m_texGen[tmu].Top();
    IXformGLModelView(m_xforms[GxXform_View].TopConst(), texMat);
    float b0 = texMat.b0;
    float c0 = texMat.c0;
    float c1 = texMat.c1;
    texMat.b0 = texMat.a1;
    texMat.c0 = texMat.a2;
    texMat.a1 = b0;
    texMat.c1 = texMat.b2;
    texMat.a2 = c0;
    texMat.b2 = c1;
    texMat.d0 = -texMat.d0;
    texMat.d1 = -texMat.d1;
    texMat.d2 = -texMat.d2;
  } else {
    m_texGen[tmu].Identity();
  }
}

void CGxDeviceOpenGl::IRsSendToHw(EGxRenderState which) {
  float color4f[4];
  float floatVal;
  int   intVal;

  floatVal = *reinterpret_cast<float *>(&mAppRenderStates[which].mValue);
  intVal = *reinterpret_cast<int *>(&mAppRenderStates[which].mValue);

  switch (which) {
    case GxRs_PolygonOffset: {
      floatVal *= -16.0f;
      if (floatVal == 0.0f) {
        DsSet(Ds_PolygonOffsetEnable, 0, 0);
      } else {
        DsSet(Ds_PolygonOffset, *reinterpret_cast<UINT *>(&floatVal), 0);
        DsSet(Ds_PolygonOffsetEnable, 1, 0);
      }
      break;
    }
    case GxRs_MatDiffuse:
      IStateSetColorSource(Cs_Material);
      IStateSetColorSourceColor(Cs_Material, *reinterpret_cast<NTempest::CImVector *>(&mAppRenderStates[which].mValue));
      break;
    case GxRs_MatEmissive: {
      color4f[0] = reinterpret_cast<NTempest::CImVector *>(&mAppRenderStates[which].mValue)->r * oo255;
      color4f[1] = reinterpret_cast<NTempest::CImVector *>(&mAppRenderStates[which].mValue)->g * oo255;
      color4f[2] = reinterpret_cast<NTempest::CImVector *>(&mAppRenderStates[which].mValue)->b * oo255;
      color4f[3] = reinterpret_cast<NTempest::CImVector *>(&mAppRenderStates[which].mValue)->a * oo255;
      glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, color4f);
      break;
    }
    case GxRs_MatSpecular: {
      color4f[0] = reinterpret_cast<NTempest::CImVector *>(&mAppRenderStates[which].mValue)->r * oo255;
      color4f[1] = reinterpret_cast<NTempest::CImVector *>(&mAppRenderStates[which].mValue)->g * oo255;
      color4f[2] = reinterpret_cast<NTempest::CImVector *>(&mAppRenderStates[which].mValue)->b * oo255;
      color4f[3] = reinterpret_cast<NTempest::CImVector *>(&mAppRenderStates[which].mValue)->a * oo255;
      glMaterialfv(GL_FRONT, GL_SPECULAR, color4f);
      break;
    }
    case GxRs_MatSpecularExp:
      glMaterialf(GL_FRONT, GL_SHININESS, floatVal);
      break;
    case GxRs_NormalizeNormals:
      if (intVal)
        glEnable(GL_NORMALIZE);
      else
        glDisable(GL_NORMALIZE);
      break;
    case GxRs_SceneAmbient: {
      color4f[0] = reinterpret_cast<NTempest::CImVector *>(&mAppRenderStates[which].mValue)->r * oo255;
      color4f[1] = reinterpret_cast<NTempest::CImVector *>(&mAppRenderStates[which].mValue)->g * oo255;
      color4f[2] = reinterpret_cast<NTempest::CImVector *>(&mAppRenderStates[which].mValue)->b * oo255;
      color4f[3] = reinterpret_cast<NTempest::CImVector *>(&mAppRenderStates[which].mValue)->a * oo255;
      glLightModelfv(GL_LIGHT_MODEL_AMBIENT, color4f);
      break;
    }
    case GxRs_Blend:
      if (intVal == GxBlend_Opaque || intVal == GxBlend_AlphaKey) {
        DsSet(Ds_BlendEnable, 0, 0);
      } else {
        DsSet(Ds_BlendEnable, 1, 0);
        DsSet(Ds_BlendFunc, (s_glSrcBlend[intVal] << 16) | s_glDstBlend[intVal], 0);
      }
      break;
    case GxRs_AlphaRef:
      if (intVal <= 0) {
        DsSet(Ds_AlphaTestEnable, 0, 0);
      } else {
        glAlphaFunc(GL_GEQUAL, intVal * oo255);
        DsSet(Ds_AlphaTestEnable, 1, 0);
      }
      break;
    case GxRs_FogStyle:
      glFogi(GL_FOG_MODE, s_fogStyle[intVal]);
      break;
    case GxRs_FogStart:
      glFogf(GL_FOG_START, floatVal);
      break;
    case GxRs_FogEnd:
      glFogf(GL_FOG_END, floatVal);
      break;
    case GxRs_FogDensity:
      glFogf(GL_FOG_DENSITY, floatVal);
      break;
    case GxRs_FogColor:
      color4f[0] = reinterpret_cast<NTempest::CImVector *>(&mAppRenderStates[which].mValue)->r * oo255;
      color4f[1] = reinterpret_cast<NTempest::CImVector *>(&mAppRenderStates[which].mValue)->g * oo255;
      color4f[2] = reinterpret_cast<NTempest::CImVector *>(&mAppRenderStates[which].mValue)->b * oo255;
      color4f[3] = reinterpret_cast<NTempest::CImVector *>(&mAppRenderStates[which].mValue)->a * oo255;
      glFogfv(GL_FOG_COLOR, color4f);
      break;
    case GxRs_Lighting:
      if ((m_appState.m_masterEnables & (1U << GxMasterEnable_Lighting)) && intVal) {
        glEnable(GL_LIGHTING);
      } else {
        glDisable(GL_LIGHTING);
      }
      break;
    case GxRs_Fog:
      if ((m_appState.m_masterEnables & (1U << GxMasterEnable_Fog)) && intVal) {
        glEnable(GL_FOG);
      } else {
        glDisable(GL_FOG);
      }
      break;
    case GxRs_DepthTest:
      if ((m_appState.m_masterEnables & (1U << GxMasterEnable_DepthTest)) && intVal) {
        glEnable(GL_DEPTH_TEST);
      } else {
        glDisable(GL_DEPTH_TEST);
      }
      break;
    case GxRs_DepthFunc:
      glDepthFunc(s_cmpFunc[intVal]);
      break;
    case GxRs_DepthWrite:
      DsSet(Ds_DepthMask, (m_appState.m_masterEnables & (1U << GxMasterEnable_DepthWrite)) ? intVal : 0, 0);
      break;
    case GxRs_Culling:
      if ((m_appState.m_masterEnables & (1U << GxMasterEnable_Culling)) && intVal) {
        glEnable(GL_CULL_FACE);
      } else {
        glDisable(GL_CULL_FACE);
      }
      break;
    case GxRs_Texture0:
    case GxRs_Texture1:
    case GxRs_Texture2:
    case GxRs_Texture3:
      ISetTexture(which - GxRs_Texture0, reinterpret_cast<CGxTex *>(intVal));
      break;
    case GxRs_TexBlend0:
    case GxRs_TexBlend1:
    case GxRs_TexBlend2:
    case GxRs_TexBlend3:
      ISetTexBlend(which - GxRs_TexBlend0, static_cast<EGxTexBlend>(intVal));
      break;
    case GxRs_TexLodBias0:
    case GxRs_TexLodBias1:
    case GxRs_TexLodBias2:
    case GxRs_TexLodBias3:
      ISetTexLodBias(which - GxRs_TexLodBias0, floatVal);
      break;
    case GxRs_TexGen0:
    case GxRs_TexGen1:
    case GxRs_TexGen2:
    case GxRs_TexGen3:
      ISetTexGen(which - GxRs_TexGen0, static_cast<EGxTexGen>(intVal));
      break;
    case GxRs_TextureShader0:
    case GxRs_TextureShader1:
    case GxRs_TextureShader2:
    case GxRs_TextureShader3:
    case GxRs_VertexShader:
      return;
    case GxRs_PixelShader:
      IPixelShaderBind(reinterpret_cast<CGxPixelShader *>(intVal));
      break;
    default:
      FATALASSERT(0);
      break;
  }
}
