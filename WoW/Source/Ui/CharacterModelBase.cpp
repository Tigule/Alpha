#include <Base/Base.h>
#include <Frame/CSimpleTop.h>
#include <Frame/CSimpleModel.h>
#include <WowConst.h>
#include <MapDefs.h>
#include <WorldClient/World.h>
#include "Net/NetClient/NetClient.h"
#include "Object/ObjectClient/Unit_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "SoundInterface/SoundInterface.h"
#include "UIUtil/InputControl.h"
#include "WorldFrame.h"
#include "GameUI.h"

#include "CharacterModelBase.h"

#include "GameUI.h"
#include "Object/ObjectClient/Unit_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"

#include <FrameScript/FrameScript.h>
#include <Services/Camera.h>
#include <Services/DataMgr.h>
#include <Tempest/caasphere.h>

#include <lauxlib.h>
#include <lua.h>

DWORDLONG Script_GetGUIDFromName(LPCSTR name);

static int Script_SetUnit(lua_State *L);
static int Script_UpdateModel(lua_State *L);
static int Script_SetRotation(lua_State *L);

static FrameScript_Method CGCharacterModelBaseMethods[3] = {
    {    "SetUnit",     Script_SetUnit},
    {"UpdateModel", Script_UpdateModel},
    {"SetRotation", Script_SetRotation}
};

TSHashTable<FrameScriptObject_Variable, HASHKEY_STR> CGCharacterModelBase::s_scriptMethods;

void CGCharacterModelBase::UpdateModel() {
  NTempest::C3Vector cameraTarg(0.0f);
  NTempest::C3Vector cameraPos(0.0f);

  if (!(m_flags & 0x1) && ModelIsLoaded(m_model, 1)) {
    FinishLoadingModel();
  }

  ConfigureCamera();

  if (!(m_flags & 0x4) && m_camera) {
    DataMgrGetCoord(reinterpret_cast<HDATAMGR>(m_camera), 7, &cameraPos);
    DataMgrGetCoord(reinterpret_cast<HDATAMGR>(m_camera), 8, &cameraTarg);
  }

  if (m_onUpdateModel) {
    FrameScript_Execute(m_onUpdateModel, this);
  } else if (!ModelAdvanceTime(m_model)) {
    return;
  }

  ModelAnimate(m_model, m_position, m_rotationScale, NTempest::C3Vector(0.0f, 0.0f, 1.0f), m_scale, cameraPos, cameraTarg - cameraPos);
}

void CGCharacterModelBase::ConfigureCamera() {
  if (!(m_flags & 0x4) && !m_camera) {
    NTempest::CAaSphere bounds;
    bounds.c = NTempest::C3Vector(0.0f);
    bounds.r = 0.0f;
    ModelGetBounds(m_model, &bounds);

    HCAMERA camera = CameraCreate();
    DataMgrSetFloat(reinterpret_cast<HDATAMGR>(camera), 4, 0.5f);
    DataMgrSetCoord(reinterpret_cast<HDATAMGR>(camera), 8, bounds.c, 0);
    DataMgrSetCoord(reinterpret_cast<HDATAMGR>(camera), 7, NTempest::C3Vector(5.5555558f, 0.0f, 2.4166667f), 0);
    DataMgrSetFloat(reinterpret_cast<HDATAMGR>(camera), 3, 0.027777778f);
    SetCamera(camera);
    HandleClose(camera);
  }
}

void CGCharacterModelBase::SetUnit(DWORDLONG unitGUID) {
  m_unit = unitGUID;

  CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(unitGUID, __FILE__, __LINE__));
  if (!unit) {
    return;
  }

  HMODEL model = unit->GetPaperDollModel(GetUniquePaperDollModel());
  InitializeModel(model);
  SetModel(model);
  HandleClose(model);
  SetCameraByIndex(1);
  ConfigureCamera();

  CGxLight light;
  light.m_enabled = 1;
  light.m_dir = NTempest::C3Vector(0.0f, -0.707106f, -0.707106f);
  light.m_ambColor.Set(0xFFFFFFFFUL);
  light.m_dirColor.Set(0xFFFFFFCCUL);
  light.m_ambIntensity = 0.7f;
  light.m_dirIntensity = 0.8f;
  SetLight(light);
}

CGCharacterModelBase::~CGCharacterModelBase() {
  SetUnit(0);
}

CGCharacterModelBase::CGCharacterModelBase(CSimpleFrame *parent) : CSimpleModel(parent), m_unit(0), m_rotationScale(0.0f) {
}

static int Script_SetUnit(lua_State *L) {
  CGCharacterModelBase *object = static_cast<CGCharacterModelBase *>(FrameScript_GetObjectThis(L));
  if (!lua_isstring(L, 2)) {
    luaL_error(L, "Usage: SetUnit(\"unit\")");
    return 0;
  }
  object->SetUnit(Script_GetGUIDFromName(lua_tostring(L, 2)));
  return 0;
}

static int Script_UpdateModel(lua_State *L) {
  CGCharacterModelBase *object = static_cast<CGCharacterModelBase *>(FrameScript_GetObjectThis(L));
  object->UpdateModel();
  return 0;
}

static int Script_SetRotation(lua_State *L) {
  CGCharacterModelBase *object = static_cast<CGCharacterModelBase *>(FrameScript_GetObjectThis(L));
  if (!lua_isnumber(L, 2)) {
    return luaL_error(L, "Usage: SetRotation(rotation (in radians))");
  }
  object->SetRotationScale(static_cast<float>(lua_tonumber(L, 2)));
  return 0;
}

void CGCharacterModelBase::RegisterScriptMethods() {
  FrameScript_Object::FillScriptMethodTable(CGCharacterModelBaseMethods, 3, s_scriptMethods);
}

void CGCharacterModelBase::UnregisterScriptMethods() {
  FrameScript_Object::EmptyScriptMethodTable(s_scriptMethods);
}

BOOL CGCharacterModelBase::LookupScriptMethod(lua_State *L, LPCSTR name) {
  if (FrameScript_Object::LookupScriptMethod(L, name, s_scriptMethods)) {
    return 1;
  }

  return CSimpleModel::LookupScriptMethod(L, name);
}
