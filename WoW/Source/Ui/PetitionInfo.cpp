#include "DB/DBClient/DBCacheInstances.h"
#include "Object/ObjectClient/Player_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "WowSvcs/WowSvcsClient/ClientServices.h"

#include <Base/CDataStore.h>
#include <Console/ConsoleClient.h>
#include <FrameScript/FrameScript.h>
#include <Net/NetClient/NetClient.h>
#include <lauxlib.h>
#include <lua.h>
#include <stpl.h>

struct PetitionSignerInfo {
  unsigned __int64 guid;
  int              choice;
};

class CGPetitionInfo {
 public:
  static void EnterWorld();
  static void LeaveWorld();
  static void SetPetition(unsigned __int64 petition, int petitionID);
  static void SetSignatures(unsigned char count, unsigned __int64 *signers, int *choices);
  static void DecrementPendingName();
  static void SetPetitionStats(int id);
  static unsigned __int64 GetPetition() {
    return m_petitionGUID;
  }
  static unsigned int GetNumSignatures() {
    return m_numSignatures;
  }
  static const PetitionSignerInfo *GetSignature(unsigned int index) {
    return index < m_numSignatures ? &m_signatures[index] : 0;
  }
  static const CGPetition *GetPetitionStats() {
    return m_petition;
  }

 private:
  static void ClearSignatures();

 protected:
  static unsigned __int64                    m_petitionGUID;
  static int                                 m_petitionID;
  static TSGrowableArray<PetitionSignerInfo> m_signatures;
  static unsigned int                        m_numSignatures;
  static unsigned int                        m_pendingNames;
  static const CGPetition                   *m_petition;
};

unsigned __int64                    CGPetitionInfo::m_petitionGUID;
int                                 CGPetitionInfo::m_petitionID;
TSGrowableArray<PetitionSignerInfo> CGPetitionInfo::m_signatures;
unsigned int                        CGPetitionInfo::m_numSignatures;
unsigned int                        CGPetitionInfo::m_pendingNames;
const CGPetition                   *CGPetitionInfo::m_petition;

unsigned __int64 Script_GetGUIDFromName(const char *name);

static void SignatureNameQueryCallback(int, const unsigned __int64 &, void *, bool) {
  CGPetitionInfo::DecrementPendingName();
}

static void PetitionQueryCallback(int id, const unsigned __int64 &, void *, bool granted) {
  if (granted) {
    CGPetitionInfo::SetPetitionStats(id);
  }
}

void CGPetitionInfo::EnterWorld() {
  m_petitionGUID = 0;
  m_petitionID = 0;
  m_numSignatures = 0;
  m_pendingNames = 0;
  m_petition = 0;
  m_signatures.SetCount(10);
}

void CGPetitionInfo::LeaveWorld() {
  m_signatures.Clear();
}

void CGPetitionInfo::ClearSignatures() {
  if (m_pendingNames) {
    for (unsigned int i = 0; i < m_numSignatures; ++i) {
      g_nameDBCache.CancelCallback(m_signatures[i].guid, SignatureNameQueryCallback, 0);
    }
  }
  m_numSignatures = 0;
  m_pendingNames = 0;
}

void CGPetitionInfo::SetPetition(unsigned __int64 petition, int petitionID) {
  if (m_petitionGUID) {
    ClearSignatures();
    FrameScript_SignalEvent(374);
  }
  m_petitionGUID = petition;
  m_petitionID = petitionID;
  if (petition) {
    m_petition = g_petitionCache.GetRecord(petitionID, petition, PetitionQueryCallback, 0);
  } else {
    m_petition = 0;
  }
}

void CGPetitionInfo::SetSignatures(unsigned char count, unsigned __int64 *signers, int *choices) {
  ClearSignatures();
  m_signatures.SetCount(count);
  m_numSignatures = count;
  for (unsigned int i = 0; i < count; ++i) {
    m_signatures[i].guid = signers[i];
    m_signatures[i].choice = choices[i];
    if (!g_nameDBCache.GetRecord(signers[i], signers[i], SignatureNameQueryCallback, 0)) {
      ++m_pendingNames;
    }
  }
  if (!m_pendingNames && m_petition) {
    FrameScript_SignalEvent(373);
    ConsoleWrite("Petition shown", DEFAULT_COLOR);
  }
}

void CGPetitionInfo::DecrementPendingName() {
  if (m_pendingNames && !--m_pendingNames && m_petition) {
    FrameScript_SignalEvent(373);
    ConsoleWrite("Petition shown", DEFAULT_COLOR);
  }
}

void CGPetitionInfo::SetPetitionStats(int id) {
  if (m_petitionID == id) {
    m_petition = g_petitionCache.GetRecord(id, 0, 0, 0);
    if (m_petition && !m_pendingNames) {
      FrameScript_SignalEvent(373);
      ConsoleWrite("Petition shown", DEFAULT_COLOR);
    }
  }
}

