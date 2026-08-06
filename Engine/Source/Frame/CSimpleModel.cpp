#include "Frame/CSimpleModel.h"

#include "Base/Status.h"
#include "Base/Coordinate.h"
#include "FrameXML/LoadXML.h"
#include "FrameXML/XMLTree.h"
#include "Gx/Gx.h"
#include "Services/Camera.h"
#include "Services/DataMgr.h"
#include "Services/SysMessage.h"
#include "Services/Texture.h"
#include "Tempest/c2vector.h"
#include "Tempest/c44matrix.h"

static int AnimFinishedCallback(LPVOID param) {
  CSimpleModel *model = static_cast<CSimpleModel *>(param);

  model->RunOnAnimFinishedScript();
  return 1;
}

CSimpleModel::CSimpleModel(CSimpleFrame *parent)
    : CSimpleFrame(parent),
      m_model(0),
      m_camera(0),
      m_position(0.0f),
      m_facing(0.0f),
      m_scale(1.0f),
      m_flags(0),
      m_fogNear(0.0f),
      m_fogFar(1.0f),
      m_cachedExtents(0.0f),
      m_onUpdateModel(0),
      m_onAnimFinished(0) {
  m_light.m_enabled = 0;
  m_light.m_isOmni = 1;
  m_light.m_ambColor.Set(
      static_cast<BYTE>(NTempest::CMath::fuint_n(255.0f)), static_cast<BYTE>(NTempest::CMath::fuint_n(255.0f)), static_cast<BYTE>(255),
      static_cast<BYTE>(255)
  );
  m_light.m_dirColor.Set(
      static_cast<BYTE>(NTempest::CMath::fuint_n(255.0f)), static_cast<BYTE>(NTempest::CMath::fuint_n(255.0f)), static_cast<BYTE>(255),
      static_cast<BYTE>(255)
  );
  m_light.m_ambIntensity = 1.0f;
  m_light.m_dirIntensity = 1.0f;

  m_fogColor.Set(0xFFFFFFFFUL);
}

CSimpleModel::~CSimpleModel() {
  SetModel(0);
  SetCameraInternal(0);
  SetOnUpdateModelScript(0);
  SetOnAnimFinishedScript(0);
}

void CSimpleModel::LoadXML(const XMLNode *node, CStatus *status) {
  CSimpleFrame::LoadXML(node, status);

  LPCSTR value = node->GetAttributeByName("file");
  if (value && *value) {
    SetModel(value, 0, status);
    if (!m_model) {
      status->Add(STATUS_WARNING, "Bad model file: %s", value);
    }
  }

  value = node->GetAttributeByName("scale");
  if (value && *value) {
    float scale = SStrToFloat(value);
    if (scale > 0.0f) {
      m_scale = scale;
    } else {
      status->Add(STATUS_WARNING, "Invalid model scale: %s", value);
    }
  }

  value = node->GetAttributeByName("fogNear");
  if (value && *value) {
    m_fogNear = max(SStrToFloat(value), 0.0f);
  }

  value = node->GetAttributeByName("fogFar");
  if (value && *value) {
    m_fogFar = max(SStrToFloat(value), 0.0f);
  }

  for (const XMLNode *child = node->GetChild(); child; child = child->GetSibling()) {
    if (!SStrCmpI(child->GetName(), "FogColor", 0x7FFFFFFF)) {
      NTempest::CImVector color;
      color.Set(0UL);
      LoadXML_Color(child, color, status);
      m_fogColor = color;
      m_flags |= 0x2;
    }
  }

  SetAlpha(m_alpha);
}

void CSimpleModel::LoadXML_Scripts(const XMLNode *node, CStatus *status) {
  CSimpleFrame::LoadXML_Scripts(node, status);

  for (const XMLNode *script = node->GetChild(); script; script = script->GetSibling()) {
    if (!SStrCmpI(script->GetName(), "OnUpdateModel", 0x7FFFFFFF)) {
      SetOnUpdateModelScript(script->GetBody());
    } else if (!SStrCmpI(script->GetName(), "OnAnimFinished", 0x7FFFFFFF)) {
      SetOnAnimFinishedScript(script->GetBody());
    }
  }
}

void CSimpleModel::SetModel(LPCSTR sourcefile, CModelCreate *data, CStatus *status) {
  HMODEL model = 0;

  if (sourcefile && *sourcefile) {
    model = ModelCreate(sourcefile, data, status);
  }

  SetModel(model);
  if (model) {
    HandleClose(model);
  }
}

