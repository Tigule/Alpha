#include "Object/ObjectClient/Player_C.h"

#include "ObjectMgrClient/ObjectMgrClient.h"
#include "Console/ConsoleClient.h"
#include "Ui/GameUI.h"
#include "Ui/WorldFrame.h"
#include "WorldClient/World.h"
#include "WowServices/WDataStore.h"
#include "WowSvcs/WowSvcsClient/ClientServices.h"

#include <storm.h>

static unsigned __int64 s_ghostTarget;
static unsigned int     s_lastGhostUpdate;
static unsigned int     s_ghostRequestPending;
static unsigned __int64 s_ghostTargetRequested;
static char             s_ghostNameRequested[256];
static unsigned __int64 s_realActivePlayer;

static void __fastcall MaybeSendGhostRequest();
int __fastcall         OnGMEvent(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg);

void __fastcall CGPlayer_C::SetRealActivePlayer(unsigned __int64 guid) {
  s_realActivePlayer = guid;
}

unsigned __int64 __fastcall CGPlayer_C::GetRealActivePlayer() {
  return s_realActivePlayer;
}

int __fastcall OnGMEvent(void *__formal, NETMESSAGE msgId, unsigned long eventTime, CDataStore *msg) {
  switch (msgId) {
    case CMSG_GHOST: {
      s_ghostRequestPending = 0;

      if (msg->Size() - msg->Tell() >= sizeof(s_ghostTarget)) {
        msg->Get(s_ghostTarget);
      } else {
        s_ghostTarget = 0;
      }

      if (!s_ghostTarget) {
        s_ghostTarget = s_realActivePlayer;
      }

      CGPlayer_C *target = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(s_ghostTarget, __FILE__, __LINE__));
      if (target) {
        CGWorldFrame::GetActive()->SetCameraTarget(target);

        CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(s_realActivePlayer, __FILE__, __LINE__));
        if (player) {
          CWorld::SetHidden(player->GetWorldObject(), player != target);

          if (player == target) {
            ClntObjMgrSetActivePlayer(player->GetGUID());
            CGPlayer_C::SetActive(player);
          } else if (target->GetType() & TYPE_PLAYER) {
            CGGameUI::LeaveWorld();
            ClntObjMgrSetActivePlayer(target->GetGUID());
            CGPlayer_C::SetActive(target);
            target->SetActiveMirrorHandlers();
            CGGameUI::EnterWorld();
          }
        }
      }
      break;
    }

    case MSG_GM_BIND_OTHER: {
      unsigned int success;
      msg->Get(*reinterpret_cast<unsigned char *>(&success));
      if (success) {
        ConsolePrintf("Player bound to current location");
      } else {
        ConsolePrintf("bindplayer failed");
      }
      break;
    }

    case MSG_GM_SUMMON: {
      unsigned int success;
      msg->Get(*reinterpret_cast<unsigned char *>(&success));
      if (success) {
        ConsolePrintf("Server is summoning now");
      } else {
        ConsolePrintf("Summon failed");
      }
      break;
    }
  }

  return 1;
}

void __fastcall CGPlayer_C::InstallGMHandlers() {
  ClientServices_SetMessageHandler(CMSG_GHOST, OnGMEvent, 0);
  ClientServices_SetMessageHandler(MSG_GM_SUMMON, OnGMEvent, 0);
  ClientServices_SetMessageHandler(MSG_GM_BIND_OTHER, OnGMEvent, 0);
}

void __fastcall CGPlayer_C::UninstallGMHandlers() {
  ClientServices_ClearMessageHandler(CMSG_GHOST);
  ClientServices_ClearMessageHandler(MSG_GM_SUMMON);
  ClientServices_ClearMessageHandler(MSG_GM_BIND_OTHER);
}

void __fastcall CGPlayer_C::StartGhosting(const char *name) {
  WDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_GHOST));
  msg.Put(static_cast<unsigned char>(1));
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

void __fastcall CGPlayer_C::StartGhosting(unsigned __int64 guid) {
  WDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_GHOST));
  msg.Put(static_cast<unsigned char>(0));
  msg.Put(guid);
  msg.Finalize();
  ClientServices_Send(&msg);

  s_ghostTarget = 0;
  s_ghostTargetRequested = guid;
  s_ghostNameRequested[0] = 0;
  s_ghostRequestPending = 1;
}

static void __fastcall MaybeSendGhostRequest() {
  if (!s_ghostRequestPending) {
    if (s_ghostTargetRequested) {
      CGPlayer_C::StartGhosting(s_ghostTargetRequested);
    } else {
      CGPlayer_C::StartGhosting(s_ghostNameRequested);
    }
  }
}

void __fastcall CGPlayer_C::GMIdle() {
  if (!s_ghostTarget) {
    return;
  }

  CGUnit_C *target = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(s_ghostTarget, __FILE__, __LINE__));
  if (!target) {
    MaybeSendGhostRequest();
    return;
  }

  CGUnit_C          *player = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(s_realActivePlayer, __FILE__, __LINE__));
  NTempest::C3Vector position = target->GetPosition();
  float              facing = target->GetFacing();
  player->OnTeleportLocalNoUpdate(GetTickCount(), position, facing);

  if (GetTickCount() - s_lastGhostUpdate > 500) {
    player->SendMovementUpdate(MSG_MOVE_HEARTBEAT);
    s_lastGhostUpdate = GetTickCount();
  }
}
