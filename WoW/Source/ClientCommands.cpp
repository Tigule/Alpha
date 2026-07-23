#include "Client.h"
#include <Base/CDataStore.h>

#include "Console/ConsoleCommand.h"
#include "WowSvcs/WowSvcsClient/ClientServices.h"

struct CMemCmdDump;

static int CCommand_DBLookup(const char* command, const char* string) {
    // TODO: implement
    return 0;
}

static int CCommand_DrawLog(const char* command, const char* arguments) {
    // TODO: implement
    return 0;
}

static int CCommand_AnimLog(const char* command, const char* arguments) {
    // TODO: implement
    return 0;
}

static int CCommand_TogglePlayer(const char* command, const char* arguments) {
    // TODO: implement
    return 0;
}

static int CCommand_ToggleAnimBlending(const char* command, const char* arguments) {
    // TODO: implement
    return 0;
}

static int CCommand_ShowBounds(const char* command, const char* arguments) {
    // TODO: implement
    return 0;
}

static int CCommand_Loc(const char*, const char*) {
    // TODO: implement
    return 0;
}

static int CCommand_TerminalVelocity(const char* command, const char* arguments) {
    // TODO: implement
    return 0;
}

static int CCommand_DLoc(const char* command, const char* arguments) {
    // TODO: implement
    return 0;
}

static int CCommand_TargetLoc(const char*, const char*) {
    // TODO: implement
    return 0;
}

static int CCommand_Facing(const char* command, const char* arguments) {
    // TODO: implement
    return 0;
}

static int CCommand_DFacing(const char* command, const char* arguments) {
    // TODO: implement
    return 0;
}

static int CCommand_Speed(const char* command, const char* arguments) {
    // TODO: implement
    return 0;
}

static int CCommand_WalkSpeed(const char* command, const char* arguments) {
    // TODO: implement
    return 0;
}

static int CCommand_SwimSpeed(const char* command, const char* arguments) {
    // TODO: implement
    return 0;
}

static int CCommand_TurnSpeed(const char* command, const char* arguments) {
    // TODO: implement
    return 0;
}

static int CCommand_Money(const char* command, const char* arguments) {
    // TODO: implement
    return 0;
}

static int CCommand_WorldTeleport(const char* command, const char* arguments) {
    // TODO: implement
    return 0;
}

static int CCommand_Teleport(const char* command, const char* arguments) {
    // TODO: implement
    return 0;
}

static int CCommand_CreateItem(const char* command, const char* arguments) {
    // TODO: implement
    return 0;
}

static int CCommand_CreateGameObject(const char* command, const char* arguments) {
    // TODO: implement
    return 0;
}

static int CCommand_CreateMonster(const char* command, const char* arguments) {
    // TODO: implement
    return 0;
}

static int CCommand_CreatePet(const char* command, const char* arguments) {
    // TODO: implement
    return 0;
}

static int CCommand_DestroyMonster(const char* command, const char* arguments) {
    // TODO: implement
    return 0;
}

static int CCommand_AttackPlayer(const char* command, const char* arguments) {
    // TODO: implement
    return 0;
}

static int CCommand_TargetAttack(const char* command, const char* arguments) {
    // TODO: implement
    return 0;
}

static int __fastcall CCommand_Save(const char *command, const char *arguments) {
  CDataStore message;
  message.Put(CMSG_SAVE_PLAYER);
  message.Finalize();
  ClientServices_Send(&message);
  return 1;
}

static int CCommand_BindPoint(const char* command, const char* arguments) {
    // TODO: implement
    return 0;
}

static int CCommand_Beastmaster(const char* command, const char* arguments) {
    // TODO: implement
    return 0;
}

static int CCommand_SendEvent(const char* command, const char* arguments) {
    // TODO: implement
    return 0;
}

static int __fastcall CCommand_Recharge(const char *command, const char *arguments) {
  CDataStore msg;
  msg.Put(CMSG_RECHARGE);
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
}

static int CCommand_Level(const char* command, const char* arguments) {
    // TODO: implement
    return 0;
}

static int CCommand_PetLevel(const char* command, const char* arguments) {
    // TODO: implement
    return 0;
}

static int CCommand_ClearQuest(const char* command, const char* arguments) {
    // TODO: implement
    return 0;
}

static int CCommand_FlagQuest(const char* command, const char* arguments) {
    // TODO: implement
    return 0;
}

static int CCommand_FlagQuestFinish(const char* command, const char* arguments) {
    // TODO: implement
    return 0;
}

static void QueryQuest(const unsigned __int64& questGiver, int questID) {
    // TODO: implement
}

static void AcceptQuest(const unsigned __int64& questGiver, int questID) {
    // TODO: implement
}

static void CompleteQuest(const unsigned __int64& questGiver, int questID) {
    // TODO: implement
}

static void QuestLogRemoveQuest(int entry) {
    // TODO: implement
}

static int CCommand_QuestCommand(const char* command, const char* arguments) {
    // TODO: implement
    return 0;
}

static int __fastcall CCommand_TaxiClearAllNodes(const char *, const char *) {
  CDataStore msg;
  msg.Put(CMSG_TAXICLEARALLNODES);
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
}

static int __fastcall CCommand_TaxiEnableAllNodes(const char *, const char *) {
  CDataStore msg;
  msg.Put(CMSG_TAXIENABLEALLNODES);
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
}

static int CCommand_ChangeCellZone(const char*, const char* args) {
    // TODO: implement
    return 0;
}

static int __fastcall CCommand_Played(const char *, const char *) {
  CDataStore msg;
  msg.Put(CMSG_PLAYED_TIME);
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
}

static int CCommand_LootMethod(const char*, const char* args) {
    // TODO: implement
    return 0;
}

static int CCommand_CameraTarget(const char*, const char*) {
    // TODO: implement
    return 0;
}

static int CCommand_Reclaim(const char* command, const char* arguments) {
    // TODO: implement
    return 0;
}

static int CCommand_BuySpell(const char* command, const char* arguments) {
    // TODO: implement
    return 0;
}

static int CCommand_SellItem(const char* command, const char* arguments) {
    // TODO: implement
    return 0;
}

static int CCommand_BuyItem(const char* command, const char* arguments) {
    // TODO: implement
    return 0;
}

static int CCommand_BuyItemInSlot(const char* command, const char* arguments) {
    // TODO: implement
    return 0;
}

static unsigned int CmdMemParseNum(const char*& str) {
    // TODO: implement
    return 0;
}

static void CmdMemOutput(HOUTPUTCONTEXT__* hOutput, const char* str) {
    // TODO: implement
}

static void DebugPrintMemDump(const CMemCmdDump& memDump) {
    // TODO: implement
}

static void FilePrintMemDump(const CMemCmdDump& memDump, const char* fileName) {
    // TODO: implement
}

static int CCommand_Mem(const char* command, const char* arguments) {
    // TODO: implement
    return 0;
}

void __fastcall InstallGameConsoleCommands() {
    // TODO: implement
}

void __fastcall UninstallGameConsoleCommands() {
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
