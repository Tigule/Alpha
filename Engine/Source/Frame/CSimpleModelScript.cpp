#include "Frame/CSimpleModel.h"

#include <lauxlib.h>
#include <lua.h>

#define GET_SIMPLE_MODEL_THIS(L, object)                         \
  CSimpleModel *object = 0;                                      \
  if (lua_type(L, 1) == LUA_TTABLE) {                            \
    lua_rawgeti(L, 1, 0);                                        \
    object = static_cast<CSimpleModel *>(lua_touserdata(L, -1)); \
    lua_pop(L, 1);                                               \
  } else {                                                       \
    luaL_error(                                                  \
        L,                                                       \
        "Attempt to find 'this' in non-table object (used '.' "  \
        "instead of ':' ?)"                                      \
    );                                                           \
  }                                                              \
  ASSERT(object)

static int CSimpleModel_SetModel(lua_State *L) {
  GET_SIMPLE_MODEL_THIS(L, model);

  if (!lua_isstring(L, 2)) {
    luaL_error(L, "Usage: SetModel(\"file\")");
  }

  const char *filename = lua_tostring(L, 2);
  model->SetModel(filename, 0, 0);
  if (!model->GetModel()) {
    char message[512];
    SStrPrintf(message, sizeof(message), "Invalid model file: %s", filename);
    luaL_error(L, message);
  }

  return 0;
}

static int CSimpleModel_ClearModel(lua_State *L) {
  GET_SIMPLE_MODEL_THIS(L, object);

  object->SetModel(0);
  return 0;
}

static int CSimpleModel_SetPosition(lua_State *L) {
  GET_SIMPLE_MODEL_THIS(L, object);

  NTempest::C3Vector pos;
  pos.x = static_cast<float>(lua_tonumber(L, 2));
  pos.y = static_cast<float>(lua_tonumber(L, 3));
  pos.z = static_cast<float>(lua_tonumber(L, 4));
  object->SetPosition(pos);
  return 0;
}

static int CSimpleModel_SetFacing(lua_State *L) {
  GET_SIMPLE_MODEL_THIS(L, object);

  if (!lua_isnumber(L, 2)) {
    luaL_error(L, "Usage: SetFacing(facing)");
  }

  object->SetFacing(static_cast<float>(lua_tonumber(L, 2)));
  return 0;
}

static int CSimpleModel_SetScale(lua_State *L) {
  GET_SIMPLE_MODEL_THIS(L, object);

  if (!lua_isnumber(L, 2)) {
    luaL_error(L, "Usage: SetScale(scale)");
  }

  object->SetScale(static_cast<float>(lua_tonumber(L, 2)));
  return 0;
}

static int CSimpleModel_SetSequence(lua_State *L) {
  GET_SIMPLE_MODEL_THIS(L, object);

  if (!lua_isnumber(L, 2)) {
    luaL_error(L, "Usage: SetSequence(sequence)");
  }

  object->SetSequence(static_cast<unsigned int>(lua_tonumber(L, 2)));
  return 0;
}

static int CSimpleModel_SetSequenceTime(lua_State *L) {
  GET_SIMPLE_MODEL_THIS(L, object);

  if (!lua_isnumber(L, 2) || !lua_isnumber(L, 3)) {
    luaL_error(L, "Usage: SetSequenceTime(sequence, time)");
  }

  object->SetSequenceTime(static_cast<unsigned int>(lua_tonumber(L, 2)), static_cast<int>(lua_tonumber(L, 3)));
  return 0;
}

static int CSimpleModel_SetAlpha(lua_State *L) {
  GET_SIMPLE_MODEL_THIS(L, object);

  if (!lua_isnumber(L, 2)) {
    luaL_error(L, "Usage: SetAlpha(alpha)");
  }

  object->SetAlpha(static_cast<unsigned char>(lua_tonumber(L, 2)));
  return 0;
}

static int CSimpleModel_SetCamera(lua_State *L) {
  GET_SIMPLE_MODEL_THIS(L, object);

  if (!lua_isnumber(L, 2)) {
    luaL_error(L, "Usage: SetCamera(index)");
  }

  object->SetCameraByIndex(static_cast<unsigned int>(lua_tonumber(L, 2)));
  return 0;
}

