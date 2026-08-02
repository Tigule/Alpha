#include "Client.h"
#include <Base/CDataStore.h>

#include "Console/ConsoleClient.h"
#include "Console/ConsoleCommand.h"
#include "DB/DBClient/AutoCode/MapRec.h"
#include "Object/MovementData.h"
#include "Object/ObjectClient/Player_C.h"
#include "Object/ObjectClient/Unit_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "Ui/WorldFrame.h"
#include "WorldClient/World.h"
#include "WowSvcs/WowSvcsClient/ClientServices.h"

#include <Model/IModel.h>
#include <Os/OsTime.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <Os/W32/Debugging.h>

struct CMemCmdItem {
  unsigned int m_allocated;
  unsigned int m_committed;
  unsigned int m_reserved;
  char         m_name[256];

  static int __cdecl Compare(const void *m1, const void *m2) {
    return SStrCmp(
        static_cast<const CMemCmdItem *>(m1)->m_name,
        static_cast<const CMemCmdItem *>(m2)->m_name,
        0x7FFFFFFF);
  }
};

struct CMemCmdDump {
  unsigned __int64             m_sumAllocated;
  unsigned __int64             m_sumCommitted;
  unsigned __int64             m_sumReserved;
  TSGrowableArray<CMemCmdItem> m_items;
};

char *OsGetLastErrorStr();
void OsFreeLastErrorStr(char *msgBuf);
static unsigned __int64 s_lastTarget;

int ModelRenderSceneLogToggle(const char *fileName);
int ModelAnimateLogToggle(const char *fileName);
int Player_C_TogglePlayerRender();
void ModelShowBoundingSphere(HMODEL model);

static int CCommand_DBLookup(const char* command, const char* string) {
  CDataStore message;
  message.Put(2);
  message.PutString(string);
  message.Finalize();
  ClientServices_Send(&message);
  return 1;
}

static int CCommand_DrawLog(const char* command, const char* arguments) {
  ConsoleWrite(
      ModelRenderSceneLogToggle("RenderLog.txt")
          ? "Model render logging started"
          : "Model render logging stopped",
      DEFAULT_COLOR);
  return 1;
}

static int CCommand_AnimLog(const char* command, const char* arguments) {
  ConsoleWrite(
      ModelAnimateLogToggle("AnimLog.txt")
          ? "Model animation logging started"
          : "Model animation logging stopped",
      DEFAULT_COLOR);
  return 1;
}

static int CCommand_TogglePlayer(const char* command, const char* arguments) {
  if (CGWorldFrame::GetActive()) {
    ConsoleWrite(
        Player_C_TogglePlayerRender() ? "player visible" : "player hidden",
        DEFAULT_COLOR);
  }
  return 1;
}

static int CCommand_ToggleAnimBlending(const char* command, const char* arguments) {
  CGObject_C *object = ClntObjMgrObjectPtr(
      CGGameUI::GetCurrentObjectTrack(), __FILE__, __LINE__);
  if (!object) {
    object = ClntObjMgrObjectPtr(
        ClntObjMgrGetActivePlayer(), __FILE__, __LINE__);
  }
  if (!object) {
    return 0;
  }

  HMODEL model = object->GetObjectModel();
  if (ModelUsesBlending(model)) {
    ModelEnableAnimBlending(model, 0);
    ConsoleWrite("Motion blending disabled", DEFAULT_COLOR);
  } else {
    ModelEnableAnimBlending(model, 1);
    ConsoleWrite("Motion blending enabled", DEFAULT_COLOR);
  }
  return 1;
}

static int CCommand_ShowBounds(const char* command, const char* arguments) {
  CGObject_C *object = ClntObjMgrObjectPtr(
      CGGameUI::GetCurrentObjectTrack(), __FILE__, __LINE__);
  if (!object) {
    object = ClntObjMgrObjectPtr(
        ClntObjMgrGetActivePlayer(), __FILE__, __LINE__);
  }
  if (!object) {
    return 0;
  }

  HMODEL model = object->GetObjectModel();
  if (ModelIsShowingBoundingSphere(model)) {
    ModelHideBounds(model);
  } else {
    ModelShowBoundingSphere(model);
  }
  return 1;
}

static int CCommand_Loc(const char*, const char*) {
  CDataStore msg;
  msg.Put(4);
  msg.Put(ClntObjMgrGetActivePlayer());
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
}

static int CCommand_TerminalVelocity(const char* command, const char* arguments) {
  float metersPerSec;
  if (*arguments) {
    metersPerSec = static_cast<float>(atof(arguments));
    if (metersPerSec < 1.0f) {
      metersPerSec = 1.0f;
    } else if (metersPerSec > 60.0f) {
      metersPerSec = 60.0f;
    }
    MovementSetTerminalVelocity(metersPerSec);
  } else {
    metersPerSec = MovementGetTerminalVelocity();
  }
  ConsoleWriteA("Terminal velocity: %g m/s", DEFAULT_COLOR, metersPerSec);
  return 1;
}

static int CCommand_DLoc(const char* command, const char* arguments) {
  CGObject_C *player = ClntObjMgrObjectPtr(
      ClntObjMgrGetActivePlayer(), __FILE__, __LINE__);
  NTempest::C3Vector position;
  player->GetPosition(position);
  ConsoleWriteA("%g, %g, %g", DEFAULT_COLOR, position.x, position.y, position.z);
  return 1;
}

