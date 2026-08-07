#include <Base/Base.h>

#include "ConsoleVar.h"
#include "ConsoleClient.h"

#include <Os/W32/OsFile.h>
#include <new>
#include <storm.h>

static char const whitespace[8] = " ,;\t\"\r\n";

typedef TSHashTable<CVar, HASHKEY_STRI> CVarHashTable;

static CVarHashTable s_registeredCVars;
static LPCSTR        s_filename;
static BOOL          s_CreatePathDirectories(LPCSTR szPath) {
  char   dwPartialPath[MAX_PATH];
  int    success = 1;
  LPCSTR separator;

  if (!szPath || !szPath[0]) {
    return 0;
  }

  separator = SStrChr(szPath + 1, '\\');
  while (separator) {
    UINT chars = static_cast<UINT>(separator - szPath);
    if (chars >= MAX_PATH) {
      success = 0;
      break;
    }

    memcpy(dwPartialPath, szPath, chars);
    dwPartialPath[chars] = 0;
    if (!OsCreateDirectory(dwPartialPath, 0)) {
      UINT attributes = OsGetFileAttributes(dwPartialPath);
      if (attributes == 0xFFFFFFFF || !(attributes & 0x10)) {
        success = 0;
        break;
      }
    }

    separator = SStrChr(separator + 1, '\\');
  }

  return success;
}

CVar::CVar() {
}

CVar::~CVar() {
  ConsoleCommandUnregister(m_name);

  FREEIFUSED(m_stringValue);
  FREEIFUSED(m_defaultValue);
  FREEIFUSED(m_resetValue);
  FREEIFUSED(m_latchedValue);
}

static BOOL CvarCommandHandler(LPCSTR command, LPCSTR arguments) {
  CVar *cvar = CVar::Lookup(command);
  ASSERT(cvar);

  while (*arguments == ' ') {
    ++arguments;
  }

  if (*arguments) {
    cvar->Set(arguments, true, true, false);
  } else {
    ConsolePrintf("CVar \"%s\" is \"%s\"", command, cvar->GetString());
  }

  return 1;
}

static BOOL SetCommandHandler(LPCSTR command, LPCSTR arguments) {
  char cvarValue[256];
  char cvarName[32];

  SStrTokenize(&arguments, cvarName, sizeof(cvarName), whitespace, 0);
  SStrTokenize(&arguments, cvarValue, sizeof(cvarValue), whitespace, 0);

  CVar *cvar = CVar::Lookup(cvarName);
  if (cvar) {
    cvar->Set(cvarValue, true, false, false);
  } else {
    CVar::Register(cvarName, "", 0, cvarValue, 0, DEFAULT, true, 0);
  }

  return 1;
}

static BOOL CvarResetCommandHandler(LPCSTR command, LPCSTR arguments) {
  char cvarName[32];

  SStrTokenize(&arguments, cvarName, sizeof(cvarName), whitespace, 0);

  if (cvarName[0]) {
    CVar *cvar = CVar::Lookup(cvarName);
    if (cvar) {
      cvar->Reset();
    } else {
      ConsoleWriteA("No such cvar \"%s\"\n", ERROR_COLOR, cvarName);
    }
    return 1;
  }

  ConsoleWrite("Resetting all cvars\n", DEFAULT_COLOR);
  ITERATELIST(CVar, s_registeredCVars, cvar) {
    cvar->Reset();
  }

  return 1;
}

static BOOL CvarDefaultCommandHandler(LPCSTR command, LPCSTR arguments) {
  char cvarName[32];

  SStrTokenize(&arguments, cvarName, sizeof(cvarName), whitespace, 0);

  if (cvarName[0]) {
    CVar *cvar = CVar::Lookup(cvarName);
    if (cvar) {
      cvar->Default();
    } else {
      ConsoleWriteA("No such cvar \"%s\"\n", ERROR_COLOR, cvarName);
    }
    return 1;
  }

  ConsoleWrite("Restoring all cvars\n", DEFAULT_COLOR);
  ITERATELIST(CVar, s_registeredCVars, cvar) {
    cvar->Default();
  }

  return 1;
}

static BOOL CvarListCommandHandler(LPCSTR command, LPCSTR arguments) {
  char text[256];
  char text2[256];
  ITERATELIST(CVar, s_registeredCVars, cvar) {
    SStrPrintf(text, sizeof(text), "  \"%s\" is \"%s\"", cvar->GetName(), cvar->GetString());

    if (cvar->GetDefaultValue() && SStrCmp(cvar->GetString(), cvar->GetDefaultValue(), 0x7FFFFFFF)) {
      SStrPrintf(text2, sizeof(text2), " (default \"%s\")", cvar->GetDefaultValue());
      SStrPack(text, text2, sizeof(text));
    }

    if (cvar->GetResetValue() && SStrCmp(cvar->GetString(), cvar->GetResetValue(), 0x7FFFFFFF)) {
      SStrPrintf(text2, sizeof(text2), " (reset \"%s\")", cvar->GetResetValue());
      SStrPack(text, text2, sizeof(text));
    }

    ConsoleWrite(text, DEFAULT_COLOR);
  }

  return 1;
}

static int CVarLoadFile() {
  char command[MAX_PATH];
  SStrPrintf(command, sizeof(command), "run %s", s_filename);
  ConsoleCommandExecute(command, 1);
  return 1;
}

