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
      if (newVal) {
        glDepthMask(GL_TRUE);
      } else {
        glDepthMask(GL_FALSE);
      }
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
      ASSERT((uint)(which - Ds_TexTarget0) == DsGet(Ds_ActiveTexture));
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
      ASSERT((uint)(which - Ds_TexGenS0) == DsGet(Ds_ActiveTexture));
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
      ASSERT((uint)(which - Ds_TexGenT0) == DsGet(Ds_ActiveTexture));
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
      ASSERT((uint)(which - Ds_TexGenR0) == DsGet(Ds_ActiveTexture));
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
      ASSERT((uint)(which - Ds_TexGenQ0) == DsGet(Ds_ActiveTexture));
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
      ASSERT((uint)(which - Ds_TexEnvMode0) == DsGet(Ds_ActiveTexture));
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
      ASSERT((uint)(which - Ds_TextureArray0) == DsGet(Ds_ActiveTexture));
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
      glPolygonOffset(*(float *)&newVal, -1.0f);
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
      glBlendFunc((newVal >> 16) & 0xFFFF, newVal & 0xFFFF);
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

inline void SetGlColor(float *dst, const NTempest::CImVector &src, float scale) {
  dst[0] = src.r * scale;
  dst[1] = src.g * scale;
  dst[2] = src.b * scale;
  dst[3] = src.a * scale;
}

