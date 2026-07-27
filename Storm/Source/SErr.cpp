#define STORM_SAPIBASE_IMPLEMENTATION

#include <storm.h>
#include <stpl.h>

#include <malloc.h>
#include <stdarg.h>

#define MAX_ERR_THREADS 64

int CheckMachineStateSymbolHelper();
void LoadMachineStateSymbols();
void UnloadMachineStateSymbols();
int LogMiniDump(void *file, EXCEPTION_POINTERS *exceptionPointers, UINT userStreamCount, char **const userStreams);
int LogMiniDumpIsAvailable();

typedef struct _MSGSRC {
  WORD      facility;
  WORD      reserved;
  HINSTANCE module;
  _MSGSRC  *next;
} MSGSRC;

struct APPFATINFO {
  LPCSTR filename;
  int    linenumber;
  DWORD  threadId;
};

struct HANDLER : public TSLinkedNode<HANDLER> {
  SERRHANDLER handler;
};

template <class T, class GETLINK>
class TSListWinHeap : public TSList<T, GETLINK> {
 public:
  T *NewNode(unsigned long location, unsigned long extrabytes, unsigned long flags);
  T *DeleteNode(T *ptr);
};

template <>
void TSList<HANDLER, TSGetLink<HANDLER> >::Clear();

template <>
void TSList<HANDLER, TSGetLink<HANDLER> >::LinkNode(HANDLER *ptr, unsigned long linktype, HANDLER *existingptr);

template <>
HANDLER *TSList<HANDLER, TSGetLink<HANDLER> >::Next(const HANDLER *ptr);

static CRITICAL_SECTION                            s_critsect;
static CRITICAL_SECTION                            s_exceptioncritsect;
static CRITICAL_SECTION                            s_threadscritsec;
static LONG                                        s_critsectinit = -1;
static TSListWinHeap<HANDLER, TSGetLink<HANDLER> > s_handlerlist;
static MSGSRC                                     *s_msgsrchead;
static DWORD                                       s_lasterror;
static BOOL                                        s_msgsrcinit;
static BOOL                                        s_suppress;
static BOOL                                        s_displaying;
static APPFATINFO                                  s_appFatInfo;
static char                                        s_logtitle[0x80];
static SERRLOGCALLBACK                             s_logCallback;
static char                                        s_logLastFile[MAX_PATH];
static HANDLE                                      s_threads[MAX_ERR_THREADS];
static DWORD                                       s_threadids[MAX_ERR_THREADS];
static int                                         s_numthreads;
static LONG                                        s_watchdogpaused;
static BOOL                                        s_watchdogrunning;
static DWORD                                       s_freezeperiod;
static BOOL                                        s_checkkeyboard;
static DWORD                                       s_pingcounter;
static HANDLE                                      s_watchdogevent;
static HANDLE                                      s_watchdogthread;
static DWORD                                       s_keysweredown;
static DWORD                                       s_secondsfrozen;
static DWORD                                       debugmode;
static DWORD                                       s_noMiniDumps;
static BOOL                                        checked;
static DWORD                                       s_debugMemory;
static EXCEPTION_POINTERS                         *s_exceptionPointers;
static char                                        buffer[0x100];

template <>
void TSList<HANDLER, TSGetLink<HANDLER> >::Clear() {
  HANDLER *ptr;

  while ((ptr = Head()) != 0) {
    UnlinkNode(ptr);
    ptr->~HANDLER();
    HeapFree(GetProcessHeap(), 0, ptr);
  }
}

template <>
void TSList<HANDLER, TSGetLink<HANDLER> >::LinkNode(HANDLER *ptr, unsigned long linktype, HANDLER *existingptr) {
  TSLink<HANDLER> *link;
  TSLink<HANDLER> *existing;

  link = Link(ptr);
  existing = Link(existingptr);
  if (link->m_prevlink) {
    link->Unlink();
  }

  if (linktype == LIST_LINK_AFTER) {
    TSLink<HANDLER> *nextlink;

    nextlink = existing->NextLink(m_linkoffset);
    link->m_prevlink = existing;
    link->m_next = existing->m_next;
    nextlink->m_prevlink = link;
    existing->m_next = ptr;
  } else {
    TSLink<HANDLER> *previous;

    if (linktype != LIST_LINK_BEFORE) {
      FATALERROR(("Invalid case: %s=%u", "linktype", linktype));
    }

    previous = existing->m_prevlink;
    link->m_prevlink = previous;
    link->m_next = previous->m_next;
    previous->m_next = ptr;
    existing->m_prevlink = link;
  }
}

template <>
HANDLER *TSList<HANDLER, TSGetLink<HANDLER> >::Next(const HANDLER *ptr) {
  return Link(ptr)->Next();
}

template <>
HANDLER *TSListWinHeap<HANDLER, TSGetLink<HANDLER> >::NewNode(unsigned long location, unsigned long extrabytes, unsigned long flags) {
  HANDLER *ptr;

  ptr = new (HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS, sizeof(HANDLER) + extrabytes)) HANDLER;
  memset(ptr, 0, sizeof(HANDLER) + extrabytes);
  if (location) {
    LinkNode(ptr, location, 0);
  }

  return ptr;
}

template <>
HANDLER *TSListWinHeap<HANDLER, TSGetLink<HANDLER> >::DeleteNode(HANDLER *ptr) {
  HANDLER *next;

  next = Next(ptr);
  ptr->~HANDLER();
  HeapFree(GetProcessHeap(), 0, ptr);
  return next;
}