static int CSimpleModel_SetLight(lua_State *L) {
  GET_SIMPLE_MODEL_THIS(L, model);

  if (!lua_isnumber(L, 2)) {
    luaL_error(L, "Usage: SetLight(enabled[, omni, dirX, dirY, dirZ, ambIntensity[, ambR, ambG, ambB], dirIntensity[, dirR, dirG, dirB]])");
  }

  CGxLight light;
  light.m_enabled = static_cast<int>(lua_tonumber(L, 2)) != 0;
  if (!light.m_enabled) {
    return 0;
  }

  if (!lua_isnumber(L, 3) || !lua_isnumber(L, 4) || !lua_isnumber(L, 5) || !lua_isnumber(L, 6) || !lua_isnumber(L, 7)) {
    luaL_error(L, "Usage: SetLight(enabled[, omni, dirX, dirY, dirZ, ambIntensity[, ambR, ambG, ambB], dirIntensity[, dirR, dirG, dirB]])");
  }

  light.m_isOmni = static_cast<int>(lua_tonumber(L, 3)) != 0;
  light.m_dir =
      NTempest::C3Vector(static_cast<float>(lua_tonumber(L, 4)), static_cast<float>(lua_tonumber(L, 5)), static_cast<float>(lua_tonumber(L, 6)));
  light.m_ambIntensity = static_cast<float>(lua_tonumber(L, 7));

  int index = 8;
  if (!NTempest::CMath::fequal_(light.m_ambIntensity, 0.0f) && lua_isnumber(L, 8) && lua_isnumber(L, 9) && lua_isnumber(L, 10)) {
    float red = static_cast<float>(lua_tonumber(L, 8));
    float green = static_cast<float>(lua_tonumber(L, 9));
    float blue = static_cast<float>(lua_tonumber(L, 10));
    light.m_ambColor.Set(1.0f, red, green, blue);
    index = 11;
  }

  if (!lua_isnumber(L, index)) {
    luaL_error(L, "Usage: SetLight(enabled[, omni, dirX, dirY, dirZ, ambIntensity[, ambR, ambG, ambB], dirIntensity[, dirR, dirG, dirB]])");
  }

  light.m_dirIntensity = static_cast<float>(lua_tonumber(L, index));
  ++index;

  if (!NTempest::CMath::fequal_(light.m_dirIntensity, 0.0f) && lua_isnumber(L, index) && lua_isnumber(L, index + 1) && lua_isnumber(L, index + 2)) {
    float red = static_cast<float>(lua_tonumber(L, index));
    float green = static_cast<float>(lua_tonumber(L, index + 1));
    float blue = static_cast<float>(lua_tonumber(L, index + 2));
    light.m_dirColor.Set(1.0f, red, green, blue);
  }

  model->SetLight(light);
  return 0;
}

static int CSimpleModel_GetPosition(lua_State *L) {
  GET_SIMPLE_MODEL_THIS(L, object);

  NTempest::C3Vector pos = object->GetPosition();
  lua_pushnumber(L, pos.x);
  lua_pushnumber(L, pos.y);
  lua_pushnumber(L, pos.z);
  return 3;
}

static int CSimpleModel_GetFacing(lua_State *L) {
  GET_SIMPLE_MODEL_THIS(L, object);

  lua_pushnumber(L, object->GetFacing());
  return 1;
}

static int CSimpleModel_GetScale(lua_State *L) {
  GET_SIMPLE_MODEL_THIS(L, object);

  lua_pushnumber(L, object->GetScale());
  return 1;
}

static int CSimpleModel_AdvanceTime(lua_State *L) {
  GET_SIMPLE_MODEL_THIS(L, object);

  object->AdvanceTime();
  return 0;
}

static int CSimpleModel_ReplaceIconTexture(lua_State *L) {
  GET_SIMPLE_MODEL_THIS(L, object);

  if (!lua_isstring(L, 2)) {
    luaL_error(L, "Usage: ReplaceIconTexture(\"texture\")");
  }

  object->ReplaceTexture(14, lua_tostring(L, 2));
  return 0;
}

static int CSimpleModel_SetFogColor(lua_State *L) {
  GET_SIMPLE_MODEL_THIS(L, object);

  float red = static_cast<float>(lua_tonumber(L, 2));
  float green = static_cast<float>(lua_tonumber(L, 3));
  float blue = static_cast<float>(lua_tonumber(L, 4));
  float alpha = 1.0f;
  if (lua_isnumber(L, 5)) {
    alpha = static_cast<float>(lua_tonumber(L, 5));
  }

  NTempest::CImVector color;
  color.Set(alpha, red, green, blue);
  object->SetFogColor(color);
  object->SetFog(1);
  return 0;
}

static int CSimpleModel_SetFogNear(lua_State *L) {
  GET_SIMPLE_MODEL_THIS(L, object);

  if (!lua_isnumber(L, 2)) {
    luaL_error(L, "Usage: SetFogNear(value)");
  }

  object->SetFogNear(static_cast<float>(lua_tonumber(L, 2)));
  return 0;
}

static int CSimpleModel_SetFogFar(lua_State *L) {
  GET_SIMPLE_MODEL_THIS(L, object);

  if (!lua_isnumber(L, 2)) {
    luaL_error(L, "Usage: SetFogFar(value)");
  }

  object->SetFogFar(static_cast<float>(lua_tonumber(L, 2)));
  return 0;
}

static int CSimpleModel_ClearFog(lua_State *L) {
  GET_SIMPLE_MODEL_THIS(L, object);

  object->SetFog(0);
  return 0;
}

#undef GET_SIMPLE_MODEL_THIS

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

int CSimpleModel::LookupScriptMethod(lua_State *L, const char *name) {
  if (FrameScript_Object::LookupScriptMethod(L, name, s_scriptMethods)) {
    return 1;
  }

  return CSimpleFrame::LookupScriptMethod(L, name);
}
