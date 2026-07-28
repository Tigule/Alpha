#include "Ui/TabardModelFrame.h"
#include "Ui/TabardCreationFrame.h"

#include "Component/CharacterCustomization.h"
#include "Component/Component.h"
#include "DB/DBClient/DBCacheInstances.h"
#include "Object/GuildStats.h"
#include "Object/ObjectClient/Player_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"

#include <BLPFile/blp.h>
#include <Base/Status.h>
#include <Frame/CSimpleRender.h>
#include <Frame/SimpleFrameRegistry.h>
#include <Gx/Gx.h>
#include <Os/OsTime.h>
#include <Services/Texture.h>
#include <Tempest/cmath.h>
#include <Tempest/crandom.h>
#include <Tempest/cimvector.h>

#include <lauxlib.h>
#include <lua.h>

#include <string.h>
#include <stdlib.h>

static void GuildCallback(int, const unsigned __int64 &, void *, bool granted);

static const unsigned int s_maxVariations[TABARDVARS_NUMVARS] = {42, 4, 2, 4, 19};

static void EmblemTextureUpdate(
    EGxTexCommand cmd,
    unsigned int  w,
    unsigned int  h,
    unsigned int  d,
    unsigned int  mipLevel,
    void         *userArg,
    unsigned int &texelStrideInBytes,
    const void  *&texels
) {
  if (cmd == GxTex_Latch) {
    texelStrideInBytes = 4 * w;
    texels = static_cast<TSFixedArray<NTempest::CImVector> *>(userArg)->Ptr();
  }
}

static void GuildCallback(int, const unsigned __int64 &, void *, bool granted) {
  if (granted) {
    FrameScript_SignalEvent(359);
  }
}

CGTabardModelFrame::CGTabardModelFrame(CSimpleFrame *parent) : CGCharacterModelBase(parent), m_charComponent(0) {
}

#define GET_TABARD_MODEL_THIS(L, object)                               \
  CGTabardModelFrame *object = 0;                                      \
  if (lua_type(L, 1) == LUA_TTABLE) {                                  \
    lua_rawgeti(L, 1, 0);                                              \
    object = static_cast<CGTabardModelFrame *>(lua_touserdata(L, -1)); \
    lua_pop(L, 1);                                                     \
  } else {                                                             \
    return luaL_error(                                                 \
        L,                                                             \
        "Attempt to find 'this' in non-table object (used '.' "        \
        "instead of ':' ?)"                                            \
    );                                                                 \
  }                                                                    \
  FATALASSERT(object)

void CGTabardModelFrame::InitializeModel(HMODEL model) {
  if (!model) {
    return;
  }

  ModelHideGeosets(model, 1202, 0);
  CGPlayer_C *playerPtr = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!playerPtr) {
    return;
  }

  if (m_charComponent) {
    HandleClose(m_charComponent);
    m_charComponent = 0;
  }

  InitializeTabardColors(playerPtr);
  HTEXTURE skinTexture = CharCustomizationSetSkin(model, playerPtr->GetDisplayRace(), playerPtr->GetDisplaySex(), playerPtr->SkinVariationID(), 0);
  if (skinTexture) {
    m_charComponent = TexComponentCreate(skinTexture, playerPtr->GetDisplayRace(), playerPtr->GetDisplaySex(), playerPtr->SkinVariationID(), 0, 1);
    if (m_charComponent) {
      TexComponentCopy(m_charComponent, playerPtr->GetTexComponent());
    }
    UpdateTabard();
    HandleClose(skinTexture);
  }
}

void CGTabardModelFrame::InitializeTabardColors(const CGPlayer_C *playerPtr) {
  const unsigned __int64 noGuid = 0;
  const GuildStats_C    *guild = g_guildInfoCache.GetRecord(playerPtr->GetGuildID(), noGuid, GuildCallback, 0);
  if (guild && guild->m_emblemStyle != -1 && guild->m_emblemColor != -1 && guild->m_borderStyle != -1 && guild->m_borderColor != -1 &&
      guild->m_backgroundColor != -1)
  {
    m_variations[0] = guild->m_emblemStyle;
    m_variations[1] = guild->m_emblemColor;
    m_variations[2] = guild->m_borderStyle;
    m_variations[3] = guild->m_borderColor;
    m_variations[4] = guild->m_backgroundColor;
  } else {
    NTempest::CRndSeed seed;
    seed.SetSeed(OsGetAsyncTimeMs());
  for (unsigned int i = 0; i < TABARDVARS_NUMVARS; ++i) {
      m_variations[i] = NTempest::CMath::mulhwu_(s_maxVariations[i], NTempest::CRandom::uint32_(seed));
    }
  }
}

