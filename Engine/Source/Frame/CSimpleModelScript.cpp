#include <Base/Base.h>

#include "Frame/CSimpleTop.h"
#include "Frame/CSimpleModel.h"

#include <lauxlib.h>
#include <lua.h>

static int CSimpleModel_SetModel(lua_State *L) {
  CSimpleModel *object = (CSimpleModel *)FrameScript_GetObjectThis(L);

  if (lua_isstring(L, 2)) {
    LPCSTR filename = lua_tostring(L, 2);
    object->SetModel(filename, 0, 0);
    if (!object->GetModel()) {
      char message[512];
      SStrPrintf(message, sizeof(message), "Invalid model file: %s", filename);
      luaL_error(L, message);
      return 0;
    }
  } else {
    luaL_error(L, "Usage: SetModel(\"file\")");
  }

  return 0;
}

static int CSimpleModel_ClearModel(lua_State *L) {
  CSimpleModel *object = (CSimpleModel *)FrameScript_GetObjectThis(L);

  object->SetModel(0);
  return 0;
}

static int CSimpleModel_SetPosition(lua_State *L) {
  CSimpleModel *object = (CSimpleModel *)FrameScript_GetObjectThis(L);

  float              x = lua_tonumber(L, 2);
  float              y = lua_tonumber(L, 3);
  NTempest::C3Vector pos(x, y, lua_tonumber(L, 4));
  object->SetPosition(pos);
  return 0;
}

static int CSimpleModel_SetFacing(lua_State *L) {
  CSimpleModel *object = (CSimpleModel *)FrameScript_GetObjectThis(L);

  if (lua_isnumber(L, 2)) {
    object->SetFacing(lua_tonumber(L, 2));
    return 0;
  }

  luaL_error(L, "Usage: SetFacing(facing)");
  return 0;
}

static int CSimpleModel_SetScale(lua_State *L) {
  CSimpleModel *object = (CSimpleModel *)FrameScript_GetObjectThis(L);

  if (lua_isnumber(L, 2)) {
    object->SetScale(lua_tonumber(L, 2));
    return 0;
  }

  luaL_error(L, "Usage: SetScale(scale)");
  return 0;
}

static int CSimpleModel_SetSequence(lua_State *L) {
  CSimpleModel *object = (CSimpleModel *)FrameScript_GetObjectThis(L);

  if (lua_isnumber(L, 2)) {
    object->SetSequence(lua_tonumber(L, 2));
    return 0;
  }

  luaL_error(L, "Usage: SetSequence(sequence)");
  return 0;
}

static int CSimpleModel_SetSequenceTime(lua_State *L) {
  CSimpleModel *object = (CSimpleModel *)FrameScript_GetObjectThis(L);

  if (lua_isnumber(L, 2) && lua_isnumber(L, 3)) {
    object->SetSequenceTime(lua_tonumber(L, 2), lua_tonumber(L, 3));
    return 0;
  }

  luaL_error(L, "Usage: SetSequenceTime(sequence, time)");
  return 0;
}

static int CSimpleModel_SetAlpha(lua_State *L) {
  CSimpleModel *object = (CSimpleModel *)FrameScript_GetObjectThis(L);

  if (lua_isnumber(L, 2)) {
    object->SetAlpha(lua_tonumber(L, 2));
    return 0;
  }

  luaL_error(L, "Usage: SetAlpha(alpha)");
  return 0;
}

static int CSimpleModel_SetCamera(lua_State *L) {
  CSimpleModel *object = (CSimpleModel *)FrameScript_GetObjectThis(L);

  if (lua_isnumber(L, 2)) {
    object->SetCameraByIndex(lua_tonumber(L, 2));
    return 0;
  }

  luaL_error(L, "Usage: SetCamera(index)");
  return 0;
}

static int CSimpleModel_SetLight(lua_State *L) {
  CSimpleModel *object = (CSimpleModel *)FrameScript_GetObjectThis(L);

  CSimpleModel *model = object;
  if (lua_isnumber(L, 2)) {
    CGxLight light;
    light.m_enabled = (int)lua_tonumber(L, 2) != 0;
    if (!light.m_enabled) {
      return 0;
    }

    if (lua_isnumber(L, 3) && lua_isnumber(L, 4) && lua_isnumber(L, 5) && lua_isnumber(L, 6) && lua_isnumber(L, 7)) {
      light.m_isOmni = (int)lua_tonumber(L, 3) != 0;
      light.m_dir = NTempest::C3Vector(lua_tonumber(L, 4), lua_tonumber(L, 5), lua_tonumber(L, 6));

      int index = 8;
      light.m_ambIntensity = lua_tonumber(L, 7);
      if (!NTempest::CMath::fequal_(light.m_ambIntensity, 0.0f) && lua_isnumber(L, index) && lua_isnumber(L, index + 1) && lua_isnumber(L, index + 2)) {
        float red = lua_tonumber(L, index);
        float green = lua_tonumber(L, index + 1);
        float blue = lua_tonumber(L, index + 2);
        light.m_ambColor.Set(1.0f, red, green, blue);
        index += 3;
      }

      if (lua_isnumber(L, index)) {
        light.m_dirIntensity = lua_tonumber(L, index++);
        if (!NTempest::CMath::fequal_(light.m_dirIntensity, 0.0f) && lua_isnumber(L, index) && lua_isnumber(L, index + 1) && lua_isnumber(L, index + 2)) {
          float red = lua_tonumber(L, index);
          float green = lua_tonumber(L, index + 1);
          float blue = lua_tonumber(L, index + 2);
          light.m_dirColor.Set(1.0f, red, green, blue);
        }

        model->SetLight(light);
        return 0;
      }
    }
  }

  luaL_error(L, "Usage: SetLight(enabled[, omni, dirX, dirY, dirZ, ambIntensity[, ambR, ambG, ambB], dirIntensity[, dirR, dirG, dirB]])");
  return 0;
}

