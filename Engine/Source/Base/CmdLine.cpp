#include "CmdLine.h"

#include <storm.h>

static char buffer[MAX_PATH];

static const ARGLIST s_argList[CMDOPTS] = {
    {  SCMD_TYPE_BOOL,         CMD_D3D,        "d3d", 0},
    {SCMD_TYPE_STRING,    CMD_DATA_DIR,    "datadir", 0},
    {  SCMD_TYPE_BOOL,  CMD_NO_LAG_FIX,   "nolagfix", 0},
    {SCMD_TYPE_STRING,    CMD_LOADFILE,   "loadfile", 0},
    {SCMD_TYPE_STRING,    CMD_GAMETYPE,   "gametype", 0},
    {  SCMD_TYPE_BOOL,      CMD_OPENGL,     "opengl", 0},
    {  SCMD_TYPE_BOOL,      CMD_SW_TNL,      "swtnl", 0},
    {  SCMD_TYPE_BOOL,    CMD_TIMEDEMO,   "timedemo", 0},
    {SCMD_TYPE_STRING,     CMD_DEMOREZ,        "rez", 0},
    {SCMD_TYPE_STRING,   CMD_DEMODEPTH,      "depth", 0},
    {SCMD_TYPE_STRING,  CMD_DEMODETAIL,     "detail", 0},
    {SCMD_TYPE_STRING,   CMD_DEMOSOUND,      "sound", 0},
    {  SCMD_TYPE_BOOL, CMD_FULL_SCREEN, "fullscreen", 0},
    {  SCMD_TYPE_BOOL,     CMD_22050HZ,      "22050", 0},
    {  SCMD_TYPE_BOOL, CMD_NO_WARNINGS, "nowarnings", 0}
};

int __fastcall CmdLineGetBool(CMDOPT opt) {
  return SCmdGetBool(opt);
}

const char *__fastcall CmdLineGetString(CMDOPT opt) {
  SCmdGetString(opt, buffer, sizeof(buffer));
  return buffer;
}

unsigned int __fastcall CmdLineGetUint(CMDOPT opt) {
  return SCmdGetNum(opt);
}

int __fastcall CmdLineProcess() {
  SCmdRegisterArgList(s_argList, CMDOPTS);
  return SCmdProcessCommandLine(0, 0);
}
