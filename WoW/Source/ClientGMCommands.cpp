#include <Base/Base.h>
#include <Gx/Gx.h>
#include <WowConst.h>

#include "Client.h"
#include <Base/CDataStore.h>

#include "Console/ConsoleClient.h"
#include "Console/ConsoleCommand.h"
#include "Glue/CGlueMgr.h"
#include "UIUtil/Tooltip.h"
#include "Ui/ChatFrame.h"
#include "Ui/WorldFrame.h"
#include "UIUtil/InputControl.h"
#include "WowSvcs/WowSvcsClient/ClientServices.h"
#include "Object/ObjectClient/Player_C.h"
#include "Ui/GameUI.h"

#include <ctype.h>

#include "WowServices/WDataStore.h"

static BOOL CCommand_Ghost(LPCSTR command, LPCSTR args) {
  WDataStore msg;
  msg.Put(CMSG_GM_INVIS);
  if (args && !SStrCmpI(args, "off", 0x7FFFFFFF)) {
    msg.PutInt(0);
    msg.Finalize();
    ClientServices_Send(&msg);
    CGPlayer_C::StopGhosting();
  } else if (*args) {
    msg.PutInt(1);
    msg.Finalize();
    ClientServices_Send(&msg);
    CGPlayer_C::StartGhosting(args);
  } else {
    msg.PutInt(1);
    msg.Finalize();
    ClientServices_Send(&msg);
    CGPlayer_C::StartGhosting(CGGameUI::GetLockedTarget());
  }
  return 1;
}

static BOOL CCommand_Invis(LPCSTR command, LPCSTR args) {
  WDataStore msg;
  msg.Put(CMSG_GM_INVIS);
  if (!args || SStrCmpI(args, "off", 0x7FFFFFFF)) {
    msg.Put(1);
  } else {
    msg.Put(0);
  }
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
}

static BOOL CCommand_BindPlayer(LPCSTR command, LPCSTR args) {
  WDataStore msg;
  msg.Put(MSG_GM_BIND_OTHER);
  if (args && *args) {
    msg.Put((BYTE)1);
    msg.PutString(args);
  } else {
    msg.Put((BYTE)0);
    msg.Put(CGGameUI::GetLockedTarget());
  }
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
}

static BOOL CCommand_Summon(LPCSTR command, LPCSTR args) {
  WDataStore msg;
  msg.Put(MSG_GM_SUMMON);
  msg.PutString(args);
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
}

static BOOL CCommand_ShowLabel(LPCSTR command, LPCSTR args) {
  WDataStore msg;
  msg.Put(MSG_GM_SHOWLABEL);
  msg.Put(SStrToInt(args));
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
}

static BOOL CCommand_SetSecurity(LPCSTR command, LPCSTR args) {
  char   name[50];
  char  *namePtr = name;
  LPCSTR argPtr = args;
  while (*argPtr && !isspace(*argPtr) && namePtr < name + 49) {
    *namePtr++ = *argPtr++;
  }
  *namePtr = 0;
  while (isspace(*argPtr)) {
    ++argPtr;
  }

  UINT security = SStrToInt(argPtr);
  ConsolePrintf("Setting '%s' to security group %d", name, security);

  WDataStore msg;
  msg.Put(CMSG_GM_SET_SECURITY_GROUP);
  msg.PutString(name);
  msg.Put(security);
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
}

static BOOL CCommand_Nuke(LPCSTR command, LPCSTR args) {
  WDataStore msg;
  msg.Put(CMSG_GM_NUKE);
  msg.PutString(args);
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
}

void InstallGMCommands() {
  ConsoleCommandRegister("ghost", CCommand_Ghost, GM, "Watch a player");
  ConsoleCommandRegister("invis", CCommand_Invis, GM, "Go GM Invis");
  ConsoleCommandRegister("bindplayer", CCommand_BindPlayer, GM, "Bind another player to their current loc");
  ConsoleCommandRegister("summon", CCommand_Summon, GM, "Summon a named player to your location");
  ConsoleCommandRegister("showlabel", CCommand_ShowLabel, GM, "Toggle showing 'GM' label to other players");
  ConsoleCommandRegister("setsecurity", CCommand_SetSecurity, GM, "Set another character's security group");
  ConsoleCommandRegister(
      "nuke", CCommand_Nuke, GM, "Nuke a player (forcibly remove from server completely)"
  );
}

void UninstallGMCommands() {
  ConsoleCommandUnregister("ghost");
  ConsoleCommandUnregister("invis");
  ConsoleCommandUnregister("bindplayer");
  ConsoleCommandUnregister("summon");
  ConsoleCommandUnregister("showlabel");
  ConsoleCommandUnregister("setsecurity");
  ConsoleCommandUnregister("nuke");
}