static int CCommand_TargetLoc(const char*, const char*) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(
      ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    CGObject_C *target = ClntObjMgrObjectPtr(
        player->GetLocalTarget(), __FILE__, __LINE__);
    if (!target) {
      target = ClntObjMgrObjectPtr(s_lastTarget, __FILE__, __LINE__);
    }
    if (!target) {
      ConsoleWrite("Error, no unit targeted!", DEFAULT_COLOR);
      return 1;
    }

    s_lastTarget = target->GetGUID();
    NTempest::C3Vector targetPosition = target->GetPosition();
    NTempest::C3Vector playerPosition = player->GetPosition();
    NTempest::C3Vector distance(
        playerPosition.x - targetPosition.x,
        playerPosition.y - targetPosition.y,
        playerPosition.z - targetPosition.z);
    ConsoleWriteA(
        "%016I64X: Local Pos: %g, %g, %g, facing: %g distance: %g",
        DEFAULT_COLOR, target->GetGUID(),
        targetPosition.x, targetPosition.y, targetPosition.z,
        target->GetFacing(), distance.Mag());

    CDataStore locMsg;
    locMsg.Put(4);
    locMsg.Put(target->GetGUID());
    locMsg.Finalize();
    ClientServices_Send(&locMsg);

    CDataStore facingMsg;
    facingMsg.Put(6);
    facingMsg.Put(target->GetGUID());
    facingMsg.Finalize();
    ClientServices_Send(&facingMsg);
  }
  return 1;
}

static int CCommand_Facing(const char* command, const char* arguments) {
  CDataStore msg;
  msg.Put(6);
  msg.Put(ClntObjMgrGetActivePlayer());
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
}

static int CCommand_DFacing(const char* command, const char* arguments) {
  CGObject_C *player = ClntObjMgrObjectPtr(
      ClntObjMgrGetActivePlayer(), __FILE__, __LINE__);
  ConsoleWriteA(
      "%g degrees", DEFAULT_COLOR,
      player->GetFacing() * 57.29578f);
  return 1;
}

static int CCommand_Speed(const char* command, const char* arguments) {
  float speed = SStrToFloat(arguments);
  if (speed > 0.0f) {
    unsigned long eventTime = OsGetAsyncTimeMs();
    CGUnit_C *unit = static_cast<CGUnit_C *>(
        ClntObjMgrObjectPtr(CGUnit_C::GetActiveMover(), __FILE__, __LINE__));
    if (unit) {
      unit->OnRunSpeedChangeLocal(
          eventTime, MSG_MOVE_SET_RUN_SPEED_CHEAT, speed);
    }
  }
  return 1;
}

static int CCommand_WalkSpeed(const char* command, const char* arguments) {
  float speed = SStrToFloat(arguments);
  if (speed > 0.0f) {
    unsigned long eventTime = OsGetAsyncTimeMs();
    CGUnit_C *unit = static_cast<CGUnit_C *>(
        ClntObjMgrObjectPtr(CGUnit_C::GetActiveMover(), __FILE__, __LINE__));
    if (unit) {
      unit->OnWalkSpeedChangeLocal(eventTime, speed);
    }
  }
  return 1;
}

static int CCommand_SwimSpeed(const char* command, const char* arguments) {
  float speed = SStrToFloat(arguments);
  if (speed > 0.0f) {
    unsigned long eventTime = OsGetAsyncTimeMs();
    CGUnit_C *unit = static_cast<CGUnit_C *>(
        ClntObjMgrObjectPtr(CGUnit_C::GetActiveMover(), __FILE__, __LINE__));
    if (unit) {
      unit->OnSwimSpeedChangeLocal(
          eventTime, MSG_MOVE_SET_SWIM_SPEED_CHEAT, speed);
    }
  }
  return 1;
}

static int CCommand_TurnSpeed(const char* command, const char* arguments) {
  float rate = SStrToFloat(arguments);
  if (rate > 0.0f) {
    unsigned long eventTime = OsGetAsyncTimeMs();
    CGUnit_C *unit = static_cast<CGUnit_C *>(
        ClntObjMgrObjectPtr(CGUnit_C::GetActiveMover(), __FILE__, __LINE__));
    if (unit) {
      unit->OnTurnRateChangeLocal(eventTime, rate);
    }
  }
  return 1;
}

static int CCommand_Money(const char* command, const char* arguments) {
  char currArg[64];
  const char whitespace[] = "\t\r\n\" ";
  SStrTokenize(&arguments, currArg, sizeof(currArg), whitespace, 0);
  if (!currArg[0]) {
    ConsoleWrite("Usage: money [copper]", static_cast<COLOR_T>(4));
    return 0;
  }
  CDataStore msg;
  msg.Put(36);
  msg.Put(SStrToUnsigned(currArg));
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
}

static int CCommand_WorldTeleport(const char* command, const char* arguments) {
  CGObject_C *player = ClntObjMgrObjectPtr(
      ClntObjMgrGetActivePlayer(), __FILE__, __LINE__);
  NTempest::C3Vector position = player->GetPosition();
  float facing = player->GetFacing();

  char currArg[64];
  const char whitespace[] = "\t\r\n\" ";
  SStrTokenize(&arguments, currArg, sizeof(currArg), whitespace, 0);
  if (!currArg[0]) {
    ConsoleWrite(
        "Usage: worldport <continentID> [x y z] [facing]",
        static_cast<COLOR_T>(4));
    return 0;
  }

  unsigned int mapID = SStrToUnsigned(currArg);
  SStrTokenize(&arguments, currArg, sizeof(currArg), whitespace, 0);
  if (currArg[0]) {
    position.x = SStrToFloat(currArg);
  }
  SStrTokenize(&arguments, currArg, sizeof(currArg), whitespace, 0);
  if (currArg[0]) {
    position.y = SStrToFloat(currArg);
  }
  SStrTokenize(&arguments, currArg, sizeof(currArg), whitespace, 0);
  if (currArg[0]) {
    position.z = SStrToFloat(currArg);
  }
  SStrTokenize(&arguments, currArg, sizeof(currArg), whitespace, 0);
  if (currArg[0]) {
    facing = SStrToFloat(currArg) * 0.017453292f;
  }

  if (!g_mapDB.GetRecord(mapID)) {
    ConsoleWriteA(
        "Bad world number: %i\n", static_cast<COLOR_T>(3), mapID);
    return 0;
  }

  CDataStore msg;
  msg.Put(8);
  msg.Put(OsGetAsyncTimeMs());
  msg.Put(static_cast<unsigned char>(mapID));
  msg.Put(position.x);
  msg.Put(position.y);
  msg.Put(position.z);
  msg.Put(facing);
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
}

