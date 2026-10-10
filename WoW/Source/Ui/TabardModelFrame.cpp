#include <Base/Base.h>
#include <Frame/CSimpleTop.h>
#include <Frame/CSimpleModel.h>
#include <WowConst.h>
#include <MapDefs.h>
#include <WorldClient/World.h>
#include "Net/NetClient/NetClient.h"
#include "Object/ObjectClient/Unit_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "UIUtil/InputControl.h"
#include "WorldFrame.h"
#include "GameUI.h"

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

#define UPPER_EMBLEM_TEXTURE_WIDTH  128
#define UPPER_EMBLEM_TEXTURE_HEIGHT 64
#define LOWER_EMBLEM_TEXTURE_WIDTH  128
#define LOWER_EMBLEM_TEXTURE_HEIGHT 32

static void EmblemTextureUpdate(EGxTexCommand cmd, UINT w, UINT h, UINT d, UINT mipLevel, LPVOID userArg, UINT &texelStrideInBytes, LPCVOID &texels);

static const int s_maxVariations[TABARDVARS_NUMVARS] = {42, 4, 2, 4, 19};

void GuildCallback(int id, const DWORDLONG &guid, LPVOID arg, bool granted) {
  if (granted) {
    FrameScript_SignalEvent(359);
  }
}

CGTabardModelFrame::CGTabardModelFrame(CSimpleFrame *parent) : CGCharacterModelBase(parent), m_charComponent(0) {
}

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
  }
  m_charComponent = 0;

  InitializeTabardColors(playerPtr);
  HTEXTURE skinTexture = CharCustomizationSetSkin(model, playerPtr->GetDisplayRace(), playerPtr->GetDisplaySex(), playerPtr->GetSkin(), 0);
  if (skinTexture) {
    m_charComponent = TexComponentCreate(skinTexture, playerPtr->GetDisplayRace(), playerPtr->GetDisplaySex(), playerPtr->GetSkin(), 0, 1);
    if (m_charComponent) {
      TexComponentCopy(m_charComponent, playerPtr->GetTexComponent());
    }
    UpdateTabard();
    HandleClose(skinTexture);
  }
}

void CGTabardModelFrame::InitializeTabardColors(const CGPlayer_C *playerPtr) {
  const GuildStats_C *guild = g_guildInfoCache.GetRecord(playerPtr->GetGuildID(), 0, 0, 0);
  if (guild && guild->m_emblemStyle != -1 && guild->m_emblemColor != -1 && guild->m_borderStyle != -1 && guild->m_borderColor != -1 &&
      guild->m_backgroundColor != -1)
  {
    m_variations[0] = guild->m_emblemStyle;
    m_variations[1] = guild->m_emblemColor;
    m_variations[2] = guild->m_borderStyle;
    m_variations[3] = guild->m_borderColor;
    m_variations[4] = guild->m_backgroundColor;
    return;
  }

  NTempest::CRndSeed seed;
  seed.SetSeed(OsGetAsyncTimeMs());
  for (int i = 0; i < TABARDVARS_NUMVARS; ++i) {
    m_variations[i] = NTempest::CRandom::dice_(0, s_maxVariations[i] - 1, seed);
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
    player->SaveTabard(m_variations[0], m_variations[1], m_variations[2], m_variations[3], m_variations[4], CGGameUI::GetInteractTarget());
  }
}

BOOL CGTabardModelFrame::CanSaveTabard() {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player) {
    return 0;
  }

  const GuildStats_C *guild = g_guildInfoCache.GetRecord(player->GetGuildID(), 0, GuildCallback, 0);
  return guild && player->GetGuildRank() == 0 && guild->m_emblemStyle == -1 && guild->m_emblemColor == -1 && guild->m_borderStyle == -1 &&
         guild->m_borderColor == -1 && guild->m_backgroundColor == -1;
}

