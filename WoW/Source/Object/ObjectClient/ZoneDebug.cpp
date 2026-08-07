#include <Base/Base.h>
#include <WowConst.h>
#include <MapDefs.h>

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

    if (run) {
      memset(next, id, run);
      next += run;
    }
  }

  ASSERT(next == &s_zoneIDMap[256 - 1][256 - 1] + 1);

  return 1;
}

void ZoneDebugInitialize() {
  memset(s_zoneIDMap, 0, sizeof(s_zoneIDMap));
  ClientServices_SetMessageHandler(SMSG_ZONE_MAP, ReceiveZoneMap, 0);
}

void ZoneDebugDestroy() {
  ClientServices_ClearMessageHandler(SMSG_ZONE_MAP);
}

bool ZoneDebugIsInCurrentZone(float x, float y) {
  NTempest::C2iVector cellPos;
  cellPos.x = static_cast<int>((x * 36.0f + 614400.0f) * 0.00020833334f);
  cellPos.y = static_cast<int>((y * 36.0f + 614400.0f) * 0.00020833334f);

  CGObject_C *player = ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__);
  if (!player) {
    return true;
  }

  NTempest::C3Vector position = player->GetPosition();
  UINT               playerX = static_cast<UINT>((position.x * 36.0f + 614400.0f) * 0.00020833334f);
  UINT               playerY = static_cast<UINT>((position.y * 36.0f + 614400.0f) * 0.00020833334f);
  return s_zoneIDMap[playerX][playerY] == s_zoneIDMap[cellPos.x][cellPos.y];
}
