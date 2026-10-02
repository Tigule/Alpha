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

static char       cmd[32];
static const char whitespace[] = " ,;\t\"\r\n";
static const char verstr[] = "WoW [Release Assertions Enabled] Build 3368 (Dec 11 2003)";
static LPCSTR     NOHELPTEXT = "No help yet";
static char       s_fileName[MAX_PATH];

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

static BOOL ValidateFileName(LPCSTR arguments) {
  if (strstr(arguments, "..")) {
    ConsoleWrite("File Name cannot contain '\\' or '..'", ERROR_COLOR);
    return 0;
  }
  if (strstr(arguments, "\\")) {
    ConsoleWrite("File Name cannot contain '\\' or '..'", ERROR_COLOR);
    return 0;
  }

  LPCSTR extension = SStrChrR(arguments, '.');
  if (extension && SStrCmpI(extension, ".wtf", 0x7FFFFFFF)) {
    ConsoleWrite("Only '.wtf' extensions are allowed", ERROR_COLOR);
    return 0;
  }

  return 1;
}

static BOOL CreateWTFFilePath(char *filename, UINT size) {
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

static BOOL ConsoleCommand_Help(LPCSTR command, LPCSTR arguments) {
  UINT index;

  (void)command;

  if (!*arguments) {
    char buffer[128] = "";

    ConsoleWrite("Console help categories: ", DEFAULT_COLOR);
    for (index = 0; index < 8; ++index) {
      SStrPack(buffer, s_translation[index].categoryString, sizeof(buffer));
      if (index + 1 < 8) {
        SStrPack(buffer, ", ", sizeof(buffer));
      }
    }
    ConsoleWrite(buffer, WARNING_COLOR);
    ConsoleWrite("For more information type 'help [command] or [category]'", WARNING_COLOR);
    return 1;
  }

  CATEGORY category = NONE;
  for (index = 0; index < 8; ++index) {
    if (!SStrCmpI(s_translation[index].categoryString, arguments, 0x7FFFFFFF)) {
      category = s_translation[index].categoryValue;
      break;
    }
  }

  if (category != NONE) {
    char buffer[128] = "";

    SStrPrintf(buffer, sizeof(buffer), "Commands registered for the category %s:", arguments);
    ConsoleWrite(buffer, WARNING_COLOR);

    UINT counter = 0;
    buffer[0] = 0;
    for (CONSOLECOMMAND *entry = g_consoleCommandHash.Head(); (int)entry > 0; entry = g_consoleCommandHash.RawNext(entry)) {
      if (entry->m_category == category) {
        SStrPack(buffer, entry->GetString(), sizeof(buffer));
        SStrPack(buffer, ", ", sizeof(buffer));
        if (++counter == 8) {
          ConsoleWrite(buffer, DEFAULT_COLOR);
          buffer[0] = 0;
          counter = 0;
        }
      }
    }

    if (buffer[0]) {
      char *separator = SStrChrR(buffer, ',');
      if (separator) {
        *separator = 0;
      }
      ConsoleWrite(buffer, DEFAULT_COLOR);
    } else {
      ConsoleWrite("NONE", DEFAULT_COLOR);
    }
  }

  CONSOLECOMMAND *entry = g_consoleCommandHash.Ptr(arguments);
  if (entry) {
    char buffer[165];

    SStrPrintf(buffer, sizeof(buffer), "Help for command %s:", arguments);
    ConsoleWrite(buffer, WARNING_COLOR);
    LPCSTR helpText = entry->m_helpText;
    if (!helpText) {
      helpText = NOHELPTEXT;
    }
    SStrPrintf(buffer, sizeof(buffer), "     %s %s", arguments, helpText);
    ConsoleWrite(buffer, DEFAULT_COLOR);
  }

  return 1;
}

static BOOL ConsoleCommand_Quit(LPCSTR command, LPCSTR arguments) {
  (void)command;
  (void)arguments;
  ConsolePostClose();
  return 1;
}

static BOOL ConsoleCommand_Ver(LPCSTR command, LPCSTR arguments) {
  (void)command;
  (void)arguments;
  ConsoleWrite(verstr, DEFAULT_COLOR);
  return 1;
}

BOOL ConsoleCommand_RunExec(LPCSTR cmd, LPCSTR arguments) {
  DWORD  bytes;
  LPVOID readData;
  int    verbose = 0;
  char   lineBuffer[128];
  LPCSTR bufferPtr;
  char   filename[MAX_PATH];
  char   param1[32];

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
    char errorString[MAX_PATH];
    if (cmd) {
      SStrPrintf(errorString, sizeof(errorString), "Unable to load file %s", filename);
    } else {
      SStrPrintf(errorString, sizeof(errorString), "Unknown command: %s", arguments);
    }
    ConsoleWrite(errorString, ERROR_COLOR);
    return 0;
  }

  char *buffer = static_cast<char *>(ALLOC(bytes + 1));
  memcpy(buffer, readData, bytes);
  SFile::Unload(readData);
  if (!buffer) {
    return 0;
  }

  buffer[bytes] = 0;
  bufferPtr = buffer;
  do {
    SStrTokenize(&bufferPtr, lineBuffer, sizeof(lineBuffer), "\r\n", 0);
    if (lineBuffer[0]) {
      if (verbose) {
        char tmp[MAX_PATH];
        SStrPrintf(tmp, sizeof(tmp), "Executing ->%s", lineBuffer);
        ConsoleWrite(tmp, ECHO_COLOR);
      }
      ConsoleCommandExecute(lineBuffer, 0);
    }
  } while (bufferPtr && *bufferPtr);

  FREE(buffer);
  return 1;
}