static DWORD WINAPI    WatchdogThreadProc(LPVOID __formal);
static void CheckKeyboard();
static void LogThreads(HANDLE *threads, LPDWORD threadids, int numthreads, LPCSTR description, LPCSTR suffix);
static LONG WINAPI     ExceptionFilterWin32(EXCEPTION_POINTERS *exceptionpointers);
static void InternalEnterCriticalSection(CRITICAL_SECTION *crit);
static void InternalLeaveCriticalSection(CRITICAL_SECTION *crit);
static void UnregisterAllThreads();
static void Breakpoint();

#define SErrEnter()          InternalEnterCriticalSection(&s_critsect)
#define SErrLeave()          InternalLeaveCriticalSection(&s_critsect)
#define SErrExceptionEnter() InternalEnterCriticalSection(&s_exceptioncritsect)
#define SErrExceptionLeave() InternalLeaveCriticalSection(&s_exceptioncritsect)
#define SErrThreadsEnter()   InternalEnterCriticalSection(&s_threadscritsec)
#define SErrThreadsLeave()   InternalLeaveCriticalSection(&s_threadscritsec)

#ifndef FORMAT_MESSAGE_IGNORE_INSERTS
#define FORMAT_MESSAGE_IGNORE_INSERTS 0x00000200
#endif

static LPCSTR const s_displaystr[] = {
    "ERROR #%u (0x%08x)",
    "This application has encountered a critical error:\n\n%s\n",
    "Program:\t%s\n",
    "File:\t%s\nLine:\t%d\n",
    "Function:\t%s\n",
    "Object:\t%s\n",
    "Handle:\t%s\n",
    "Expr:\t%s\n\n",
    "\n%s\n\n",
    "Press OK to terminate the application.",
    "Do you wish to keep running anyway?\n\tOK:\tKeep running\n\tCancel:\tTerminate application",
    "File:\t%s\n",
    "Do you wish to break to the debugger?\n\tYes:\tBreak to debugger\n\tNo:\tTerminate application",
    "Do you wish to break to the debugger?\n\tYes:\tBreak to debugger\n\tNo:\tTerminate application\n\tCancel:\tKeep running",
    "Exception:\t%s\n",
    "The instruction at \"0x%08X\" referenced memory at \"0x%08X\".\nThe memory could not be \"%s\".",
    "read",
    "written",
    "Missing Debugging DLL",
    "Couldn't locate the \"DbgHelp.dll\" debugging DLL.\n\nSome debugging information will be missing from error log files. Press OK to continue, or "
    "press Cancel to terminate the program."
};

static LPCSTR GetErrorString(UINT id) {
  if (LoadStringA(StormGetInstance(), id + 0x5100, buffer, sizeof(buffer))) {
    return buffer;
  }

  if (id < sizeof(s_displaystr) / sizeof(s_displaystr[0])) {
    return s_displaystr[id];
  }

  return "";
}

static void AddStormFacility(WORD facility) {
  MSGSRC **link;
  MSGSRC  *source;

  link = &s_msgsrchead;
  while (*link) {
    link = &(*link)->next;
  }

  source = (MSGSRC *)HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS, sizeof(MSGSRC));
  *link = source;
  source->facility = facility;
  source->module = StormGetInstance();
  source->next = NULL;
}

static void AddStormMessages() {
  AddStormFacility(0x510);
  AddStormFacility(0x876);
  AddStormFacility(0x878);
}

static void Breakpoint() {
#if defined(_MSC_VER) && defined(_M_IX86)
  __try {
    DebugBreak();
  } __except (EXCEPTION_EXECUTE_HANDLER) {
  }
#endif
}

static void InternalEnterCriticalSection(CRITICAL_SECTION *critsect) {
  if (!InterlockedIncrement(&s_critsectinit)) {
    InitializeCriticalSection(&s_critsect);
    InitializeCriticalSection(&s_exceptioncritsect);
    InitializeCriticalSection(&s_threadscritsec);
  } else {
    InterlockedDecrement(&s_critsectinit);
  }
  EnterCriticalSection(critsect);
}

static void InternalLeaveCriticalSection(CRITICAL_SECTION *critsect) {
  LeaveCriticalSection(critsect);
}

static int UndecorateObjectName(const char *source, char *dest, DWORD destchars) {
  char *end;

  if (!source || !dest || SStrLen(source) < 6 || source[0] != '.' || source[3] != 'U') {
    return FALSE;
  }

  source += 4;
  if (!strpbrk(source, "?@")) {
    return FALSE;
  }

  SStrCopy(dest, source, destchars);
  end = strpbrk(dest, "?@");
  if (!end) {
    return FALSE;
  }
  *end = 0;

  while (end > dest && end[-1] == '_') {
    --end;
    *end = 0;
  }

  SStrPack(dest, " (", destchars);
  SStrPack(dest, source - 4, destchars);
  SStrPack(dest, ")", destchars);
  return TRUE;
}

void SErrInitialize() {
  SRegLoadValue("Internal", "Debug Error Output", 0, &debugmode);
  SRegLoadValue("Internal", "No Minidumps", 0, &s_noMiniDumps);
  SErrRegisterThread(GetCurrentThread(), GetCurrentThreadId());
}

static int CanBreakToDebugger() {
  HMODULE kernel;
  FARPROC proc;
  int     present;

  present = FALSE;
  kernel = LoadLibraryA("KERNEL32.DLL");
  if (!kernel) {
    return FALSE;
  }

  proc = GetProcAddress(kernel, "IsDebuggerPresent");
  if (proc) {
    present = ((BOOL(WINAPI *)(void))proc)();
  }

  FreeLibrary(kernel);
  return present;
}

static int MakeDirectory(LPCSTR pszFullPath) {
  DWORD attributes;
  DWORD error;

  if (CreateDirectoryA(pszFullPath, NULL)) {
    return TRUE;
  }

  error = GetLastError();
  if (error != ERROR_ALREADY_EXISTS && error != ERROR_ACCESS_DENIED) {
    return FALSE;
  }

  attributes = GetFileAttributesA(pszFullPath);
  return attributes != (DWORD)-1 && (attributes & FILE_ATTRIBUTE_DIRECTORY);
}

