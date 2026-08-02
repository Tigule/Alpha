#include <Base/Base.h>

#include "ConsoleClient.h"

#include <Os/W32/OsFile.h>
#include <storm.h>
#include <stpl.h>

#include <ctype.h>
#include <new>
#include <stdio.h>
#include <string.h>
#include <windows.h>

TSHashTable<CONSOLECOMMAND, HASHKEY_CONSTSTRI> g_consoleCommandHash;
CONSOLECOMMANDHANDLER                          g_defaultCommand = 0;

static char        cmd[32];
static const char  whitespace[] = " ,;\t\"\r\n";
static const char  verstr[] = "WoW [Release Assertions Enabled] Build 3368 (Dec 11 2003)";
static const char *NOHELPTEXT = "No help yet";
static char        s_fileName[MAX_PATH];

struct CategoryTranslation {
  CATEGORY categoryValue;
  char     categoryString[20];
};

static CategoryTranslation s_translation[8] = {
    {   DEBUG,    "debug"},
    {GRAPHICS, "graphics"},
    { CONSOLE,  "console"},
    {  COMBAT,   "combat"},
    {    GAME,     "game"},
    { DEFAULT,  "default"},
    {     NET,      "net"},
    {   SOUND,    "sound"}
};

static int ValidateFileName(const char *arguments) {
  const char *extension;

  if (strstr(arguments, "..") || strstr(arguments, "\\")) {
    ConsoleWrite("File Name cannot contain '\\' or '..'", ERROR_COLOR);
    return 0;
  }

  extension = SStrChrR(arguments, '.');
  if (extension && SStrCmpI(extension, ".wtf", 0x7FFFFFFF)) {
    ConsoleWrite("Only '.wtf' extensions are allowed", ERROR_COLOR);
    return 0;
  }

  return 1;
}

static int CreateWTFFilePath(char *filename, unsigned int size) {
  char  buffer[MAX_PATH] = "";
  char *extension;

  SStrPack(buffer, "WTF\\", sizeof(buffer));
  SStrPack(buffer, filename, sizeof(buffer));
  extension = SStrChrR(filename, '.');
  if (extension) {
    if (SStrCmpI(extension, ".wtf", 0x7FFFFFFF)) {
      ConsoleWrite("File must have '.wtf' extension", ERROR_COLOR);
      return 0;
    }
  } else {
    SStrPack(buffer, ".wtf", sizeof(buffer));
  }

  SStrCopy(filename, buffer, size);
  return 1;
}

static int ConsoleCommand_Help(const char *command, const char *arguments) {
  unsigned int    index;
  unsigned int    categoryCount;
  CONSOLECOMMAND *entry;
  const char     *helpText;
  char           *separator;

  (void)command;

  if (!*arguments) {
    char buffer[128];

    buffer[0] = 0;
    ConsoleWrite("Console help categories: ", DEFAULT_COLOR);
    for (index = 0; index < 8; ++index) {
      SStrPack(buffer, s_translation[index].categoryString, 128);
      if (index + 1 < 8) {
        SStrPack(buffer, ", ", 128);
      }
    }
    ConsoleWrite(buffer, WARNING_COLOR);
    ConsoleWrite("For more information type 'help [command] or [category]'", WARNING_COLOR);
    return 1;
  }

  for (index = 0; index < 8; ++index) {
    if (!SStrCmpI(s_translation[index].categoryString, arguments, 0x7FFFFFFF)) {
      if (s_translation[index].categoryValue != NONE) {
        char buffer[128];

        buffer[0] = 0;
        SStrPrintf(buffer, 128, "Commands registered for the category %s:", arguments);
        ConsoleWrite(buffer, WARNING_COLOR);

        categoryCount = 0;
        buffer[0] = 0;
        ITERATELIST(CONSOLECOMMAND, g_consoleCommandHash, categoryEntry) {
          if (categoryEntry->m_category == s_translation[index].categoryValue) {
            SStrPack(buffer, categoryEntry->GetString(), 128);
            SStrPack(buffer, ", ", 128);
            ++categoryCount;
            if (categoryCount == 8) {
              ConsoleWrite(buffer, DEFAULT_COLOR);
              buffer[0] = 0;
              categoryCount = 0;
            }
          }
        }

        if (buffer[0]) {
          separator = SStrChrR(buffer, ',');
          if (separator) {
            *separator = 0;
          }
          ConsoleWrite(buffer, DEFAULT_COLOR);
        } else {
          ConsoleWrite("NONE", DEFAULT_COLOR);
        }
      }
      break;
    }
  }

  entry = g_consoleCommandHash.Ptr(arguments);
  if (entry) {
    char buffer[165];

    SStrPrintf(buffer, sizeof(buffer), "Help for command %s:", arguments);
    ConsoleWrite(buffer, WARNING_COLOR);
    helpText = entry->m_helpText;
    if (!helpText) {
      helpText = NOHELPTEXT;
    }
    SStrPrintf(buffer, sizeof(buffer), "     %s %s", arguments, helpText);
    ConsoleWrite(buffer, DEFAULT_COLOR);
  }

  return 1;
}

