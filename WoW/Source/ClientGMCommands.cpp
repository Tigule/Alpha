#include "Console/ConsoleCommand.h"

void __fastcall UninstallGMCommands() {
  ConsoleCommandUnregister("ghost");
  ConsoleCommandUnregister("invis");
  ConsoleCommandUnregister("bindplayer");
  ConsoleCommandUnregister("summon");
  ConsoleCommandUnregister("showlabel");
  ConsoleCommandUnregister("setsecurity");
  ConsoleCommandUnregister("nuke");
}