static void __cdecl WriteLine(void *param, const char *format, ...) {
  char    buffer[0x800];
  DWORD   written;
  va_list args;

  if (!format) {
    format = "";
  }

  va_start(args, format);
  _vsnprintf(buffer, sizeof(buffer) - 3, format, args);
  va_end(args);
  buffer[sizeof(buffer) - 3] = 0;
  SStrPack(buffer, "\r\n", sizeof(buffer));
  written = 0;
  WriteFile((HANDLE)param, buffer, SStrLen(buffer), &written, NULL);
}

static void WriteMessageToLog(HANDLE logfile, LPCSTR message) {
  DWORD byteswritten;
  char  prevc;
  DWORD length;
  char *buffer;
  DWORD i;
  DWORD out;

  length = strlen(message);
  buffer = (char *)_alloca(length * 2 + 4);
  buffer[0] = '\r';
  buffer[1] = '\n';
  out = 2;
  prevc = 0;

  for (i = 0; i < length; i++) {
    char ch = message[i];
    if (ch == '\n' && prevc != '\r') {
      buffer[out++] = '\r';
    }
    buffer[out++] = ch;
    prevc = ch;
  }

  if (prevc != '\n') {
    buffer[out++] = '\r';
    buffer[out++] = '\n';
  }

  buffer[out] = 0;
  WriteFile(logfile, buffer, out, &byteswritten, NULL);
}