static int ConsoleCommand_Quit(const char *command, const char *arguments) {
  (void)command;
  (void)arguments;
  ConsolePostClose();
  return 1;
}

static int ConsoleCommand_Ver(const char *command, const char *arguments) {
  (void)command;
  (void)arguments;
  ConsoleWrite(verstr, DEFAULT_COLOR);
  return 1;
}

int ConsoleCommand_RunExec(const char *cmd, const char *arguments) {
  char          filename[MAX_PATH];
  char          errorString[MAX_PATH];
  char          tmp[MAX_PATH];
  char          lineBuffer[128];
  char          param1[32];
  void         *readData;
  int           verbose = 0;
  const char   *bufferPtr;
  unsigned long bytes;

  if (sscanf(arguments, "%s %s", filename, param1) < 1) {
    ConsoleWrite("Invalid number of parameters", ERROR_COLOR);
    ConsoleCommandWriteHelp(cmd);
    return 0;
  }

  if (!SStrCmpI(param1, "verbose", 0x7FFFFFFF)) {
    verbose = 1;
  }

  if (!CreateWTFFilePath(filename, sizeof(filename))) {
    return 0;
  }

  if (!SFile::LoadFile(filename, &readData, &bytes, 1, 0)) {
    if (cmd) {
      SStrPrintf(errorString, sizeof(errorString), "Unable to load file %s", filename);
    } else {
      SStrPrintf(errorString, sizeof(errorString), "Unknown command: %s", arguments);
    }
    ConsoleWrite(errorString, ERROR_COLOR);
    return 0;
  }

  bufferPtr = static_cast<const char *>(readData);
  readData = ALLOC(bytes + 1);
  if (!readData) {
    SFile::Unload(const_cast<char *>(bufferPtr));
    return 0;
  }

  memcpy(readData, bufferPtr, bytes);
  SFile::Unload(const_cast<char *>(bufferPtr));
  static_cast<char *>(readData)[bytes] = 0;
  bufferPtr = static_cast<const char *>(readData);
  do {
    SStrTokenize(&bufferPtr, lineBuffer, sizeof(lineBuffer), "\r\n", 0);
    if (lineBuffer[0]) {
      if (verbose) {
        SStrPrintf(tmp, sizeof(tmp), "Executing ->%s", lineBuffer);
        ConsoleWrite(tmp, ECHO_COLOR);
      }
      ConsoleCommandExecute(lineBuffer, 0);
    }
  } while (bufferPtr && *bufferPtr);

  FREE(readData);
  return 1;
}

static int ConsoleCommand_CreateExec(const char *cmd, const char *arguments) {
  char  folder[MAX_PATH];
  char  filePath[MAX_PATH];
  char *lastSlash;

  (void)cmd;

  if (!ValidateFileName(arguments)) {
    return 0;
  }

  SStrCopy(s_fileName, arguments, sizeof(s_fileName));
  SStrCopy(filePath, s_fileName, sizeof(filePath));
  if (!CreateWTFFilePath(filePath, sizeof(filePath))) {
    return 0;
  }

  if (SFile::FileExists(filePath)) {
    ConsoleWrite("File exists are you sure you want to Overwrite Y/N ?", WARNING_COLOR);
    g_ExecCreateMode = EM_PROMPTOVERWRITE;
    return 0;
  }

  SStrCopy(folder, filePath, 0x7FFFFFFF);
  lastSlash = SStrChrR(folder, '\\');
  if (lastSlash) {
    *lastSlash = 0;
  }

  if (!OsDirectoryExists(folder)) {
    ConsoleWrite("Error! WTF folder does not exist.", ERROR_COLOR);
    g_ExecCreateMode = EM_NOTACTIVE;
    return 0;
  }

  g_ExecCreateMode = EM_RECORDING;
  ConsoleWrite("Begin Typing the commands", ECHO_COLOR);
  return 1;
}