static int Script_ClosePetition(lua_State *__formal) {
  CGPetitionInfo::SetPetition(0, 0);
  return 0;
}

static int Script_GetPetitionInfo(lua_State *L) {
  const CGPetition *petition = CGPetitionInfo::GetPetitionStats();
  if (petition) {
    lua_pushstring(L, petition->m_flags & 1 ? "charter" : "petition");
    lua_pushstring(L, petition->m_title);
    lua_pushstring(L, petition->m_bodyText);
    lua_pushnumber(L, static_cast<double>(petition->m_maxSignatures));
    const NameCache *name = g_nameDBCache.GetRecord(petition->m_petitioner, petition->m_petitioner, 0, 0);
    lua_pushstring(L, name ? name->m_name : 0);
    if (petition->m_petitioner == ClntObjMgrGetActivePlayer()) {
      lua_pushnumber(L, 1.0);
    } else {
      lua_pushnil(L);
    }
  } else {
    lua_pushnil(L);
    lua_pushnil(L);
    lua_pushnil(L);
    lua_pushnumber(L, 0.0);
    lua_pushnil(L);
    lua_pushnil(L);
  }
  return 6;
}

static int Script_GetNumPetitionNames(lua_State *L) {
  lua_pushnumber(L, static_cast<double>(CGPetitionInfo::GetNumSignatures()));
  return 1;
}

static int Script_GetPetitionNameInfo(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetPetitionNameInfo(index)");
  }
  const PetitionSignerInfo *signer = CGPetitionInfo::GetSignature(static_cast<unsigned int>(lua_tonumber(L, 1)) - 1);
  const NameCache    *name = signer ? g_nameDBCache.GetRecord(signer->guid, signer->guid, 0, 0) : 0;
  lua_pushstring(L, name ? name->m_name : 0);
  return 1;
}

static int Script_CanSignPetition(lua_State *L) {
  const CGPetition *petition = CGPetitionInfo::GetPetitionStats();
  int               canSign = petition != 0;
  CGPlayer_C       *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (petition && (petition->m_flags & 1) &&
      ((!player || player->GetBag()->GetItem(0)) ||
       CGPetitionInfo::GetNumSignatures() >= static_cast<unsigned int>(petition->m_maxSignatures)))
  {
    canSign = 0;
  }
  if (petition && petition->m_petitioner == ClntObjMgrGetActivePlayer()) {
    canSign = 0;
  }
  for (unsigned int i = 0; canSign && i < CGPetitionInfo::GetNumSignatures(); ++i) {
    if (CGPetitionInfo::GetSignature(i)->guid == ClntObjMgrGetActivePlayer()) {
      canSign = 0;
    }
  }
  if (canSign) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int Script_SignPetition(lua_State *L) {
  unsigned char choice = 1;
  if (lua_isnumber(L, 1)) {
    choice = static_cast<unsigned char>(lua_tonumber(L, 1));
  }
  unsigned __int64 petitionGUID = CGPetitionInfo::GetPetition();
  if (petitionGUID) {
    CDataStore msg;
    msg.Put(static_cast<unsigned int>(CMSG_PETITION_SIGN));
    msg.Put(petitionGUID);
    msg.Put(choice);
    msg.Finalize();
    ClientServices_Send(&msg);
  }
  return 0;
}

static int Script_OfferPetition(lua_State *__formal) {
  unsigned __int64 petition = CGPetitionInfo::GetPetition();
  unsigned __int64 target = Script_GetGUIDFromName("target");
  if (petition && target) {
    CDataStore msg;
    msg.Put(static_cast<unsigned int>(CMSG_OFFER_PETITION));
    msg.Put(petition);
    msg.Put(target);
    msg.Finalize();
    ClientServices_Send(&msg);
  }
  return 0;
}

static FrameScript_Method s_ScriptFunctions[7] = {
    {      "ClosePetition",       Script_ClosePetition},
    {    "GetPetitionInfo",     Script_GetPetitionInfo},
    {"GetNumPetitionNames", Script_GetNumPetitionNames},
    {"GetPetitionNameInfo", Script_GetPetitionNameInfo},
    {    "CanSignPetition",     Script_CanSignPetition},
    {       "SignPetition",        Script_SignPetition},
    {      "OfferPetition",       Script_OfferPetition}
};

void PetitionInfoRegisterScriptFunctions() {
  for (unsigned int i = 0; i < 7; ++i) {
    FrameScript_RegisterFunction(s_ScriptFunctions[i].name, s_ScriptFunctions[i].method);
  }
}

void PetitionInfoUnregisterScriptFunctions() {
  for (unsigned int i = 0; i < 7; ++i) {
    FrameScript_UnregisterFunction(s_ScriptFunctions[i].name);
  }
}