static HANDLE CreateErrorLogFile(LPCSTR suffix, LPCSTR ext, char *logpath, DWORD logpathchars, SYSTEMTIME &time) {
  char  logfilename[MAX_PATH];
  char  exefullpath[MAX_PATH];
  char *pathend;

  if (!suffix) {
    suffix = " Log";
  }

  GetModuleFileNameA(NULL, exefullpath, sizeof(exefullpath));
  SStrCopy(logpath, exefullpath, logpathchars);
  pathend = SStrChrR(logpath, '\\');
  if (pathend) {
    pathend[1] = 0;
  } else {
    pathend = logpath + SStrLen(logpath);
  }

  SStrPack(logpath, "Errors\\", logpathchars);
  if (!MakeDirectory(logpath)) {
    pathend[1] = 0;
  }

  sprintf(
      logfilename, "%04d-%02d-%02d %02d.%02d.%02d %s.%s", time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond, suffix, ext
  );
  SStrPack(logpath, logfilename, logpathchars);

  return CreateFileA(logpath, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
}

static HANDLE CreateErrorLog(LPCSTR suffix, LPCSTR message, SYSTEMTIME *time) {
  HANDLE     file;
  char       logpath[MAX_PATH];
  SYSTEMTIME localTime;

  if (!time) {
    GetLocalTime(&localTime);
    time = &localTime;
  }

  file = CreateErrorLogFile(suffix, "txt", logpath, sizeof(logpath), *time);
  if (file == INVALID_HANDLE_VALUE) {
    return INVALID_HANDLE_VALUE;
  }

  LogComputerInfoHeader(1, WriteLine, file, s_logtitle, time);
  WriteMessageToLog(file, message);
  if (s_logCallback) {
    char *extra = (char *)_alloca(0x1450);
    extra[0] = 0;
    s_logCallback(extra, 0x1450);
    if (SStrLen(extra)) {
      WriteMessageToLog(file, extra);
    }
  }
  SStrCopy(s_logLastFile, logpath, sizeof(s_logLastFile));

  return file;
}

static void LogContext(HANDLE logfile, int framestoskip, CONTEXT *context) {
  LogMachineState(0x40001, WriteLine, logfile, framestoskip, context);
}

static void CloseErrorLog(HANDLE logfile) {
  if (logfile != INVALID_HANDLE_VALUE) {
    CloseHandle(logfile);
  }
}

static void GetExceptionNameWin32(DWORD exceptioncode, char *buffer, DWORD buffersize) {
  switch (exceptioncode) {
    case EXCEPTION_ACCESS_VIOLATION:
      SStrCopy(buffer, "ACCESS_VIOLATION", buffersize);
      return;
    case EXCEPTION_DATATYPE_MISALIGNMENT:
      SStrCopy(buffer, "DATATYPE_MISALIGNMENT", buffersize);
      return;
    case EXCEPTION_BREAKPOINT:
      SStrCopy(buffer, "BREAKPOINT", buffersize);
      return;
    case EXCEPTION_SINGLE_STEP:
      SStrCopy(buffer, "SINGLE_STEP", buffersize);
      return;
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
      SStrCopy(buffer, "ARRAY_BOUNDS_EXCEEDED", buffersize);
      return;
    case EXCEPTION_FLT_DENORMAL_OPERAND:
      SStrCopy(buffer, "FLT_DENORMAL_OPERAND", buffersize);
      return;
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
      SStrCopy(buffer, "FLT_DIVIDE_BY_ZERO", buffersize);
      return;
    case EXCEPTION_FLT_INEXACT_RESULT:
      SStrCopy(buffer, "FLT_INEXACT_RESULT", buffersize);
      return;
    case EXCEPTION_FLT_INVALID_OPERATION:
      SStrCopy(buffer, "FLT_INVALID_OPERATION", buffersize);
      return;
    case EXCEPTION_FLT_OVERFLOW:
      SStrCopy(buffer, "FLT_OVERFLOW", buffersize);
      return;
    case EXCEPTION_FLT_STACK_CHECK:
      SStrCopy(buffer, "FLT_STACK_CHECK", buffersize);
      return;
    case EXCEPTION_FLT_UNDERFLOW:
      SStrCopy(buffer, "FLT_UNDERFLOW", buffersize);
      return;
    case EXCEPTION_ILLEGAL_INSTRUCTION:
      SStrCopy(buffer, "ILLEGAL_INSTRUCTION", buffersize);
      return;
    case EXCEPTION_IN_PAGE_ERROR:
      SStrCopy(buffer, "IN_PAGE_ERROR", buffersize);
      return;
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
      SStrCopy(buffer, "INT_DIVIDE_BY_ZERO", buffersize);
      return;
    case EXCEPTION_INT_OVERFLOW:
      SStrCopy(buffer, "INT_OVERFLOW", buffersize);
      return;
    case EXCEPTION_INVALID_DISPOSITION:
      SStrCopy(buffer, "INVALID_DISPOSITION", buffersize);
      return;
    case EXCEPTION_NONCONTINUABLE_EXCEPTION:
      SStrCopy(buffer, "NONCONTINUABLE_EXCEPTION", buffersize);
      return;
    case EXCEPTION_PRIV_INSTRUCTION:
      SStrCopy(buffer, "PRIV_INSTRUCTION", buffersize);
      return;
    case EXCEPTION_STACK_OVERFLOW:
      SStrCopy(buffer, "STACK_OVERFLOW", buffersize);
      return;
    case EXCEPTION_GUARD_PAGE:
      SStrCopy(buffer, "GUARD_PAGE", buffersize);
      return;
    case EXCEPTION_INVALID_HANDLE:
      SStrCopy(buffer, "INVALID_HANDLE", buffersize);
      return;
  }

  HMODULE ntdll = GetModuleHandleA("ntdll.dll");
  if (!ntdll || !FormatMessageA(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS, ntdll, exceptioncode, 0, buffer, buffersize, NULL)) {
    SStrCopy(buffer, "unknown exception", buffersize);
  }
}

static LONG WINAPI ExceptionFilterWin32(EXCEPTION_POINTERS *exceptionpointers) {
  char format[0x100];
  char message[0x100];
  char buffer[0x50];
  char exceptionname[0x28];

  GetExceptionNameWin32(exceptionpointers->ExceptionRecord->ExceptionCode, exceptionname, sizeof(exceptionname));
  SStrPrintf(
      buffer, sizeof(buffer), "0x%08X (%s) at %04X:%08X", exceptionpointers->ExceptionRecord->ExceptionCode, exceptionname,
      exceptionpointers->ContextRecord->SegCs, exceptionpointers->ExceptionRecord->ExceptionAddress
  );
  message[0] = 0;

  if (exceptionpointers->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && exceptionpointers->ExceptionRecord->NumberParameters == 2) {
    SStrCopy(format, GetErrorString(15), sizeof(format));
    SStrPrintf(
        message, sizeof(message), format, exceptionpointers->ExceptionRecord->ExceptionAddress,
        exceptionpointers->ExceptionRecord->ExceptionInformation[1],
        GetErrorString(exceptionpointers->ExceptionRecord->ExceptionInformation[0] ? 17 : 16)
    );
  }

  SErrExceptionEnter();
  if (!s_exceptionPointers) {
    s_exceptionPointers = exceptionpointers;
  }
  SErrExceptionLeave();

  SErrDisplayError(0x85100084, buffer, SERR_LINECODE_EXCEPTION, message, FALSE, 1);
  return EXCEPTION_CONTINUE_SEARCH;
}

extern "C" BOOL APIENTRY SErrCheckDebugSymbolLibrary(BOOL warnnotfound) {
  BOOL available;

  available = CheckMachineStateSymbolHelper();
  if (!available && warnnotfound) {
    HWND window;
    char title[40];

    window = SMsgGetDefaultWindow();
    if (window && (!IsWindow(window) || !IsWindowVisible(window))) {
      window = NULL;
    }

    SStrCopy(title, GetErrorString(19), sizeof(title));
    if (MessageBoxA(window, GetErrorString(18), title, 0x52031) == IDCANCEL) {
      ExitProcess(1);
    }
  }

  return available;
}

extern "C" BOOL APIENTRY SErrDestroy() {
  MSGSRC *source;

  s_handlerlist.Clear();

  s_msgsrcinit = FALSE;
  source = s_msgsrchead;
  while (source) {
    MSGSRC *next = source->next;
    HeapFree(GetProcessHeap(), 0, source);
    source = next;
  }
  s_msgsrchead = NULL;

  UnregisterAllThreads();

  if (s_critsectinit != -1) {
    DeleteCriticalSection(&s_critsect);
    DeleteCriticalSection(&s_exceptioncritsect);
    DeleteCriticalSection(&s_threadscritsec);
    s_critsectinit = -1;
  }

  return TRUE;
}

extern "C" void __cdecl SErrDisplayAppFatal(LPCSTR format, ...) {
  char    buffer[0x800];
  va_list args;
  LPCSTR  file;
  int     line;

  file = NULL;
  line = 0;
  SErrEnter();
  if (s_appFatInfo.threadId == GetCurrentThreadId()) {
    file = s_appFatInfo.filename;
    line = s_appFatInfo.linenumber;
    s_appFatInfo.filename = NULL;
    s_appFatInfo.linenumber = 0;
    s_appFatInfo.threadId = 0;
  }
  SErrLeave();

  va_start(args, format);
  _vsnprintf(buffer, sizeof(buffer) - 1, format, args);
  va_end(args);
  buffer[sizeof(buffer) - 1] = 0;

  SErrDisplayError(0x85100084, file, line, buffer, FALSE, 1);
  ExitProcess(1);
}

extern "C" BOOL APIENTRY SErrDisplayError(DWORD errorcode, LPCSTR filename, int linenumber, LPCSTR description, BOOL recoverable, UINT exitcode) {
  HANDLER *handler;
  char     localnamebuffer[0x100];
  union {
    WIN32_FIND_DATAA finddata;
    char             logpath[MAX_PATH];
  };
  char                outstr[0x800];
  char                appfilename[MAX_PATH];
  char                appname[MAX_PATH];
  char                errorstr[0x100];
  SYSTEMTIME          time;
  char               *cursor;
  LPCSTR              localnameptr;
  LPCSTR              effectiveDescription;
  EXCEPTION_POINTERS *exceptionPointers;
  DWORD               terminationcode;
  LPCSTR              suffix;
  char               *userStrings[1];
  HANDLE              logfile;
  HWND                window;
  UINT                messageFlags;
  int                 defaultResult;
  int                 breakResult;
  int                 continueResult;
  int                 result;
  BOOL                canBreak;

  if (s_suppress || s_displaying) {
    return FALSE;
  }

  s_displaying = TRUE;
  appfilename[0] = 0;
  appname[0] = 0;
  GetModuleFileNameA(NULL, appfilename, sizeof(appfilename));
  memset(&finddata, 0, sizeof(finddata));
  logfile = FindFirstFileA(appfilename, &finddata);
  if (logfile != INVALID_HANDLE_VALUE) {
    FindClose(logfile);
    SStrCopy(appname, finddata.cFileName, sizeof(appname));
  }
  cursor = SStrChrR(appname, '.');
  if (cursor) {
    *cursor = 0;
  }

  errorstr[0] = 0;
  SErrGetErrorStr(errorcode, errorstr, sizeof(errorstr));
  if (!errorstr[0]) {
    SStrPrintf(errorstr, sizeof(errorstr), GetErrorString(0), errorcode & 0xFFFF, errorcode);
  }

  localnameptr = filename;
  if (filename && *filename && (linenumber == -2 || linenumber == -3) && UndecorateObjectName(filename, localnamebuffer, sizeof(localnamebuffer))) {
    localnameptr = localnamebuffer;
  }

  cursor = outstr;
  if (localnameptr) {
    SStrCopy(outstr, localnameptr, sizeof(outstr));
    cursor += SStrLen(cursor);
  }
  if (linenumber > 0) {
    SStrPrintf(cursor, sizeof(outstr) - (cursor - outstr), "(%d)", linenumber);
    cursor += SStrLen(cursor);
  }
  SStrPrintf(cursor, sizeof(outstr) - (cursor - outstr), " : error %u: ", errorcode & 0xFFFF);
  cursor += SStrLen(cursor);
  SStrCopy(cursor, errorstr, sizeof(outstr) - (cursor - outstr));

  OutputDebugStringA(outstr);
  effectiveDescription = description ? description : "";

  SStrPrintf(outstr, sizeof(outstr), GetErrorString(1), errorstr);
  cursor = outstr + SStrLen(outstr);
  SStrPrintf(cursor, sizeof(outstr) - (cursor - outstr), GetErrorString(2), appfilename);
  cursor += SStrLen(cursor);
  if (localnameptr && *localnameptr) {
    switch (linenumber) {
      case SERR_LINECODE_EXCEPTION:
        SStrPrintf(cursor, sizeof(outstr) - (cursor - outstr), GetErrorString(14), localnameptr);
        break;
      case -4:
        SStrPrintf(cursor, sizeof(outstr) - (cursor - outstr), GetErrorString(11), localnameptr);
        break;
      case -3:
        SStrPrintf(cursor, sizeof(outstr) - (cursor - outstr), GetErrorString(6), localnameptr);
        break;
      case -2:
        SStrPrintf(cursor, sizeof(outstr) - (cursor - outstr), GetErrorString(5), localnameptr);
        break;
      case -1:
        SStrPrintf(cursor, sizeof(outstr) - (cursor - outstr), GetErrorString(4), localnameptr);
        break;
      default:
        SStrPrintf(cursor, sizeof(outstr) - (cursor - outstr), GetErrorString(3), localnameptr, linenumber);
        break;
    }
    cursor += SStrLen(cursor);
  }

  if (errorcode == STORM_ERROR_ASSERTION) {
    SStrPrintf(cursor, sizeof(outstr) - (cursor - outstr), GetErrorString(7), effectiveDescription);
  } else {
    SStrPrintf(cursor, sizeof(outstr) - (cursor - outstr), GetErrorString(8), effectiveDescription);
  }
  cursor += SStrLen(cursor);

  if (!g_opt.serrsuppresslogs) {
    LoadMachineStateSymbols();

    SErrExceptionEnter();
    exceptionPointers = s_exceptionPointers;
    s_exceptionPointers = NULL;
    SErrExceptionLeave();

    GetLocalTime(&time);
    suffix = exceptionPointers && exceptionPointers->ContextRecord ? "Crash" : "Error";

    logfile = CreateErrorLog(suffix, outstr, &time);
    if (logfile != INVALID_HANDLE_VALUE) {
      LogContext(logfile, exceptionPointers && exceptionPointers->ContextRecord ? 0 : 2, exceptionPointers ? exceptionPointers->ContextRecord : NULL);
      CloseErrorLog(logfile);
    }

    if (LogMiniDumpIsAvailable() && !s_noMiniDumps) {
      logfile = CreateErrorLogFile(suffix, "dmp", logpath, sizeof(logpath), time);
      if (logfile != INVALID_HANDLE_VALUE) {
        userStrings[0] = outstr;
        LogMiniDump(logfile, exceptionPointers, 1, userStrings);
        CloseErrorLog(logfile);
      }
    }

    UnloadMachineStateSymbols();
  }

  canBreak = CanBreakToDebugger();
  defaultResult = IDOK;
  breakResult = 0;
  continueResult = 0;
  messageFlags = MB_ICONHAND;

  if (canBreak) {
    if (recoverable) {
      SStrCopy(cursor, GetErrorString(13), (DWORD)(sizeof(outstr) - (cursor - outstr)));
      messageFlags = MB_YESNOCANCEL | MB_DEFBUTTON2 | MB_ICONEXCLAMATION;
      defaultResult = IDNO;
      breakResult = IDYES;
      continueResult = IDCANCEL;
    } else {
      SStrCopy(cursor, GetErrorString(12), (DWORD)(sizeof(outstr) - (cursor - outstr)));
      messageFlags = MB_YESNO | MB_DEFBUTTON2 | MB_ICONHAND;
      defaultResult = IDNO;
      breakResult = IDYES;
    }
  } else if (recoverable) {
    SStrCopy(cursor, GetErrorString(10), (DWORD)(sizeof(outstr) - (cursor - outstr)));
    messageFlags = MB_OKCANCEL | MB_DEFBUTTON2 | MB_ICONEXCLAMATION;
    defaultResult = IDCANCEL;
    continueResult = IDOK;
  } else {
    SStrCopy(cursor, GetErrorString(9), (DWORD)(sizeof(outstr) - (cursor - outstr)));
    messageFlags = MB_OK | MB_ICONHAND;
    defaultResult = IDOK;
  }

  result = continueResult ? continueResult : defaultResult;
  SErrEnter();
  handler = s_handlerlist.Head();
  while ((LONG)handler > 0) {
    if (!handler->handler(errorcode, errorstr, localnameptr, linenumber, effectiveDescription)) {
      break;
    }
    handler = s_handlerlist.RawNext(handler);
  }
  if ((LONG)handler <= 0) {
    window = SMsgGetDefaultWindow();
    if (window && (!IsWindow(window) || !IsWindowVisible(window))) {
      window = NULL;
    }
    result = MessageBoxA(window, outstr, appname, messageFlags | 0x52000);
  }
  SErrLeave();

  if (result == breakResult) {
    Breakpoint();
    s_displaying = FALSE;
    return TRUE;
  }

  if (result == continueResult) {
    s_displaying = FALSE;
    return TRUE;
  }

  SErrSuppressErrors(TRUE);
  if ((LONG)GetVersion() < 0) {
    if (GetExitCodeProcess(GetCurrentProcess(), &terminationcode) && terminationcode != STILL_ACTIVE) {
      goto terminateDone;
    }
  }
  TerminateProcess(GetCurrentProcess(), exitcode);
terminateDone:
  s_displaying = FALSE;
  return FALSE;
}

extern "C" BOOL __cdecl SErrDisplayErrorFmt(DWORD errorcode, LPCSTR filename, int linenumber, BOOL recoverable, UINT exitcode, LPCSTR format, ...) {
  char    buffer[0x800];
  va_list args;

  va_start(args, format);
  _vsnprintf(buffer, sizeof(buffer) - 1, format, args);
  va_end(args);
  buffer[sizeof(buffer) - 1] = 0;

  return SErrDisplayError(errorcode, filename, linenumber, buffer, recoverable, exitcode);
}

extern "C" BOOL APIENTRY SErrGetErrorStr(DWORD errorcode, char *buffer, DWORD bufferchars) {
  MSGSRC   *source;
  HINSTANCE module;
  WORD      facility;
  DWORD     flags;
  DWORD     chars;

  FATALASSERT(buffer);
  FATALASSERT(bufferchars);

  buffer[0] = 0;
  module = NULL;
  facility = (WORD)((errorcode >> 16) & 0x0FFF);
  SErrEnter();
  if (!s_msgsrcinit) {
    s_msgsrcinit = TRUE;
    AddStormMessages();
  }
  for (source = s_msgsrchead; source; source = source->next) {
    if (source->facility == facility) {
      module = source->module;
      break;
    }
  }
  SErrLeave();

  flags = module ? FORMAT_MESSAGE_FROM_HMODULE : FORMAT_MESSAGE_FROM_SYSTEM;
  chars = FormatMessageA(flags, module, errorcode, 0x400, buffer, bufferchars, NULL);

  return chars != 0;
}

extern "C" DWORD APIENTRY SErrGetLastError() {
  return s_lasterror;
}

extern "C" BOOL APIENTRY SErrIsDisplayingError() {
  return s_displaying;
}

extern "C" void APIENTRY SErrCatchUnhandledExceptions() {
  SetUnhandledExceptionFilter(ExceptionFilterWin32);
}

extern "C" void APIENTRY SErrPrepareAppFatal(LPCSTR filename, int linenumber) {
  SErrEnter();
  s_appFatInfo.filename = filename;
  s_appFatInfo.linenumber = linenumber;
  s_appFatInfo.threadId = GetCurrentThreadId();
  SErrLeave();
}

extern "C" void APIENTRY SErrRegisterHandler(SERRHANDLER handler) {
  HANDLER *node;

  SErrEnter();
  node = s_handlerlist.NewNode(LIST_HEAD, 0, 0x08000000);
  node->handler = handler;
  SErrLeave();
}

extern "C" BOOL APIENTRY SErrRegisterMessageSource(WORD facility, HINSTANCE module, LPVOID reserved) {
  MSGSRC *source;

  (void)reserved;
  SErrEnter();
  source = (MSGSRC *)HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS, sizeof(MSGSRC));
  source->facility = facility;
  source->module = module;
  source->next = s_msgsrchead;
  s_msgsrchead = source;
  SErrLeave();

  return TRUE;
}