void CSimpleModel::SetModel(HMODEL model) {
  if (m_model) {
    HandleClose(m_model);
  }

  m_model = static_cast<HMODEL>(HandleDuplicate(model));
  if (!m_model) {
    return;
  }

  ModelSetLightSelectCallback(m_model, 0, 0, 1);
  ModelSetVertexAlpha(m_model, m_alpha, 1);
  ModelSetSeqFinishedHandler(m_model, AnimFinishedCallback, this);

  if (ModelIsLoaded(m_model, 1)) {
    FinishLoadingModel();
  } else {
    m_flags &= ~0x1;
  }
}

void CSimpleModel::SetCameraInternal(HCAMERA camera) {
  if (!(m_flags & 0x4) && m_camera) {
    HandleClose(m_camera);
  }

  m_camera = camera;
  m_flags &= ~0x4;
}

void CSimpleModel::FinishLoadingModel() {
  if (m_flags & 0x4) {
    SetCameraInternal(ModelGetCamera(m_model, m_cameraIndex));
  }

  ModelGetExtents(m_model, &m_cachedExtents);
  m_flags |= 0x1;
  CLayoutFrame::Resize(1);
}

void CSimpleModel::SetCamera(HCAMERA camera) {
  SetCameraInternal(static_cast<HCAMERA>(HandleDuplicate(camera)));
}

void CSimpleModel::SetCameraByIndex(UINT index) {
  ASSERT(m_model);

  if (m_flags & 0x1) {
    SetCameraInternal(ModelGetCamera(m_model, index));
  } else {
    if (!(m_flags & 0x4) && m_camera) {
      HandleClose(m_camera);
    }

    m_cameraIndex = index;
    m_flags |= 0x4;
  }
}

void CSimpleModel::SetLight(const CGxLight &light) {
  m_light = light;
}

void CSimpleModel::SetSequence(UINT index) {
  if (m_model) {
    ModelSetSequence(m_model, index, 8);
  }
}

int CSimpleModel::SetSequenceTime(UINT index, int timeOffset) {
  if (m_model) {
    return ModelForceSequenceTime(m_model, index, timeOffset, 0);
  }

  return 1;
}

int CSimpleModel::AdvanceTime() {
  if (m_model) {
    return ModelAdvanceTime(m_model);
  }

  return 1;
}

void CSimpleModel::ReplaceTexture(UINT materialID, LPCSTR textureName) {
  if (m_model) {
    CStatus  status;
    HTEXTURE texture = TextureCreate(textureName, CGxTexFlags(GxTex_LinearMipNearest, 0, 0, 0, 0, 0, 1), &status, 0);

    SysMsgAdd(status, 4);
    ModelReplaceTexture(m_model, materialID, texture, 0);
    HandleClose(texture);
  }
}

int CSimpleModel::ModelJustLoaded() const {
  return m_model && !(m_flags & 0x1) && ModelIsLoaded(m_model, 1);
}

float CSimpleModel::GetWidth() {
  float width = CLayoutFrame::GetWidth();
  if (width == 0.0f) {
    if (ModelJustLoaded()) {
      FinishLoadingModel();
    }
    width = m_cachedExtents.t.x - m_cachedExtents.b.x;
  }
  return width;
}

float CSimpleModel::GetHeight() {
  float height = CLayoutFrame::GetHeight();
  if (height == 0.0f) {
    if (ModelJustLoaded()) {
      FinishLoadingModel();
    }
    height = m_cachedExtents.t.y - m_cachedExtents.b.y;
  }
  return height;
}

void CSimpleModel::SetAlpha(BYTE alpha) {
  CSimpleFrame::SetAlpha(alpha);

  if (m_model) {
    ModelSetVertexAlpha(m_model, m_alpha, 1);
  }
}

void CSimpleModel::OnFrameRender(CRenderBatch *batch, UINT layer) {
  CSimpleFrame::OnFrameRender(batch, layer);

  if (m_model && layer == 2) {
    batch->QueueCallback(RenderModel, this);
  }
}

void CSimpleModel::UpdateModel() {
  NTempest::C3Vector cameraTarg(0.0f);
  NTempest::C3Vector cameraPos(0.0f);

  if (!(m_flags & 0x1) && ModelIsLoaded(m_model, 0)) {
    FinishLoadingModel();
  }

  if (!(m_flags & 0x4) && m_camera) {
    DataMgrGetCoord(reinterpret_cast<HDATAMGR>(m_camera), 7, &cameraPos);
    DataMgrGetCoord(reinterpret_cast<HDATAMGR>(m_camera), 8, &cameraTarg);
  }

  if (m_onUpdateModel) {
    FrameScript_Execute(m_onUpdateModel, this);
  } else if (!ModelAdvanceTime(m_model)) {
    return;
  }

  ModelAnimate(
      m_model, m_position * m_layoutScale, m_facing, NTempest::C3Vector(0.0f, 0.0f, 1.0f), m_scale * m_layoutScale, cameraPos, cameraTarg - cameraPos
  );
}

