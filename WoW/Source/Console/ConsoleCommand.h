#pragma once

#include <stpl.h>

enum CATEGORY {
  DEBUG = 0,
  GRAPHICS = 1,
  CONSOLE = 2,
  COMBAT = 3,
  GAME = 4,
  DEFAULT = 5,
  NET = 6,
  SOUND = 7,
  GM = 8,
  NONE = 9,
  LAST = 10
};

enum COLOR_T {
  DEFAULT_COLOR = 0,
  INPUT_COLOR = 1,
  ECHO_COLOR = 2,
  ERROR_COLOR = 3,
  WARNING_COLOR = 4,
  GLOBAL_COLOR = 5,
  ADMIN_COLOR = 6,
  HIGHLIGHT_COLOR = 7,
  BACKGROUND_COLOR = 8,
  NUM_COLORTYPES = 9
};

enum EXECMODE {
  EM_PROMPTOVERWRITE = 0,
  EM_RECORDING = 1,
  EM_APPEND = 2,
  EM_WRITEFILE = 3,
  EM_NOTACTIVE = 4,
  EM_NUM_EXECMODES = 5
};

typedef int(__fastcall *CONSOLECOMMANDHANDLER)(const char *command, const char *arguments);

struct CONSOLECOMMAND : public TSHashObject<CONSOLECOMMAND, HASHKEY_CONSTSTRI> {
  CONSOLECOMMAND() : m_helpText(0) {
  }

  CONSOLECOMMANDHANDLER m_handler;
  const char           *m_helpText;
  CATEGORY              m_category;
};

extern EXECMODE                                       g_ExecCreateMode;
extern char                                           g_ExecBuffer[0x2000];
extern unsigned int                                   g_commandHistoryIndex;
extern char                                           g_commandHistory[32][80];
extern CONSOLECOMMANDHANDLER                          g_defaultCommand;
extern TSHashTable<CONSOLECOMMAND, HASHKEY_CONSTSTRI> g_consoleCommandHash;

int __fastcall  AddLineToExecFile(const char *currentLine);
void __fastcall AddToHistory(const char *command);

CONSOLECOMMAND *__fastcall ParseCommand(const char *commandLine, const char **command, const char **arguments);

void __fastcall         ConsoleCommandExecute(const char *commandLine, int addToHistory);
unsigned int __fastcall ConsoleCommandHistoryDepth();
const char *__fastcall  ConsoleCommandHistory(unsigned int offset);
int __fastcall          ConsoleCommandRegister(const char *command, CONSOLECOMMANDHANDLER handler, CATEGORY category, const char *helpText);
void __fastcall         ConsoleCommandUnregister(const char *command);
int __fastcall          ConsoleCommandComplete(const char *partial, const char **previous, int direction);
void __fastcall         ConsoleCommandWriteHelp(const char *cmd);
void __fastcall         ConsoleCommandRegisterDefault(CONSOLECOMMANDHANDLER handler);
void __fastcall         ConsoleCommandInitialize();
void __fastcall         ConsoleCommandDestroy();
