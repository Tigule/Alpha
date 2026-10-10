#include <storm.h>
#include <SAPIEXTEND.H>
#include <ctype.h>
#include <imagehlp.h>

enum {
  LOG_DEFAULT_OPTIONS_ONLY = 0,
  LOG_DEFAULT_OPTIONS = 1,
  LOG_CUSTOM_TITLE = 2,
  LOG_EXE_NAME = 4,
  LOG_DATE_TIME = 8,
  LOG_COMPUTER_NAME = 16,
  LOG_USER_NAME = 32,
  LOG_REGISTERS = 65536,
  LOG_DBGHELP_STACK_TRACE = 131072,
  LOG_DBGHELP_STACK_TRACE_PARAMETERS = 262144,
  LOG_MANUAL_STACK_TRACE = 524288,
  LOG_CODE_MEMORY = 1048576,
  LOG_STACK_MEMORY = 2097152,
  LOG_LOADED_MODULES = 4194304,
  LOG_LOADED_MODULE_SYMBOLS = 8388608,
  LOG_VERBOSE = 0x80000000
};

typedef BOOL(WINAPI *PFN_SYMGETMODULEINFO)(HANDLE process, DWORD address, PIMAGEHLP_MODULE moduleInfo);
typedef BOOL(WINAPI *PFN_SYMENUMERATESYMBOLS)(HANDLE process, DWORD baseOfDll, PSYM_ENUMSYMBOLS_CALLBACK callback, PVOID userContext);
typedef BOOL(WINAPI *PFN_SYMGETSYMFROMADDR)(HANDLE process, DWORD address, PDWORD displacement, PIMAGEHLP_SYMBOL symbol);
typedef BOOL(WINAPI *PFN_SYMGETLINEFROMADDR)(HANDLE process, DWORD address, PDWORD displacement, PIMAGEHLP_LINE line);
typedef DWORD(WINAPI *PFN_SYMGETOPTIONS)(void);
typedef DWORD(WINAPI *PFN_SYMSETOPTIONS)(DWORD options);
typedef BOOL(WINAPI *PFN_SYMINITIALIZE)(HANDLE process, LPSTR searchPath, BOOL invadeProcess);
typedef BOOL(WINAPI *PFN_SYMCLEANUP)(HANDLE process);
typedef BOOL(WINAPI *PFN_SYMENUMERATEMODULES)(HANDLE process, PSYM_ENUMMODULES_CALLBACK callback, PVOID userContext);
typedef BOOL(WINAPI *PFN_READPROCESSMEMORY)(HANDLE process, DWORD address, PVOID buffer, DWORD size, PDWORD bytesRead);
typedef BOOL(WINAPI *PFN_STACKWALK)(
    DWORD                          machineType,
    HANDLE                         process,
    HANDLE                         thread,
    LPSTACKFRAME                   stackFrame,
    LPVOID                         contextRecord,
    PFN_READPROCESSMEMORY          readMemoryRoutine,
    PFUNCTION_TABLE_ACCESS_ROUTINE functionTableAccessRoutine,
    PGET_MODULE_BASE_ROUTINE       getModuleBaseRoutine,
    PTRANSLATE_ADDRESS_ROUTINE     translateAddress
);
typedef LPVOID(WINAPI *PFN_SYMFUNCTIONTABLEACCESS)(HANDLE process, DWORD addressBase);
typedef DWORD(WINAPI *PFN_SYMGETMODULEBASE)(HANDLE process, DWORD returnAddress);

typedef BOOL(WINAPI *PFN_MINIDUMPWRITEDUMP)(
    HANDLE                                  process,
    DWORD                                   processId,
    HANDLE                                  file,
    MINIDUMP_TYPE                           dumpType,
    MINIDUMP_EXCEPTION_INFORMATION *const   exceptionInfo,
    MINIDUMP_USER_STREAM_INFORMATION *const userStreamInfo,
    MINIDUMP_CALLBACK_INFORMATION *const    callbackInfo
);

static int s_StackInit;

class CDbgHelpDll {
 private:
  HINSTANCE hInstance;
  DWORD     loadCount;

 public:
  PFN_STACKWALK              StackWalk;
  PFN_SYMFUNCTIONTABLEACCESS SymFunctionTableAccess;
  PFN_SYMGETLINEFROMADDR     SymGetLineFromAddr;
  PFN_SYMGETMODULEBASE       SymGetModuleBase;
  PFN_SYMGETMODULEINFO       SymGetModuleInfo;
  PFN_SYMGETOPTIONS          SymGetOptions;
  PFN_SYMGETSYMFROMADDR      SymGetSymFromAddr;
  PFN_SYMINITIALIZE          SymInitialize;
  PFN_SYMCLEANUP             SymCleanup;
  PFN_SYMSETOPTIONS          SymSetOptions;
  PFN_SYMENUMERATEMODULES    SymEnumerateModules;
  PFN_SYMENUMERATESYMBOLS    SymEnumerateSymbols;
  PFN_MINIDUMPWRITEDUMP      MiniDumpWriteDump;

  int  Load();
  void Unload();
  BOOL IsLoaded() {
    return hInstance != NULL;
  }
  CDbgHelpDll();
  ~CDbgHelpDll();
};

static CRITICAL_SECTION s_CrawlCritsect;

static CDbgHelpDll sgDbgHelpDll;
static LONG        sgRecursionLevel;
static char        sgMonthString[12][4] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

extern BOOL g_memFullError;

struct MEMDUMP {
  LOGMACHINESTATEPROC logLineProc;
  LPVOID              logLineProcParam;
};