void CGxDeviceOpenGl::IStateSyncColorSource() {
  float color4f[4];

  if (m_colorSourceDirty || m_colorSourceColor[m_colorSource].m_dirty) {
    switch (m_colorSource) {
      case Cs_Material:
      case Cs_Constant:
        SetGlColor(color4f, m_colorSourceColor[m_colorSource].m_color, oo255);
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

static inline void SetLightColor(float intensity, NTempest::CImVector color, ULONG light, ULONG pname) {
  float scale = intensity * (1.0f / 255.0f);
  NTempest::C4Vector glColor(color.r * scale, color.g * scale, color.b * scale, color.a * scale);
  glLightfv(light, pname, &glColor.x);
}

void CGxDeviceOpenGl::IStateSyncLights() {
  BOOL haveSetView;
  int  updateNeeded;

  haveSetView = 0;
  m_worldViewChange |= m_xforms[GxXform_World].m_dirty | m_xforms[GxXform_View].m_dirty;

  for (UINT which = 0; which != 8; ++which) {
    updateNeeded = m_hwState.m_lightsDirty[which];
    CGxLight          &app = m_appState.m_lights[which];
    CGxLight          &hw = m_hwState.m_lights[which];
    UINT               light = GL_LIGHT0 + which;
    NTempest::C4Vector glTmp;

    if (updateNeeded || m_worldViewChange) {
      if (m_worldViewChange || hw.m_isOmni != app.m_isOmni || hw.m_dir != app.m_dir) {
        if (!haveSetView) {
          IXformSetModelView(m_xforms[GxXform_View].TopConst());
          haveSetView = 1;
        }

        if (app.m_enabled) {
          if (app.m_isOmni) {
            glTmp = NTempest::C4Vector(app.m_dir.x, app.m_dir.y, app.m_dir.z, 1.0f);
          } else {
            glTmp = NTempest::C4Vector(-app.m_dir.x, -app.m_dir.y, -app.m_dir.z, 0.0f);
            if (NTempest::CMath::fnotequal_(glTmp.SquaredMag(), 1.0f))
              glTmp *= NTempest::CMath::sqrtinv_(glTmp.SquaredMag());
          }
          glLightfv(light, GL_POSITION, &glTmp.x);
        }
      }
    }

    if (!updateNeeded) {
      continue;
    }

    if (hw.m_enabled != app.m_enabled) {
      if (app.m_enabled)
        glEnable(light);
      else
        glDisable(light);
    }

    if (*hw.m_ambColor.IV_() != *app.m_ambColor.IV_() || hw.m_ambIntensity != app.m_ambIntensity)
      SetLightColor(app.m_ambIntensity, app.m_ambColor, light, GL_AMBIENT);

    if (*hw.m_dirColor.IV_() != *app.m_dirColor.IV_() || hw.m_dirIntensity != app.m_dirIntensity)
      SetLightColor(app.m_dirIntensity, app.m_dirColor, light, GL_DIFFUSE);

    if (*hw.m_specColor.IV_() != *app.m_specColor.IV_() || hw.m_specIntensity != app.m_specIntensity)
      SetLightColor(app.m_specIntensity, app.m_specColor, light, GL_SPECULAR);

    if (hw.m_constantAttenuation != app.m_constantAttenuation)
      glLightf(light, GL_CONSTANT_ATTENUATION, app.m_constantAttenuation);

    if (hw.m_linearAttenuation != app.m_linearAttenuation)
      glLightf(light, GL_LINEAR_ATTENUATION, app.m_linearAttenuation);

    if (hw.m_quadraticAttenuation != app.m_constantAttenuation)
      glLightf(light, GL_QUADRATIC_ATTENUATION, app.m_quadraticAttenuation);

    hw = app;
    m_hwState.m_lightsDirty[which] = 0;
  }

  if (m_worldViewChange || haveSetView) {
    NTempest::C44Matrix mwv = m_xforms[GxXform_World].TopConst() * m_xforms[GxXform_View].TopConst();
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
      if (enable) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      }
    }

    m_hwState.m_masterEnables = m_appState.m_masterEnables;
  }
}

void CGxDeviceOpenGl::IStateSyncTexTransforms() {
  int texture;

  for (UINT tmu = 0; tmu < m_caps.m_numTmus; ++tmu) {
    RsGet((EGxRenderState)(GxRs_Texture0 + tmu), texture);
    if (texture && (m_xforms[tmu].m_dirty || m_texGen[tmu].m_dirty)) {
      IStateSyncTexTransform(tmu);
    }
  }
}

void CGxDeviceOpenGl::IStateSyncTexTransform(UINT tmu) {
  FATALASSERT(tmu < m_caps.m_numTmus);
  DsSet(Ds_ActiveTexture, tmu, 0);
  DsSet(Ds_MatrixMode, GL_TEXTURE, 0);

  int ts;
  GxRsGet((EGxRenderState)(GxRs_TextureShader0 + tmu), ts);
  switch (ts) {
    case GxTS_PassThru:
      glLoadIdentity();
      break;

    case GxTS_Affine:
    case GxTS_Proj: {
      NTempest::C44Matrix concatMat = m_texGen[tmu].TopConst() * m_xforms[tmu].TopConst();
      glLoadMatrixf(&concatMat.a0);
      break;
    }

    default:
      FATALASSERT(0);
      break;
  }

  m_xforms[tmu].m_dirty = 0;
  m_texGen[tmu].m_dirty = 0;
  DsSet(Ds_MatrixMode, GL_MODELVIEW, 0);
}

void CGxDeviceOpenGl::IStateSetContextDefaults() {
  DsInit();

  NTempest::C4Vector opaqueBlack(0.0f);
  DsSet(Ds_MatrixMode, GL_MODELVIEW, 1);
  glLoadIdentity();

  float rPlane[4] = {0.0f, 0.0f, 1.0f, 0.0f};
  float qPlane[4] = {0.0f, 0.0f, 0.0f, 1.0f};
  UINT  maxTex = m_caps.m_numTmus;
  for (UINT tmu = 0; tmu != maxTex; ++tmu) {
    DsSet(Ds_ActiveTexture, tmu, 1);
    glTexGenfv(GL_R, GL_EYE_PLANE, rPlane);
    glTexGenfv(GL_Q, GL_EYE_PLANE, qPlane);
    glTexGenfv(GL_R, GL_OBJECT_PLANE, rPlane);
    glTexGenfv(GL_Q, GL_OBJECT_PLANE, qPlane);
  }

  glLightModelfv(GL_LIGHT_MODEL_AMBIENT, &opaqueBlack.x);
  glLightModeli(0x81F8, 0x81FA);
  glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);
  IXformSetModelView(m_xforms[GxXform_View].TopConst());

  for (UINT which = 0; which != 8; ++which) {
    const CGxLight    &light = m_hwState.m_lights[which];
    UINT               glLight = GL_LIGHT0 + which;
    NTempest::C4Vector glTmp;

    glLightfv(glLight, GL_SPECULAR, &opaqueBlack.x);
    if (light.m_isOmni) {
      glTmp = NTempest::C4Vector(light.m_dir.x, light.m_dir.y, light.m_dir.z, 1.0f);
    } else {
      glTmp = NTempest::C4Vector(-light.m_dir.x, -light.m_dir.y, -light.m_dir.z, 0.0f);
      glTmp *= NTempest::CMath::sqrtinv_(glTmp.SquaredMag());
    }
    glLightfv(glLight, GL_POSITION, &glTmp.x);

    light.m_enabled ? glEnable(glLight) : glDisable(glLight);

    float scale = oo255 * light.m_ambIntensity;
    glTmp = NTempest::C4Vector(light.m_ambColor.r, light.m_ambColor.g, light.m_ambColor.b, light.m_ambColor.a) * scale;
    glLightfv(glLight, GL_AMBIENT, &glTmp.x);

    scale = oo255 * light.m_dirIntensity;
    glTmp = NTempest::C4Vector(light.m_dirColor.r, light.m_dirColor.g, light.m_dirColor.b, light.m_dirColor.a) * scale;
    glLightfv(glLight, GL_DIFFUSE, &glTmp.x);

    m_hwState.m_lightsDirty[which] = 1;
    glLightf(glLight, GL_LINEAR_ATTENUATION, m_hwState.m_lightLinearFalloff);
    glLightf(glLight, GL_QUADRATIC_ATTENUATION, m_hwState.m_lightQuadraticFalloff);
  }

  NTempest::C44Matrix mwv = m_xforms[GxXform_World].TopConst() * m_xforms[GxXform_View].TopConst();
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
    DsSet((EDeviceState)(Ds_TexTarget0 + tmu), GL_TEXTURE_2D, 0);
    BindTexture(tex, tmu);
    ITexMarkAsUpdated(tex, tmu);
    ITexBind(tex);
  } else {
    DsSet((EDeviceState)(Ds_TexTarget0 + tmu), 0, 0);
  }
}