static int CCommand_Teleport(const char* command, const char* arguments) {
  CGObject_C *player = ClntObjMgrObjectPtr(
      ClntObjMgrGetActivePlayer(), __FILE__, __LINE__);
  if (!player) {
    return 1;
  }

  if (!isdigit(static_cast<unsigned char>(*arguments)) && *arguments != '-') {
    CDataStore msg;
    msg.Put(9);
    msg.PutString(arguments);
    msg.Finalize();
    ClientServices_Send(&msg);
    return 1;
  }

  char currArg[64];
  const char whitespace[] = "\t\r\n\" ,";
  SStrTokenize(&arguments, currArg, sizeof(currArg), whitespace, 0);
  if (!currArg[0]) {
    return 0;
  }
  float x = SStrToFloat(currArg);

  SStrTokenize(&arguments, currArg, sizeof(currArg), whitespace, 0);
  if (!currArg[0]) {
    return 0;
  }
  float y = SStrToFloat(currArg);

  float mapX = 17066.666f - x;
  float mapY = 17066.666f - y;
  if (mapX < 0.0f || mapX >= 34133.332f ||
      mapY < 0.0f || mapY >= 34133.332f) {
    ConsoleWrite("Coordinates out of range\n", DEFAULT_COLOR);
    return 1;
  }

  SStrTokenize(&arguments, currArg, sizeof(currArg), whitespace, 0);
  float z = currArg[0]
      ? SStrToFloat(currArg)
      : CWorld::CalcAltitude(x, y, 0.0f);
  SStrTokenize(&arguments, currArg, sizeof(currArg), whitespace, 0);
  float facing = currArg[0]
      ? SStrToFloat(currArg) * 0.017453292f
      : player->GetFacing();

  CDataStore msg;
  msg.Put(198);
  msg.Put(x);
  msg.Put(y);
  msg.Put(z);
  msg.Put(facing);
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
}

static int CCommand_CreateItem(const char* command, const char* arguments) {
  CDataStore msg;
  msg.Put(19);
  msg.Put(SStrToInt(arguments));
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
}

static int CCommand_CreateGameObject(const char* command, const char* arguments) {
  CDataStore msg;
  msg.Put(20);
  msg.Put(SStrToInt(arguments));
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
}

static int CCommand_CreateMonster(const char* command, const char* arguments) {
  char type[32];
  SStrTokenize(&arguments, type, sizeof(type), " \t", 0);
  CDataStore msg;
  msg.Put(17);
  msg.Put(atoi(type));
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
}

static int CCommand_CreatePet(const char* command, const char* arguments) {
  char type[32];
  SStrTokenize(&arguments, type, sizeof(type), " \t", 0);
  CDataStore msg;
  msg.Put(17);
  msg.Put(-atoi(type));
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
}

static int CCommand_DestroyMonster(const char* command, const char* arguments) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(
      ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    CGObject_C *target = ClntObjMgrObjectPtr(
        player->GetLocalTarget(), __FILE__, __LINE__);
    if (!target) {
      ConsoleWrite("Error, no unit targeted!", DEFAULT_COLOR);
      return 1;
    }
    CDataStore msg;
    msg.Put(18);
    msg.Put(target->GetGUID());
    msg.Finalize();
    ClientServices_Send(&msg);
  }
  return 1;
}

static int CCommand_AttackPlayer(const char* command, const char* arguments) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(
      ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    CGObject_C *target = ClntObjMgrObjectPtr(
        player->GetLocalTarget(), __FILE__, __LINE__);
    if (!target) {
      ConsoleWrite("Error, no unit targeted!", DEFAULT_COLOR);
      return 1;
    }
    CDataStore msg;
    msg.Put(21);
    msg.Put(target->GetGUID());
    msg.Finalize();
    ClientServices_Send(&msg);
  }
  return 1;
}

static int CCommand_TargetAttack(const char* command, const char* arguments) {
  if (!arguments || !*arguments) {
    ConsoleWrite("GUID needed!", DEFAULT_COLOR);
    return 1;
  }

  unsigned __int64 victimGUID;
  sscanf(arguments, "%I64d", &victimGUID);
  unsigned __int64 activePlayer = ClntObjMgrGetActivePlayer();
  CGPlayer_C *player = activePlayer
      ? static_cast<CGPlayer_C *>(
            ClntObjMgrObjectPtr(activePlayer, __FILE__, __LINE__))
      : 0;
  if (!player) {
    ConsoleWrite("No active player!", DEFAULT_COLOR);
    return 1;
  }

  unsigned __int64 target = player->GetLocalTarget();
  if (!target || !ClntObjMgrObjectPtr(target, __FILE__, __LINE__)) {
    ConsoleWrite("Player has no target!", DEFAULT_COLOR);
    return 1;
  }

  CDataStore msg;
  msg.Put(22);
  msg.Put(target);
  msg.Put(victimGUID);
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
}

static int CCommand_Save(const char *command, const char *arguments) {
  CDataStore message;
  message.Put(CMSG_SAVE_PLAYER);
  message.Finalize();
  ClientServices_Send(&message);
  return 1;
}

static int CCommand_BindPoint(const char* command, const char* arguments) {
  unsigned __int64 activePlayer = ClntObjMgrGetActivePlayer();
  if (activePlayer) {
    CGObject_C *player = ClntObjMgrObjectPtr(
        activePlayer, __FILE__, __LINE__);
    if (player) {
      NTempest::C3Vector position;
      player->GetPosition(position);
      CDataStore message;
      message.Put(327);
      message.Finalize();
      ClientServices_Send(&message);
    }
  }
  return 1;
}