void CGTabardModelFrame::CycleVariation(UINT index, int delta) {
  FATALASSERT(index < TABARDVARS_NUMVARS);
  if (abs(delta) < s_maxVariations[index]) {
    m_variations[index] += delta + s_maxVariations[index];
    m_variations[index] %= s_maxVariations[index];
    UpdateTabard();
  }
}

static int CGTabardModelFrame_Save(lua_State *L) {
  CGTabardModelFrame *object = (CGTabardModelFrame *)FrameScript_GetObjectThis(L);
  object->SaveTabard();
  return 0;
}

static int CGTabardModelFrame_CanSave(lua_State *L) {
  CGTabardModelFrame *object = (CGTabardModelFrame *)FrameScript_GetObjectThis(L);
  if (object->CanSaveTabard()) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int CGTabardModelFrame_CycleVariation(lua_State *L) {
  CGTabardModelFrame *object = (CGTabardModelFrame *)FrameScript_GetObjectThis(L);
  if (!lua_isnumber(L, 2) || !lua_isnumber(L, 3)) {
    luaL_error(L, "Usage: CycleVariation(variationIndex, delta)");
    return 0;
  }
  UINT index = (UINT)lua_tonumber(L, 2) - 1;
  if (index >= 5) {
    luaL_error(L, "Invalid variationIndex in CycleVariation");
    return 0;
  }
  object->CycleVariation(index, lua_tonumber(L, 3));
  return 0;
}

static int CGTabardModelFrame_GetUpperBackgroundFileName(lua_State *L) {
  CGTabardModelFrame *object = (CGTabardModelFrame *)FrameScript_GetObjectThis(L);
  char string[MAX_PATH];
  GetTabardBackgroundFileName(5, object->GetVariation(4), string, MAX_PATH);
  lua_pushstring(L, string);
  return 1;
}

static int CGTabardModelFrame_GetLowerBackgroundFileName(lua_State *L) {
  CGTabardModelFrame *object = (CGTabardModelFrame *)FrameScript_GetObjectThis(L);
  char string[MAX_PATH];
  GetTabardBackgroundFileName(6, object->GetVariation(4), string, MAX_PATH);
  lua_pushstring(L, string);
  return 1;
}

static int CGTabardModelFrame_GetUpperEmblemFileName(lua_State *L) {
  CGTabardModelFrame *object = (CGTabardModelFrame *)FrameScript_GetObjectThis(L);
  char string[MAX_PATH];
  GetTabardEmblemFileName(5, object->GetVariation(0), object->GetVariation(1), string, MAX_PATH);
  lua_pushstring(L, string);
  return 1;
}

static int CGTabardModelFrame_GetLowerEmblemFileName(lua_State *L) {
  CGTabardModelFrame *object = (CGTabardModelFrame *)FrameScript_GetObjectThis(L);
  char string[MAX_PATH];
  GetTabardEmblemFileName(6, object->GetVariation(0), object->GetVariation(1), string, MAX_PATH);
  lua_pushstring(L, string);
  return 1;
}

static int CGTabardModelFrame_GetUpperEmblemTexture(lua_State *L) {
  if (!lua_isstring(L, 2)) {
    luaL_error(L, "Usage: GetUpperEmblemTexture(textureName)");
    return 0;
  }
  CGTabardModelFrame *object = (CGTabardModelFrame *)FrameScript_GetObjectThis(L);
  CSimpleTexture *texture = SimpleTextureRegistryGetEntry(lua_tostring(L, 2), 0);
  if (!texture) {
    luaL_error(L, "Invalid texture name in GetUpperEmblemTexture");
    return 0;
  }

  static TSFixedArray<NTempest::CImVector> pixels;
  if (!pixels.Count()) {
    pixels.SetCount(UPPER_EMBLEM_TEXTURE_WIDTH * UPPER_EMBLEM_TEXTURE_HEIGHT);
  }

  char file[MAX_PATH];
  GetTabardEmblemFileName(5, object->GetVariation(0), object->GetVariation(1), file, MAX_PATH);
  SStrPack(file, ".BLP", MAX_PATH);

  CBLPFile image;
  if (image.Open(file)) {
    FATALASSERT(image.Width() == UPPER_EMBLEM_TEXTURE_WIDTH);
    FATALASSERT(image.Height() == UPPER_EMBLEM_TEXTURE_HEIGHT);
    BYTE *imageData;
    UINT  stride;
    if (image.Lock(PIXEL_ARGB8888, 0, imageData, stride)) {
      for (UINT i = 0; i < pixels.Count(); ++i) {
        pixels[i].Set(0x00FFFFFF | ((DWORD)imageData[4 * i + 3] << 24));
      }
    }
    image.Unlock(0);
    image.Close();
  }

  CGxTex *gxTex;
  GxTexCreate(UPPER_EMBLEM_TEXTURE_WIDTH, UPPER_EMBLEM_TEXTURE_HEIGHT, GxTex_Argb8888, CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1), &pixels, EmblemTextureUpdate, gxTex);
  HTEXTURE handle = TextureCreate(gxTex);
  texture->SetTexture(handle);
  HandleClose(handle);
  return 0;
}

