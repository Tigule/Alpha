#include "Object/ObjectClient/NPC_C.h"

#include "Net/NetClient/NetClient.h"
#include "Object/ObjectClient/Unit_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "WowSvcs/WowSvcsClient/ClientServices.h"

#include <Base/CDataStore.h>
#include <Services/SysMessage.h>

static int              s_questQueriesPending;
static int              s_questRewardQueriesPending;
static unsigned __int64 s_npcGUID;

static int NPCResponseHandler(void *, NETMESSAGE, unsigned long, CDataStore *msg) {
  unsigned __int64 npcGUID;
  msg->Get(npcGUID);
  if (!npcGUID) {
    return 1;
  }

  s_npcGUID = npcGUID;
  CGObject_C *object = ClntObjMgrObjectPtr(npcGUID, __FILE__, __LINE__);
  if (object && (object->GetType() & TYPE_UNIT)) {
    CGUnit_C *unit = static_cast<CGUnit_C *>(object);
    if (!unit->GetUnitData()->npcFlags) {
      SysMsgPrintf(SYSMSG_ERROR, 2, "UNITNOTNPC|%d|0x%016I64X", unit->GetEntryID(), s_npcGUID);
    }
    return 1;
  }

  return 0;
}

void NPC_C_Initialize() {
  s_questQueriesPending = 0;
  s_questRewardQueriesPending = 0;
  ClientServices_SetMessageHandler(SMSG_NPC_HYPERTEXT, NPCResponseHandler, 0);
  ClientServices_SetMessageHandler(SMSG_NPC_WONT_TALK, NPCResponseHandler, 0);
}

void NPC_C_Destroy() {
  ClientServices_ClearMessageHandler(SMSG_NPC_HYPERTEXT);
  ClientServices_ClearMessageHandler(SMSG_NPC_WONT_TALK);
}