static int CCommand_Beastmaster(const char* command, const char* arguments) {
  CDataStore msg;
  msg.Put(33);
  msg.Put(static_cast<unsigned char>(
      SStrCmpI(arguments, "off", 0x7FFFFFFF) != 0));
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
}

static int CCommand_SendEvent(const char* command, const char* arguments) {
  CDataStore msg;
  msg.Put(45);
  msg.Put(SStrToInt(arguments));
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
}

static int CCommand_Recharge(const char *command, const char *arguments) {
  CDataStore msg;
  msg.Put(CMSG_RECHARGE);
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
}

static int CCommand_Level(const char* command, const char* arguments) {
  int level = SStrToInt(arguments);
  if (level <= 0 || level > 100) {
    ConsoleWrite("Invalid level specified\n", DEFAULT_COLOR);
  } else {
    CDataStore msg;
    msg.Put(37);
    msg.Put(level);
    msg.Finalize();
    ClientServices_Send(&msg);
  }
  return 1;
}

static int CCommand_PetLevel(const char* command, const char* arguments) {
  int level = SStrToInt(arguments);
  if (level <= 0 || level > 100) {
    ConsoleWrite("Invalid level specified\n", DEFAULT_COLOR);
  } else {
    CDataStore msg;
    msg.Put(38);
    msg.Put(level);
    msg.Finalize();
    ClientServices_Send(&msg);
  }
  return 1;
}

static int CCommand_ClearQuest(const char* command, const char* arguments) {
  int quest = SStrToInt(arguments);
  if (quest < 0) {
    ConsoleWrite("Invalid quest specified\n", DEFAULT_COLOR);
  } else {
    CDataStore msg;
    msg.Put(44);
    msg.Put(quest);
    msg.Finalize();
    ClientServices_Send(&msg);
  }
  return 1;
}

static int CCommand_FlagQuest(const char* command, const char* arguments) {
  int quest = SStrToInt(arguments);
  if (quest < 1) {
    ConsoleWrite("Invalid quest specified\n", DEFAULT_COLOR);
  } else {
    CDataStore msg;
    msg.Put(42);
    msg.Put(quest);
    msg.Finalize();
    ClientServices_Send(&msg);
  }
  return 1;
}

static int CCommand_FlagQuestFinish(const char* command, const char* arguments) {
  int quest = SStrToInt(arguments);
  if (quest < 1) {
    ConsoleWrite("Invalid quest specified\n", DEFAULT_COLOR);
  } else {
    CDataStore msg;
    msg.Put(43);
    msg.Put(quest);
    msg.Finalize();
    ClientServices_Send(&msg);
  }
  return 1;
}

static void QueryQuest(const unsigned __int64& questGiver, int questID) {
  CDataStore msg;
  msg.Put(386);
  msg.Put(questGiver);
  msg.Put(questID);
  msg.Finalize();
  ClientServices_Send(&msg);
}

static void AcceptQuest(const unsigned __int64& questGiver, int questID) {
  CDataStore msg;
  msg.Put(389);
  msg.Put(questGiver);
  msg.Put(questID);
  msg.Finalize();
  ClientServices_Send(&msg);
}

static void CompleteQuest(const unsigned __int64& questGiver, int questID) {
  CDataStore msg;
  msg.Put(390);
  msg.Put(questGiver);
  msg.Put(questID);
  msg.Finalize();
  ClientServices_Send(&msg);
}

static void QuestLogRemoveQuest(int entry) {
  CDataStore msg;
  msg.Put(400);
  msg.Put(static_cast<unsigned char>(entry));
  msg.Finalize();
  ClientServices_Send(&msg);
}

static int CCommand_QuestCommand(const char* command, const char* arguments) {
  char buffer[64];
  SStrTokenize(&arguments, buffer, sizeof(buffer), " \t", 0);
  if (!buffer[0]) {
    ConsoleWrite("Expected questGiver.", DEFAULT_COLOR);
    return 0;
  }

  unsigned __int64 questGiver;
  sscanf(buffer, "%I64X", &questGiver);
  SStrTokenize(&arguments, buffer, sizeof(buffer), " \t", 0);
  if (!buffer[0]) {
    ConsoleWrite("Expected questID.", DEFAULT_COLOR);
    return 0;
  }

  int questID = SStrToInt(buffer);
  if (!SStrCmpI(command, "questquery", 0x7FFFFFFF)) {
    QueryQuest(questGiver, questID);
  } else if (!SStrCmpI(command, "questaccept", 0x7FFFFFFF)) {
    AcceptQuest(questGiver, questID);
  } else if (!SStrCmpI(command, "questcomplete", 0x7FFFFFFF)) {
    CompleteQuest(questGiver, questID);
  } else if (!SStrCmpI(command, "questcancel", 0x7FFFFFFF)) {
    QuestLogRemoveQuest(questID);
  }
  return 1;
}

static int CCommand_TaxiClearAllNodes(const char *, const char *) {
  CDataStore msg;
  msg.Put(CMSG_TAXICLEARALLNODES);
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
}

static int CCommand_TaxiEnableAllNodes(const char *, const char *) {
  CDataStore msg;
  msg.Put(CMSG_TAXIENABLEALLNODES);
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
}

static int CCommand_ChangeCellZone(const char*, const char* args) {
  CDataStore msg;
  msg.Put(12);
  msg.Put(SStrToInt(args));
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
}

static int CCommand_Played(const char *, const char *) {
  CDataStore msg;
  msg.Put(CMSG_PLAYED_TIME);
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
}

