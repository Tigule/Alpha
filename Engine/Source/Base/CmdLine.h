#pragma once

#include <windows.h>

enum CMDOPT {
  CMD_D3D = 0,
  CMD_DATA_DIR = 1,
  CMD_NO_LAG_FIX = 2,
  CMD_LOADFILE = 3,
  CMD_GAMETYPE = 4,
  CMD_OPENGL = 5,
  CMD_SW_TNL = 6,
  CMD_TIMEDEMO = 7,
  CMD_DEMOREZ = 8,
  CMD_DEMODEPTH = 9,
  CMD_DEMODETAIL = 10,
  CMD_DEMOSOUND = 11,
  CMD_FULL_SCREEN = 12,
  CMD_22050HZ = 13,
  CMD_NO_WARNINGS = 14,
  CMDOPTS = 15
};

int    CmdLineGetBool(CMDOPT opt);
LPCSTR CmdLineGetString(CMDOPT opt);
UINT   CmdLineGetUint(CMDOPT opt);
int    CmdLineProcess();