extern "C" void APIENTRY SErrReportResourceLeak(LPCSTR handlename) {
  SErrReportNamedResourceLeak(handlename, NULL);
}

extern "C" void APIENTRY SErrReportNamedResourceLeak(LPCSTR handlename, LPCSTR resourcename) {
  char szMessage[0xC8];
  char errormessage[0x100];

  if (!checked) {
    checked = TRUE;
    SRegLoadValue("Internal", "Debug Memory", 0, &s_debugMemory);
  }

  if (!s_debugMemory) {
    return;
  }

  if (resourcename && *resourcename) {
    SStrPrintf(errormessage, sizeof(errormessage), "%s (%s)", handlename, resourcename);
  } else {
    SStrPrintf(errormessage, sizeof(errormessage), "%s", handlename);
  }

  if (!debugmode) {
    SErrDisplayError(STORM_ERROR_HANDLE_NEVER_RELEASED, errormessage, -3, NULL, TRUE, 1);
    return;
  }

  SStrPrintf(szMessage, sizeof(szMessage), "Storm Error : handle never released -- %s\n", errormessage);
  OutputDebugStringA(szMessage);
}

extern "C" void APIENTRY SErrSetLastError(DWORD errorcode) {
  s_lasterror = errorcode;
  SetLastError(errorcode);
}