static int CCommand_LootMethod(const char*, const char* args) {
  CDataStore msg;
  msg.Put(122);
  switch (*args) {
    case 'F':
    case 'f':
      msg.Put(static_cast<unsigned int>(0));
      msg.Put(static_cast<unsigned __int64>(0));
      break;
    case 'M':
    case 'm':
      msg.Put(static_cast<unsigned int>(2));
      msg.Put(CGGameUI::GetLockedTarget());
      break;
    case 'R':
    case 'r':
      msg.Put(static_cast<unsigned int>(1));
      msg.Put(static_cast<unsigned __int64>(0));
      break;
    default:
      ConsolePrintf(
          "Valid lootMethods are freeforall, roundrobin, and master");
      return 1;
  }
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
}

static int CCommand_CameraTarget(const char*, const char*) {
  CGWorldFrame *worldFrame = CGWorldFrame::GetActive();
  CGObject_C *target = ClntObjMgrObjectPtr(
      CGGameUI::GetLockedTarget(), __FILE__, __LINE__);
  if (target) {
    worldFrame->SetCameraTarget(target);
  }
  return 1;
}

static int CCommand_Reclaim(const char* command, const char* arguments) {
  char buffer[64];
  SStrTokenize(&arguments, buffer, sizeof(buffer), " \t", 0);
  if (!buffer[0]) {
    ConsoleWrite("Expected corpseGUID.", DEFAULT_COLOR);
    return 0;
  }

  unsigned __int64 corpseGUID;
  sscanf(buffer, "%I64X", &corpseGUID);
  CDataStore msg;
  msg.Put(451);
  msg.Put(corpseGUID);
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
}

static int CCommand_BuySpell(const char* command, const char* arguments) {
  char buffer[64];
  SStrTokenize(&arguments, buffer, sizeof(buffer), " \t", 0);
  if (!buffer[0]) {
    ConsoleWrite("Expected trainer.", DEFAULT_COLOR);
    return 0;
  }
  unsigned __int64 trainer;
  sscanf(buffer, "%I64X", &trainer);
  SStrTokenize(&arguments, buffer, sizeof(buffer), " \t", 0);
  if (!buffer[0]) {
    ConsoleWrite("Expected spellID", DEFAULT_COLOR);
    return 0;
  }
  CDataStore msg;
  msg.Put(420);
  msg.Put(trainer);
  msg.Put(SStrToInt(buffer));
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
}

static int CCommand_SellItem(const char* command, const char* arguments) {
  char buffer[64];
  SStrTokenize(&arguments, buffer, sizeof(buffer), " \t", 0);
  if (!buffer[0]) {
    ConsoleWrite("Expected merchant.", DEFAULT_COLOR);
    return 0;
  }
  unsigned __int64 merchant;
  sscanf(buffer, "%I64X", &merchant);
  SStrTokenize(&arguments, buffer, sizeof(buffer), " \t", 0);
  if (!buffer[0]) {
    ConsoleWrite("Expected item.", DEFAULT_COLOR);
    return 0;
  }
  unsigned __int64 item;
  sscanf(buffer, "%I64X", &item);
  unsigned char amount = 0;
  SStrTokenize(&arguments, buffer, sizeof(buffer), " \t", 0);
  if (buffer[0]) {
    amount = static_cast<unsigned char>(SStrToInt(buffer));
  }
  CDataStore sellMsg;
  sellMsg.Put(368);
  sellMsg.Put(merchant);
  sellMsg.Put(item);
  sellMsg.Put(amount);
  sellMsg.Finalize();
  ClientServices_Send(&sellMsg);
  return 1;
}

static int CCommand_BuyItem(const char* command, const char* arguments) {
  char buffer[64];
  SStrTokenize(&arguments, buffer, sizeof(buffer), " \t", 0);
  if (!buffer[0]) {
    ConsoleWrite("Expected merchant.", DEFAULT_COLOR);
    return 0;
  }
  unsigned __int64 merchant;
  sscanf(buffer, "%I64X", &merchant);
  SStrTokenize(&arguments, buffer, sizeof(buffer), " \t", 0);
  if (!buffer[0]) {
    ConsoleWrite("Expected muid.", DEFAULT_COLOR);
    return 0;
  }
  unsigned int muid = SStrToInt(buffer);
  SStrTokenize(&arguments, buffer, sizeof(buffer), " \t", 0);
  if (!buffer[0]) {
    ConsoleWrite("Expected quantity.", DEFAULT_COLOR);
    return 0;
  }
  unsigned int quantity = SStrToInt(buffer);
  SStrTokenize(&arguments, buffer, sizeof(buffer), " \t", 0);
  CDataStore buyMsg;
  buyMsg.Put(370);
  buyMsg.Put(merchant);
  buyMsg.Put(muid);
  buyMsg.Put(quantity);
  buyMsg.Put(static_cast<unsigned char>(buffer[0] && SStrToInt(buffer) != 0));
  buyMsg.Finalize();
  ClientServices_Send(&buyMsg);
  return 1;
}

static int CCommand_BuyItemInSlot(const char* command, const char* arguments) {
  char buffer[64];
  SStrTokenize(&arguments, buffer, sizeof(buffer), " \t", 0);
  if (!buffer[0]) {
    ConsoleWrite("Expected merchant.", DEFAULT_COLOR);
    return 0;
  }
  unsigned __int64 merchant;
  sscanf(buffer, "%I64X", &merchant);
  SStrTokenize(&arguments, buffer, sizeof(buffer), " \t", 0);
  if (!buffer[0]) {
    ConsoleWrite("Expected muid.", DEFAULT_COLOR);
    return 0;
  }
  unsigned int muid = SStrToInt(buffer);
  SStrTokenize(&arguments, buffer, sizeof(buffer), " \t", 0);
  if (!buffer[0]) {
    ConsoleWrite("Expected container.", DEFAULT_COLOR);
    return 0;
  }
  unsigned __int64 container;
  sscanf(buffer, "%I64X", &container);
  SStrTokenize(&arguments, buffer, sizeof(buffer), " \t", 0);
  if (!buffer[0]) {
    ConsoleWrite("Expected quantity.", DEFAULT_COLOR);
    return 0;
  }
  unsigned int quantity = SStrToInt(buffer);
  unsigned char slot = static_cast<unsigned char>(-1);
  SStrTokenize(&arguments, buffer, sizeof(buffer), " \t", 0);
  if (buffer[0]) {
    slot = static_cast<unsigned char>(SStrToInt(buffer));
  }
  CDataStore buyMsg;
  buyMsg.Put(371);
  buyMsg.Put(merchant);
  buyMsg.Put(muid);
  buyMsg.Put(container);
  buyMsg.Put(slot);
  buyMsg.Put(quantity);
  buyMsg.Finalize();
  ClientServices_Send(&buyMsg);
  return 1;
}