static int ConsoleCommand_AppendExec(const char *cmd, const char *arguments) {
  char errorString[MAX_PATH];
  char filePath[MAX_PATH];

  (void)cmd;

  if (!ValidateFileName(arguments)) {
    return 0;
  }

  SStrCopy(s_fileName, arguments, sizeof(s_fileName));
  SStrCopy(filePath, s_fileName, sizeof(filePath));
  if (!CreateWTFFilePath(filePath, sizeof(filePath))) {
    return 0;
  }

  if (!SFile::FileExists(filePath)) {
    SStrPrintf(errorString, sizeof(errorString), "Unable to find file %s", filePath);
    ConsoleWrite(errorString, ERROR_COLOR);
    g_ExecCreateMode = EM_NOTACTIVE;
    return 0;
  }

  g_ExecCreateMode = EM_APPEND;
  ConsoleWrite("Begin Typing the commands", ECHO_COLOR);
  return 1;
}

static int ConsoleCommand_CloseExec(const char *cmd, const char *arguments) {
  char          filePath[MAX_PATH];
  HOSFILE__    *file;
  unsigned long count;

  (void)cmd;
  (void)arguments;

  if (g_ExecCreateMode < EM_APPEND || g_ExecCreateMode > EM_WRITEFILE) {
    ConsoleWrite("You must type 'new' 'filename' to begin creating an Exec file", WARNING_COLOR);
    return 1;
  }

  SStrCopy(filePath, s_fileName, sizeof(filePath));
  if (CreateWTFFilePath(filePath, sizeof(filePath))) {
    file = OsCreateFile(filePath, 0x40000000, 0, 2, 0x80, 0x3F3F3F3F);
    if (file == (HOSFILE__ *)-1) {
      ConsoleWrite("Error trying to create the file.", ERROR_COLOR);
    } else {
      if (g_ExecCreateMode == EM_APPEND) {
        OsSetFilePointer(file, 0, 2);
      }

      count = 0;
      OsWriteFile(file, g_ExecBuffer, SStrLen(g_ExecBuffer), &count);
      if (count) {
        ConsoleWrite("File written successfully", ECHO_COLOR);
      } else {
        ConsoleWrite("Error Writing ExecFile", ERROR_COLOR);
      }
      OsCloseFile(file);
    }
  }

  g_ExecCreateMode = EM_NOTACTIVE;
  SStrCopy(g_ExecBuffer, "", 0x7FFFFFFF);
  return 1;
}

static int ConsoleCommand_TypeExec(const char *cmd, const char *arguments) {
  char          errorString[MAX_PATH];
  char          filePath[MAX_PATH];
  char          lineBuffer[128];
  void         *readData;
  const char   *bufferPtr;
  unsigned long bytes;

  (void)cmd;

  if (!ValidateFileName(arguments)) {
    return 0;
  }

  SStrCopy(filePath, arguments, sizeof(filePath));
  if (!CreateWTFFilePath(filePath, sizeof(filePath))) {
    return 0;
  }

  if (!SFile::LoadFile(filePath, &readData, &bytes, 1, 0)) {
    SStrPrintf(errorString, sizeof(errorString), "Unable to load file %s", filePath);
    ConsoleWrite(errorString, ERROR_COLOR);
    return 0;
  }

  bufferPtr = static_cast<const char *>(readData);
  readData = ALLOC(bytes + 1);
  if (!readData) {
    SFile::Unload(const_cast<char *>(bufferPtr));
    return 0;
  }

  memcpy(readData, bufferPtr, bytes);
  SFile::Unload(const_cast<char *>(bufferPtr));
  static_cast<char *>(readData)[bytes] = 0;
  bufferPtr = static_cast<const char *>(readData);
  do {
    SStrTokenize(&bufferPtr, lineBuffer, sizeof(lineBuffer), "\r\n", 0);
    if (lineBuffer[0]) {
      ConsoleWrite(lineBuffer, DEFAULT_COLOR);
    }
  } while (bufferPtr && *bufferPtr);

  FREE(readData);
  return 1;
}