extern "C" void APIENTRY SErrSetLogTitleString(LPCSTR title) {
  SErrEnter();
  SStrCopy(s_logtitle, title, sizeof(s_logtitle));
  SErrLeave();
}

extern "C" void APIENTRY SErrSetLogCallback(SERRLOGCALLBACK cb) {
  SErrEnter();
  s_logCallback = cb;
  SErrLeave();
}

extern "C" BOOL APIENTRY SErrGetLogLastPath(char *buf, int size) {
  BOOL result;

  result = FALSE;
  SErrEnter();
  if (s_logLastFile[0]) {
    SStrCopy(buf, s_logLastFile, size);
    result = TRUE;
  }
  SErrLeave();

  return result;
}

extern "C" void APIENTRY SErrSuppressErrors(BOOL suppress) {
  s_suppress = suppress;
}

extern "C" void APIENTRY SErrUnregisterHandler(SERRHANDLER handler) {
  HANDLER *node;

  SErrEnter();
  node = s_handlerlist.Head();
  while ((LONG)node > 0) {
    if (node->handler == handler) {
      node = s_handlerlist.DeleteNode(node);
    } else {
      node = s_handlerlist.RawNext(node);
    }
  }
  SErrLeave();
}

static void UnregisterAllThreads() {
  int i;

  SErrThreadsEnter();
  for (i = 0; i < s_numthreads; i++) {
    CloseHandle(s_threads[i]);
  }
  s_numthreads = 0;
  SErrThreadsLeave();
}

