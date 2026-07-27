#include "ItemTextFrame.h"

#include "DB/DBClient/DBCacheInstances.h"
#include "DB/DBClient/AutoCode/PageTextMaterialRec.h"
#include "Object/ItemStats.h"
#include "Object/ObjectClient/GameObject_C.h"
#include "Object/ObjectClient/Item_C.h"
#include "Object/ObjectClient/Player_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "Ui/ChatFrame.h"
#include "Ui/GameUI.h"

#include <FrameScript/FrameScript.h>
#include <lauxlib.h>
#include <lua.h>
#include <string.h>

static const float MAX_SHOP_DISTANCE = 5.5555553f;
static const float MAX_SHOP_DISTANCE_SQUARED = MAX_SHOP_DISTANCE * MAX_SHOP_DISTANCE;

unsigned __int64     CGItemText::m_itemGUID;
unsigned int         CGItemText::m_currentPage;
TSGrowableArray<int> CGItemText::m_pages;
char                 CGItemText::m_text[0x200];

void CGItemText::InitializeGame() {
  m_itemGUID = 0;
  m_currentPage = 0;
  m_pages.SetCount(2);
  m_pages[0] = 0;
  m_pages[1] = 0;
  m_text[0] = 0;
}

void CGItemText::ShutdownGame() {
  m_pages.Clear();
}

void CGItemText::EnterWorld() {
}

void CGItemText::LeaveWorld() {
  const unsigned __int64 noItem = 0;
  SetItem(noItem, 0);
}

void CGItemText::ItemTextCallback(int, const unsigned __int64 &guid, void *, bool granted) {
  if (granted && m_itemGUID == guid) {
    SetItem(guid, 1);
  }
}

void CGItemText::SetItem(const unsigned __int64 &item, int callback) {
  if (!callback && !item && item == m_itemGUID) {
    CGGameUI::ClearInteractTarget(m_itemGUID);
    FrameScript_SignalEvent(276);
    return;
  }

  if (item != m_itemGUID) {
    CGGameUI::ClearInteractTarget(m_itemGUID);
    m_itemGUID = item;
    m_currentPage = 0;
    m_pages[0] = 0;
    m_pages[1] = 0;
    m_text[0] = 0;
    if (!m_itemGUID) {
      FrameScript_SignalEvent(276);
      return;
    }
  }

  CGObject_C *object = ClntObjMgrObjectPtr(item, __FILE__, __LINE__);
  if (!object) {
    m_itemGUID = 0;
    return;
  }

  int page = object->GetPageTextID(ItemTextCallback);
  if (!page) {
    return;
  }

  m_pages[0] = page;
  const PageTextCache_C *text = g_pageTextCache.GetRecord(m_pages[m_currentPage], object->GetGUID(), ItemTextCallback, 0);
  if (!text) {
    return;
  }

  CGGameUI::SetInteractTarget(item, MAX_SHOP_DISTANCE_SQUARED);
  FrameScript_SignalEvent(273);

  if (!(object->GetType() & TYPE_ITEM) || static_cast<CGItem_C *>(object)->IsTranslated())
  {
    DisplayText(item, 1);
    return;
  }

  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    player->ReadItem(item, 0);
  }
}

void CGItemText::DisplayText(const unsigned __int64 &item, int useSkill) {
  if (item != m_itemGUID) {
    return;
  }

  CGObject_C *object = ClntObjMgrObjectPtr(item, __FILE__, __LINE__);
  if (!object) {
    return;
  }

  const PageTextCache_C *text = g_pageTextCache.GetRecord(m_pages[m_currentPage], object->GetGUID(), ItemTextCallback, 0);
  if (!text) {
    return;
  }

  if (m_pages.Count() == m_currentPage + 2) {
    m_pages.SetCount(m_pages.Count() * 2);
  }
  m_pages[m_currentPage + 1] = text->m_nextPage;

  unsigned int language = 0;
  if (object->GetType() & TYPE_ITEM) {
    CGItem_C *itemObject = static_cast<CGItem_C *>(object);
    if (itemObject->IsTranslated()) {
      SStrCopy(m_text, text->m_text, sizeof(m_text));
      FrameScript_SignalEvent(275);
      return;
    }

    const unsigned __int64 noGuid = 0;
    const ItemStats_C     *stats = g_itemDBCache.GetRecord(*object->GetData(3), noGuid, 0, 0);
    FATALASSERT(stats);
    language = stats->m_languageID;
  } else if (object->GetType() & TYPE_GAMEOBJECT) {
    language = static_cast<CGGameObject_C *>(object)->GetPageTextLanguage();
  } else {
    FrameScript_SignalEvent(275);
    return;
  }

  unsigned int skill = 0;
  if (useSkill) {
    CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
    if (player) {
      player->GetLanguageSkill(language, skill);
    }
  }

  CGChat::TranslateMessage(language, skill, text->m_text, m_text, sizeof(m_text), 1);
  FrameScript_SignalEvent(275);
}