static int ConsoleCommand_DirWtf(const char *cmd, const char *arguments) {
  char          line[80];
  const char   *readBuffer;
  char          endOfLine[4] = " \r\n";
  unsigned long bytes;
  void         *readData;

  (void)cmd;
  (void)arguments;

  if (!SFile::LoadFile("wtfdir.txt", &readData, &bytes, 0, 0)) {
    ConsoleWrite("Unable to open wtfDir.txt", ERROR_COLOR);
    return 0;
  }

  ConsoleWrite("The wtf files are :", ECHO_COLOR);
  ((char *)readData)[bytes - 1] = 0;
  readBuffer = (const char *)readData;
  do {
    SStrTokenize(&readBuffer, line, sizeof(line), endOfLine, 0);
    if (!line[0]) {
      break;
    }
    ConsoleWrite(line, ECHO_COLOR);
  } while (line[0]);

  ConsoleWrite("[end]", ECHO_COLOR);
  SFile::Unload(readData);
  return 1;
}

CONSOLECOMMAND *ParseCommand(const char *commandLine, const char **command, const char **arguments) {
  const char *args;

  ASSERT(commandLine);

  args = commandLine;
  SStrTokenize(&args, cmd, sizeof(cmd), whitespace, 0);
  if (command) {
    *command = cmd;
  }

  if (arguments) {
    while (isspace(*args)) {
      ++args;
    }
    *arguments = args;
  }

  return g_consoleCommandHash.Ptr(cmd);
}

unsigned int ConsoleCommandHistoryDepth() {
  return 32;
}

const char *ConsoleCommandHistory(unsigned int offset) {
  return g_commandHistory[(g_commandHistoryIndex - offset - 1) & 0x1F];
}

int ConsoleCommandRegister(const char *command, CONSOLECOMMANDHANDLER handler, CATEGORY category, const char *helpText) {
  CONSOLECOMMAND *entry;

  ASSERT(command);
  ASSERT(handler);

  if (SStrLen(command) >= sizeof(cmd)) {
    return 0;
  }

  if (g_consoleCommandHash.Ptr(command)) {
    return 0;
  }

  entry = g_consoleCommandHash.New(command, 0, 0);
  entry->m_category = category;
  entry->m_handler = handler;
  entry->m_helpText = helpText;
  return 1;
}

void ConsoleCommandUnregister(const char *command) {
  CONSOLECOMMAND *entry = g_consoleCommandHash.Ptr(command);

  if (!entry) {
    return;
  }

  g_consoleCommandHash.Delete(entry);
}

int ConsoleCommandComplete(const char *partial, const char **previous, int direction) {
  unsigned int    partialLength;
  CONSOLECOMMAND *entry;

  ASSERT(previous);

  if (*previous) {
    entry = g_consoleCommandHash.Ptr(*previous);
    if (!entry) {
      return 0;
    }

    if (direction) {
      entry = g_consoleCommandHash.Prev(entry);
    } else {
      entry = g_consoleCommandHash.Next(entry);
    }
  } else {
    entry = g_consoleCommandHash.Head();
  }

  partialLength = SStrLen(partial);
  while (entry) {
    if (!SStrCmpI(partial, entry->GetString(), partialLength)) {
      *previous = entry->GetString();
      return 1;
    }

    if (direction) {
      entry = g_consoleCommandHash.Prev(entry);
    } else {
      entry = g_consoleCommandHash.Next(entry);
    }
  }

  return 0;
}

void ConsoleCommandWriteHelp(const char *cmd) {
  ConsoleCommand_Help(cmd, "help");
}

void ConsoleCommandRegisterDefault(CONSOLECOMMANDHANDLER handler) {
  g_defaultCommand = handler;
}

void ConsoleCommandInitialize() {
  ConsoleCommandRegister("help", ConsoleCommand_Help, DEFAULT, 0);
  ConsoleCommandRegister("quit", ConsoleCommand_Quit, DEFAULT, 0);
  ConsoleCommandRegister("ver", ConsoleCommand_Ver, DEFAULT, 0);
  ConsoleCommandRegister("run", ConsoleCommand_RunExec, CONSOLE, "[File Name] Runs a wtf file from the WTF folder");
  ConsoleCommandRegister("new", ConsoleCommand_CreateExec, CONSOLE, "[File Name] starts recording a new script");
  ConsoleCommandRegister("append", ConsoleCommand_AppendExec, CONSOLE, "[File Name] adds command to the end of an existing file");
  ConsoleCommandRegister("end", ConsoleCommand_CloseExec, CONSOLE, "Stops recording script");
  ConsoleCommandRegister("type", ConsoleCommand_TypeExec, CONSOLE, "[File Name] types the script to the console");
  ConsoleCommandRegister("dirwtf", ConsoleCommand_DirWtf, CONSOLE, "Lists the WTF files");
}

void ConsoleCommandDestroy() {
  g_consoleCommandHash.Clear();
}