static BOOL ConsoleCommand_CreateExec(LPCSTR cmd, LPCSTR arguments) {
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

static BOOL ConsoleCommand_AppendExec(LPCSTR cmd, LPCSTR arguments) {
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

static BOOL ConsoleCommand_CloseExec(LPCSTR cmd, LPCSTR arguments) {
  char       filePath[MAX_PATH];
  HOSFILE__ *file;
  DWORD      count;

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
      if (!count) {
        ConsoleWrite("Error Writing ExecFile", ERROR_COLOR);
      } else {
        ConsoleWrite("File written successfully", ECHO_COLOR);
      }
      OsCloseFile(file);
    }
  }

  g_ExecCreateMode = EM_NOTACTIVE;
  SStrCopy(g_ExecBuffer, "", 0x7FFFFFFF);
  return 1;
}

static BOOL ConsoleCommand_TypeExec(LPCSTR cmd, LPCSTR arguments) {
  DWORD  bytes;
  LPVOID readData;
  char   lineBuffer[128];
  LPCSTR bufferPtr;
  char   filePath[MAX_PATH];

  (void)cmd;

  if (!ValidateFileName(arguments)) {
    return 0;
  }

  SStrCopy(filePath, arguments, sizeof(filePath));
  if (!CreateWTFFilePath(filePath, sizeof(filePath))) {
    return 0;
  }

  if (!SFile::LoadFile(filePath, &readData, &bytes, 1, 0)) {
    char errorString[MAX_PATH];
    SStrPrintf(errorString, sizeof(errorString), "Unable to load file %s", filePath);
    ConsoleWrite(errorString, ERROR_COLOR);
    return 0;
  }

  char *buffer = static_cast<char *>(ALLOC(bytes + 1));
  memcpy(buffer, readData, bytes);
  SFile::Unload(readData);
  if (!buffer) {
    return 0;
  }

  buffer[bytes] = 0;
  bufferPtr = buffer;
  do {
    SStrTokenize(&bufferPtr, lineBuffer, sizeof(lineBuffer), "\r\n", 0);
    if (lineBuffer[0]) {
      ConsoleWrite(lineBuffer, DEFAULT_COLOR);
    }
  } while (bufferPtr && *bufferPtr);

  FREE(buffer);
  return 1;
}

static BOOL ConsoleCommand_DirWtf(LPCSTR cmd, LPCSTR arguments) {
  DWORD      bytes;
  LPCSTR     readBuffer;
  const char endOfLine[4] = " \r\n";
  LPVOID     readData;
  char       line[80];

  (void)cmd;
  (void)arguments;

  if (!SFile::LoadFile("wtfdir.txt", &readData, &bytes, 0, 0)) {
    ConsoleWrite("Unable to open wtfDir.txt", ERROR_COLOR);
    return 0;
  }

  ConsoleWrite("The wtf files are :", ECHO_COLOR);
  char *buffer = static_cast<char *>(readData);
  buffer[bytes - 1] = 0;
  readBuffer = buffer;
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

CONSOLECOMMAND *ParseCommand(LPCSTR commandLine, LPCSTR *command, LPCSTR *arguments) {
  LPCSTR args;

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

UINT ConsoleCommandHistoryDepth() {
  return 32;
}

LPCSTR ConsoleCommandHistory(UINT offset) {
  return g_commandHistory[(g_commandHistoryIndex - offset - 1) & 0x1F];
}

BOOL ConsoleCommandRegister(LPCSTR command, CONSOLECOMMANDHANDLER handler, CATEGORY category, LPCSTR helpText) {
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

void ConsoleCommandUnregister(LPCSTR command) {
  CONSOLECOMMAND *entry = g_consoleCommandHash.Ptr(command);

  if (!entry) {
    return;
  }

  g_consoleCommandHash.Delete(entry);
}

BOOL ConsoleCommandComplete(LPCSTR partial, LPCSTR *previous, int direction) {
  UINT            partialLength;
  CONSOLECOMMAND *entry;

  ASSERT(previous);

  if (!*previous) {
    entry = g_consoleCommandHash.Head();
  } else {
    entry = g_consoleCommandHash.Ptr(*previous);
    if (!entry) {
      return 0;
    }

    if (direction) {
      entry = g_consoleCommandHash.Prev(entry);
    } else {
      entry = g_consoleCommandHash.Next(entry);
    }
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

void ConsoleCommandWriteHelp(LPCSTR cmd) {
  ConsoleCommand_Help("help", cmd);
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
