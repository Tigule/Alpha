#include "Client.h"
#include <Base/CDataStore.h>

#include "Console/ConsoleCommand.h"
#include "WowSvcs/WowSvcsClient/ClientServices.h"

static int __fastcall CCommand_Save(const char *command, const char *arguments) {
  CDataStore message;
  message.Put(CMSG_SAVE_PLAYER);
  message.Finalize();
  ClientServices_Send(&message);
  return 1;
}

static int __fastcall CCommand_Recharge(const char *command, const char *arguments) {
  CDataStore msg;
  msg.Put(CMSG_RECHARGE);
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
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

static int __fastcall CCommand_Played(const char *, const char *) {
  CDataStore msg;
  msg.Put(CMSG_PLAYED_TIME);
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
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