void CSimpleModel::RenderModel(LPVOID param) {
  CSimpleModel *simpleModel = static_cast<CSimpleModel *>(param);
  if (!simpleModel->m_model) {
    return;
  }

  NTempest::CRect viewRect(0.0f);
  if (!simpleModel->GetRect(&viewRect)) {
    if (!simpleModel->m_model || !simpleModel->ModelJustLoaded()) {
      return;
    }

    simpleModel->FinishLoadingModel();
    if (!simpleModel->GetRect(&viewRect)) {
      return;
    }
  }

  NTempest::C44Matrix saved_proj;
  NTempest::C44Matrix saved_view;
  float               minX;
  float               maxX;
  float               minY;
  float               maxY;
  float               minZ;
  float               maxZ;

  GxXformProjection(saved_proj);
  GxXformView(saved_view);
  GxXformViewport(minX, maxX, minY, maxY, minZ, maxZ);

  HCAMERA            camera = (simpleModel->m_flags & 0x4) ? 0 : simpleModel->m_camera;
  NTempest::C3Vector cameraTarg(0.0f);
  NTempest::C3Vector cameraPos(0.0f);

  if (camera) {
    DataMgrGetCoord(reinterpret_cast<HDATAMGR>(camera), 7, &cameraPos);
    DataMgrGetCoord(reinterpret_cast<HDATAMGR>(camera), 8, &cameraTarg);
    CameraSetupWorldProjection(camera, viewRect, 0);
  } else {
    CameraSetupScreenProjection(viewRect, NTempest::C2Vector(viewRect.l, viewRect.t), 0.0f);
  }

  DDCToNDC(viewRect.l, viewRect.t, &viewRect.l, &viewRect.t);
  DDCToNDC(viewRect.r, viewRect.b, &viewRect.r, &viewRect.b);

  if (viewRect.l <= 0.0f) {
    viewRect.l = 0.0f;
  }
  if (viewRect.r <= 0.0f) {
    viewRect.r = 0.0f;
  }
  if (viewRect.t <= 0.0f) {
    viewRect.t = 0.0f;
  }
  if (viewRect.b <= 0.0f) {
    viewRect.b = 0.0f;
  }

  if (viewRect.l != viewRect.r && viewRect.t != viewRect.b) {
    GxXformSetViewport(viewRect.l, viewRect.r, viewRect.t, viewRect.b, 0.0f, 1.0f);
    GxSceneClear(2);

    CGxLight nullLight;
    nullLight.m_enabled = 0;
    UINT   whichLight = 0;
    HMODEL model = simpleModel->m_model;

    if (simpleModel->m_light.m_enabled) {
      GxLightSet(0, simpleModel->m_light, NTempest::C3Vector(0.0f));
      whichLight = 1;
    } else {
      const UINT numLights = ModelGetNumLights(model);
      for (; whichLight < numLights; ++whichLight) {
        const CGxLight *modelLight = ModelGetLight(model, whichLight);
        ASSERT(modelLight);

        CGxLight light = *modelLight;
        if (camera && light.m_isOmni) {
          light.m_dir = light.m_dir - cameraPos;
        }
        GxLightSet(whichLight, light, NTempest::C3Vector(0.0f));
      }
    }

    for (; whichLight < 8; ++whichLight) {
      GxLightSet(whichLight, nullLight, NTempest::C3Vector(0.0f));
    }

    const int fogEnabled = GxMasterEnable(GxMasterEnable_Fog);
    GxRsPush();
    if (simpleModel->m_flags & 0x2) {
      GxMasterEnableSet(GxMasterEnable_Fog, 1);
      GxRsSet(GxRs_Fog, 1);
      GxRsSet(GxRs_FogStyle, 0);
      GxRsSet(GxRs_FogStart, simpleModel->m_fogNear);
      GxRsSet(GxRs_FogEnd, simpleModel->m_fogFar);
      GxRsSet(GxRs_FogColor, simpleModel->GetFogColor());
    } else {
      GxMasterEnableSet(GxMasterEnable_Fog, 0);
    }

    simpleModel->UpdateModel();

    NTempest::C3Vector cameraDir = cameraTarg - cameraPos;
    const float        cameraDirMag = cameraDir.Mag();
    if (NTempest::CMath::fabs_(cameraDirMag) >= 0.00000023841858f) {
      cameraDir = cameraDir * (1.0f / cameraDirMag);
    }

    ModelScenePlaceCamera(cameraPos, cameraDir);
    ModelAddToScene(model, 0);
    ModelRenderScene(0);

    GxRsPop();
    GxMasterEnableSet(GxMasterEnable_Fog, fogEnabled);
  }

  GxXformSetProjection(saved_proj);
  GxXformSetView(saved_view);
  GxXformSetViewport(minX, maxX, minY, maxY, minZ, maxZ);
}