void CGxDeviceOpenGl::ISetTexBlend(UINT tmu, EGxTexBlend blend) {
  if (tmu < m_caps.m_numTmus) {
    DsSet(Ds_ActiveTexture, tmu, 0);
    DsSet((EDeviceState)(Ds_TexEnvMode0 + tmu), s_texEnv[blend], 0);
  }
}

void CGxDeviceOpenGl::ISetTexLodBias(UINT tmu, float bias) {
  if (tmu < m_caps.m_numTmus && glExtTextureLodBias) {
    DsSet(Ds_ActiveTexture, tmu, 0);
    glTexEnvf(GL_TEXTURE_FILTER_CONTROL_EXT, GL_TEXTURE_LOD_BIAS_EXT, bias);
  }
}

template <class T> inline void TSSwap(T &a, T &b) {
  T temp = a;
  a = b;
  b = temp;
}

void CGxDeviceOpenGl::ISetTexGen(UINT tmu, EGxTexGen texGen) {
  if (tmu >= m_caps.m_numTmus) {
    return;
  }
  DsSet(Ds_ActiveTexture, tmu, 0);
  switch (texGen) {
    case GxTexGen_Disable:
      DsSet((EDeviceState)(Ds_TexGenS0 + tmu), 0, 0);
      DsSet((EDeviceState)(Ds_TexGenT0 + tmu), 0, 0);
      DsSet((EDeviceState)(Ds_TexGenR0 + tmu), 0, 0);
      DsSet((EDeviceState)(Ds_TexGenQ0 + tmu), 0, 0);
      break;
    case GxTexGen_SphereMap:
      DsSet((EDeviceState)(Ds_TexGenS0 + tmu), GL_SPHERE_MAP, 0);
      DsSet((EDeviceState)(Ds_TexGenT0 + tmu), GL_SPHERE_MAP, 0);
      DsSet((EDeviceState)(Ds_TexGenR0 + tmu), 0, 0);
      DsSet((EDeviceState)(Ds_TexGenQ0 + tmu), 0, 0);
      break;
    case GxTexGen_World:
    case GxTexGen_View:
      DsSet((EDeviceState)(Ds_TexGenS0 + tmu), GL_EYE_LINEAR, 0);
      DsSet((EDeviceState)(Ds_TexGenT0 + tmu), GL_EYE_LINEAR, 0);
      DsSet((EDeviceState)(Ds_TexGenR0 + tmu), GL_EYE_LINEAR, 0);
      DsSet((EDeviceState)(Ds_TexGenQ0 + tmu), 0, 0);
      break;
    case GxTexGen_Object:
      DsSet((EDeviceState)(Ds_TexGenS0 + tmu), GL_OBJECT_LINEAR, 0);
      DsSet((EDeviceState)(Ds_TexGenT0 + tmu), GL_OBJECT_LINEAR, 0);
      DsSet((EDeviceState)(Ds_TexGenR0 + tmu), GL_OBJECT_LINEAR, 0);
      DsSet((EDeviceState)(Ds_TexGenQ0 + tmu), 0, 0);
      break;
    case GxTexGen_ViewReflection:
      DsSet((EDeviceState)(Ds_TexGenS0 + tmu), GL_REFLECTION_MAP_NV, 0);
      DsSet((EDeviceState)(Ds_TexGenT0 + tmu), GL_REFLECTION_MAP_NV, 0);
      DsSet((EDeviceState)(Ds_TexGenR0 + tmu), GL_REFLECTION_MAP_NV, 0);
      DsSet((EDeviceState)(Ds_TexGenQ0 + tmu), 0, 0);
      break;
    case GxTexGen_ViewNormal:
      DsSet((EDeviceState)(Ds_TexGenS0 + tmu), GL_NORMAL_MAP_NV, 0);
      DsSet((EDeviceState)(Ds_TexGenT0 + tmu), GL_NORMAL_MAP_NV, 0);
      DsSet((EDeviceState)(Ds_TexGenR0 + tmu), GL_NORMAL_MAP_NV, 0);
      DsSet((EDeviceState)(Ds_TexGenQ0 + tmu), 0, 0);
      break;
    default:
      FATALASSERT(0);
      break;
  }

  switch (texGen) {
    case GxTexGen_World: {
      NTempest::C44Matrix &texMat = m_texGen[tmu].Top();
      IXformGLModelView(m_xforms[GxXform_View].TopConst(), texMat);
      TSSwap(texMat.a1, texMat.b0);
      TSSwap(texMat.a2, texMat.c0);
      TSSwap(texMat.b2, texMat.c1);
      texMat.d0 *= -1.0f;
      texMat.d1 *= -1.0f;
      texMat.d2 *= -1.0f;
      break;
    }
    default:
      m_texGen[tmu].Identity();
      break;
  }
}

