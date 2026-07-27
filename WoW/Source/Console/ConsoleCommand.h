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

typedef int(*CONSOLECOMMANDHANDLER)(const char *command, const char *arguments);

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

int AddLineToExecFile(const char *currentLine);
void AddToHistory(const char *command);

CONSOLECOMMAND *ParseCommand(const char *commandLine, const char **command, const char **arguments);

void ConsoleCommandExecute(const char *commandLine, int addToHistory);
unsigned int ConsoleCommandHistoryDepth();
const char *ConsoleCommandHistory(unsigned int offset);
int ConsoleCommandRegister(const char *command, CONSOLECOMMANDHANDLER handler, CATEGORY category, const char *helpText);
void ConsoleCommandUnregister(const char *command);
int ConsoleCommandComplete(const char *partial, const char **previous, int direction);
void ConsoleCommandWriteHelp(const char *cmd);
void ConsoleCommandRegisterDefault(CONSOLECOMMANDHANDLER handler);
void ConsoleCommandInitialize();
void ConsoleCommandDestroy();
