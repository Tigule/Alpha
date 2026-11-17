#include <Base/Base.h>
#include <Gx/Gx.h>
#include <MapDefs.h>
#include <WorldClient/World.h>
#include "Net/NetClient/NetClient.h"
#include <Frame/CSimpleTop.h>
#include "Object/ObjectClient/Unit_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "SoundInterface/SoundInterface.h"
#include "UIUtil/InputControl.h"
#include "Ui/WorldFrame.h"
#include "Ui/GameUI.h"
#include "Base/CDataStore.h"
#include "Console/ConsoleClient.h"
#include "Console/ConsoleCommand.h"
#include "DB/DBClient/DBCacheInstances.h"
#include "Object/ObjectClient/Item_C.h"
#include "Object/ObjectClient/Bag_C.h"
#include "Object/ObjectClient/Player_C.h"
#include "WowSvcs/WowSvcsClient/ClientServices.h"
#include <FrameScript/FrameScript.h>

#include "ZoneDebug.h"

#include <Base/CDataStore.h>
#include "Object/ObjectClient/Object_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "Tempest/c2ivector.h"
#include <storm.h>

#include "WowSvcs/WowSvcsClient/ClientServices.h"

#include <string.h>

static BYTE s_zoneIDMap[256][256];

static BOOL ReceiveZoneMap(LPVOID, NETMESSAGE, DWORD, CDataStore *msg) {
  BYTE *next = &s_zoneIDMap[0][0];

  while (!msg->IsRead()) {
    BYTE id;
    UINT run;

    msg->Get(id);
    msg->Get(run);

    for (UINT i = 0; i < run; ++i) {
      *next++ = id;
    }
  }

  ASSERT(next == &s_zoneIDMap[256 - 1][256 - 1] + 1);

  return TRUE;
}

void ZoneDebugInitialize() {
  memset(s_zoneIDMap, 0, sizeof(s_zoneIDMap));
  ClientServices_SetMessageHandler(SMSG_ZONE_MAP, ReceiveZoneMap, 0);
}

void ZoneDebugDestroy() {
  ClientServices_ClearMessageHandler(SMSG_ZONE_MAP);
}

inline NTempest::C2iVector ProxMapCellPos(int x, int y) {
  return NTempest::C2iVector((x * 36 + 614400) / 4800, (y * 36 + 614400) / 4800);
}

inline NTempest::C2iVector ProxMapCellPos(const NTempest::C3Vector &pos) {
  return NTempest::C2iVector((int)((pos.x * 36.0f + 614400.0f) / 4800.0f), (int)((pos.y * 36.0f + 614400.0f) / 4800.0f));
}

bool ZoneDebugIsInCurrentZone(float x, float y) {
  NTempest::C2iVector cellPos = ProxMapCellPos((int)x, (int)y);
  CGObject_C *player = ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__);
  if (!player) {
    return true;
  }

  NTempest::C2iVector playerPos = ProxMapCellPos(player->GetPosition());

  return s_zoneIDMap[playerPos.x][playerPos.y] == s_zoneIDMap[cellPos.x][cellPos.y];
}