static int CSimpleModel_GetPosition(lua_State *L) {
  CSimpleModel *object = (CSimpleModel *)FrameScript_GetObjectThis(L);

  NTempest::C3Vector pos = object->GetPosition();
  lua_pushnumber(L, pos.x);
  lua_pushnumber(L, pos.y);
  lua_pushnumber(L, pos.z);
  return 3;
}

static int CSimpleModel_GetFacing(lua_State *L) {
  CSimpleModel *object = (CSimpleModel *)FrameScript_GetObjectThis(L);

  lua_pushnumber(L, object->GetFacing());
  return 1;
}

static int CSimpleModel_GetScale(lua_State *L) {
  CSimpleModel *object = (CSimpleModel *)FrameScript_GetObjectThis(L);

  lua_pushnumber(L, object->GetScale());
  return 1;
}

static int CSimpleModel_AdvanceTime(lua_State *L) {
  CSimpleModel *object = (CSimpleModel *)FrameScript_GetObjectThis(L);

  object->AdvanceTime();
  return 0;
}

static int CSimpleModel_ReplaceIconTexture(lua_State *L) {
  CSimpleModel *object = (CSimpleModel *)FrameScript_GetObjectThis(L);

  if (lua_isstring(L, 2)) {
    object->ReplaceTexture(14, lua_tostring(L, 2));
    return 0;
  }

  luaL_error(L, "Usage: ReplaceIconTexture(\"texture\")");
  return 0;
}

static int CSimpleModel_SetFogColor(lua_State *L) {
  CSimpleModel *object = (CSimpleModel *)FrameScript_GetObjectThis(L);

  float red = lua_tonumber(L, 2);
  float green = lua_tonumber(L, 3);
  float blue = lua_tonumber(L, 4);
  float alpha = 1.0f;
  if (lua_isnumber(L, 5)) {
    alpha = lua_tonumber(L, 5);
  }

  NTempest::CImVector color;
  color.Set(alpha, red, green, blue);
  object->SetFogColor(color);
  object->SetFog(1);
  return 0;
}

static int CSimpleModel_SetFogNear(lua_State *L) {
  CSimpleModel *object = (CSimpleModel *)FrameScript_GetObjectThis(L);

  if (lua_isnumber(L, 2)) {
    object->SetFogNear(lua_tonumber(L, 2));
    return 0;
  }

  luaL_error(L, "Usage: SetFogNear(value)");
  return 0;
}

static int CSimpleModel_SetFogFar(lua_State *L) {
  CSimpleModel *object = (CSimpleModel *)FrameScript_GetObjectThis(L);

  if (lua_isnumber(L, 2)) {
    object->SetFogFar(lua_tonumber(L, 2));
    return 0;
  }

  luaL_error(L, "Usage: SetFogFar(value)");
  return 0;
}

static int CSimpleModel_ClearFog(lua_State *L) {
  CSimpleModel *object = (CSimpleModel *)FrameScript_GetObjectThis(L);

  object->SetFog(0);
  return 0;
}

static FrameScript_Method SimpleModelMethods[19] = {
    {          "SetModel",           CSimpleModel_SetModel},
    {        "ClearModel",         CSimpleModel_ClearModel},
    {       "SetPosition",        CSimpleModel_SetPosition},
    {         "SetFacing",          CSimpleModel_SetFacing},
    {          "SetScale",           CSimpleModel_SetScale},
    {       "SetSequence",        CSimpleModel_SetSequence},
    {   "SetSequenceTime",    CSimpleModel_SetSequenceTime},
    {          "SetAlpha",           CSimpleModel_SetAlpha},
    {         "SetCamera",          CSimpleModel_SetCamera},
    {          "SetLight",           CSimpleModel_SetLight},
    {       "GetPosition",        CSimpleModel_GetPosition},
    {         "GetFacing",          CSimpleModel_GetFacing},
    {          "GetScale",           CSimpleModel_GetScale},
    {       "AdvanceTime",        CSimpleModel_AdvanceTime},
    {"ReplaceIconTexture", CSimpleModel_ReplaceIconTexture},
    {       "SetFogColor",        CSimpleModel_SetFogColor},
    {        "SetFogNear",         CSimpleModel_SetFogNear},
    {         "SetFogFar",          CSimpleModel_SetFogFar},
    {          "ClearFog",           CSimpleModel_ClearFog}
};

TSHashTable<FrameScriptObject_Variable, HASHKEY_STR> CSimpleModel::s_scriptMethods;

void CSimpleModel::RegisterScriptMethods() {
  FrameScript_Object::FillScriptMethodTable(SimpleModelMethods, 19, s_scriptMethods);
}

void CSimpleModel::UnregisterScriptMethods() {
  FrameScript_Object::EmptyScriptMethodTable(s_scriptMethods);
}

BOOL CSimpleModel::LookupScriptMethod(lua_State *L, LPCSTR name) {
  if (FrameScript_Object::LookupScriptMethod(L, name, s_scriptMethods)) {
    return 1;
  }

  return CSimpleFrame::LookupScriptMethod(L, name);
}
