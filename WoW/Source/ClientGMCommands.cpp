#include "Console/ConsoleCommand.h"

static int CCommand_Ghost(const char* command, const char* args) {
    // TODO: implement
    return 0;
}

static int CCommand_Invis(const char* command, const char* args) {
    // TODO: implement
    return 0;
}

static int CCommand_BindPlayer(const char* command, const char* args) {
    // TODO: implement
    return 0;
}

static int CCommand_Summon(const char* command, const char* args) {
    // TODO: implement
    return 0;
}

static int CCommand_ShowLabel(const char* command, const char* args) {
    // TODO: implement
    return 0;
}

static int CCommand_SetSecurity(const char* command, const char* args) {
    // TODO: implement
    return 0;
}

static int CCommand_Nuke(const char* command, const char* args) {
    // TODO: implement
    return 0;
}

void __fastcall InstallGMCommands() {
    // TODO: implement
}

void __fastcall UninstallGMCommands() {
  ConsoleCommandUnregister("ghost");
  ConsoleCommandUnregister("invis");
  ConsoleCommandUnregister("bindplayer");
  ConsoleCommandUnregister("summon");
  ConsoleCommandUnregister("showlabel");
  ConsoleCommandUnregister("setsecurity");
  ConsoleCommandUnregister("nuke");
}