static void EmblemTextureUpdate(EGxTexCommand cmd, UINT w, UINT h, UINT d, UINT mipLevel, LPVOID userArg, UINT &texelStrideInBytes, LPCVOID &texels) {
  if (cmd == GxTex_Latch) {
    texelStrideInBytes = 4 * w;
    texels = ((TSFixedArray<NTempest::CImVector> *)userArg)->Ptr();
  }
}

static int CGTabardModelFrame_GetLowerEmblemTexture(lua_State *L) {
  if (!lua_isstring(L, 2)) {
    luaL_error(L, "Usage: GetLowerEmblemTexture(textureName)");
    return 0;
  }
  CGTabardModelFrame *object = (CGTabardModelFrame *)FrameScript_GetObjectThis(L);
  CSimpleTexture *texture = SimpleTextureRegistryGetEntry(lua_tostring(L, 2), 0);
  if (!texture) {
    luaL_error(L, "Invalid texture name in GetLowerEmblemTexture");
    return 0;
  }

  static TSFixedArray<NTempest::CImVector> pixels;
  if (!pixels.Count()) {
    pixels.SetCount(LOWER_EMBLEM_TEXTURE_WIDTH * LOWER_EMBLEM_TEXTURE_HEIGHT);
  }

  char file[MAX_PATH];
  GetTabardEmblemFileName(6, object->GetVariation(0), object->GetVariation(1), file, MAX_PATH);
  SStrPack(file, ".BLP", MAX_PATH);

  CBLPFile image;
  if (image.Open(file)) {
    FATALASSERT(image.Width() == LOWER_EMBLEM_TEXTURE_WIDTH);
    FATALASSERT(image.Height() == LOWER_EMBLEM_TEXTURE_HEIGHT);
    BYTE *imageData;
    UINT  stride;
    if (image.Lock(PIXEL_ARGB8888, 0, imageData, stride)) {
      for (UINT i = 0; i < pixels.Count(); ++i) {
        pixels[i].Set(0x00FFFFFF | ((DWORD)imageData[4 * i + 3] << 24));
      }
    }
    image.Unlock(0);
    image.Close();
  }

  CGxTex *gxTex;
  GxTexCreate(LOWER_EMBLEM_TEXTURE_WIDTH, LOWER_EMBLEM_TEXTURE_HEIGHT, GxTex_Argb8888, CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1), &pixels, EmblemTextureUpdate, gxTex);
  HTEXTURE handle = TextureCreate(gxTex);
  texture->SetTexture(handle);
  HandleClose(handle);
  return 0;
}

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

BOOL CGTabardModelFrame::LookupScriptMethod(lua_State *L, LPCSTR name) {
  if (FrameScript_Object::LookupScriptMethod(L, name, s_scriptMethods)) {
    return 1;
  }
  return CGCharacterModelBase::LookupScriptMethod(L, name);
}