void CGTabardModelFrame::UpdateTabard() {
  if (!m_charComponent) {
    return;
  }

  ComponentApplyTabardTexture(m_charComponent, m_variations[0], m_variations[1], m_variations[2], m_variations[3], m_variations[4]);
  ComponentForceTabardDraw(m_charComponent);

  CStatus status;
  TexComponentCommitSections(&status, m_charComponent, 1);
}

void CGTabardModelFrame::SaveTabard() {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    player->SaveTabard(m_variations[0], m_variations[1], m_variations[2], m_variations[3], m_variations[4], CGTabardCreationFrame::GetVendor());
  }
}

int CGTabardModelFrame::CanSaveTabard() {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player) {
    return 0;
  }

  const unsigned __int64 noGuid = 0;
  const GuildStats_C    *guild = g_guildInfoCache.GetRecord(player->GetGuildID(), noGuid, GuildCallback, 0);
  return guild && player->GetGuildRank() == 0 && guild->m_emblemStyle == -1 && guild->m_emblemColor == -1 && guild->m_borderStyle == -1 &&
         guild->m_borderColor == -1 && guild->m_backgroundColor == -1;
}

void CGTabardModelFrame::CycleVariation(unsigned int index, int delta) {
  FATALASSERT(index < TABARDVARS_NUMVARS);
  if (static_cast<unsigned int>(abs(delta)) < s_maxVariations[index]) {
    m_variations[index] = (m_variations[index] + delta + s_maxVariations[index]) % s_maxVariations[index];
    UpdateTabard();
  }
}

static int CGTabardModelFrame_Save(lua_State *L) {
  GET_TABARD_MODEL_THIS(L, object);
  object->SaveTabard();
  return 0;
}

static int CGTabardModelFrame_CanSave(lua_State *L) {
  GET_TABARD_MODEL_THIS(L, object);
  if (object->CanSaveTabard()) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int CGTabardModelFrame_CycleVariation(lua_State *L) {
  GET_TABARD_MODEL_THIS(L, object);
  if (!lua_isnumber(L, 2) || !lua_isnumber(L, 3)) {
    return luaL_error(L, "Usage: CycleVariation(index, delta)");
  }
  unsigned int index = static_cast<unsigned int>(lua_tonumber(L, 2)) - 1;
  if (index >= 5) {
    return luaL_error(L, "Invalid variation index");
  }
  object->CycleVariation(index, static_cast<int>(lua_tonumber(L, 3)));
  return 0;
}

static int CGTabardModelFrame_GetUpperBackgroundFileName(lua_State *L) {
  GET_TABARD_MODEL_THIS(L, object);
  char string[260];
  GetTabardBackgroundFileName(5, object->GetVariation(4), string, 260);
  lua_pushstring(L, string);
  return 1;
}

static int CGTabardModelFrame_GetLowerBackgroundFileName(lua_State *L) {
  GET_TABARD_MODEL_THIS(L, object);
  char string[260];
  GetTabardBackgroundFileName(6, object->GetVariation(4), string, 260);
  lua_pushstring(L, string);
  return 1;
}

static int CGTabardModelFrame_GetUpperEmblemFileName(lua_State *L) {
  GET_TABARD_MODEL_THIS(L, object);
  char string[260];
  GetTabardEmblemFileName(5, object->GetVariation(0), object->GetVariation(1), string, 260);
  lua_pushstring(L, string);
  return 1;
}

static int CGTabardModelFrame_GetLowerEmblemFileName(lua_State *L) {
  GET_TABARD_MODEL_THIS(L, object);
  char string[260];
  GetTabardEmblemFileName(6, object->GetVariation(0), object->GetVariation(1), string, 260);
  lua_pushstring(L, string);
  return 1;
}

static int CGTabardModelFrame_GetUpperEmblemTexture(lua_State *L) {
  if (!lua_isstring(L, 2)) {
    return luaL_error(L, "Usage: GetUpperEmblemTexture(textureName)");
  }
  GET_TABARD_MODEL_THIS(L, object);
  CSimpleTexture *texture = SimpleTextureRegistryGetEntry(lua_tostring(L, 2), 0);
  if (!texture) {
    return luaL_error(L, "Invalid texture name in GetUpperEmblemTexture");
  }

  static TSFixedArray<NTempest::CImVector> pixels;
  if (!pixels.Count()) {
    pixels.SetCount(128 * 64);
  }

  char file[260];
  GetTabardEmblemFileName(5, object->GetVariation(0), object->GetVariation(1), file, 260);
  strncat(file, ".BLP", 259 - strlen(file));

  CBLPFile image;
  if (image.Open(file)) {
    FATALASSERT(image.Width() == 128);
    FATALASSERT(image.Height() == 64);
    unsigned char *imageData;
    unsigned int   stride;
    if (image.Lock(PIXEL_ARGB8888, 0, imageData, stride)) {
      for (unsigned int i = 0; i < pixels.Count(); ++i) {
        pixels[i] = NTempest::CImVector(0x00FFFFFF | (static_cast<unsigned long>(imageData[4 * i + 3]) << 24));
      }
      image.Unlock(0);
    }
  }

  CGxTex *gxTex;
  GxTexCreate(128, 64, GxTex_Argb8888, CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1), &pixels, EmblemTextureUpdate, gxTex);
  HTEXTURE handle = TextureCreate(gxTex);
  texture->SetTexture(handle);
  HandleClose(handle);
  image.Close();
  return 0;
}

