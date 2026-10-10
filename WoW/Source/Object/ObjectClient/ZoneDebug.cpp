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
  cellPos.x = (x * 36.0f + 614400.0f) * 0.00020833334f;
  cellPos.y = (y * 36.0f + 614400.0f) * 0.00020833334f;

  CGObject_C *player = ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__);
  if (!player) {
    return true;
  }

  NTempest::C3Vector position = player->GetPosition();
  UINT               playerX = (position.x * 36.0f + 614400.0f) * 0.00020833334f;
  UINT               playerY = (position.y * 36.0f + 614400.0f) * 0.00020833334f;
  return s_zoneIDMap[playerX][playerY] == s_zoneIDMap[cellPos.x][cellPos.y];
}
