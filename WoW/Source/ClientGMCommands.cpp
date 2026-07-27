#include "Console/ConsoleCommand.h"
#include "Console/ConsoleClient.h"
#include "Object/ObjectClient/Player_C.h"
#include "Ui/GameUI.h"
#include "WowSvcs/WowSvcsClient/ClientServices.h"

#include <Base/CDataStore.h>
#include <ctype.h>

static int CCommand_Ghost(const char* command, const char* args) {
  CDataStore msg;
  msg.Put(471);
  if (!args || SStrCmpI(args, "off", 0x7FFFFFFF)) {
    msg.Put(1);
    ClientServices_Send(&msg);
    if (*args) {
      CGPlayer_C::StartGhosting(args);
    } else {
      CGPlayer_C::StartGhosting(CGGameUI::GetLockedTarget());
    }
  } else {
    msg.Put(0);
    ClientServices_Send(&msg);
    CGPlayer_C::StopGhosting();
  }
  return 1;
}

static int CCommand_Invis(const char* command, const char* args) {
  CDataStore msg;
  msg.Put(471);
  msg.Put(!args || SStrCmpI(args, "off", 0x7FFFFFFF) ? 1 : 0);
  ClientServices_Send(&msg);
  return 1;
}

static int CCommand_BindPlayer(const char* command, const char* args) {
  CDataStore msg;
  msg.Put(473);
  if (args && *args) {
    msg.Put(static_cast<unsigned char>(1));
    msg.PutString(args);
  } else {
    msg.Put(static_cast<unsigned char>(0));
    msg.Put(CGGameUI::GetLockedTarget());
  }
  ClientServices_Send(&msg);
  return 1;
}

static int CCommand_Summon(const char* command, const char* args) {
  CDataStore msg;
  msg.Put(474);
  msg.PutString(args);
  ClientServices_Send(&msg);
  return 1;
}

static int CCommand_ShowLabel(const char* command, const char* args) {
  CDataStore msg;
  msg.Put(480);
  msg.Put(SStrToInt(args));
  ClientServices_Send(&msg);
  return 1;
}

static int CCommand_SetSecurity(const char* command, const char* args) {
  char        name[50];
  char       *namePtr = name;
  const char *argPtr = args;
  while (*argPtr && !isspace(*argPtr) && namePtr < name + 49) {
    *namePtr++ = *argPtr++;
  }
  *namePtr = 0;
  while (isspace(*argPtr)) {
    ++argPtr;
  }

  unsigned int security = SStrToInt(argPtr);
  ConsolePrintf("Setting '%s' to security group %d", name, security);

  CDataStore msg;
  msg.Put(490);
  msg.PutString(name);
  msg.Put(security);
  ClientServices_Send(&msg);
  return 1;
}

static int CCommand_Nuke(const char* command, const char* args) {
  CDataStore msg;
  msg.Put(491);
  msg.PutString(args);
  ClientServices_Send(&msg);
  return 1;
}

void InstallGMCommands() {
  ConsoleCommandRegister("ghost", reinterpret_cast<CONSOLECOMMANDHANDLER>(CCommand_Ghost), GM, "Watch a player");
  ConsoleCommandRegister("invis", reinterpret_cast<CONSOLECOMMANDHANDLER>(CCommand_Invis), GM, "Go GM Invis");
  ConsoleCommandRegister(
      "bindplayer", reinterpret_cast<CONSOLECOMMANDHANDLER>(CCommand_BindPlayer), GM,
      "Bind another player to their current loc");
  ConsoleCommandRegister(
      "summon", reinterpret_cast<CONSOLECOMMANDHANDLER>(CCommand_Summon), GM,
      "Summon a named player to your location");
  ConsoleCommandRegister(
      "showlabel", reinterpret_cast<CONSOLECOMMANDHANDLER>(CCommand_ShowLabel), GM,
      "Toggle showing 'GM' label to other players");
  ConsoleCommandRegister(
      "setsecurity", reinterpret_cast<CONSOLECOMMANDHANDLER>(CCommand_SetSecurity), GM,
      "Set another character's security group");
  ConsoleCommandRegister(
      "nuke", reinterpret_cast<CONSOLECOMMANDHANDLER>(CCommand_Nuke), GM,
      "Nuke a player (forcibly remove from server completely)");
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