void CGxDeviceOpenGl::IRsSendToHw(EGxRenderState which) {
  CGxAppRenderState &state = mAppRenderStates[which];
  float              floatVal;
  int                intVal;
  float              color4f[4];

  intVal = 0;

  switch (which) {
    case GxRs_PolygonOffset:
      floatVal = state.mValue.GetAsFloat() * -16.0f;
      if (floatVal != 0.0f) {
        DsSet(Ds_PolygonOffset, *(UINT *)&floatVal, 0);
        DsSet(Ds_PolygonOffsetEnable, 1, 0);
      } else {
        DsSet(Ds_PolygonOffsetEnable, 0, 0);
      }
      break;
    case GxRs_MatDiffuse:
#if !BUILD_FIXES
      // known misbehavior: the draw already chose the colour source and this overrides it during deferred state sync
      //   in practice this means text shadows can render white
      IStateSetColorSource(Cs_Material);
#endif
      IStateSetColorSourceColor(Cs_Material, state.mValue.GetAsCArgb());
      break;
    case GxRs_MatEmissive:
      SetGlColor(color4f, state.mValue.GetAsCArgb(), oo255);
      glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, color4f);
      break;
    case GxRs_MatSpecular:
      SetGlColor(color4f, state.mValue.GetAsCArgb(), oo255);
      glMaterialfv(GL_FRONT, GL_SPECULAR, color4f);
      break;
    case GxRs_MatSpecularExp:
      glMaterialf(GL_FRONT, GL_SHININESS, state.mValue.GetAsFloat());
      break;
    case GxRs_NormalizeNormals:
      if (state.mValue.GetAsInt())
        glEnable(GL_NORMALIZE);
      else
        glDisable(GL_NORMALIZE);
      break;
    case GxRs_SceneAmbient:
      SetGlColor(color4f, state.mValue.GetAsCArgb(), oo255);
      glLightModelfv(GL_LIGHT_MODEL_AMBIENT, color4f);
      break;
    case GxRs_Blend:
      intVal = state.mValue.GetAsInt();
      if (intVal == GxBlend_Opaque || intVal == GxBlend_AlphaKey) {
        DsSet(Ds_BlendEnable, 0, 0);
      } else {
        DsSet(Ds_BlendEnable, 1, 0);
        DsSet(Ds_BlendFunc, (s_glSrcBlend[intVal] << 16) | s_glDstBlend[intVal], 0);
      }
      break;
    case GxRs_AlphaRef:
      intVal = state.mValue.GetAsInt();
      if (intVal > 0) {
        glAlphaFunc(GL_GEQUAL, intVal * oo255);
        DsSet(Ds_AlphaTestEnable, 1, 0);
      } else {
        DsSet(Ds_AlphaTestEnable, 0, 0);
      }
      break;
    case GxRs_FogStyle:
      glFogi(GL_FOG_MODE, s_fogStyle[state.mValue.GetAsInt()]);
      break;
    case GxRs_FogStart:
      glFogf(GL_FOG_START, state.mValue.GetAsFloat());
      break;
    case GxRs_FogEnd:
      glFogf(GL_FOG_END, state.mValue.GetAsFloat());
      break;
    case GxRs_FogDensity:
      glFogf(GL_FOG_DENSITY, state.mValue.GetAsFloat());
      break;
    case GxRs_FogColor:
      SetGlColor(color4f, state.mValue.GetAsCArgb(), oo255);
      glFogfv(GL_FOG_COLOR, color4f);
      break;
    case GxRs_Lighting:
      if ((m_appState.m_masterEnables & (1U << GxMasterEnable_Lighting)) && state.mValue.GetAsInt()) {
        glEnable(GL_LIGHTING);
      } else {
        glDisable(GL_LIGHTING);
      }
      break;
    case GxRs_Fog:
      if ((m_appState.m_masterEnables & (1U << GxMasterEnable_Fog)) && state.mValue.GetAsInt()) {
        glEnable(GL_FOG);
      } else {
        glDisable(GL_FOG);
      }
      break;
    case GxRs_DepthTest:
      if ((m_appState.m_masterEnables & (1U << GxMasterEnable_DepthTest)) && state.mValue.GetAsInt()) {
        glEnable(GL_DEPTH_TEST);
      } else {
        glDisable(GL_DEPTH_TEST);
      }
      break;
    case GxRs_DepthFunc:
      glDepthFunc(s_cmpFunc[state.mValue.GetAsInt()]);
      break;
    case GxRs_DepthWrite:
      if (m_appState.m_masterEnables & (1U << GxMasterEnable_DepthWrite))
        intVal = state.mValue.GetAsInt();
      DsSet(Ds_DepthMask, intVal, 0);
      break;
    case GxRs_Culling:
      if ((m_appState.m_masterEnables & (1U << GxMasterEnable_Culling)) && state.mValue.GetAsInt()) {
        glEnable(GL_CULL_FACE);
      } else {
        glDisable(GL_CULL_FACE);
      }
      break;
    case GxRs_Texture0: ISetTexture(0, (CGxTex *)state.mValue.GetAsPointer()); break;
    case GxRs_Texture1: ISetTexture(1, (CGxTex *)state.mValue.GetAsPointer()); break;
    case GxRs_Texture2: ISetTexture(2, (CGxTex *)state.mValue.GetAsPointer()); break;
    case GxRs_Texture3: ISetTexture(3, (CGxTex *)state.mValue.GetAsPointer()); break;
    case GxRs_TexBlend0: ISetTexBlend(0, (EGxTexBlend)state.mValue.GetAsInt()); break;
    case GxRs_TexBlend1: ISetTexBlend(1, (EGxTexBlend)state.mValue.GetAsInt()); break;
    case GxRs_TexBlend2: ISetTexBlend(2, (EGxTexBlend)state.mValue.GetAsInt()); break;
    case GxRs_TexBlend3: ISetTexBlend(3, (EGxTexBlend)state.mValue.GetAsInt()); break;
    case GxRs_TexGen0: ISetTexGen(0, (EGxTexGen)state.mValue.GetAsInt()); break;
    case GxRs_TexGen1: ISetTexGen(1, (EGxTexGen)state.mValue.GetAsInt()); break;
    case GxRs_TexGen2: ISetTexGen(2, (EGxTexGen)state.mValue.GetAsInt()); break;
    case GxRs_TexGen3: ISetTexGen(3, (EGxTexGen)state.mValue.GetAsInt()); break;
    case GxRs_TexLodBias0: ISetTexLodBias(0, state.mValue.GetAsFloat()); break;
    case GxRs_TexLodBias1: ISetTexLodBias(1, state.mValue.GetAsFloat()); break;
    case GxRs_TexLodBias2: ISetTexLodBias(2, state.mValue.GetAsFloat()); break;
    case GxRs_TexLodBias3: ISetTexLodBias(3, state.mValue.GetAsFloat()); break;
    case GxRs_TextureShader0:
    case GxRs_TextureShader1:
    case GxRs_TextureShader2:
    case GxRs_TextureShader3:
    case GxRs_VertexShader:
      return;
    case GxRs_PixelShader:
      IPixelShaderBind((CGxPixelShader *)state.mValue.GetAsPointer());
      break;
    default:
      FATALASSERT(0);
      break;
  }
}
