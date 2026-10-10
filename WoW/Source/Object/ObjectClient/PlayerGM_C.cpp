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

#include "Object/ObjectClient/Player_C.h"

#include "ObjectMgrClient/ObjectMgrClient.h"
#include "Console/ConsoleClient.h"
#include "Ui/GameUI.h"
#include "Ui/WorldFrame.h"
#include "WorldClient/World.h"
#include "WowServices/WDataStore.h"
#include "WowSvcs/WowSvcsClient/ClientServices.h"

#include <Os/OsTime.h>
#include <storm.h>

static DWORDLONG s_ghostTarget;
static UINT      s_lastGhostUpdate;
static BYTE      s_ghostRequestPending;
static DWORDLONG s_ghostTargetRequested;
static char      s_ghostNameRequested[256];
static DWORDLONG s_realActivePlayer;

static void MaybeSendGhostRequest();
static BOOL        OnGMEvent(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg);

void CGPlayer_C::SetRealActivePlayer(DWORDLONG guid) {
  s_realActivePlayer = guid;
}

DWORDLONG CGPlayer_C::GetRealActivePlayer() {
  FATALASSERT(!GetActive() || s_realActivePlayer);
  return s_realActivePlayer;
}

static BOOL OnGMEvent(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg) {
  switch (msgId) {
    case CMSG_GHOST: {
      s_ghostRequestPending = 0;

      if (msg->Tell() - msg->Size() < sizeof(s_ghostTarget)) {
        msg->Seek(msg->Tell());
        s_ghostTarget = 0;
      } else {
        msg->Get(s_ghostTarget);
      }

      CGWorldFrame *worldFrame = CGWorldFrame::GetActive();
      CGPlayer_C   *target;
      if (s_ghostTarget) {
        target = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(s_ghostTarget, __FILE__, __LINE__));
      } else {
        target = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(s_realActivePlayer, __FILE__, __LINE__));
      }
      if (target) {
        worldFrame->SetCameraTarget(target);

        CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(s_realActivePlayer, __FILE__, __LINE__));
        if (player) {
          if (player != target) {
            CWorld::SetHidden(player->GetWorldObject(), 1);
            if (target->IsA(TYPE_PLAYER)) {
              CGGameUI::LeaveWorld();
              ClntObjMgrSetActivePlayer(target->GetGUID());
              CGPlayer_C::SetActive(target);
              target->SetActiveMirrorHandlers();
              CGGameUI::EnterWorld();
            }
          } else {
            CWorld::SetHidden(player->GetWorldObject(), 0);
            ClntObjMgrSetActivePlayer(player->GetGUID());
            CGPlayer_C::SetActive(player);
          }
        }
      }
      break;
    }

    case MSG_GM_BIND_OTHER: {
      BYTE success;
      msg->Get(success);
      if (!success) {
        ConsolePrintf("bindplayer failed");
      } else {
        ConsolePrintf("Player bound to current location");
      }
      break;
    }

    case MSG_GM_SUMMON: {
      BYTE success;
      msg->Get(success);
      if (!success) {
        ConsolePrintf("Summon failed");
      } else {
        ConsolePrintf("Server is summoning now");
      }
      break;
    }
  }

  return 1;
}

void CGPlayer_C::InstallGMHandlers() {
  ClientServices_SetMessageHandler(CMSG_GHOST, OnGMEvent, 0);
  ClientServices_SetMessageHandler(MSG_GM_SUMMON, OnGMEvent, 0);
  ClientServices_SetMessageHandler(MSG_GM_BIND_OTHER, OnGMEvent, 0);
}

void CGPlayer_C::UninstallGMHandlers() {
  ClientServices_ClearMessageHandler(CMSG_GHOST);
  ClientServices_ClearMessageHandler(MSG_GM_SUMMON);
  ClientServices_ClearMessageHandler(MSG_GM_BIND_OTHER);
}

void CGPlayer_C::StartGhosting(LPCSTR name) {
  WDataStore msg;
  msg.Put(CMSG_GHOST);
  msg.Put((BYTE)1);
  msg.PutString(name);
  msg.Finalize();
  ClientServices_Send(&msg);

  s_ghostTarget = 0;
  s_ghostTargetRequested = 0;
  if (name != s_ghostNameRequested) {
    SStrCopy(s_ghostNameRequested, name, sizeof(s_ghostNameRequested));
  }
  s_ghostRequestPending = 1;
}

void CGPlayer_C::StartGhosting(DWORDLONG guid) {
  WDataStore msg;
  msg.Put(CMSG_GHOST);
  msg.Put((BYTE)0);
  msg.Put(guid);
  msg.Finalize();
  ClientServices_Send(&msg);

  s_ghostTarget = 0;
  s_ghostTargetRequested = guid;
  s_ghostNameRequested[0] = 0;
  s_ghostRequestPending = 1;
}

void CGPlayer_C::StopGhosting() {
  WDataStore msg;
  msg.Put(CMSG_GHOST);
  msg.Put((BYTE)0);
  msg.Put((DWORDLONG)0);
  msg.Finalize();
  ClientServices_Send(&msg);

  s_ghostTarget = 0;
  s_ghostTargetRequested = 0;
  s_ghostNameRequested[0] = 0;
}

static void MaybeSendGhostRequest() {
  if (!s_ghostRequestPending) {
    if (s_ghostTargetRequested) {
      CGPlayer_C::StartGhosting(s_ghostTargetRequested);
    } else {
      CGPlayer_C::StartGhosting(s_ghostNameRequested);
    }
  }
}

void CGPlayer_C::GMIdle() {
  if (!s_ghostTarget) {
    return;
  }

  CGUnit_C *target = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(s_ghostTarget, __FILE__, __LINE__));
  if (!target) {
    MaybeSendGhostRequest();
    return;
  }

  CGUnit_C *player = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(s_realActivePlayer, __FILE__, __LINE__));
  player->OnTeleportLocalNoUpdate(OsGetAsyncTimeMs(), target->GetPosition(), target->GetFacing());

  if (OsGetAsyncTimeMs() - s_lastGhostUpdate > 500) {
    player->SendMovementUpdate(MSG_MOVE_HEARTBEAT);
    s_lastGhostUpdate = OsGetAsyncTimeMs();
  }
}
