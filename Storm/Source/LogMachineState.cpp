#include <storm.h>
#include <ctype.h>
#include <imagehlp.h>

typedef BOOL(WINAPI *PFN_SYMGETMODULEINFO)(HANDLE process, DWORD address, PIMAGEHLP_MODULE moduleInfo);
typedef BOOL(WINAPI *PFN_SYMENUMERATESYMBOLS)(HANDLE process, DWORD baseOfDll, PSYM_ENUMSYMBOLS_CALLBACK callback, PVOID userContext);
typedef BOOL(WINAPI *PFN_SYMGETSYMFROMADDR)(HANDLE process, DWORD address, PDWORD displacement, PIMAGEHLP_SYMBOL symbol);
typedef BOOL(WINAPI *PFN_SYMGETLINEFROMADDR)(HANDLE process, DWORD address, PDWORD displacement, PIMAGEHLP_LINE line);
typedef DWORD(WINAPI *PFN_SYMGETOPTIONS)(void);
typedef DWORD(WINAPI *PFN_SYMSETOPTIONS)(DWORD options);
typedef BOOL(WINAPI *PFN_SYMINITIALIZE)(HANDLE process, LPSTR searchPath, BOOL invadeProcess);
typedef BOOL(WINAPI *PFN_SYMCLEANUP)(HANDLE process);
typedef BOOL(WINAPI *PFN_SYMENUMERATEMODULES)(HANDLE process, PSYM_ENUMMODULES_CALLBACK callback, PVOID userContext);
typedef BOOL(WINAPI *PFN_STACKWALK)(
    DWORD                          machineType,
    HANDLE                         process,
    HANDLE                         thread,
    LPSTACKFRAME                   stackFrame,
    LPVOID                         contextRecord,
    PREAD_PROCESS_MEMORY_ROUTINE   readMemoryRoutine,
    PFUNCTION_TABLE_ACCESS_ROUTINE functionTableAccessRoutine,
    PGET_MODULE_BASE_ROUTINE       getModuleBaseRoutine,
    PTRANSLATE_ADDRESS_ROUTINE     translateAddress
);
typedef LPVOID(WINAPI *PFN_SYMFUNCTIONTABLEACCESS)(HANDLE process, DWORD addressBase);
typedef DWORD(WINAPI *PFN_SYMGETMODULEBASE)(HANDLE process, DWORD returnAddress);

typedef enum _MINIDUMP_TYPE {
  MiniDumpNormal = 0,
  MiniDumpWithDataSegs = 1,
  MiniDumpWithFullMemory = 2,
  MiniDumpWithHandleData = 4,
  MiniDumpFilterMemory = 8,
  MiniDumpScanMemory = 16,
  MiniDumpWithIndirectlyReferencedMemory = 64
} MINIDUMP_TYPE;

typedef struct _MINIDUMP_EXCEPTION_INFORMATION {
  DWORD               ThreadId;
  EXCEPTION_POINTERS *ExceptionPointers;
  BOOL                ClientPointers;
} MINIDUMP_EXCEPTION_INFORMATION;

typedef struct _MINIDUMP_USER_STREAM {
  DWORD Type;
  DWORD BufferSize;
  PVOID Buffer;
} MINIDUMP_USER_STREAM;

typedef struct _MINIDUMP_USER_STREAM_INFORMATION {
  DWORD                 UserStreamCount;
  MINIDUMP_USER_STREAM *UserStreamArray;
} MINIDUMP_USER_STREAM_INFORMATION;

typedef struct _MINIDUMP_CALLBACK_INFORMATION MINIDUMP_CALLBACK_INFORMATION;

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

  CDbgHelpDll();
  ~CDbgHelpDll();
  int  Load();
  void Unload();
  int  IsLoaded() {
    return hInstance != NULL;
  }
};

static CRITICAL_SECTION s_CrawlCritsect;

