#include <WowConst.h>
#include <MapDefs.h>

#include <Base/Handle.h>
#include <Base/Status.h>
#include <Frame/CSimpleFrame.h>
#include <Gx/Gx.h>
#include <Model/IModel.h>
#include <Model/ModelInternal.h>
#include <Services/SysMessage.h>
#include <Services/Texture.h>
#include <Tempest/c2vector.h>
#include <Tempest/c3vector.h>
#include <Tempest/cimvector.h>
#include <storm.h>

#include "DB/DBClient/DBClient.h"
#include "Object/ObjectClient/Item_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "SoundInterface/SoundInterface.h"
#include "Ui/GameUI.h"
#include "UIUtil/Cursor.h"

static LPCSTR s_animationNames[NUM_CURSOR_ANIMS] = {
    "Point",       "Cast",       "Buy",       "Attack",       "Interact",       "Speak",       "RangedAttack",       "Pickup",       "Taxi",
    "UnablePoint", "UnableCast", "UnableBuy", "UnableAttack", "UnableInteract", "UnableSpeak", "UnableRangedAttack", "UnablePickup", "UnableTaxi"
};

static CURSORANIMATIONS s_cursorType;
static CURSORANIMATIONS s_cursorMode;
static HMODEL           s_cursorModel;
CGCursor               *g_cursor;

void CursorResetCursor(int force);

void CursorInitialize() {
  CursorDestroy();
  g_cursor = NEW(CGCursor);
  g_cursor->SetArt(ClientDBStringLookup(SLOOKUP_DEFAULTCURSOR));
}

void CursorDestroy() {
  if (g_cursor) {
    DEL(g_cursor);
    g_cursor = 0;
  }

  if (s_cursorModel) {
    HandleClose(s_cursorModel);
    s_cursorModel = 0;
  }
}

BOOL CursorGrabMoney(HMODEL model) {
  if (!g_cursor || g_cursor->GetItemType() != CURSOR_EMPTY) {
    return 0;
  }
  g_cursor->SetItemType(CURSOR_MONEY);
  g_cursor->Grab(model);
  return 1;
}

BOOL CursorGrabSpell(HMODEL model) {
  if (!g_cursor || g_cursor->GetItemType() != CURSOR_EMPTY) {
    return 0;
  }
  g_cursor->SetItemType(CURSOR_SPELL);
  g_cursor->Grab(model);
  return 1;
}

static void CreateCursorIconModel(HTEXTURE texture) {
  NTempest::C3Vector vertices[4] = {
      NTempest::C3Vector(-0.0144f, -0.0144f, 0.01f), NTempest::C3Vector(-0.0144f, 0.0144f, 0.01f), NTempest::C3Vector(0.0144f, -0.0144f, 0.01f),
      NTempest::C3Vector(0.0144f, 0.0144f, 0.01f)
  };
  NTempest::C3Vector normals[4] = {
      NTempest::C3Vector(0.0f, 0.0f, 1.0f), NTempest::C3Vector(0.0f, 0.0f, 1.0f), NTempest::C3Vector(0.0f, 0.0f, 1.0f),
      NTempest::C3Vector(0.0f, 0.0f, 1.0f)
  };
  NTempest::C2Vector texCoords[4] = {
      NTempest::C2Vector(0.0f, 1.0f), NTempest::C2Vector(0.0f, 0.0f), NTempest::C2Vector(1.0f, 1.0f), NTempest::C2Vector(1.0f, 0.0f)
  };
  WORD primVertIndices[4] = {1, 0, 3, 2};

  s_cursorModel = ModelCreateSimpleMesh(
      "CursorGrabSpell", 4, vertices, normals, texCoords, GxPrim_TriangleStrip, primVertIndices, 4, texture, GxBlend_Alpha, 0x21,
      NTempest::CImVector(255, 255, 255, 255), 1
  );
  FATALASSERT(s_cursorModel);
}