struct MiniDumpParam {
  HANDLE              logfile;
  EXCEPTION_POINTERS *exceptionPointers;
  DWORD               threadId;
  int                 result;
  UINT                userStringCount;
  char              **userStrings;
};

struct LogLineParams {
  LOGMACHINESTATEPROC logLineProc;
  LPVOID              logLineProcParam;
};

struct ModuleData {
  char  name[0x100];
  DWORD baseAddress;
};

struct EnumModuleData {
  UINT       count;
  ModuleData modules[0x100];
};

CDbgHelpDll::CDbgHelpDll() {
  hInstance = NULL;
  loadCount = 0;
}
CDbgHelpDll::~CDbgHelpDll() {
  Unload();
}
int CDbgHelpDll::Load() {
  HMODULE module;

  ++loadCount;
  if (hInstance) {
    return TRUE;
  }

  InitializeCriticalSection(&s_CrawlCritsect);
  EnterCriticalSection(&s_CrawlCritsect);
  module = LoadLibraryA("dbghelp.dll");
  if (!module) {
    goto load_failed;
  }

  StackWalk = (PFN_STACKWALK)GetProcAddress(module, "StackWalk");
  if (!StackWalk)
    goto cleanup;
  SymFunctionTableAccess = (PFN_SYMFUNCTIONTABLEACCESS)GetProcAddress(module, "SymFunctionTableAccess");
  if (!SymFunctionTableAccess)
    goto cleanup;
  SymGetLineFromAddr = (PFN_SYMGETLINEFROMADDR)GetProcAddress(module, "SymGetLineFromAddr");
  if (!SymGetLineFromAddr)
    goto cleanup;
  SymGetModuleBase = (PFN_SYMGETMODULEBASE)GetProcAddress(module, "SymGetModuleBase");
  if (!SymGetModuleBase)
    goto cleanup;
  SymGetModuleInfo = (PFN_SYMGETMODULEINFO)GetProcAddress(module, "SymGetModuleInfo");
  if (!SymGetModuleInfo)
    goto cleanup;
  SymGetOptions = (PFN_SYMGETOPTIONS)GetProcAddress(module, "SymGetOptions");
  if (!SymGetOptions)
    goto cleanup;
  SymGetSymFromAddr = (PFN_SYMGETSYMFROMADDR)GetProcAddress(module, "SymGetSymFromAddr");
  if (!SymGetSymFromAddr)
    goto cleanup;
  SymInitialize = (PFN_SYMINITIALIZE)GetProcAddress(module, "SymInitialize");
  if (!SymInitialize)
    goto cleanup;
  SymCleanup = (PFN_SYMCLEANUP)GetProcAddress(module, "SymCleanup");
  if (!SymCleanup)
    goto cleanup;
  SymSetOptions = (PFN_SYMSETOPTIONS)GetProcAddress(module, "SymSetOptions");
  if (!SymSetOptions)
    goto cleanup;
  SymEnumerateModules = (PFN_SYMENUMERATEMODULES)GetProcAddress(module, "SymEnumerateModules");
  if (!SymEnumerateModules)
    goto cleanup;
  SymEnumerateSymbols = (PFN_SYMENUMERATESYMBOLS)GetProcAddress(module, "SymEnumerateSymbols");
  if (!SymEnumerateSymbols)
    goto cleanup;

  MiniDumpWriteDump = (PFN_MINIDUMPWRITEDUMP)GetProcAddress(module, "MiniDumpWriteDump");
  hInstance = module;
  LeaveCriticalSection(&s_CrawlCritsect);
  return TRUE;

cleanup:
  FreeLibrary(module);
load_failed:
  LeaveCriticalSection(&s_CrawlCritsect);
  DeleteCriticalSection(&s_CrawlCritsect);
  return FALSE;
}
void CDbgHelpDll::Unload() {
  if (!--loadCount) {
    EnterCriticalSection(&s_CrawlCritsect);
    if (hInstance) {
      FreeLibrary(hInstance);
      hInstance = NULL;
    }
    LeaveCriticalSection(&s_CrawlCritsect);
    DeleteCriticalSection(&s_CrawlCritsect);
  }
}
static void sLogSeparatorLine(LOGMACHINESTATEPROC logLineProc, LPVOID logLineProcParam, char character, int longLine) {
  char line[80];
  int  chars;

  chars = longLine ? 0x4E : 0x28;
  memset(line, character, chars);
  line[chars] = 0;
  logLineProc(logLineProcParam, "%s", line);
}
static void sLogHeader(LOGMACHINESTATEPROC logLineProc, LPVOID logLineProcParam, LPCSTR headerString) {
  logLineProc(logLineProcParam, "");
  sLogSeparatorLine(logLineProc, logLineProcParam, '-', 0);
  logLineProc(logLineProcParam, "    %s", headerString);
  sLogSeparatorLine(logLineProc, logLineProcParam, '-', 0);
  logLineProc(logLineProcParam, "");
}
static void sLogMemoryHexDump(LOGMACHINESTATEPROC logLineProc, LPVOID logLineProcParam, BYTE *address, DWORD numBytes, int alignedLines) {
  DWORD numLines = numBytes / 16;
  DWORD offset = (DWORD)address & 0x0F;

  if (alignedLines) {
    char buffer[80];

    memset(buffer, ' ', sizeof(buffer) - 2);
    buffer[sizeof(buffer) - 2] = 0;
    buffer[0] = '*';
    buffer[2] = '=';
    buffer[4] = 'a';
    buffer[5] = 'd';
    buffer[6] = 'd';
    buffer[7] = 'r';
    char *marker = &buffer[0x0A + offset * 3 + (offset >> 2)];
    marker[0] = '*';
    marker[1] = '*';
    buffer[0x3E + offset] = '*';
    logLineProc(logLineProcParam, "%s", buffer);

    if (offset) {
      address -= offset;
      ++numLines;
    }
  }

  while (numLines > 0) {
    char  buffer[80];
    char *cursor = buffer + sprintf(buffer, "%08X: ", address);

    if (IsBadReadPtr(address, 16)) {
      strcpy(cursor, "<can't read from this address>");
      logLineProc(logLineProcParam, "%s", buffer);
      return;
    }

    cursor += sprintf(cursor, "%02X %02X %02X %02X  ", address[0], address[1], address[2], address[3]);
    cursor += sprintf(cursor, "%02X %02X %02X %02X  ", address[4], address[5], address[6], address[7]);
    cursor += sprintf(cursor, "%02X %02X %02X %02X  ", address[8], address[9], address[10], address[11]);
    cursor += sprintf(cursor, "%02X %02X %02X %02X  ", address[12], address[13], address[14], address[15]);

    for (int i = 0; i < 16; i++) {
      cursor[i] = isprint(address[i]) ? address[i] : '.';
    }
    cursor[16] = 0;
    logLineProc(logLineProcParam, "%s", buffer);

    address += 16;
    --numLines;
  }
}
static void sLogVerboseMessage(UINT logOptions, LOGMACHINESTATEPROC logLineProc, LPVOID logLineProcParam, LPCSTR format, DWORD error) {
  if (logOptions & 0x80000000) {
    logLineProc(logLineProcParam, format, error);
  }
}
static LPCSTR sGetPathLeaf(LPCSTR path) {
  LPCSTR leaf;
  LPCSTR slash;

  leaf = NULL;
  slash = strrchr(path, '/');
  if (slash > leaf) {
    leaf = slash;
  }
  slash = strrchr(path, '\\');
  if (slash && slash > leaf) {
    leaf = slash;
  }
  slash = strrchr(path, ':');
  if (slash && slash > leaf) {
    leaf = slash;
  }

  return leaf ? leaf + 1 : path;
}
static void sLogExeFile(LOGMACHINESTATEPROC logLineProc, LPVOID logLineProcParam) {
  char exeFullPath[MAX_PATH];

  GetModuleFileNameA(NULL, exeFullPath, sizeof(exeFullPath));
  logLineProc(logLineProcParam, "%-10s%s", "Exe:", exeFullPath);
}
static void sLogDateTimeString(LOGMACHINESTATEPROC logLineProc, LPVOID logLineProcParam, const SYSTEMTIME *time) {
  WORD hour;
  char suffix;

  hour = time->wHour;
  suffix = hour < 12 ? 'A' : 'P';
  if (hour > 12) {
    hour -= 12;
  } else if (!hour) {
    hour = 12;
  }

  logLineProc(
      logLineProcParam, "%-10s%3s %2d, %4d %2d:%02d:%02d.%03d %cM", "Time:", sgMonthString[time->wMonth - 1], time->wDay, time->wYear, hour,
      time->wMinute, time->wSecond, time->wMilliseconds, suffix
  );
}
static void sLogUserName(LOGMACHINESTATEPROC logLineProc, LPVOID logLineProcParam) {
  char  userName[0x101];
  DWORD size;

  size = sizeof(userName);
  if (!GetUserNameA(userName, &size)) {
    strcpy(userName, "<unknown>");
  }
  logLineProc(logLineProcParam, "%-10s%s", "User:", userName);
}
static void sLogComputerName(LOGMACHINESTATEPROC logLineProc, LPVOID logLineProcParam) {
  char  computerName[0x10];
  DWORD size;

  size = sizeof(computerName);
  if (!GetComputerNameA(computerName, &size)) {
    strcpy(computerName, "<unknown>");
  }
  logLineProc(logLineProcParam, "%-10s%s", "Computer:", computerName);
}
static void sLogMemory(UINT logOptions, LOGMACHINESTATEPROC logLineProc, LPVOID logLineProcParam, DWORD instructionPointr, DWORD stackPointer) {
  sLogHeader(logLineProc, logLineProcParam, "Memory Dump");

  if (logOptions & 0x00100000) {
    logLineProc(logLineProcParam, "Code: %d bytes starting at (EIP = %08X)", 0x10, instructionPointr);
    logLineProc(logLineProcParam, "");
    sLogMemoryHexDump(logLineProc, logLineProcParam, (BYTE *)instructionPointr, 0x10, 0);
    logLineProc(logLineProcParam, "");
    logLineProc(logLineProcParam, "");
  }

  if (logOptions & 0x00200000) {
    logLineProc(logLineProcParam, "Stack: %d bytes starting at (ESP = %08X)", 0x400, stackPointer);
    logLineProc(logLineProcParam, "");
    sLogMemoryHexDump(logLineProc, logLineProcParam, (BYTE *)stackPointer, 0x400, 1);
    logLineProc(logLineProcParam, "");
    logLineProc(logLineProcParam, "");
  }
}
int __cdecl sModuleCompareProc(LPCVOID elem1, LPCVOID elem2) {
  if (((const ModuleData *)elem1)->baseAddress > ((const ModuleData *)elem2)->baseAddress) {
    return 1;
  }
  if (((const ModuleData *)elem1)->baseAddress < ((const ModuleData *)elem2)->baseAddress) {
    return -1;
  }
  return 0;
}
static BOOL CALLBACK sEnumModulesCallback(LPSTR ModuleName, ULONG BaseOfDll, PVOID UserContext) {
  EnumModuleData *data = (EnumModuleData *)UserContext;
  ModuleData     *module = &data->modules[data->count];
  data->count++;

  if (!ModuleName || !ModuleName[0]) {
    ModuleName = "<unknown>";
  }
  strncpy(module->name, ModuleName, sizeof(module->name) - 1);
  module->name[sizeof(module->name) - 1] = 0;
  module->baseAddress = BaseOfDll;

  return data->count < sizeof(data->modules) / sizeof(data->modules[0]);
}
static BOOL CALLBACK sEnumSymbolsCallback(LPSTR SymbolName, ULONG SymbolAddress, ULONG SymbolSize, PVOID UserContext) {
  ((LogLineParams *)UserContext)
      ->logLineProc(
          ((LogLineParams *)UserContext)->logLineProcParam, "    0x%08X: %s (%d : 0x%08X)", SymbolAddress, SymbolName, SymbolSize, SymbolSize
      );
  return TRUE;
}
static void sLogModule(UINT logOptions, LOGMACHINESTATEPROC logLineProc, LPVOID logLineProcParam, DWORD baseAddress, char *moduleName) {
  IMAGEHLP_MODULE module;

  memset(&module, 0, sizeof(module));
  module.SizeOfStruct = sizeof(module);

  if (sgDbgHelpDll.SymGetModuleInfo(GetCurrentProcess(), baseAddress, &module) && module.ImageSize > 0) {
    logLineProc(logLineProcParam, "0x%08X - 0x%08X  %s", baseAddress, baseAddress + module.ImageSize, module.ImageName);
  } else {
    logLineProc(logLineProcParam, "0x%08X - ??????????  %s", baseAddress, moduleName);
  }

  if (logOptions & 0x00800000) {
    logLineProc(logLineProcParam, "");
    logLineProc(logLineProcParam, "    Dumping Symbols");

    LogLineParams params;
    params.logLineProc = logLineProc;
    params.logLineProcParam = logLineProcParam;

    if (!sgDbgHelpDll.SymEnumerateSymbols(GetCurrentProcess(), baseAddress, sEnumSymbolsCallback, &params)) {
      logLineProc(logLineProcParam, "****  SymEnumerateModules couldn't enumerate symbols, error: %d", GetLastError());
    }

    logLineProc(logLineProcParam, "");
  }
}
static void sGetLogicalAddress(LPVOID addr, char *moduleName, DWORD moduleNameSize, DWORD *section, DWORD *offset) {
  MEMORY_BASIC_INFORMATION memInfo;
  HMODULE                  module;
  PIMAGE_DOS_HEADER        dosHeader;
  PIMAGE_NT_HEADERS        ntHeaders;
  PIMAGE_SECTION_HEADER    sectionHeader;
  DWORD                    rva;
  DWORD                    sectionStart;
  DWORD                    sectionSize;
  UINT                     i;

  lstrcpynA(moduleName, "<unknown>", moduleNameSize);
  *offset = 0;
  *section = 0;

  if (!VirtualQuery(addr, &memInfo, sizeof(memInfo))) {
    return;
  }

  module = (HMODULE)memInfo.AllocationBase;
  if (!module) {
    module = GetModuleHandleA(NULL);
  }

  if (!GetModuleFileNameA(module, moduleName, moduleNameSize)) {
    lstrcpynA(moduleName, "<unknown>", moduleNameSize);
    return;
  }

  if (!module) {
    return;
  }

  dosHeader = (PIMAGE_DOS_HEADER)module;
  if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE || !dosHeader->e_lfanew) {
    return;
  }

  ntHeaders = (PIMAGE_NT_HEADERS)((BYTE *)module + dosHeader->e_lfanew);
  if (ntHeaders->Signature != IMAGE_NT_SIGNATURE) {
    return;
  }

  sectionHeader = IMAGE_FIRST_SECTION(ntHeaders);
  rva = (BYTE *)addr - (BYTE *)module;

  for (i = 0; i < ntHeaders->FileHeader.NumberOfSections; i++, sectionHeader++) {
    sectionStart = sectionHeader->VirtualAddress;
    sectionSize = max(sectionHeader->SizeOfRawData, sectionHeader->Misc.VirtualSize);

    if (rva >= sectionStart && rva <= sectionStart + sectionSize) {
      *section = i + 1;
      *offset = rva - sectionStart;
      return;
    }
  }
}
static int
sDbgHelpGetStackFrameInfo(DWORD address, char *moduleName, char *symbolName, DWORD &symbolDisplacement, char *fileName, DWORD &lineNumber) {
  IMAGEHLP_MODULE module;
  char            buffer[0x118];
  IMAGEHLP_LINE   line;
  DWORD           displacement;
  int             err;

  err = 0;

  memset(&module, 0, sizeof(module));
  module.SizeOfStruct = sizeof(module);
  if (sgDbgHelpDll.SymGetModuleInfo(GetCurrentProcess(), address, &module)) {
    strcpy(moduleName, sGetPathLeaf(module.ImageName));
  } else {
    strcpy(moduleName, "<unknown module>");
    err |= 1;
  }

  fileName[0] = 0;
  lineNumber = 0;

  memset(buffer, 0, sizeof(buffer));
  ((PIMAGEHLP_SYMBOL)buffer)->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
  ((PIMAGEHLP_SYMBOL)buffer)->Address = address;
  ((PIMAGEHLP_SYMBOL)buffer)->MaxNameLength = sizeof(buffer) - sizeof(IMAGEHLP_SYMBOL);
  if (sgDbgHelpDll.SymGetSymFromAddr(GetCurrentProcess(), address, &displacement, (PIMAGEHLP_SYMBOL)buffer)) {
    strcpy(symbolName, ((PIMAGEHLP_SYMBOL)buffer)->Name);
    symbolDisplacement = displacement;

    memset(&line, 0, sizeof(line));
    line.SizeOfStruct = sizeof(line);
    if (sgDbgHelpDll.SymGetLineFromAddr(GetCurrentProcess(), address, &displacement, &line)) {
      LPCSTR leaf = sGetPathLeaf(line.FileName);
      strncpy(fileName, leaf, MAX_PATH - 1);
      fileName[MAX_PATH - 1] = 0;
      lineNumber = line.LineNumber;
    } else {
      err |= 2;
    }
  } else {
    strcpy(symbolName, "<unknown symbol>");
    symbolDisplacement = 0;
    err |= 4;
  }

  return err;
}
static void sLogDbgHelpStackFrame(UINT logOptions, LOGMACHINESTATEPROC logLineProc, LPVOID logLineProcParam, const STACKFRAME *stackFrame) {
  char  symbolName[0x100];
  char  fileName[0x104];
  char  moduleName[0x20];
  DWORD lineNumber;
  DWORD symbolDisplacement;
  DWORD address;
  int   err;

  address = stackFrame->AddrPC.Offset;
  err = sDbgHelpGetStackFrameInfo(address, moduleName, symbolName, symbolDisplacement, fileName, lineNumber);

  if (err & 1) {
    sLogVerboseMessage(logOptions, logLineProc, logLineProcParam, "**** SymGetModuleInfo() failed, error: %d", GetLastError());
  }
  if (err & 2) {
    sLogVerboseMessage(logOptions, logLineProc, logLineProcParam, "**** SymGetLineFromAddr() failed, error: %d", GetLastError());
  }
  if (err & 4) {
    sLogVerboseMessage(logOptions, logLineProc, logLineProcParam, "**** SymGetSymFromAddr() failed, error: %d", GetLastError());
  }

  if (logOptions & 0x00040000) {
    LPCSTR format = fileName[0] ? "%08X %-12s %s+%d (0x%08X,0x%08X,0x%08X,0x%08X) (%s,%d)" : "%08X %-12s %s+%d (0x%08X,0x%08X,0x%08X,0x%08X)";
    logLineProc(
        logLineProcParam, format, address, moduleName, symbolName, symbolDisplacement, stackFrame->Params[0], stackFrame->Params[1],
        stackFrame->Params[2], stackFrame->Params[3], fileName, lineNumber
    );
  } else {
    LPCSTR format = fileName[0] ? "%08X %-12s %s+%d (%s,%d)" : "%08X %-12s %s+%d";
    logLineProc(logLineProcParam, format, address, moduleName, symbolName, symbolDisplacement, fileName, lineNumber);
  }
}
static int sSymInitialize(LPVOID process) {
  char path[0x104];
  path[0] = 0;
  GetModuleFileNameA(NULL, path, sizeof(path));
  char *pathEnd = SStrChrR(path, '\\');
  if (pathEnd) {
    *pathEnd = 0;
  }

  return sgDbgHelpDll.SymInitialize(process, path, TRUE);
}
int sQuickStackWalkInit(int init) {
  HANDLE process;
  int    initialized;

  process = GetCurrentProcess();
  if (init) {
    if (s_StackInit) {
      return TRUE;
    }
    if (!sgDbgHelpDll.Load()) {
      return FALSE;
    }

    EnterCriticalSection(&s_CrawlCritsect);
    sgDbgHelpDll.SymSetOptions(sgDbgHelpDll.SymGetOptions() | 0x10);
    initialized = sSymInitialize(process);
    LeaveCriticalSection(&s_CrawlCritsect);

    if (!initialized) {
      sgDbgHelpDll.Unload();
      return FALSE;
    }

    s_StackInit = TRUE;
    return TRUE;
  }

  if (!s_StackInit) {
    return FALSE;
  }

  EnterCriticalSection(&s_CrawlCritsect);
  sgDbgHelpDll.SymCleanup(process);
  LeaveCriticalSection(&s_CrawlCritsect);
  sgDbgHelpDll.Unload();
  s_StackInit = FALSE;
  return TRUE;
}
extern "C" int APIENTRY QuickStackWalk(DWORD *crawl, int &depth, int stackFramesToSkip) {
#if defined(_M_IX86) || defined(_X86_)
  STACKFRAME stackFrame;
  DWORD      registerEsp;
  int        maxDepth;
  DWORD      registerEbp;
  HANDLE     thread;
  DWORD      registerEip;
  HANDLE     process;
  int        frameIndex;

  maxDepth = depth;
  depth = 0;

  if (!sQuickStackWalkInit(TRUE)) {
    return FALSE;
  }

  EnterCriticalSection(&s_CrawlCritsect);
  process = GetCurrentProcess();
  thread = GetCurrentThread();

  __asm {
    call nextline
  nextline:
    pop registerEip
    push ebp
    pop registerEbp
    push esp
    pop registerEsp
  }

  memset(&stackFrame, 0, sizeof(stackFrame));
  stackFrame.AddrPC.Offset = registerEip;
  stackFrame.AddrPC.Mode = AddrModeFlat;
  stackFrame.AddrFrame.Offset = registerEbp;
  stackFrame.AddrFrame.Mode = AddrModeFlat;
  stackFrame.AddrStack.Offset = registerEsp;
  stackFrame.AddrStack.Mode = AddrModeFlat;

  for (frameIndex = 0; frameIndex < 0x20; ++frameIndex) {
    if (!sgDbgHelpDll.StackWalk(
            IMAGE_FILE_MACHINE_I386, process, thread, &stackFrame, NULL, NULL, sgDbgHelpDll.SymFunctionTableAccess, sgDbgHelpDll.SymGetModuleBase,
            NULL
        ))
    {
      break;
    }

    if (stackFrame.AddrPC.Offset && frameIndex >= stackFramesToSkip) {
      crawl[depth] = stackFrame.AddrPC.Offset;
      ++depth;
      if (depth >= maxDepth) {
        break;
      }
    }
  }

  LeaveCriticalSection(&s_CrawlCritsect);
  return TRUE;
#else
  (void)crawl;
  depth = 0;
  (void)stackFramesToSkip;
  return FALSE;
#endif
}
extern "C" int APIENTRY StackWalkAddrsToNames(DWORD *crawlAddrs, int depth, char **crawlNames) {
  if (!sQuickStackWalkInit(TRUE)) {
    return FALSE;
  }

  SErrPauseWatchdog();
  EnterCriticalSection(&s_CrawlCritsect);

  for (int i = 0; i < depth; i++) {
    DWORD address = crawlAddrs[i];
    char  key[0x20];

    SStrPrintf(key, sizeof(key), "<sym-%08X>", address);
    if (STypeCache::Get(key)) {
      SStrCopy(crawlNames[i], STypeCache::Get(NULL), 0x100);
    } else {
      char  moduleName[0x20];
      char  symbolName[0x100];
      DWORD symbolDisplacement;
      char  fileName[MAX_PATH];
      DWORD lineNumber;

      sDbgHelpGetStackFrameInfo(address, moduleName, symbolName, symbolDisplacement, fileName, lineNumber);
      SStrPrintf(crawlNames[i], 0x100, "%s %s+%d %s(%d)", moduleName, symbolName, symbolDisplacement, fileName, lineNumber);
      STypeCache::Set(key, crawlNames[i]);
    }
  }

  LeaveCriticalSection(&s_CrawlCritsect);
  SErrResumeWatchdog();
  return TRUE;
}
static void APIENTRY CmdMemOutput(HOUTPUTCONTEXT hOutput, LPCSTR str) {
  MEMDUMP                *dump;
  SMemReportByCallerInfo *info;

  dump = (MEMDUMP *)hOutput;
  info = (SMemReportByCallerInfo *)str;
  dump->logLineProc(dump->logLineProcParam, "%5d %5d %s(%d)", info->allocatedBlocks, info->allocatedBytes, info->fileName, info->lineNumber);
}
static void sShowOutOfMemory(LOGMACHINESTATEPROC logLineProc, LPVOID logLineProcParam) {
  MEMDUMP dump;

  if (!g_memFullError) {
    return;
  }

  dump.logLineProc = logLineProc;
  dump.logLineProcParam = logLineProcParam;

  SMemGenerateReport(SMEM_REPORT_BY_CALLER, CmdMemOutput, (HOUTPUTCONTEXT)&dump);
  sLogSeparatorLine(logLineProc, logLineProcParam, '-', 1);
}
static void sLogDbgHelpStackTrace(
    UINT                logOptions,
    LOGMACHINESTATEPROC logLineProc,
    LPVOID              logLineProcParam,
    DWORD               registerEip,
    DWORD               registerEbp,
    DWORD               registerEsp,
    UINT                stackFramesToSkip
) {
  HANDLE         process;
  HANDLE         thread;
  STACKFRAME     stackFrame;
  UINT           frameIndex;
  EnumModuleData data;
  DWORD          moduleIndex;
  BOOL           success;

  sLogHeader(logLineProc, logLineProcParam, "Stack Trace (Using DBGHELP.DLL)");

  process = GetCurrentProcess();
  if (!sgDbgHelpDll.Load()) {
    logLineProc(logLineProcParam, "****  Couldn't load DBGHELP.DLL, error: %d", GetLastError());
    goto cleanup;
  }

  sgDbgHelpDll.SymSetOptions(sgDbgHelpDll.SymGetOptions() | 0x14);

  if (!sSymInitialize(process)) {
    logLineProc(logLineProcParam, "****  Couldn't initialize Debug Help library, error: %d", GetLastError());
    goto cleanup;
  }

  if (logOptions & 0x00020000) {
    thread = GetCurrentThread();
    memset(&stackFrame, 0, sizeof(stackFrame));
    stackFrame.AddrPC.Offset = registerEip;
    stackFrame.AddrPC.Mode = AddrModeFlat;
    stackFrame.AddrFrame.Offset = registerEbp;
    stackFrame.AddrFrame.Mode = AddrModeFlat;
    stackFrame.AddrStack.Offset = registerEsp;
    stackFrame.AddrStack.Mode = AddrModeFlat;

    for (frameIndex = 0; frameIndex < 100; frameIndex++) {
      success = sgDbgHelpDll.StackWalk(
          IMAGE_FILE_MACHINE_I386, process, thread, &stackFrame, NULL, NULL, sgDbgHelpDll.SymFunctionTableAccess, sgDbgHelpDll.SymGetModuleBase,
          NULL
      );
      if (!success) {
        sLogVerboseMessage(logOptions, logLineProc, logLineProcParam, "**** StackWalk() returned FALSE, error: %d", GetLastError());
        break;
      }

      if (!stackFrame.AddrPC.Offset) {
        sLogVerboseMessage(logOptions, logLineProc, logLineProcParam, "**** StackWalk() returned zero address - skipping stack frame", 0);
        continue;
      }

      if (frameIndex < stackFramesToSkip) {
        continue;
      }

      sLogDbgHelpStackFrame(logOptions, logLineProc, logLineProcParam, &stackFrame);
    }
  }

  if (logOptions & 0x00400000) {
    logLineProc(logLineProcParam, "");
    sLogHeader(logLineProc, logLineProcParam, "Loaded Modules");

    data.count = 0;
    success = sgDbgHelpDll.SymEnumerateModules(GetCurrentProcess(), sEnumModulesCallback, &data);

    qsort(data.modules, data.count, sizeof(data.modules[0]), sModuleCompareProc);

    for (moduleIndex = 0; moduleIndex < data.count; moduleIndex++) {
      ModuleData *module = &data.modules[moduleIndex];
      sLogModule(logOptions, logLineProc, logLineProcParam, module->baseAddress, module->name);
    }

    if (!success) {
      logLineProc(logLineProcParam, "****  SymEnumerateModules couldn't enumerate modules, error: %d", GetLastError());
    }
  }

cleanup:
  sgDbgHelpDll.Unload();
  logLineProc(logLineProcParam, "");
}
static void sLogX86ManualStackTrace(
    UINT,
    LOGMACHINESTATEPROC logLineProc,
    LPVOID              logLineProcParam,
    DWORD               registerEip,
    DWORD               registerEbp,
    UINT                stackFramesToSkip
) {
  UINT i;

  sLogHeader(logLineProc, logLineProcParam, "Stack Trace (Manual)");
  logLineProc(logLineProcParam, "%s", "Address  Frame    Logical addr  Module");
  logLineProc(logLineProcParam, "");

  DWORD  pc = registerEip;
  DWORD *frame = (DWORD *)registerEbp;

  for (i = 0; i < 100; i++) {
    if (i >= stackFramesToSkip) {
      char  modulePath[MAX_PATH];
      DWORD section;
      DWORD offset;

      sGetLogicalAddress((LPVOID)pc, modulePath, sizeof(modulePath), &section, &offset);
      logLineProc(logLineProcParam, "%08X %08X %04X:%08X %s", pc, frame, section, offset, modulePath);
    }

    if (IsBadWritePtr(frame, 2 * sizeof(DWORD))) {
      break;
    }

    pc = frame[1];
    DWORD *prevFrame = frame;
    frame = (DWORD *)frame[0];

    if ((DWORD)frame & 3) {
      break;
    }
    if (frame <= prevFrame) {
      break;
    }
    if (IsBadWritePtr(frame, 2 * sizeof(DWORD))) {
      break;
    }
  }
}
static void sLogX86ContextRegisters(LOGMACHINESTATEPROC logLineProc, LPVOID logLineProcParam, CONTEXT *context) {
  sLogHeader(logLineProc, logLineProcParam, "x86 Registers");

  logLineProc(
      logLineProcParam, "EAX=%08X  EBX=%08X  ECX=%08X  EDX=%08X  ESI=%08X", context->Eax, context->Ebx, context->Ecx, context->Edx, context->Esi
  );
  logLineProc(
      logLineProcParam, "EDI=%08X  EBP=%08X  ESP=%08X  EIP=%08X  FLG=%08X", context->Edi, context->Ebp, context->Esp, context->Eip, context->EFlags
  );
  logLineProc(
      logLineProcParam, "CS =%04X      DS =%04X      ES =%04X      SS =%04X      FS =%04X      GS =%04X", context->SegCs, context->SegDs,
      context->SegEs, context->SegSs, context->SegFs, context->SegGs
  );
  logLineProc(logLineProcParam, "");
}
int CheckMachineStateSymbolHelper() {
  int loaded;

  loaded = sgDbgHelpDll.Load();
  sgDbgHelpDll.Unload();

  return loaded;
}
void LoadMachineStateSymbols() {
  sgDbgHelpDll.Load();
}
void UnloadMachineStateSymbols() {
  sgDbgHelpDll.Unload();
}
void LogComputerInfoHeader(UINT logOptions, LOGMACHINESTATEPROC logLineProc, LPVOID logLineProcParam, LPCSTR headerTitleLine, SYSTEMTIME *time) {
  SYSTEMTIME currentTime;

  if (InterlockedIncrement(&sgRecursionLevel) == 1) {
    if (!logOptions || (logOptions & 0x00000001)) {
      logOptions |= 0x0000003C;
      if (headerTitleLine && headerTitleLine[0]) {
        logOptions |= 0x00000002;
      }
    }

    sLogSeparatorLine(logLineProc, logLineProcParam, '=', 1);
    if ((logOptions & 0x00000002) && headerTitleLine && headerTitleLine[0]) {
      logLineProc(logLineProcParam, "%s", headerTitleLine);
      logLineProc(logLineProcParam, "");
    }
    if (logOptions & 0x00000004) {
      sLogExeFile(logLineProc, logLineProcParam);
    }
    if (logOptions & 0x00000008) {
      if (!time) {
        GetLocalTime(&currentTime);
        time = &currentTime;
      }
      sLogDateTimeString(logLineProc, logLineProcParam, time);
    }
    if (logOptions & 0x00000020) {
      sLogUserName(logLineProc, logLineProcParam);
    }
    if (logOptions & 0x00000010) {
      sLogComputerName(logLineProc, logLineProcParam);
    }
    sLogSeparatorLine(logLineProc, logLineProcParam, '-', 1);
  }

  InterlockedDecrement(&sgRecursionLevel);
}
void LogMachineState(UINT logOptions, LOGMACHINESTATEPROC logLineProc, LPVOID logLineProcParam, UINT stackFramesToSkip, CONTEXT *context) {
#if defined(_M_IX86) || defined(_X86_)
  DWORD registerEip;
  DWORD registerEbp;
  DWORD registerEsp;
#endif

  if (InterlockedIncrement(&sgRecursionLevel) != 1) {
    goto cleanup;
  }

  if (!logOptions || (0x00000001 & logOptions)) {
    logOptions |= 0x006A0000;
    if (context) {
      logOptions |= 0x00110000;
    }
  }
  if (logOptions & 0x00800000) {
    logOptions |= 0x00400000;
  }
  if (context) {
#if defined(_M_IX86) || defined(_X86_)
    registerEip = context->Eip;
    registerEbp = context->Ebp;
    registerEsp = context->Esp;
#endif
    stackFramesToSkip = 0;
  } else {
#if defined(_M_IX86) || defined(_X86_)
    __asm {
      call nextline
    nextline:
      pop registerEip
      push ebp
      pop registerEbp
      push esp
      pop registerEsp
    }
#endif
    logOptions &= 0xFFEEFFFF;
    ++stackFramesToSkip;
  }

  sLogSeparatorLine(logLineProc, logLineProcParam, '-', 1);

  if ((logOptions & 0x00010000) && context) {
    sLogX86ContextRegisters(logLineProc, logLineProcParam, context);
  }
#if defined(_M_IX86) || defined(_X86_)
  if (logOptions & 0x00080000) {
    sLogX86ManualStackTrace(logOptions, logLineProc, logLineProcParam, registerEip, registerEbp, stackFramesToSkip);
  }
  if (logOptions & 0x00420000) {
    sLogDbgHelpStackTrace(logOptions, logLineProc, logLineProcParam, registerEip, registerEbp, registerEsp, stackFramesToSkip);
  }
  if (logOptions & 0x00300000) {
    sLogMemory(logOptions, logLineProc, logLineProcParam, registerEip, registerEsp);
  }
#endif

  sLogSeparatorLine(logLineProc, logLineProcParam, '-', 1);
  sShowOutOfMemory(logLineProc, logLineProcParam);

cleanup:
  InterlockedDecrement(&sgRecursionLevel);
}
static DWORD WINAPI MiniDumpThreadProc(LPVOID param) {
  MINIDUMP_USER_STREAM             miniDumpUserStreamArray[16];
  MINIDUMP_EXCEPTION_INFORMATION   miniDumpExceptionInfo;
  MINIDUMP_USER_STREAM_INFORMATION miniDumpUserStreamInfo;
  int                              result = FALSE;
  UINT                             userStreamCount = 0;

  if (sgDbgHelpDll.Load()) {
    if (sgDbgHelpDll.MiniDumpWriteDump) {
      miniDumpExceptionInfo.ThreadId = ((MiniDumpParam *)param)->threadId;
      miniDumpExceptionInfo.ExceptionPointers = ((MiniDumpParam *)param)->exceptionPointers;
      miniDumpExceptionInfo.ClientPointers = FALSE;

      for (UINT i = 0; i < ((MiniDumpParam *)param)->userStringCount; i++) {
        if (((MiniDumpParam *)param)->userStrings[i]
            && userStreamCount < sizeof(miniDumpUserStreamArray) / sizeof(miniDumpUserStreamArray[0])) {
          miniDumpUserStreamArray[userStreamCount].Type = userStreamCount + 0x1000;
          miniDumpUserStreamArray[userStreamCount].BufferSize = strlen(((MiniDumpParam *)param)->userStrings[i]) + 1;
          miniDumpUserStreamArray[userStreamCount].Buffer = ((MiniDumpParam *)param)->userStrings[i];
          userStreamCount++;
        }
      }

      miniDumpUserStreamInfo.UserStreamCount = userStreamCount;
      miniDumpUserStreamInfo.UserStreamArray = miniDumpUserStreamArray;

      result = sgDbgHelpDll.MiniDumpWriteDump(
          GetCurrentProcess(), GetCurrentProcessId(), ((MiniDumpParam *)param)->logfile, (MINIDUMP_TYPE)0x40,
          miniDumpExceptionInfo.ExceptionPointers ? &miniDumpExceptionInfo : NULL, &miniDumpUserStreamInfo, NULL
      );
    }

    sgDbgHelpDll.Unload();
  }

  ((MiniDumpParam *)param)->result = result;
  return 0;
}
int LogMiniDump(LPVOID logfile, EXCEPTION_POINTERS *exceptionPointers, UINT userStringCount, char *userStrings[]) {
  int           result = FALSE;
  MiniDumpParam miniDumpParam;
  DWORD         threadid = 0;
  HANDLE        thread;

  if (InterlockedIncrement(&sgRecursionLevel) != 1) {
    goto cleanup;
  }

  miniDumpParam.logfile = logfile;
  miniDumpParam.threadId = GetCurrentThreadId();
  miniDumpParam.userStringCount = userStringCount;
  miniDumpParam.userStrings = userStrings;
  miniDumpParam.exceptionPointers = exceptionPointers;

  thread = CreateThread(NULL, 0, MiniDumpThreadProc, &miniDumpParam, 0, &threadid);
  if (!thread) {
    goto cleanup;
  }

  WaitForSingleObject(thread, INFINITE);
  CloseHandle(thread);
  result = miniDumpParam.result;

cleanup:
  InterlockedDecrement(&sgRecursionLevel);
  return result;
}
int LogMiniDumpIsAvailable() {
  int loaded;
  int available;

  available = FALSE;
  loaded = sgDbgHelpDll.Load();
  if (loaded) {
    available = sgDbgHelpDll.MiniDumpWriteDump != NULL;
    sgDbgHelpDll.Unload();
  }

  return available;
}
