#include "TaxiMapFrame.h"

#include "DB/DBClient/AutoCode/TaxiNodesRec.h"
#include "DB/WowLocale.h"
#include "Game/GameClient/TaxiMap.h"
#include "Object/ObjectClient/Player_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "Ui/GameUI.h"

#include <Frame/CSimpleModel.h>
#include <Frame/CSimpleRender.h>
#include <Frame/SimpleFrameRegistry.h>
#include <FrameScript/FrameScript.h>
#include <Tempest/crect.h>
#include <storm.h>

#include <lauxlib.h>
#include <lua.h>

static int Script_SetTaxiMap(lua_State *L);
static int Script_SetTaxiRoute(lua_State *L);
static int Script_NumTaxiNodes(lua_State *L);
static int Script_TaxiNodeName(lua_State *L);
static int Script_TaxiNodePosition(lua_State *L);
static int Script_TaxiNodeCost(lua_State *L);
static int Script_TakeTaxiNode(lua_State *L);
static int Script_CloseTaxiMap(lua_State *L);
static int Script_GetTextureCoordinates(lua_State *L);
static int Script_TaxiNodeGetType(lua_State *L);

static const char *s_taxiNodeNames[4] = {"NONE", "CURRENT", "REACHABLE", "DISTANT"};

unsigned __int64       CGTaxiMap::m_unit;
unsigned int           CGTaxiMap::m_startNode;
TSCArray<TaxiNode, 64> CGTaxiMap::m_nodes;

void CGTaxiMap::InitializeGame() {
  m_unit = 0;
}

void CGTaxiMap::ShutdownGame() {
}

void CGTaxiMap::EnterWorld() {
}

void CGTaxiMap::LeaveWorld() {
  CloseMap();
}

void CGTaxiMap::SetupMap(
    const unsigned __int64 &unit,
    unsigned int            node,
    __int64                 destNodes,
    __int64                 knownNodes,
    const NTempest::CRect  &visibleArea
) {
  FATALASSERT(unit);
  if (m_unit) {
    if (m_unit == unit) {
      FATALASSERT(node == m_startNode);
      return;
    }
    CloseMap();
  }

  __int64 allNodes = destNodes | knownNodes;
  CGGameUI::SetInteractTarget(unit, 0.0f);
  m_unit = unit;
  m_startNode = node;

  TaxiNodesRec *currentNode = g_taxiNodesDB.GetRecord(node);
  if (!currentNode) {
    m_nodes.SetCount(0);
    CloseMap();
    return;
  }

  unsigned int count = 0;
  m_nodes.SetCount(64);
  for (unsigned int nodeID = 1; nodeID <= 64; ++nodeID) {
    TaxiNodesRec *taxiNode = g_taxiNodesDB.GetRecord(nodeID);
    if (taxiNode && taxiNode->m_ContinentID == currentNode->m_ContinentID && (allNodes & (static_cast<__int64>(1) << (taxiNode->m_ID - 1))) &&
        taxiNode->m_X <= visibleArea.r && taxiNode->m_X >= visibleArea.l && taxiNode->m_Y <= visibleArea.b && taxiNode->m_Y >= visibleArea.t)
    {
      TaxiNode &out = m_nodes[count++];
      out.id = taxiNode->m_ID;
      out.x = (visibleArea.b - taxiNode->m_Y) / (visibleArea.r - visibleArea.l);
      out.y = (taxiNode->m_X - visibleArea.l) / (visibleArea.b - visibleArea.t);
    }
  }
  FATALASSERT(count <= 64);
  m_nodes.SetCount(count);
  FrameScript_SignalEvent(283);
}

void CGTaxiMap::CloseMap() {
  if (m_unit) {
    CGGameUI::ClearInteractTarget(m_unit);
    m_unit = 0;
    m_nodes.SetCount(0);
    FrameScript_SignalEvent(284);
  }
}

const char *CGTaxiMap::TaxiNodeName(unsigned int slot) {
  FATALASSERT(slot < NumTaxiNodes());
  TaxiNodesRec *node = g_taxiNodesDB.GetRecord(m_nodes[slot].id);
  FATALASSERT(node);
  return node->m_Name_lang[CURRENT_LANGUAGE];
}

const char *CGTaxiMap::TaxiNodeType(unsigned int slot) {
  FATALASSERT(slot < NumTaxiNodes());
  return s_taxiNodeNames[TaxiNodeGetNodeType(m_nodes[slot].id)];
}

void CGTaxiMap::TaxiNodePosition(unsigned int slot, float &x, float &y) {
  FATALASSERT(slot < NumTaxiNodes());
  x = m_nodes[slot].x;
  y = m_nodes[slot].y;
}

unsigned int CGTaxiMap::TaxiNodeCost(unsigned int slot) {
  FATALASSERT(slot < NumTaxiNodes());
  return ::TaxiNodeCost(m_startNode, m_nodes[slot].id);
}

void CGTaxiMap::TakeTaxiNode(unsigned int slot) {
  FATALASSERT(slot < NumTaxiNodes());
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player) {
    return;
  }

  unsigned int flags = player->GetUnitData()->flags;
  if ((flags & 0x2000) || (flags & 0x1000)) {
    CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(163));
  } else if (m_startNode == m_nodes[slot].id) {
    CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(155));
  } else if (!TaxiRouteExists(m_startNode, m_nodes[slot].id)) {
    CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(156));
  } else {
    player->StartTaxi(m_unit, m_startNode, m_nodes[slot].id);
  }
}

