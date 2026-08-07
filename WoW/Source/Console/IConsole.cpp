#include <Base/Base.h>

#include "ConsoleClient.h"
#include "ConsoleCommand.h"

#include <storm.h>

char     g_commandHistory[32][80];
UINT     g_commandHistoryIndex;
char     g_ExecBuffer[0x2000];
EXECMODE g_ExecCreateMode = EM_NOTACTIVE;

BOOL AddLineToExecFile(LPCSTR currentLine) {
  char stringToWrite[0x104];
  int  spaceRemaining;

  if (g_ExecCreateMode == EM_PROMPTOVERWRITE) {
    if (*currentLine == 'n') {
      ConsoleWrite("Canceled File Creation", ECHO_COLOR);
      g_ExecCreateMode = EM_NOTACTIVE;
      return 0;
    }
    if (*currentLine == 'y') {
      ConsoleWrite("Begin Typing the commands", ECHO_COLOR);
      g_ExecCreateMode = EM_RECORDING;
      return 0;
    }

    ConsoleWrite("You must type 'y' to confirm overwrite. Process aborted!", ERROR_COLOR);
    g_ExecCreateMode = EM_NOTACTIVE;
    return 0;
  }

  if (!SStrCmpI(currentLine, "end", 0x7FFFFFFF)) {
    if (g_ExecCreateMode != EM_APPEND) {
      g_ExecCreateMode = EM_WRITEFILE;
    }
    return 1;
  }

  SStrPrintf(stringToWrite, sizeof(stringToWrite), "%s\n", currentLine);
  spaceRemaining = 0x1FFF - SStrLen(g_ExecBuffer);
  if (spaceRemaining != (int)SStrLen(stringToWrite)) {
    SStrPack(g_ExecBuffer, stringToWrite, sizeof(g_ExecBuffer));
  }
  return 0;
}

void AddToHistory(LPCSTR command) {
  SStrCopy(g_commandHistory[g_commandHistoryIndex], command, 80);
  g_commandHistoryIndex = (g_commandHistoryIndex + 1) & 0x1F;
}