static unsigned int CmdMemParseNum(const char*& str) {
  while (*str && (*str < '0' || *str > '9')) {
    ++str;
  }
  unsigned int result = 0;
  while (*str >= '0' && *str <= '9') {
    result = *str++ + 10 * result - '0';
  }
  return result;
}

static void APIENTRY CmdMemOutput(HOUTPUTCONTEXT__* hOutput, const char* str) {
  const char *strNum = SStrChrR(str, ' ');
  if (!strNum) {
    return;
  }

  CMemCmdDump *memDump = reinterpret_cast<CMemCmdDump *>(hOutput);
  CMemCmdItem *item = memDump->m_items.New();
  item->m_allocated = CmdMemParseNum(strNum);
  item->m_committed = CmdMemParseNum(strNum);
  item->m_reserved = CmdMemParseNum(strNum);
  SStrCopy(item->m_name, str, sizeof(item->m_name));
  memDump->m_sumAllocated += item->m_allocated;
  memDump->m_sumCommitted += item->m_committed;
  memDump->m_sumReserved += item->m_reserved;
}

static void DebugPrintMemDump(const CMemCmdDump& memDump) {
  unsigned int numItems = memDump.m_items.Count();
  unsigned int avgAllocated =
      static_cast<unsigned int>(memDump.m_sumAllocated / numItems);
  unsigned int avgCommitted =
      static_cast<unsigned int>(memDump.m_sumCommitted / numItems);
  unsigned int avgReserved =
      static_cast<unsigned int>(memDump.m_sumReserved / numItems);
  OsOutputDebugString("*** MEMORY DUMP BEGIN ***\n");
  OsOutputDebugString(
      "   ***     sums: %I64dk/%I64dk/%I64dk ***\n",
      memDump.m_sumAllocated, memDump.m_sumCommitted,
      memDump.m_sumReserved);
  OsOutputDebugString(
      "   *** averages: %uk/%uk/%uk ***\n",
      avgAllocated, avgCommitted, avgReserved);
  for (unsigned int i = 0; i < numItems; ++i) {
    OsOutputDebugString("%s\n", memDump.m_items[i].m_name);
  }
  OsOutputDebugString("*** MEMORY DUMP END ***\n");
  OsOutputDebugString(
      "   ***     sums: %I64dk/%I64dk/%I64dk ***\n",
      memDump.m_sumAllocated, memDump.m_sumCommitted,
      memDump.m_sumReserved);
  OsOutputDebugString(
      "   *** averages: %uk/%uk/%uk ***\n",
      avgAllocated, avgCommitted, avgReserved);
}

static void FilePrintMemDump(const CMemCmdDump& memDump, const char* fileName) {
  unsigned int numItems = memDump.m_items.Count();
  unsigned int avgAllocated =
      static_cast<unsigned int>(memDump.m_sumAllocated / numItems);
  unsigned int avgCommitted =
      static_cast<unsigned int>(memDump.m_sumCommitted / numItems);
  unsigned int avgReserved =
      static_cast<unsigned int>(memDump.m_sumReserved / numItems);
  FILE *file = fopen(fileName, "wt");
  if (!file) {
    char *error = OsGetLastErrorStr();
    ConsoleWriteA(
        "Failed to open %s for writing.", static_cast<COLOR_T>(2),
        fileName);
    ConsoleWrite(error, static_cast<COLOR_T>(4));
    return;
  }

  fprintf(file, "*** MEMORY DUMP BEGIN ***\n");
  fprintf(
      file, "   ***     sums: %I64dk/%I64dk/%I64dk ***\n",
      memDump.m_sumAllocated, memDump.m_sumCommitted,
      memDump.m_sumReserved);
  fprintf(
      file, "   *** averages: %uk/%uk/%uk ***\n",
      avgAllocated, avgCommitted, avgReserved);
  for (unsigned int i = 0; i < numItems; ++i) {
    fprintf(file, "%s\n", memDump.m_items[i].m_name);
  }
  fprintf(file, "*** MEMORY DUMP END ***\n");
  fprintf(
      file, "   ***     sums: %I64dk/%I64dk/%I64dk ***\n",
      memDump.m_sumAllocated, memDump.m_sumCommitted,
      memDump.m_sumReserved);
  fprintf(
      file, "   *** averages: %uk/%uk/%uk ***\n",
      avgAllocated, avgCommitted, avgReserved);
  fclose(file);
}

static int CCommand_Mem(const char* command, const char* arguments) {
  CMemCmdDump memDump;
  memset(&memDump, 0, 24);
  memDump.m_items.SetChunkSize(256);
  SMemDumpState(
      reinterpret_cast<SMEMDUMPPROC>(CmdMemOutput),
      reinterpret_cast<HOUTPUTCONTEXT>(&memDump));
  if (memDump.m_items.Count()) {
    qsort(
        memDump.m_items.Ptr(), memDump.m_items.Count(),
        sizeof(CMemCmdItem), CMemCmdItem::Compare);
    if (arguments && *arguments) {
      FilePrintMemDump(memDump, arguments);
    } else {
      DebugPrintMemDump(memDump);
    }
  }
  return 1;
}