typedef VOID(WINAPI *PFN_RTLCAPTURECONTEXT)(PCONTEXT context);
static CDbgHelpDll sgDbgHelpDll;
static LONG        sgRecursionLevel;
static char        sgMonthString[12][4] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

extern BOOL g_memFullError;

struct MEMDUMP {
  LOGMACHINESTATEPROC logLineProc;
  void               *logLineProcParam;
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
  void               *logLineProcParam;
};

struct ModuleData {
  char  name[0x100];
  DWORD baseAddress;
};

struct EnumModuleData {
  UINT       count;
  ModuleData modules[0x100];
};

void SMemGenerateReport(SMEMREPORTTYPE reporttype, SMEMREPORTPROC outputproc, HOUTPUTCONTEXT outputcontext) {
  StormCallService(2, reporttype, outputproc, outputcontext);
}
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
  hInstance = (HINSTANCE)module;
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
static void sLogSeparatorLine(LOGMACHINESTATEPROC logLineProc, void *logLineProcParam, char character, int longLine) {
  char line[80];
  int  chars;

  chars = longLine ? 0x4E : 0x28;
  memset(line, character, chars);
  line[chars] = 0;
  logLineProc(logLineProcParam, "%s", line);
}
static void sLogHeader(LOGMACHINESTATEPROC logLineProc, void *logLineProcParam, const char *headerString) {
  logLineProc(logLineProcParam, "");
  sLogSeparatorLine(logLineProc, logLineProcParam, '-', 0);
  logLineProc(logLineProcParam, "    %s", headerString);
  sLogSeparatorLine(logLineProc, logLineProcParam, '-', 0);
  logLineProc(logLineProcParam, "");
}
static void
sLogMemoryHexDump(LOGMACHINESTATEPROC logLineProc, void *logLineProcParam, unsigned char *address, unsigned long numBytes, int alignedLines) {
  char           buffer[80];
  unsigned char *row;
  unsigned long  numLines;
  unsigned long  offset;
  int            i;
  char          *cursor;

  row = (unsigned char *)address;
  numLines = numBytes >> 4;
  offset = ((DWORD)address) & 0x0F;

  if (alignedLines) {
    memset(buffer, ' ', 0x4E);
    buffer[0] = '*';
    buffer[2] = '=';
    buffer[4] = 'a';
    buffer[5] = 'd';
    buffer[6] = 'd';
    buffer[7] = 'r';
    buffer[0x4E] = 0;
    buffer[0x0A + offset * 3 + (offset >> 2)] = '*';
    buffer[0x0B + offset * 3 + (offset >> 2)] = '*';
    buffer[0x3E + offset] = '*';
    logLineProc(logLineProcParam, "%s", buffer);

    if (offset) {
      row -= offset;
      ++numLines;
    }
  }

  while (numLines) {
    cursor = buffer + sprintf(buffer, "%08X: ", (DWORD)row);

    if (IsBadReadPtr(row, 0x10)) {
      strcpy(cursor, "<can't read from this address>");
      logLineProc(logLineProcParam, "%s", buffer);
      return;
    }

    for (i = 0; i < 16; i += 4) {
      cursor += sprintf(cursor, "%02X %02X %02X %02X  ", row[i], row[i + 1], row[i + 2], row[i + 3]);
    }

    for (i = 0; i < 16; i++) {
      cursor[i] = isprint(row[i]) ? row[i] : '.';
    }
    cursor[16] = 0;
    logLineProc(logLineProcParam, "%s", buffer);

    row += 0x10;
    --numLines;
  }
}
static void
sLogVerboseMessage(UINT logOptions, LOGMACHINESTATEPROC logLineProc, void *logLineProcParam, const char *format, unsigned long error) {
  if (logOptions & 0x80000000) {
    logLineProc(logLineProcParam, format, error);
  }
}
static const char *sGetPathLeaf(const char *path) {
  const char *leaf;
  const char *slash;

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
static void sLogExeFile(LOGMACHINESTATEPROC logLineProc, void *logLineProcParam) {
  char exeFullPath[MAX_PATH];

  GetModuleFileNameA(NULL, exeFullPath, sizeof(exeFullPath));
  logLineProc(logLineProcParam, "%-10s%s", "Exe:", exeFullPath);
}
static void sLogDateTimeString(LOGMACHINESTATEPROC logLineProc, void *logLineProcParam, const SYSTEMTIME *time) {
  WORD hour;
  char suffix;

  hour = time->wHour;
  suffix = hour < 12 ? 'A' : 'P';
  if (hour <= 12) {
    if (!hour) {
      hour = 12;
    }
  } else {
    hour -= 12;
  }

  logLineProc(
      logLineProcParam, "%-10s%3s %2d, %4d %2d:%02d:%02d.%03d %cM", "Time:", sgMonthString[time->wMonth - 1], time->wDay, time->wYear, hour,
      time->wMinute, time->wSecond, time->wMilliseconds, suffix
  );
}
static void sLogUserName(LOGMACHINESTATEPROC logLineProc, void *logLineProcParam) {
  char  userName[0x101];
  DWORD size;

  size = sizeof(userName);
  if (!GetUserNameA(userName, &size)) {
    SStrCopy(userName, "<unknown>", sizeof(userName));
  }
  logLineProc(logLineProcParam, "%-10s%s", "User:", userName);
}
static void sLogComputerName(LOGMACHINESTATEPROC logLineProc, void *logLineProcParam) {
  char  computerName[0x10];
  DWORD size;

  size = sizeof(computerName);
  if (!GetComputerNameA(computerName, &size)) {
    SStrCopy(computerName, "<unknown>", sizeof(computerName));
  }
  logLineProc(logLineProcParam, "%-10s%s", "Computer:", computerName);
}
static void
sLogMemory(UINT logOptions, LOGMACHINESTATEPROC logLineProc, void *logLineProcParam, unsigned long instructionPointr, unsigned long stackPointer) {
  sLogHeader(logLineProc, logLineProcParam, "Memory Dump");

  if (logOptions & 0x00100000) {
    logLineProc(logLineProcParam, "Code: %d bytes starting at (EIP = %08X)", 0x10, instructionPointr);
    logLineProc(logLineProcParam, "");
    sLogMemoryHexDump(logLineProc, logLineProcParam, (unsigned char *)instructionPointr, 0x10, 0);
    logLineProc(logLineProcParam, "");
    logLineProc(logLineProcParam, "");
  }

  if (logOptions & 0x00200000) {
    logLineProc(logLineProcParam, "Stack: %d bytes starting at (ESP = %08X)", 0x400, stackPointer);
    logLineProc(logLineProcParam, "");
    sLogMemoryHexDump(logLineProc, logLineProcParam, (unsigned char *)stackPointer, 0x400, 1);
    logLineProc(logLineProcParam, "");
    logLineProc(logLineProcParam, "");
  }
}
static int __cdecl sModuleCompareProc(const void *elem1, const void *elem2) {
  if (((const ModuleData *)elem1)->baseAddress > ((const ModuleData *)elem2)->baseAddress) {
    return 1;
  }
  if (((const ModuleData *)elem1)->baseAddress < ((const ModuleData *)elem2)->baseAddress) {
    return -1;
  }
  return 0;
}
static BOOL CALLBACK sEnumModulesCallback(LPSTR ModuleName, ULONG BaseOfDll, PVOID UserContext) {
  if (!ModuleName || !ModuleName[0]) {
    ModuleName = "<unknown>";
  }

  strncpy(((EnumModuleData *)UserContext)->modules[((EnumModuleData *)UserContext)->count].name, ModuleName, 0xFF);
  ((EnumModuleData *)UserContext)->modules[((EnumModuleData *)UserContext)->count].name[0xFF] = 0;
  ((EnumModuleData *)UserContext)->modules[((EnumModuleData *)UserContext)->count].baseAddress = BaseOfDll;

  return ++((EnumModuleData *)UserContext)->count < 0x100;
}
static BOOL CALLBACK sEnumSymbolsCallback(LPSTR SymbolName, ULONG SymbolAddress, ULONG SymbolSize, PVOID UserContext) {
  ((LogLineParams *)UserContext)
      ->logLineProc(
          ((LogLineParams *)UserContext)->logLineProcParam, "    0x%08X: %s (%d : 0x%08X)", SymbolAddress, SymbolName, SymbolSize, SymbolSize
      );
  return TRUE;
}
static void
sLogModule(UINT logOptions, LOGMACHINESTATEPROC logLineProc, void *logLineProcParam, unsigned long baseAddress, char *moduleName) {
  IMAGEHLP_MODULE module;
  LogLineParams   params;

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

    params.logLineProc = logLineProc;
    params.logLineProcParam = logLineProcParam;

    if (!sgDbgHelpDll.SymEnumerateSymbols(GetCurrentProcess(), baseAddress, (PSYM_ENUMSYMBOLS_CALLBACK)sEnumSymbolsCallback, &params)) {
      logLineProc(logLineProcParam, "****  SymEnumerateModules couldn't enumerate symbols, error: %d", GetLastError());
    }

    logLineProc(logLineProcParam, "");
  }
}
static void sGetLogicalAddress(void *addr, char *moduleName, unsigned long moduleNameSize, unsigned long *section, unsigned long *offset) {
  MEMORY_BASIC_INFORMATION memInfo;
  HMODULE                  module;
  PIMAGE_DOS_HEADER        dosHeader;
  PIMAGE_NT_HEADERS        ntHeaders;
  PIMAGE_SECTION_HEADER    sectionHeader;
  DWORD                    rva;
  DWORD                    sectionSize;
  unsigned int             i;

  lstrcpynA(moduleName, "<unknown>", moduleNameSize);
  *section = 0;
  *offset = 0;

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

  rva = (DWORD)((BYTE *)addr - (BYTE *)module);
  sectionHeader = IMAGE_FIRST_SECTION(ntHeaders);

  for (i = 0; i < ntHeaders->FileHeader.NumberOfSections; i++, sectionHeader++) {
    sectionSize = sectionHeader->SizeOfRawData;
    if (sectionSize <= sectionHeader->Misc.VirtualSize) {
      sectionSize = sectionHeader->Misc.VirtualSize;
    }

    if (rva >= sectionHeader->VirtualAddress && rva <= sectionHeader->VirtualAddress + sectionSize) {
      *section = i + 1;
      *offset = rva - sectionHeader->VirtualAddress;
      return;
    }
  }
}
static int sDbgHelpGetStackFrameInfo(
    unsigned long  address,
    char          *moduleName,
    char          *symbolName,
    unsigned long &symbolDisplacement,
    char          *fileName,
    unsigned long &lineNumber
) {
  IMAGEHLP_MODULE module;
  char            buffer[0x118];
  IMAGEHLP_LINE   line;
  DWORD           displacement;
  int             err;

  memset(&module, 0, sizeof(module));
  err = 0;
  module.SizeOfStruct = sizeof(module);

  if (sgDbgHelpDll.SymGetModuleInfo(GetCurrentProcess(), address, &module)) {
    strcpy(moduleName, sGetPathLeaf(module.ModuleName));
  } else {
    strcpy(moduleName, "<unknown module>");
    err |= 1;
  }

  fileName[0] = 0;
  lineNumber = 0;

  memset(buffer, 0, sizeof(buffer));
  ((PIMAGEHLP_SYMBOL)buffer)->SizeOfStruct = 0x18;
  ((PIMAGEHLP_SYMBOL)buffer)->Address = address;
  ((PIMAGEHLP_SYMBOL)buffer)->MaxNameLength = 0x100;

  if (!sgDbgHelpDll.SymGetSymFromAddr(GetCurrentProcess(), address, &displacement, (PIMAGEHLP_SYMBOL)buffer)) {
    strcpy(symbolName, "<unknown symbol>");
    symbolDisplacement = 0;
    return err | 4;
  }

  strcpy(symbolName, ((PIMAGEHLP_SYMBOL)buffer)->Name);
  symbolDisplacement = displacement;

  line.Key = 0;
  line.LineNumber = 0;
  line.FileName = NULL;
  line.Address = 0;
  line.SizeOfStruct = 0x14;

  if (!sgDbgHelpDll.SymGetLineFromAddr(GetCurrentProcess(), address, &displacement, &line)) {
    return err | 2;
  }

  strncpy(fileName, sGetPathLeaf(line.FileName), 0x103);
  fileName[0x103] = 0;
  lineNumber = line.LineNumber;

  return err;
}
static void sLogDbgHelpStackFrame(UINT logOptions, LOGMACHINESTATEPROC logLineProc, void *logLineProcParam, const STACKFRAME *stackFrame) {
  char          symbolName[0x100];
  char          fileName[0x104];
  char          moduleName[0x20];
  unsigned long lineNumber;
  unsigned long symbolDisplacement;
  unsigned long address;
  int           err;

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
    const char *format = fileName[0] ? "%08X %-12s %s+%d (0x%08X,0x%08X,0x%08X,0x%08X) (%s,%d)" : "%08X %-12s %s+%d (0x%08X,0x%08X,0x%08X,0x%08X)";
    logLineProc(
        logLineProcParam, format, address, moduleName, symbolName, symbolDisplacement, stackFrame->Params[0], stackFrame->Params[1],
        stackFrame->Params[2], stackFrame->Params[3], fileName, lineNumber
    );
  } else {
    const char *format = fileName[0] ? "%08X %-12s %s+%d (%s,%d)" : "%08X %-12s %s+%d";
    logLineProc(logLineProcParam, format, address, moduleName, symbolName, symbolDisplacement, fileName, lineNumber);
  }
}
static int sSymInitialize(void *process) {
  char path[0x104];
  path[0] = 0;
  GetModuleFileNameA(NULL, path, sizeof(path));
  char *pathEnd = SStrChrR(path, '\\');
  if (pathEnd) {
    *pathEnd = 0;
  }

  return sgDbgHelpDll.SymInitialize((HANDLE)process, path, TRUE);
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
  STACKFRAME            stackFrame;
  DWORD                 registerEsp;
  int                   maxDepth;
  DWORD                 registerEbp;
  HANDLE                thread;
  DWORD                 registerEip;
  HANDLE                process;
  CONTEXT               context;
  PFN_RTLCAPTURECONTEXT captureContext;
  int                   frameIndex;

  maxDepth = depth;
  depth = 0;

  if (!sQuickStackWalkInit(TRUE)) {
    return FALSE;
  }

  EnterCriticalSection(&s_CrawlCritsect);
  process = GetCurrentProcess();
  thread = GetCurrentThread();

  captureContext = reinterpret_cast<PFN_RTLCAPTURECONTEXT>(GetProcAddress(GetModuleHandleA("ntdll.dll"), "RtlCaptureContext"));
  memset(&context, 0, sizeof(context));
  if (captureContext) {
    captureContext(&context);
    registerEip = context.Eip;
    registerEbp = context.Ebp;
    registerEsp = context.Esp;
  } else {
    registerEip = 0;
    registerEbp = 0;
    registerEsp = 0;
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
  char          fileName[0x104];
  char          symbolName[0x100];
  char          moduleName[0x20];
  char          key[0x20];
  unsigned long symbolDisplacement;
  unsigned long lineNumber;

  if (!sQuickStackWalkInit(TRUE)) {
    return FALSE;
  }

  SErrPauseWatchdog();
  EnterCriticalSection(&s_CrawlCritsect);

  while (depth > 0) {
    SStrPrintf(key, sizeof(key), "<sym-%08X>", *crawlAddrs);
    if (STypeCache::Get(key)) {
      SStrCopy(*crawlNames, STypeCache::Get(key), 0x100);
    } else {
      sDbgHelpGetStackFrameInfo(*crawlAddrs, moduleName, symbolName, symbolDisplacement, fileName, lineNumber);
      SStrPrintf(*crawlNames, 0x100, "%s %s+%d %s(%d)", moduleName, symbolName, symbolDisplacement, fileName, lineNumber);
      STypeCache::Set(key, *crawlNames);
    }

    ++crawlAddrs;
    ++crawlNames;
    --depth;
  }

  LeaveCriticalSection(&s_CrawlCritsect);
  SErrResumeWatchdog();
  return TRUE;
}
static void APIENTRY CmdMemOutput(HOUTPUTCONTEXT hOutput, const char *str) {
  MEMDUMP                *dump;
  SMemReportByCallerInfo *info;

  dump = (MEMDUMP *)hOutput;
  info = (SMemReportByCallerInfo *)str;
  dump->logLineProc(dump->logLineProcParam, "%5d %5d %s(%d)", info->allocatedBlocks, info->allocatedBytes, info->fileName, info->lineNumber);
}
static void sShowOutOfMemory(LOGMACHINESTATEPROC logLineProc, void *logLineProcParam) {
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
    void               *logLineProcParam,
    unsigned long       registerEip,
    unsigned long       registerEbp,
    unsigned long       registerEsp,
    unsigned int        stackFramesToSkip
) {
  HANDLE         process;
  HANDLE         thread;
  STACKFRAME     stackFrame;
  unsigned int   frameIndex;
  EnumModuleData data;
  DWORD          moduleIndex;
  BOOL           success;

  sLogHeader(logLineProc, logLineProcParam, "Stack Trace (Using DBGHELP.DLL)");

  process = GetCurrentProcess();
  if (!sgDbgHelpDll.Load()) {
    logLineProc(logLineProcParam, "****  Couldn't load DBGHELP.DLL, error: %d", GetLastError());
    sgDbgHelpDll.Unload();
    logLineProc(logLineProcParam, "");
    return;
  }

  if (sgDbgHelpDll.SymGetOptions && sgDbgHelpDll.SymSetOptions) {
    sgDbgHelpDll.SymSetOptions(sgDbgHelpDll.SymGetOptions() | 0x14);
  }

  if (!sSymInitialize(process)) {
    logLineProc(logLineProcParam, "****  Couldn't initialize Debug Help library, error: %d", GetLastError());
    sgDbgHelpDll.Unload();
    logLineProc(logLineProcParam, "");
    return;
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

    for (frameIndex = 0; frameIndex < 0x64; frameIndex++) {
      if (!sgDbgHelpDll.StackWalk || !sgDbgHelpDll.StackWalk(
                                         IMAGE_FILE_MACHINE_I386, process, thread, &stackFrame, NULL, NULL, sgDbgHelpDll.SymFunctionTableAccess,
                                         sgDbgHelpDll.SymGetModuleBase, NULL
                                     ))
      {
        sLogVerboseMessage(logOptions, logLineProc, logLineProcParam, "**** StackWalk() returned FALSE, error: %d", GetLastError());
        break;
      }

      if (!stackFrame.AddrPC.Offset) {
        sLogVerboseMessage(logOptions, logLineProc, logLineProcParam, "**** StackWalk() returned zero address - skipping stack frame", 0);
      } else if (frameIndex >= stackFramesToSkip) {
        sLogDbgHelpStackFrame(logOptions, logLineProc, logLineProcParam, &stackFrame);
      }
    }
  }

  if (logOptions & 0x00400000) {
    logLineProc(logLineProcParam, "");
    sLogHeader(logLineProc, logLineProcParam, "Loaded Modules");

    data.count = 0;
    success = FALSE;
    if (sgDbgHelpDll.SymEnumerateModules) {
      success = sgDbgHelpDll.SymEnumerateModules(process, (PSYM_ENUMMODULES_CALLBACK)sEnumModulesCallback, &data);
    }

    qsort(data.modules, data.count, sizeof(data.modules[0]), sModuleCompareProc);

    for (moduleIndex = 0; moduleIndex < data.count; moduleIndex++) {
      sLogModule(logOptions, logLineProc, logLineProcParam, data.modules[moduleIndex].baseAddress, data.modules[moduleIndex].name);
    }

    if (!success) {
      logLineProc(logLineProcParam, "****  SymEnumerateModules couldn't enumerate modules, error: %d", GetLastError());
    }
  }

  sgDbgHelpDll.Unload();
  logLineProc(logLineProcParam, "");
}
static void sLogX86ManualStackTrace(
    UINT                __formal,
    LOGMACHINESTATEPROC logLineProc,
    void               *logLineProcParam,
    unsigned long       registerEip,
    unsigned long       registerEbp,
    unsigned int        stackFramesToSkip
) {
  char          modulePath[0x104];
  unsigned long section;
  unsigned long offset;
  unsigned int  i;

  sLogHeader(logLineProc, logLineProcParam, "Stack Trace (Manual)");
  logLineProc(logLineProcParam, "%s", "Address  Frame    Logical addr  Module");
  logLineProc(logLineProcParam, "");

  i = 0;

  while (i < 0x64) {
    if (i >= stackFramesToSkip) {
      sGetLogicalAddress((void *)registerEip, modulePath, sizeof(modulePath), &section, &offset);
      logLineProc(logLineProcParam, "%08X %08X %04X:%08X %s", registerEip, registerEbp, section, offset, modulePath);
    }

    if (IsBadWritePtr((void *)registerEbp, 8)) {
      return;
    }

    registerEip = ((unsigned long *)registerEbp)[1];

    if ((((unsigned long *)registerEbp)[0] & 3) || ((unsigned long *)registerEbp)[0] <= registerEbp ||
        IsBadWritePtr((void *)((unsigned long *)registerEbp)[0], 8))
    {
      return;
    }

    registerEbp = ((unsigned long *)registerEbp)[0];
    ++i;
  }
}
static void sLogX86ContextRegisters(LOGMACHINESTATEPROC logLineProc, void *logLineProcParam, CONTEXT *context) {
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
void
LogComputerInfoHeader(UINT logOptions, LOGMACHINESTATEPROC logLineProc, void *logLineProcParam, const char *headerTitleLine, SYSTEMTIME *time) {
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
void LogMachineState(UINT logOptions, LOGMACHINESTATEPROC logLineProc, void *logLineProcParam, UINT stackFramesToSkip, CONTEXT *context) {
#if defined(_M_IX86) || defined(_X86_)
  DWORD                 registerEip;
  DWORD                 registerEbp;
  DWORD                 registerEsp;
  CONTEXT               capturedContext;
  PFN_RTLCAPTURECONTEXT captureContext;
#endif

  if (InterlockedIncrement(&sgRecursionLevel) == 1) {
    if (!logOptions || (logOptions & 0x00000001)) {
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
      captureContext = reinterpret_cast<PFN_RTLCAPTURECONTEXT>(GetProcAddress(GetModuleHandleA("ntdll.dll"), "RtlCaptureContext"));
      memset(&capturedContext, 0, sizeof(capturedContext));
      if (captureContext) {
        captureContext(&capturedContext);
        registerEip = capturedContext.Eip;
        registerEbp = capturedContext.Ebp;
        registerEsp = capturedContext.Esp;
      } else {
        registerEip = 0;
        registerEbp = 0;
        registerEsp = 0;
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
  }
  InterlockedDecrement(&sgRecursionLevel);
}
static DWORD WINAPI MiniDumpThreadProc(void *param) {
  MINIDUMP_USER_STREAM             miniDumpUserStreamArray[16];
  MINIDUMP_EXCEPTION_INFORMATION   miniDumpExceptionInfo;
  MINIDUMP_USER_STREAM_INFORMATION miniDumpUserStreamInfo;

  if (!sgDbgHelpDll.Load()) {
    ((MiniDumpParam *)param)->result = FALSE;
    return 0;
  }

  if (sgDbgHelpDll.MiniDumpWriteDump) {
    miniDumpExceptionInfo.ThreadId = ((MiniDumpParam *)param)->threadId;
    miniDumpExceptionInfo.ExceptionPointers = ((MiniDumpParam *)param)->exceptionPointers;
    miniDumpExceptionInfo.ClientPointers = FALSE;

    miniDumpUserStreamInfo.UserStreamCount = 0;
    for (UINT i = 0; i < ((MiniDumpParam *)param)->userStringCount; i++) {
      if (((MiniDumpParam *)param)->userStrings[i] && miniDumpUserStreamInfo.UserStreamCount < 16) {
        miniDumpUserStreamArray[miniDumpUserStreamInfo.UserStreamCount].Type = miniDumpUserStreamInfo.UserStreamCount + 0x1000;
        miniDumpUserStreamArray[miniDumpUserStreamInfo.UserStreamCount].BufferSize = strlen(((MiniDumpParam *)param)->userStrings[i]) + 1;
        miniDumpUserStreamArray[miniDumpUserStreamInfo.UserStreamCount].Buffer = ((MiniDumpParam *)param)->userStrings[i];
        miniDumpUserStreamInfo.UserStreamCount++;
      }
    }

    miniDumpUserStreamInfo.UserStreamArray = miniDumpUserStreamArray;

    ((MiniDumpParam *)param)->result = sgDbgHelpDll.MiniDumpWriteDump(
        GetCurrentProcess(), GetCurrentProcessId(), ((MiniDumpParam *)param)->logfile, MiniDumpWithIndirectlyReferencedMemory,
        ((MiniDumpParam *)param)->exceptionPointers ? &miniDumpExceptionInfo : NULL, &miniDumpUserStreamInfo, NULL
    );
  } else {
    ((MiniDumpParam *)param)->result = FALSE;
  }

  sgDbgHelpDll.Unload();
  return 0;
}
int LogMiniDump(void *logfile, EXCEPTION_POINTERS *exceptionPointers, UINT userStringCount, char **const userStrings) {
  MiniDumpParam miniDumpParam;
  DWORD         threadid;

  miniDumpParam.result = FALSE;
  threadid = 0;

  if (InterlockedIncrement(&sgRecursionLevel) == 1) {
    miniDumpParam.logfile = (HANDLE)logfile;
    miniDumpParam.exceptionPointers = exceptionPointers;
    miniDumpParam.threadId = GetCurrentThreadId();
    miniDumpParam.userStringCount = userStringCount;
    miniDumpParam.userStrings = userStrings;

    HANDLE thread = CreateThread(NULL, 0, MiniDumpThreadProc, &miniDumpParam, 0, &threadid);
    if (thread) {
      WaitForSingleObject(thread, INFINITE);
      CloseHandle(thread);
    }
  }

  InterlockedDecrement(&sgRecursionLevel);
  return miniDumpParam.result;
}
int LogMiniDumpIsAvailable() {
  int loaded;
  int available;

  available = FALSE;
  loaded = sgDbgHelpDll.Load();
  if (!loaded) {
    return FALSE;
  }

  available = sgDbgHelpDll.MiniDumpWriteDump != NULL;
  sgDbgHelpDll.Unload();

  return available;
}