static BOOL CVarSaveFile() {
  char  buffer[MAX_PATH];
  char  fileName[MAX_PATH];
  DWORD count;

  SStrCopy(fileName, "WTF\\", sizeof(fileName));
  SStrPack(fileName, s_filename, sizeof(fileName));
  HOSFILE file = OsCreateFile(fileName, 0x40000000, 0, 2, 0x80, 0x3F3F3F3F);
  if (file == reinterpret_cast<HOSFILE>(-1)) {
    return 0;
  }

  ITERATELIST(CVar, s_registeredCVars, cvar) {
    if (cvar->IsArchived()) {
      SStrPrintf(buffer, sizeof(buffer), "SET %s \"%s\"\n", cvar->GetName(), cvar->GetString());
      count = 0;
      OsWriteFile(file, buffer, SStrLen(buffer), &count);
      if (!count) {
        OsCloseFile(file);
        return 0;
      }
    }
  }

  OsCloseFile(file);
  return 1;
}

void CVar::Initialize(LPCSTR filename) {
  char path[MAX_PATH];

  ASSERT(filename);

  s_filename = filename;
  SFile::GetBasePath(path, MAX_PATH);
  SStrPrintf(path, MAX_PATH, "%s%s\\", path, "WTF");
  s_CreatePathDirectories(path);

  ConsoleCommandRegister("set", SetCommandHandler, DEFAULT, "Set the value of a CVar");
  ConsoleCommandRegister("cvar_reset", CvarResetCommandHandler, DEFAULT, "Set the value of a CVar to it's startup value");
  ConsoleCommandRegister("cvar_default", CvarDefaultCommandHandler, DEFAULT, "Set the value of a CVar to it's coded default value");
  ConsoleCommandRegister("cvarlist", CvarListCommandHandler, DEFAULT, "List cvars");
  CVarLoadFile();
}

void CVar::Destroy() {
  CVarSaveFile();
  ConsoleCommandUnregister("set");
  ConsoleCommandUnregister("cvar_reset");
  ConsoleCommandUnregister("cvar_default");
  ConsoleCommandUnregister("cvarlist");
  s_registeredCVars.Clear();
}

CVar *CVar::Register(LPCSTR name, LPCSTR help, UINT flags, LPCSTR value, CVar::CVARCALLBACKFCN fcn, UINT category, bool setCommand, LPVOID arg) {
  ASSERT(name);
  ASSERT(value);

  CVar *cvar = s_registeredCVars.Ptr(name);
  if (cvar) {
    ASSERT(!setCommand);

    bool setReset = cvar->m_resetValue == 0;
    bool setDefault = cvar->m_defaultValue == 0;
    cvar->m_flags |= flags;
    cvar->m_callback = fcn;
    cvar->m_arg = arg;
    if (fcn) {
      fcn(cvar, cvar->m_stringValue, cvar->m_stringValue, arg);
    }
    cvar->Set(value, false, setReset, setDefault);
    return cvar;
  }

  cvar = s_registeredCVars.New(name, 0, 0);

  cvar->m_stringValue = 0;
  cvar->m_floatValue = 0.0f;
  cvar->m_intValue = 0;
  cvar->m_modified = 0;
  SStrCopy(cvar->m_name, name, sizeof(cvar->m_name));
  cvar->m_callback = fcn;
  cvar->m_arg = arg;
  cvar->m_category = category;
  cvar->m_defaultValue = 0;
  cvar->m_resetValue = 0;
  cvar->m_latchedValue = 0;
  cvar->m_flags = 0;

  if (setCommand) {
    cvar->Set(value, true, true, false);
  } else {
    cvar->Set(value, true, false, true);
  }

  cvar->m_flags = flags | 1;
  ConsoleCommandRegister(cvar->m_name, CvarCommandHandler, static_cast<CATEGORY>(category), help);
  return cvar;
}

CVar *CVar::Lookup(LPCSTR name) {
  return s_registeredCVars.Ptr(name);
}

bool CVar::Set(LPCSTR value, bool setValue, bool setReset, bool setDefault) {
  ASSERT(value);

  if (setValue) {
    if (m_callback && !m_callback(this, m_stringValue, value, m_arg)) {
      return true;
    }

    ++m_modified;
    if (m_flags & 2) {
      FREEIFUSED(m_latchedValue);
      m_latchedValue = SStrDupA(value, __FILE__, __LINE__);
      return true;
    }
  }

  InternalSet(value, setValue, setReset, setDefault);
  return true;
}

void CVar::Reset() {
  LPCSTR value = m_resetValue ? m_resetValue : m_defaultValue;
  if (value) {
    InternalSet(value, true, false, false);
  }
}

void CVar::Default() {
  LPCSTR value = m_defaultValue ? m_defaultValue : m_resetValue;
  if (value) {
    InternalSet(value, true, false, false);
  }
}

bool CVar::Update() {
  if (!(m_flags & 2) || !m_latchedValue) {
    return false;
  }

  InternalSet(m_latchedValue, true, false, false);
  FREE(m_latchedValue);
  m_latchedValue = 0;
  return true;
}

void CVar::InternalSet(LPCSTR value, bool setValue, bool setReset, bool setDefault) {
  if (setValue) {
    FREEIFUSED(m_stringValue);
    m_stringValue = SStrDupA(value, __FILE__, __LINE__);
    m_intValue = SStrToInt(value);
    m_floatValue = SStrToFloat(value);
  }

  if (setReset && !m_resetValue) {
    m_resetValue = SStrDupA(value, __FILE__, __LINE__);
  }
  if (setDefault && !m_defaultValue) {
    m_defaultValue = SStrDupA(value, __FILE__, __LINE__);
  }
}