void InstallGameConsoleCommands() {
  ConsoleCommandRegister("loc", reinterpret_cast<CONSOLECOMMANDHANDLER>(CCommand_Loc), DEBUG, 0);
  ConsoleCommandRegister("dloc", reinterpret_cast<CONSOLECOMMANDHANDLER>(CCommand_DLoc), DEBUG, 0);
  ConsoleCommandRegister("facing", reinterpret_cast<CONSOLECOMMANDHANDLER>(CCommand_Facing), DEBUG, 0);
  ConsoleCommandRegister("dfacing", reinterpret_cast<CONSOLECOMMANDHANDLER>(CCommand_DFacing), DEBUG, 0);
  ConsoleCommandRegister("showbounds", reinterpret_cast<CONSOLECOMMANDHANDLER>(CCommand_ShowBounds), DEBUG, 0);
  ConsoleCommandRegister("tloc", reinterpret_cast<CONSOLECOMMANDHANDLER>(CCommand_TargetLoc), DEBUG, 0);
  ConsoleCommandRegister("TerminalVelocity", reinterpret_cast<CONSOLECOMMANDHANDLER>(CCommand_TerminalVelocity), DEBUG, 0);
  ConsoleCommandRegister("ci", reinterpret_cast<CONSOLECOMMANDHANDLER>(CCommand_CreateItem), GAME, 0);
  ConsoleCommandRegister("cm", reinterpret_cast<CONSOLECOMMANDHANDLER>(CCommand_CreateMonster), GAME, 0);
  ConsoleCommandRegister("cgo", reinterpret_cast<CONSOLECOMMANDHANDLER>(CCommand_CreateGameObject), GAME, 0);
  ConsoleCommandRegister("pet", reinterpret_cast<CONSOLECOMMANDHANDLER>(CCommand_CreatePet), GAME, 0);
  ConsoleCommandRegister("dm", reinterpret_cast<CONSOLECOMMANDHANDLER>(CCommand_DestroyMonster), GAME, 0);
  ConsoleCommandRegister("attackme", reinterpret_cast<CONSOLECOMMANDHANDLER>(CCommand_AttackPlayer), COMBAT, 0);
  ConsoleCommandRegister("targetattack", reinterpret_cast<CONSOLECOMMANDHANDLER>(CCommand_TargetAttack), COMBAT, 0);
  ConsoleCommandRegister("speed", reinterpret_cast<CONSOLECOMMANDHANDLER>(CCommand_Speed), DEBUG, 0);
  ConsoleCommandRegister("walkspeed", reinterpret_cast<CONSOLECOMMANDHANDLER>(CCommand_WalkSpeed), DEBUG, 0);
  ConsoleCommandRegister("swimspeed", reinterpret_cast<CONSOLECOMMANDHANDLER>(CCommand_SwimSpeed), DEBUG, 0);
  ConsoleCommandRegister("turnspeed", reinterpret_cast<CONSOLECOMMANDHANDLER>(CCommand_TurnSpeed), DEBUG, 0);
  ConsoleCommandRegister("port", reinterpret_cast<CONSOLECOMMANDHANDLER>(CCommand_Teleport), DEBUG, 0);
  ConsoleCommandRegister("worldport", reinterpret_cast<CONSOLECOMMANDHANDLER>(CCommand_WorldTeleport), DEBUG, 0);
  ConsoleCommandRegister("money", reinterpret_cast<CONSOLECOMMANDHANDLER>(CCommand_Money), DEBUG, 0);
  ConsoleCommandRegister("save", reinterpret_cast<CONSOLECOMMANDHANDLER>(CCommand_Save), GAME, 0);
  ConsoleCommandRegister("deathbind", reinterpret_cast<CONSOLECOMMANDHANDLER>(CCommand_BindPoint), GAME, 0);
  ConsoleCommandRegister(
      "db", reinterpret_cast<CONSOLECOMMANDHANDLER>(CCommand_DBLookup), DEBUG,
      "TableName (Name or #ID) Note:Wildcard use * in TableName or Name not ID though");
  ConsoleCommandRegister("drawlog", reinterpret_cast<CONSOLECOMMANDHANDLER>(CCommand_DrawLog), DEBUG, 0);
  ConsoleCommandRegister("animlog", reinterpret_cast<CONSOLECOMMANDHANDLER>(CCommand_AnimLog), DEBUG, 0);
  ConsoleCommandRegister("showplayer", reinterpret_cast<CONSOLECOMMANDHANDLER>(CCommand_TogglePlayer), GRAPHICS, 0);
  ConsoleCommandRegister("motionBlend", reinterpret_cast<CONSOLECOMMANDHANDLER>(CCommand_ToggleAnimBlending), GRAPHICS, 0);
  ConsoleCommandRegister("beastmaster", reinterpret_cast<CONSOLECOMMANDHANDLER>(CCommand_Beastmaster), DEBUG, 0);
  ConsoleCommandRegister("sendevent", reinterpret_cast<CONSOLECOMMANDHANDLER>(CCommand_SendEvent), DEBUG, 0);
  ConsoleCommandRegister("recharge", reinterpret_cast<CONSOLECOMMANDHANDLER>(CCommand_Recharge), GAME, 0);
  ConsoleCommandRegister("mem", reinterpret_cast<CONSOLECOMMANDHANDLER>(CCommand_Mem), DEBUG, 0);
  ConsoleCommandRegister("level", reinterpret_cast<CONSOLECOMMANDHANDLER>(CCommand_Level), DEBUG, 0);
  ConsoleCommandRegister("petlevel", reinterpret_cast<CONSOLECOMMANDHANDLER>(CCommand_PetLevel), DEBUG, 0);
  ConsoleCommandRegister("clearquest", reinterpret_cast<CONSOLECOMMANDHANDLER>(CCommand_ClearQuest), DEBUG, 0);
  ConsoleCommandRegister("flagquest", reinterpret_cast<CONSOLECOMMANDHANDLER>(CCommand_FlagQuest), DEBUG, 0);
  ConsoleCommandRegister("TaxiClearAllNodes", reinterpret_cast<CONSOLECOMMANDHANDLER>(CCommand_TaxiClearAllNodes), DEBUG, 0);
  ConsoleCommandRegister("TaxiEnableAllNodes", reinterpret_cast<CONSOLECOMMANDHANDLER>(CCommand_TaxiEnableAllNodes), DEBUG, 0);
  ConsoleCommandRegister("ChangeCellZone", reinterpret_cast<CONSOLECOMMANDHANDLER>(CCommand_ChangeCellZone), DEBUG, 0);
  ConsoleCommandRegister("played", reinterpret_cast<CONSOLECOMMANDHANDLER>(CCommand_Played), GAME, 0);
  ConsoleCommandRegister("lootMethod", reinterpret_cast<CONSOLECOMMANDHANDLER>(CCommand_LootMethod), GAME, 0);
  ConsoleCommandRegister("finishquest", reinterpret_cast<CONSOLECOMMANDHANDLER>(CCommand_FlagQuestFinish), DEBUG, 0);
  ConsoleCommandRegister("cameratarget", reinterpret_cast<CONSOLECOMMANDHANDLER>(CCommand_CameraTarget), DEBUG, 0);
  ConsoleCommandRegister("questquery", reinterpret_cast<CONSOLECOMMANDHANDLER>(CCommand_QuestCommand), DEBUG, 0);
  ConsoleCommandRegister("questaccept", reinterpret_cast<CONSOLECOMMANDHANDLER>(CCommand_QuestCommand), DEBUG, 0);
  ConsoleCommandRegister("questcomplete", reinterpret_cast<CONSOLECOMMANDHANDLER>(CCommand_QuestCommand), DEBUG, 0);
  ConsoleCommandRegister("questcancel", reinterpret_cast<CONSOLECOMMANDHANDLER>(CCommand_QuestCommand), DEBUG, 0);
  ConsoleCommandRegister("reclaim", reinterpret_cast<CONSOLECOMMANDHANDLER>(CCommand_Reclaim), DEBUG, 0);
  ConsoleCommandRegister("buyspell", reinterpret_cast<CONSOLECOMMANDHANDLER>(CCommand_BuySpell), DEBUG, 0);
  ConsoleCommandRegister("sellitem", reinterpret_cast<CONSOLECOMMANDHANDLER>(CCommand_SellItem), DEBUG, 0);
  ConsoleCommandRegister("buyitem", reinterpret_cast<CONSOLECOMMANDHANDLER>(CCommand_BuyItem), DEBUG, 0);
  ConsoleCommandRegister("buyiteminslot", reinterpret_cast<CONSOLECOMMANDHANDLER>(CCommand_BuyItemInSlot), DEBUG, 0);

  InstallGMCommands();
}