extern "C" void APIENTRY SErrRegisterThread(HANDLE thread, DWORD threadid) {
  HANDLE process;
  HANDLE duplicate;
  int    i;

  duplicate = NULL;
  SErrThreadsEnter();
  for (i = 0; i < s_numthreads; i++) {
    if (s_threadids[i] == threadid) {
      break;
    }
  }
  if (i == s_numthreads && s_numthreads < MAX_ERR_THREADS) {
    process = GetCurrentProcess();
    if (DuplicateHandle(process, thread, process, &duplicate, 0, FALSE, DUPLICATE_SAME_ACCESS)) {
      s_threads[s_numthreads] = duplicate;
      s_threadids[s_numthreads] = threadid;
      s_numthreads++;
    }
  }
  SErrThreadsLeave();
}

extern "C" void APIENTRY SErrUnregisterThread(HANDLE thread, DWORD threadid) {
  int i;

  SErrThreadsEnter();
  for (i = 0; i < s_numthreads; i++) {
    if (s_threadids[i] == threadid) {
      CloseHandle(s_threads[i]);
      s_numthreads--;
      if (i < s_numthreads) {
        s_threads[i] = s_threads[s_numthreads];
        s_threadids[i] = s_threadids[s_numthreads];
      }
      break;
    }
  }
  SErrThreadsLeave();
}

static void LogThreads(HANDLE *threads, LPDWORD threadids, int numthreads, LPCSTR description, LPCSTR suffix) {
  BOOL   suspended[MAX_ERR_THREADS];
  HANDLE log;
  DWORD  currentThreadId;
  int    currentIndex;
  int    suspendedCount;
  int    count;
  int    i;
  int    loggedIndex;
  char   buffer[0x100];

  if (numthreads <= 0) {
    return;
  }

  SErrThreadsEnter();
  log = CreateErrorLog(suffix, description, NULL);
  if (log == INVALID_HANDLE_VALUE) {
    SErrThreadsLeave();
    return;
  }

  count = numthreads;

  LoadMachineStateSymbols();
  ZeroMemory(suspended, sizeof(suspended));
  currentThreadId = GetCurrentThreadId();
  currentIndex = -1;
  suspendedCount = 0;

  for (i = 0; i < count; i++) {
    if (threadids[i] == currentThreadId) {
      currentIndex = i;
      suspendedCount++;
    } else if (SuspendThread(threads[i]) != (DWORD)-1) {
      suspended[i] = TRUE;
      suspendedCount++;
    }
  }

  SStrPrintf(buffer, sizeof(buffer), "================================================================\n\nLogging %d threads\n\n", suspendedCount);
  WriteMessageToLog(log, buffer);

  loggedIndex = 0;
  if (currentIndex >= 0) {
    loggedIndex++;
    SStrPrintf(
        buffer, sizeof(buffer),
        "================================================================\n================================================================"
        "\n\nLogging thread %d of %d (the current thread), thread id = 0x%08X, thread handle = 0x%08X\n\n",
        loggedIndex, count, threadids[currentIndex], (DWORD)threads[currentIndex]
    );
    WriteMessageToLog(log, buffer);
    LogContext(log, 1, NULL);
  }

  for (i = 0; i < count; i++) {
    CONTEXT context;

    if (!suspended[i]) {
      continue;
    }

    loggedIndex++;
    SStrPrintf(
        buffer, sizeof(buffer),
        "================================================================\n================================================================"
        "\n\nLogging thread %d of %d, thread id = 0x%08X, thread handle = 0x%08X\n\n",
        loggedIndex, count, threadids[i], (DWORD)threads[i]
    );
    WriteMessageToLog(log, buffer);

    ZeroMemory(&context, sizeof(context));
    context.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER | CONTEXT_SEGMENTS;
    if (GetThreadContext(threads[i], &context)) {
      LogContext(log, 0, &context);
    }
  }

  for (i = 0; i < count; i++) {
    if (suspended[i]) {
      ResumeThread(threads[i]);
    }
  }

  UnloadMachineStateSymbols();
  CloseErrorLog(log);
  SErrThreadsLeave();
}