void CGItemText::PrevPage() {
  if (m_itemGUID && m_currentPage) {
    --m_currentPage;
    DisplayText(m_itemGUID, 1);
  }
}

void CGItemText::NextPage() {
  if (m_itemGUID && m_pages[m_currentPage + 1] > 0) {
    ++m_currentPage;
    DisplayText(m_itemGUID, 1);
  }
}

static int Script_ItemTextGetItem(lua_State *L) {
  CGObject_C *object = ClntObjMgrObjectPtr(CGItemText::GetItem(), __FILE__, __LINE__);
  lua_pushstring(L, object ? object->GetObjectName() : 0);
  return 1;
}

static int Script_ItemTextGetMaterial(lua_State *L) {
  CGObject_C *object = ClntObjMgrObjectPtr(CGItemText::GetItem(), __FILE__, __LINE__);
  int         material = 0;
  if (object) {
    if (object->GetType() & TYPE_ITEM) {
      const unsigned __int64 noGuid = 0;
      const ItemStats_C     *stats = g_itemDBCache.GetRecord(object->GetEntryID(), noGuid, 0, 0);
      if (stats) {
        material = stats->m_pageMaterial;
      }
    } else if (object->GetType() & TYPE_GAMEOBJECT) {
      material = static_cast<CGGameObject_C *>(object)->GetPageTextMaterial();
    }
  }

  const PageTextMaterialRec *rec = material > 0 ? g_pageTextMaterialDB.GetRecord(material) : 0;
  lua_pushstring(L, rec ? rec->m_name : 0);
  return 1;
}

static int Script_ItemTextGetPage(lua_State *L) {
  lua_pushnumber(L, static_cast<double>(CGItemText::GetCurrentPage() + 1));
  return 1;
}

static int Script_ItemTextGetText(lua_State *L) {
  lua_pushstring(L, CGItemText::GetText());
  return 1;
}

static int Script_ItemTextHasNextPage(lua_State *L) {
  if (CGItemText::HasNextPage()) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int Script_ItemTextPrevPage(lua_State *L) {
  CGItemText::PrevPage();
  return 0;
}

static int Script_ItemTextNextPage(lua_State *L) {
  CGItemText::NextPage();
  return 0;
}

static int Script_CloseItemText(lua_State *__formal) {
  const unsigned __int64 noItem = 0;
  CGItemText::SetItem(noItem, 0);
  return 0;
}

static FrameScript_Method s_ScriptFunctions[8] = {
    {    "ItemTextGetItem",     Script_ItemTextGetItem},
    {"ItemTextGetMaterial", Script_ItemTextGetMaterial},
    {    "ItemTextGetPage",     Script_ItemTextGetPage},
    {    "ItemTextGetText",     Script_ItemTextGetText},
    {"ItemTextHasNextPage", Script_ItemTextHasNextPage},
    {   "ItemTextPrevPage",    Script_ItemTextPrevPage},
    {   "ItemTextNextPage",    Script_ItemTextNextPage},
    {      "CloseItemText",       Script_CloseItemText}
};

void ItemTextRegisterScriptFunctions() {
  for (unsigned int i = 0; i < 8; ++i) {
    FrameScript_RegisterFunction(s_ScriptFunctions[i].name, s_ScriptFunctions[i].method);
  }
}

void ItemTextUnregisterScriptFunctions() {
  for (unsigned int i = 0; i < 8; ++i) {
    FrameScript_UnregisterFunction(s_ScriptFunctions[i].name);
  }
}