void UninstallGameConsoleCommands() {
  ConsoleCommandUnregister("loc");
  ConsoleCommandUnregister("dloc");
  ConsoleCommandUnregister("facing");
  ConsoleCommandUnregister("dfacing");
  ConsoleCommandUnregister("showbounds");
  ConsoleCommandUnregister("tloc");
  ConsoleCommandUnregister("TerminalVelocity");
  ConsoleCommandUnregister("ci");
  ConsoleCommandUnregister("cm");
  ConsoleCommandUnregister("cgo");
  ConsoleCommandUnregister("pet");
  ConsoleCommandUnregister("dm");
  ConsoleCommandUnregister("attackme");
  ConsoleCommandUnregister("targetattack");
  ConsoleCommandUnregister("speed");
  ConsoleCommandUnregister("walkspeed");
  ConsoleCommandUnregister("swimspeed");
  ConsoleCommandUnregister("turnspeed");
  ConsoleCommandUnregister("port");
  ConsoleCommandUnregister("worldport");
  ConsoleCommandUnregister("money");
  ConsoleCommandUnregister("save");
  ConsoleCommandUnregister("deathbind");
  ConsoleCommandUnregister("db");
  ConsoleCommandUnregister("drawlog");
  ConsoleCommandUnregister("animlog");
  ConsoleCommandUnregister("showplayer");
  ConsoleCommandUnregister("motionBlend");
  ConsoleCommandUnregister("beastmaster");
  ConsoleCommandUnregister("sendevent");
  ConsoleCommandUnregister("recharge");
  ConsoleCommandUnregister("mem");
  ConsoleCommandUnregister("level");
  ConsoleCommandUnregister("petlevel");
  ConsoleCommandUnregister("clearquest");
  ConsoleCommandUnregister("flagquest");
  ConsoleCommandUnregister("TaxiClearAllNodes");
  ConsoleCommandUnregister("TaxiEnableAllNodes");
  ConsoleCommandUnregister("ChangeCellZone");
  ConsoleCommandUnregister("played");
  ConsoleCommandUnregister("lootMethod");
  ConsoleCommandUnregister("finishquest");
  ConsoleCommandUnregister("cameratarget");
  ConsoleCommandUnregister("questquery");
  ConsoleCommandUnregister("questaccept");
  ConsoleCommandUnregister("questcomplete");
  ConsoleCommandUnregister("questcancel");
  ConsoleCommandUnregister("reclaim");
  ConsoleCommandUnregister("buyspell");
  ConsoleCommandUnregister("sellitem");
  ConsoleCommandUnregister("buyitem");
  ConsoleCommandUnregister("buyiteminslot");

  UninstallGMCommands();
}