extern "C" void APIENTRY SErrLogRegisteredThreads(LPCSTR description, LPCSTR suffix) {
  SErrThreadsEnter();
  LogThreads(s_threads, s_threadids, s_numthreads, description, suffix);
  SErrThreadsLeave();
}

extern "C" void APIENTRY SErrLogThreads(HANDLE *threads, LPDWORD threadids, int numthreads, LPCSTR description, LPCSTR suffix) {
  LogThreads(threads, threadids, numthreads, description, suffix);
}

static void CheckKeyboard() {
  BOOL chordDown;

  chordDown = TRUE;
  if (!(GetAsyncKeyState(VK_CONTROL) & 0x8000)) {
    chordDown = FALSE;
  }
  if (!(GetAsyncKeyState(VK_SHIFT) & 0x8000)) {
    chordDown = FALSE;
  }
  if (!(GetAsyncKeyState(VK_MENU) & 0x8000)) {
    chordDown = FALSE;
  }
  if (!(GetAsyncKeyState(VK_NEXT) & 0x8000)) {
    s_keysweredown = FALSE;
    return;
  }

  if (chordDown && !s_keysweredown) {
    InternalEnterCriticalSection(&s_threadscritsec);
    LogThreads(s_threads, s_threadids, s_numthreads, "User initiated log -- The user held down Ctrl-Alt-Shift-PageDown", "Log");
    InternalLeaveCriticalSection(&s_threadscritsec);
  }

  s_keysweredown = chordDown;
}

static DWORD WINAPI WatchdogThreadProc(LPVOID __formal /* __formal */) {
  DWORD lastFreezePingCount;

  lastFreezePingCount = s_pingcounter - 1;

  while (s_watchdogrunning) {
    DWORD waitResult;
    DWORD pingBefore;
    DWORD pingAfter;

    pingBefore = s_pingcounter;
    waitResult = WaitForSingleObject(s_watchdogevent, 1000);
    pingAfter = s_pingcounter;

    if (s_watchdogpaused > 0) {
      s_secondsfrozen = 0;
      Sleep(500);
      continue;
    }

    if (!s_watchdogrunning) {
      break;
    }

    if (s_checkkeyboard) {
      CheckKeyboard();
    }

    if (s_freezeperiod && waitResult == WAIT_TIMEOUT && !s_displaying) {
      if (pingBefore != pingAfter) {
        s_secondsfrozen = 0;
      } else if (lastFreezePingCount == pingAfter) {
        s_secondsfrozen = 0;
      } else {
        s_secondsfrozen++;
        if (s_secondsfrozen >= s_freezeperiod) {
          char description[0x100];
          SStrPrintf(description, sizeof(description), "Freeze Log: The game was frozen for %u seconds", s_freezeperiod);
          s_secondsfrozen = 0;
          lastFreezePingCount = pingAfter;
          SErrThreadsEnter();
          LogThreads(s_threads, s_threadids, s_numthreads, description, "Freeze");
          SErrThreadsLeave();
        }
      }
    }
  }

  return 0;
}

extern "C" void APIENTRY SErrStartWatchdog(DWORD freezeSeconds, BOOL checkKeyboard) {
  DWORD threadid;

  if (!freezeSeconds && !checkKeyboard) {
    SErrStopWatchdog();
    return;
  }

  s_freezeperiod = freezeSeconds;
  s_secondsfrozen = 0;
  s_checkkeyboard = checkKeyboard;
  s_keysweredown = 0;

  if (!s_watchdogrunning) {
    s_watchdogrunning = TRUE;
    s_watchdogevent = CreateEventA(NULL, FALSE, FALSE, NULL);
    s_watchdogthread = CreateThread(NULL, 0, WatchdogThreadProc, NULL, 0, &threadid);
  } else {
    SetEvent(s_watchdogevent);
  }
}

extern "C" void APIENTRY SErrPauseWatchdog() {
  InterlockedIncrement(&s_watchdogpaused);
  SErrPingWatchdog();
}

extern "C" void APIENTRY SErrResumeWatchdog() {
  InterlockedDecrement(&s_watchdogpaused);
  SErrPingWatchdog();
}

extern "C" void APIENTRY SErrStopWatchdog() {
  if (s_watchdogrunning) {
    s_watchdogrunning = FALSE;
    SetEvent(s_watchdogevent);
    WaitForSingleObject(s_watchdogthread, 1000);
    CloseHandle(s_watchdogevent);
    CloseHandle(s_watchdogthread);
    s_watchdogevent = NULL;
    s_watchdogthread = NULL;
  }
}

extern "C" void APIENTRY SErrPingWatchdog() {
  s_pingcounter++;
}