BOOL CursorGrabMoney(UINT amount) {
  LPCSTR path = ClientDBStringLookup(SLOOKUP_INVENTORYICONPATH);
  LPCSTR separator = path && *path ? "\\" : "";
  char   buffer[MAX_PATH];
  SStrPrintf(buffer, sizeof(buffer), "%s%s", path, separator);

  LPCSTR art;
  if (amount < 10) {
    art = "INV_Misc_Coin_05";
  } else if (amount < 100) {
    art = "INV_Misc_Coin_06";
  } else if (amount < 1000) {
    art = "INV_Misc_Coin_03";
  } else if (amount < 10000) {
    art = "INV_Misc_Coin_04";
  } else if (amount < 100000) {
    art = "INV_Misc_Coin_01";
  } else {
    art = "INV_Misc_Coin_02";
  }
  SStrCopy(buffer + SStrLen(buffer), art, sizeof(buffer) - SStrLen(buffer));

  CStatus     status;
  CGxTexFlags flags(GxTex_Linear, 0, 0, 0, 0, 0, 1);
  HTEXTURE    texture = TextureCreate(buffer, flags, &status, 0);
  SysMsgAdd(status, 4);
  if (!texture) {
    return 0;
  }

  if (s_cursorModel) {
    ModelReplaceTexture(s_cursorModel, 1, texture, 0);
  } else {
    CreateCursorIconModel(texture);
  }
  HandleClose(texture);
  return CursorGrabMoney(s_cursorModel);
}

BOOL CursorGrabSpell(LPCSTR filename) {
  if (!filename) {
    return 0;
  }

  CStatus     status;
  CGxTexFlags flags(GxTex_Linear, 0, 0, 0, 0, 0, 1);
  HTEXTURE    texture = TextureCreate(filename, flags, &status, 0);
  SysMsgAdd(status, 4);
  if (!texture) {
    return 0;
  }

  if (s_cursorModel) {
    ModelReplaceTexture(s_cursorModel, 1, texture, 0);
  } else {
    CreateCursorIconModel(texture);
  }
  HandleClose(texture);
  return CursorGrabSpell(s_cursorModel);
}

void CursorDropMoney() {
  if (g_cursor && g_cursor->GetItemType() == CURSOR_MONEY) {
    g_cursor->Drop();
    g_cursor->SetItemType(CURSOR_EMPTY);
  }
}

void CursorDropSpell() {
  if (g_cursor && g_cursor->GetItemType() == CURSOR_SPELL) {
    g_cursor->Drop();
    g_cursor->SetItemType(CURSOR_EMPTY);
  }
}

BOOL CursorHasSpell() {
  return g_cursor && g_cursor->GetItemType() == CURSOR_SPELL;
}

void CursorModelSetSequence(CURSORANIMATIONS sequence) {
  if (g_cursor && sequence < NUM_CURSOR_ANIMS && (s_cursorMode != CAST_CURSOR || sequence == CAST_CURSOR || sequence == CAST_ERROR_CURSOR)) {
    s_cursorType = sequence;
    if (s_cursorMode != CAST_CURSOR || sequence != ATTACK_CURSOR) {
      g_cursor->SetCursorAnim(sequence);
    }
  }
}

UINT CursorGetCursorType() {
  return s_cursorType;
}

UINT CursorGetCursorMode() {
  return s_cursorMode;
}

void CursorSetCursorMode(CURSORANIMATIONS mode) {
  if (g_cursor && mode < NUM_CURSOR_ANIMS) {
    g_cursor->SetCursorMode(mode);
    CURSORANIMATIONS cursorMode = s_cursorMode;
    CURSORANIMATIONS cursorType = s_cursorType;
    s_cursorMode = mode;
    CursorResetCursor(1);
    if (cursorType != cursorMode && cursorType != cursorMode + 9) {
      CursorModelSetSequence(cursorType);
    }
  }
}

void CursorResetCursor(int force) {
  if (g_cursor && (s_cursorMode != CAST_CURSOR || force)) {
    s_cursorType = s_cursorMode;
    g_cursor->ResetCursor();
  }
}