#define GET_TAXI_OBJECT_THIS(L, type, object)                   \
  type *object = 0;                                             \
  if (lua_type(L, 1) == LUA_TTABLE) {                           \
    lua_rawgeti(L, 1, 0);                                       \
    object = static_cast<type *>(lua_touserdata(L, -1));        \
    lua_pop(L, 1);                                              \
  } else {                                                      \
    luaL_error(                                                 \
        L,                                                      \
        "Attempt to find 'this' in non-table object (used '.' " \
        "instead of ':' ?)"                                     \
    );                                                          \
  }                                                             \
  FATALASSERT(object)

static int Script_SetTaxiMap(lua_State *L) {
  GET_TAXI_OBJECT_THIS(L, CSimpleTexture, object);
  object->SetTexture(TaxiMapGetTexture());
  return 0;
}

static int Script_SetTaxiRoute(lua_State *L) {
  CSimpleModel *model = static_cast<CSimpleModel *>(SimpleFrameRegistryGetEntry("TaxiRouteMap", 0));
  if (model && !model->GetModel()) {
    HMODEL route = TaxiGetRouteModel(model->GetWidth(), model->GetHeight());
    model->SetModel(route);
    if (route) {
      HandleClose(route);
    }
  }
  return 0;
}

static int Script_NumTaxiNodes(lua_State *L) {
  lua_pushnumber(L, static_cast<double>(CGTaxiMap::NumTaxiNodes()));
  return 1;
}

static int Script_TaxiNodeName(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: TaxiNodeName(slot)");
  }
  unsigned int slot = static_cast<unsigned int>(lua_tonumber(L, 1)) - 1;
  if (slot > CGTaxiMap::NumTaxiNodes()) {
    return luaL_error(L, "Invalid taxi node slot");
  }
  lua_pushstring(L, CGTaxiMap::TaxiNodeName(slot));
  return 1;
}

static int Script_TaxiNodePosition(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: TaxiNodeTaxiNodeLocation(slot)");
  }
  unsigned int slot = static_cast<unsigned int>(lua_tonumber(L, 1)) - 1;
  if (slot > CGTaxiMap::NumTaxiNodes()) {
    return luaL_error(L, "Invalid taxi node slot");
  }
  float x;
  float y;
  CGTaxiMap::TaxiNodePosition(slot, x, y);
  lua_pushnumber(L, y);
  lua_pushnumber(L, x);
  return 2;
}

static int Script_TaxiNodeCost(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: TaxiNodeCost(slot)");
  }
  unsigned int slot = static_cast<unsigned int>(lua_tonumber(L, 1)) - 1;
  if (slot > CGTaxiMap::NumTaxiNodes()) {
    return luaL_error(L, "Invalid taxi node slot");
  }
  lua_pushnumber(L, static_cast<double>(CGTaxiMap::TaxiNodeCost(slot)));
  return 1;
}

static int Script_TakeTaxiNode(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: TakeTaxiNode(slot)");
  }
  unsigned int slot = static_cast<unsigned int>(lua_tonumber(L, 1)) - 1;
  if (slot < CGTaxiMap::NumTaxiNodes()) {
    CGTaxiMap::TakeTaxiNode(slot);
  }
  return 0;
}

static int Script_CloseTaxiMap(lua_State *L) {
  CGTaxiMap::CloseMap();
  return 0;
}

static int Script_GetTextureCoordinates(lua_State *L) {
  NTempest::CRect rect = TaxiMapGetRect();
  lua_pushnumber(L, rect.l);
  lua_pushnumber(L, rect.r);
  lua_pushnumber(L, rect.t);
  lua_pushnumber(L, rect.b);
  return 4;
}

static int Script_TaxiNodeGetType(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: TakeTaxiNode(slot)");
  }
  unsigned int slot = static_cast<unsigned int>(lua_tonumber(L, 1)) - 1;
  if (slot > CGTaxiMap::NumTaxiNodes()) {
    return luaL_error(L, "Invalid taxi node slot");
  }
  lua_pushstring(L, CGTaxiMap::TaxiNodeType(slot));
  return 1;
}

#undef GET_TAXI_OBJECT_THIS

static FrameScript_Method s_ScriptFunctions[10] = {
    {      "SetTaxiMap",            Script_SetTaxiMap},
    {    "SetTaxiRoute",          Script_SetTaxiRoute},
    {    "NumTaxiNodes",          Script_NumTaxiNodes},
    {    "TaxiNodeName",          Script_TaxiNodeName},
    {"TaxiNodePosition",      Script_TaxiNodePosition},
    {    "TaxiNodeCost",          Script_TaxiNodeCost},
    {    "TakeTaxiNode",          Script_TakeTaxiNode},
    {    "CloseTaxiMap",          Script_CloseTaxiMap},
    {"TaxiGetTexCoords", Script_GetTextureCoordinates},
    { "TaxiNodeGetType",       Script_TaxiNodeGetType}
};

void CGTaxiMap::RegisterScriptFunctions() {
  for (unsigned int i = 0; i < 10; ++i) {
    FrameScript_RegisterFunction(s_ScriptFunctions[i].name, s_ScriptFunctions[i].method);
  }
}

void CGTaxiMap::UnregisterScriptFunctions() {
  for (unsigned int i = 0; i < 10; ++i) {
    FrameScript_UnregisterFunction(s_ScriptFunctions[i].name);
  }
}