static int CGTabardModelFrame_GetLowerEmblemTexture(lua_State *L) {
  if (!lua_isstring(L, 2)) {
    return luaL_error(L, "Usage: GetLowerEmblemTexture(textureName)");
  }
  GET_TABARD_MODEL_THIS(L, object);
  CSimpleTexture *texture = SimpleTextureRegistryGetEntry(lua_tostring(L, 2), 0);
  if (!texture) {
    return luaL_error(L, "Invalid texture name in GetLowerEmblemTexture");
  }

  static TSFixedArray<NTempest::CImVector> pixels;
  if (!pixels.Count()) {
    pixels.SetCount(128 * 64);
  }

  char file[260];
  GetTabardEmblemFileName(6, object->GetVariation(0), object->GetVariation(1), file, 260);
  strncat(file, ".BLP", 259 - strlen(file));

  CBLPFile image;
  if (image.Open(file)) {
    FATALASSERT(image.Width() == 128);
    FATALASSERT(image.Height() == 64);
    unsigned char *imageData;
    unsigned int   stride;
    if (image.Lock(PIXEL_ARGB8888, 0, imageData, stride)) {
      for (unsigned int i = 0; i < pixels.Count(); ++i) {
        pixels[i] = NTempest::CImVector(0x00FFFFFF | (static_cast<unsigned long>(imageData[4 * i + 3]) << 24));
      }
      image.Unlock(0);
    }
  }

  CGxTex *gxTex;
  GxTexCreate(128, 64, GxTex_Argb8888, CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1), &pixels, EmblemTextureUpdate, gxTex);
  HTEXTURE handle = TextureCreate(gxTex);
  texture->SetTexture(handle);
  HandleClose(handle);
  image.Close();
  return 0;
}

#undef GET_TABARD_MODEL_THIS

static FrameScript_Method CGTabardModelFrameMethods[9] = {
    {                      "Save",                       CGTabardModelFrame_Save},
    {                   "CanSave",                    CGTabardModelFrame_CanSave},
    {            "CycleVariation",             CGTabardModelFrame_CycleVariation},
    {"GetUpperBackgroundFileName", CGTabardModelFrame_GetUpperBackgroundFileName},
    {"GetLowerBackgroundFileName", CGTabardModelFrame_GetLowerBackgroundFileName},
    {    "GetUpperEmblemFileName",     CGTabardModelFrame_GetUpperEmblemFileName},
    {    "GetLowerEmblemFileName",     CGTabardModelFrame_GetLowerEmblemFileName},
    {     "GetUpperEmblemTexture",      CGTabardModelFrame_GetUpperEmblemTexture},
    {     "GetLowerEmblemTexture",      CGTabardModelFrame_GetLowerEmblemTexture}
};

TSHashTable<FrameScriptObject_Variable, HASHKEY_STR> CGTabardModelFrame::s_scriptMethods;

void CGTabardModelFrame::RegisterScriptMethods() {
  FrameScript_Object::FillScriptMethodTable(CGTabardModelFrameMethods, 9, s_scriptMethods);
}

void CGTabardModelFrame::UnregisterScriptMethods() {
  FrameScript_Object::EmptyScriptMethodTable(s_scriptMethods);
}

int CGTabardModelFrame::LookupScriptMethod(lua_State *L, const char *name) {
  if (FrameScript_Object::LookupScriptMethod(L, name, s_scriptMethods)) {
    return 1;
  }
  return CGCharacterModelBase::LookupScriptMethod(L, name);
}