void CursorSetHeldItem(DWORDLONG itemGuid) {
  CGItem_C *item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(itemGuid, __FILE__, __LINE__));
  if (!item) {
    g_cursor->Drop();
    g_cursor->SetItemType(CURSOR_EMPTY);
    return;
  }

  CStatus status;
  LPCSTR  path = ClientDBStringLookup(SLOOKUP_INVENTORYICONPATH);
  LPCSTR  art = item->GetInventoryArt();
  LPCSTR  separator = path && *path && art && *art ? "\\" : "";
  char    buffer[MAX_PATH];
  SStrPrintf(buffer, sizeof(buffer), "%s%s%s", path, separator, art);

  g_cursor->Drop();
  g_cursor->SetItemType(CURSOR_ITEM);

  CGxTexFlags flags(GxTex_Linear, 0, 0, 0, 0, 0, 1);
  HTEXTURE    texture = TextureCreate(buffer, flags, &status, 0);
  SysMsgAdd(status, 4);
  if (texture) {
    if (s_cursorModel) {
      ModelReplaceTexture(s_cursorModel, 1, texture, 0);
    } else {
      CreateCursorIconModel(texture);
    }
    HandleClose(texture);
  }

  g_cursor->Grab(s_cursorModel);
}

void CursorSetHeldVirtualItem(UINT displayID) {
  if (!displayID) {
    g_cursor->Drop();
    g_cursor->SetItemType(CURSOR_EMPTY);
    return;
  }

  LPCSTR path = ClientDBStringLookup(SLOOKUP_INVENTORYICONPATH);
  LPCSTR art = CGItem_C::GetInventoryArt(displayID);
  LPCSTR separator = path && *path && art && *art ? "\\" : "";
  char   buffer[MAX_PATH];
  SStrPrintf(buffer, sizeof(buffer), "%s%s%s", path, separator, art);

  CStatus     status;
  CGxTexFlags flags(GxTex_Linear, 0, 0, 0, 0, 0, 1);
  HTEXTURE    texture = TextureCreate(buffer, flags, &status, 0);
  SysMsgAdd(status, 4);
  if (texture) {
    if (s_cursorModel) {
      ModelReplaceTexture(s_cursorModel, 1, texture, 0);
    } else {
      CreateCursorIconModel(texture);
    }
    HandleClose(texture);
  }
  g_cursor->SetItemType(CURSOR_ITEM);
  g_cursor->Grab(s_cursorModel);
}

CGCursor::~CGCursor() {
  if (m_model) {
    HandleClose(m_model);
  }
}

void CGCursor::Drop() {
  ModelClearAllLinks(m_model);
}

void CGCursor::SetItemType(CURSORITEMTYPE type) {
  if ((type == CURSOR_SPELL || m_heldItem == CURSOR_SPELL) && type != m_heldItem) {
    if (type == CURSOR_ITEM || type == CURSOR_SPELL) {
      SndInterfacePlayInterfaceSound("INTERFACESOUND_CURSORGRABOBJECT");
    } else if (type == CURSOR_EMPTY && (m_heldItem == CURSOR_ITEM || m_heldItem == CURSOR_SPELL)) {
      SndInterfacePlayInterfaceSound("INTERFACESOUND_CURSORDROPOBJECT");
    }
  }
  m_heldItem = type;
}

void CGCursor::Grab(HMODEL model) {
  Drop();
  if (!ModelAddLink(m_model, 0, model, CGGameUI::m_UISimpleParent->GetLayoutScale())) {
    SysMsgAdd("Cursor model missing necessary attachment point", SYSMSG_ERROR, 4);
  }
}

void CGCursor::SetArt(LPCSTR art) {
  if (m_model) {
    HandleClose(m_model);
  }

  CModelCreate createData;
  createData.boneNames = 0;
  createData.numBones = 0;
  createData.cameraNames = 0;
  createData.numCameras = 0;
  createData.sequenceNames = s_animationNames;
  createData.numSequences = NUM_CURSOR_ANIMS;
  createData.flags = 2;
  CStatus status;
  m_model = ModelCreate(art, &createData, &status);
  SysMsgAdd(status, 4);
}

void CGCursor::SetCursorAnim(CURSORANIMATIONS sequence) {
  ModelSetSequence(m_model, sequence, 0);
  m_mouseOver = sequence;
}

void CGCursor::SetCursorMode(CURSORANIMATIONS sequence) {
  bool reset = m_mouseOver == NO_CURSOR;
  m_cursorMode = sequence;
  if (reset) {
    ModelSetSequence(m_model, sequence, 0);
  }
}

void CGCursor::ResetCursor() {
  ModelSetSequence(m_model, m_cursorMode, 0);
  m_mouseOver = NO_CURSOR;
}
