#include <storm.h>
#include <stpl.h>

#include <malloc.h>
#include <stdarg.h>

#define MAX_ERR_THREADS 64

int  CheckMachineStateSymbolHelper();
void LoadMachineStateSymbols();
void UnloadMachineStateSymbols();
int  LogMiniDump(LPVOID file, EXCEPTION_POINTERS *exceptionPointers, UINT userStreamCount, char *userStreams[]);
int  LogMiniDumpIsAvailable();

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

NODEDECL(HANDLER) {
  SERRHANDLER handler;
};

template <class T, class GETLINK>
class TSListWinHeap : public TSList<T, GETLINK> {
 public:
  T *NewNode(DWORD location, DWORD extrabytes, DWORD flags) {
    T *ptr = new (HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS, sizeof(T) + extrabytes)) T;

    memset(ptr, 0, sizeof(T) + extrabytes);
    if (location) {
      this->LinkNode(ptr, location, 0);
    }

    return ptr;
  }

  T *DeleteNode(T *ptr) {
    T *next = this->Next(ptr);

    ptr->~T();
    HeapFree(GetProcessHeap(), 0, ptr);
    return next;
  }
};

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
static int                                         s_keysweredown;
static DWORD                                       s_secondsfrozen;
static int                                         debugmode;
static int                                         s_noMiniDumps;
static BOOL                                        checked;
static EXCEPTION_POINTERS                         *s_exceptionPointers;
static char                                        buffer[0x100];

static DWORD WINAPI WatchdogThreadProc(LPVOID);
static void         CheckKeyboard();
static void         LogThreads(HANDLE *threads, LPDWORD threadids, int numthreads, LPCSTR description, LPCSTR suffix);
static LONG WINAPI  ExceptionFilterWin32(EXCEPTION_POINTERS *exceptionpointers);
static void         InternalEnterCriticalSection(CRITICAL_SECTION *crit);
static void         InternalLeaveCriticalSection(CRITICAL_SECTION *crit);
static void         UnregisterAllThreads();
static void         Breakpoint();

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
    "Couldn't locate the \"DbgHelp.dll\" debugging DLL.\n\nSome debugging information will be missing from error log files. Press OK to continue, or "
    "press Cancel to terminate the program.",
    "Missing Debugging DLL"
};

static LPCSTR GetErrorString(UINT id) {
  if (LoadStringA(StormGetInstance(), id + 0x5100, buffer, sizeof(buffer))) {
    return buffer;
  }

  return s_displaystr[id];
}

static void AddStormFacility(WORD facility) {
  MSGSRC **link;

  link = &s_msgsrchead;
  while (*link) {
    link = &(*link)->next;
  }

  *link = (MSGSRC *)HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS, sizeof(MSGSRC));
  (*link)->facility = facility;
  (*link)->module = StormGetInstance();
  (*link)->next = NULL;
}

static void AddStormMessages() {
  AddStormFacility(0x510);
  AddStormFacility(0x876);
  AddStormFacility(0x878);
}

static void Breakpoint() {
#if defined(_MSC_VER) && defined(_M_IX86)
  __try {
    __asm int 3
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

static int UndecorateObjectName(LPCSTR source, char *dest, DWORD destchars) {
  char *end;

  if (SStrLen(source) < 6 || source[0] != '.' || source[3] != 'U') {
    return FALSE;
  }

  if (!strpbrk(source + 4, "?@")) {
    return FALSE;
  }

  SStrCopy(dest, source + 4, destchars);
  end = strpbrk(dest, "?@");
  if (!end) {
    return FALSE;
  }

  *end-- = 0;
  while (end >= dest && *end == '_') {
    *end-- = 0;
  }

  SStrPack(dest, " (", destchars);
  SStrPack(dest, source, destchars);
  SStrPack(dest, ")", destchars);
  return TRUE;
}

void SErrInitialize() {
  SRegLoadValue("Internal", "Debug Error Output", 0, (LPDWORD)&debugmode);
  SRegLoadValue("Internal", "No Minidumps", 0, (LPDWORD)&s_noMiniDumps);
  SErrRegisterThread(GetCurrentThread(), GetCurrentThreadId());
}

static BOOL CanBreakToDebugger() {
  typedef BOOL(WINAPI * T_IsDebuggerPresent)();
  HMODULE             kernel;
  T_IsDebuggerPresent proc;
  int                 present;

  present = FALSE;
  kernel = LoadLibraryA("KERNEL32.DLL");
  if (kernel) {
    proc = GetProcAddress(kernel, "IsDebuggerPresent");
    if (proc) {
      present = proc();
    }
    FreeLibrary(kernel);
  }

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
  if (attributes == (DWORD)-1) {
    return FALSE;
  }

  return ((BYTE)attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

static void __cdecl WriteLine(LPVOID param, LPCSTR format, ...) {
  char    buffer[0x800];
  DWORD   byteswritten;
  va_list args;

  if (!format) {
    format = "";
  }

  va_start(args, format);
  _vsnprintf(buffer, sizeof(buffer) - 3, format, args);
  va_end(args);
  buffer[sizeof(buffer) - 3] = 0;
  SStrPack(buffer, "\r\n", sizeof(buffer));
  WriteFile(param, buffer, strlen(buffer), &byteswritten, NULL);
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

static HANDLE CreateErrorLogFile(LPCSTR suffix, LPCSTR ext, char logpath[], DWORD logpathchars, SYSTEMTIME &time) {
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
    if (strlen(extra)) {
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
  LPCSTR name;

  switch (exceptioncode) {
    case EXCEPTION_ACCESS_VIOLATION:
      name = "ACCESS_VIOLATION";
      break;
    case EXCEPTION_DATATYPE_MISALIGNMENT:
      name = "DATATYPE_MISALIGNMENT";
      break;
    case EXCEPTION_BREAKPOINT:
      name = "BREAKPOINT";
      break;
    case EXCEPTION_SINGLE_STEP:
      name = "SINGLE_STEP";
      break;
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
      name = "ARRAY_BOUNDS_EXCEEDED";
      break;
    case EXCEPTION_FLT_DENORMAL_OPERAND:
      name = "FLT_DENORMAL_OPERAND";
      break;
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
      name = "FLT_DIVIDE_BY_ZERO";
      break;
    case EXCEPTION_FLT_INEXACT_RESULT:
      name = "FLT_INEXACT_RESULT";
      break;
    case EXCEPTION_FLT_INVALID_OPERATION:
      name = "FLT_INVALID_OPERATION";
      break;
    case EXCEPTION_FLT_OVERFLOW:
      name = "FLT_OVERFLOW";
      break;
    case EXCEPTION_FLT_STACK_CHECK:
      name = "FLT_STACK_CHECK";
      break;
    case EXCEPTION_FLT_UNDERFLOW:
      name = "FLT_UNDERFLOW";
      break;
    case EXCEPTION_ILLEGAL_INSTRUCTION:
      name = "ILLEGAL_INSTRUCTION";
      break;
    case EXCEPTION_IN_PAGE_ERROR:
      name = "IN_PAGE_ERROR";
      break;
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
      name = "INT_DIVIDE_BY_ZERO";
      break;
    case EXCEPTION_INT_OVERFLOW:
      name = "INT_OVERFLOW";
      break;
    case EXCEPTION_NONCONTINUABLE_EXCEPTION:
      name = "NONCONTINUABLE_EXCEPTION";
      break;
    case EXCEPTION_INVALID_DISPOSITION:
      name = "INVALID_DISPOSITION";
      break;
    case EXCEPTION_PRIV_INSTRUCTION:
      name = "PRIV_INSTRUCTION";
      break;
    case EXCEPTION_STACK_OVERFLOW:
      name = "STACK_OVERFLOW";
      break;
    case EXCEPTION_GUARD_PAGE:
      name = "GUARD_PAGE";
      break;
    case EXCEPTION_INVALID_HANDLE:
      name = "INVALID_HANDLE";
      break;
    default: {
      HMODULE ntdll = GetModuleHandleA("ntdll.dll");
      if (!ntdll || FormatMessageA(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS, ntdll, exceptioncode, 0, buffer, buffersize, NULL) <= 0) {
        SStrCopy(buffer, "unknown exception", buffersize);
      }
      return;
    }
  }

  SStrCopy(buffer, name, buffersize);
}

static LONG WINAPI ExceptionFilterWin32(EXCEPTION_POINTERS *exceptionpointers) {
  EXCEPTION_RECORD *record = exceptionpointers->ExceptionRecord;

  char buffer[0x50];
  char message[0x100];
  char exceptionname[0x28];
  GetExceptionNameWin32(record->ExceptionCode, exceptionname, sizeof(exceptionname));
  SStrPrintf(
      buffer, sizeof(buffer), "0x%08X (%s) at %04X:%08X", record->ExceptionCode, exceptionname, exceptionpointers->ContextRecord->SegCs,
      record->ExceptionAddress
  );
  message[0] = 0;

  if (record->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && record->NumberParameters == 2) {
    char format[0x100];
    SStrCopy(format, GetErrorString(15), sizeof(format));
    SStrPrintf(
        message, sizeof(message), format, record->ExceptionAddress, record->ExceptionInformation[1],
        !record->ExceptionInformation[0] ? GetErrorString(16) : GetErrorString(17)
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
  s_handlerlist.Clear();

  s_msgsrcinit = FALSE;
  while (s_msgsrchead) {
    MSGSRC *next = s_msgsrchead->next;
    HeapFree(GetProcessHeap(), 0, s_msgsrchead);
    s_msgsrchead = next;
  }

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
    ZeroMemory(&s_appFatInfo, sizeof(s_appFatInfo));
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
  if (s_suppress)
    return FALSE;

  if (s_displaying)
    return FALSE;
  s_displaying = TRUE;

  char   appname[MAX_PATH];
  char   outstr[0x800];
  UINT   messageboxflags;
  int    continueresult;
  char   localnamebuffer[0x100];
  LPCSTR localnameptr;
  char   appfilename[MAX_PATH];
  char   errorstr[0x100];
  int    debuggerresult;

  appfilename[0] = 0;
  appname[0] = 0;
  GetModuleFileNameA(NULL, appfilename, sizeof(appfilename));
  {
    WIN32_FIND_DATAA finddata;
    ZeroMemory(&finddata, sizeof(finddata));
    HANDLE handle = FindFirstFileA(appfilename, &finddata);
    if (handle)
      FindClose(handle);
    SStrCopy(appname, finddata.cFileName, sizeof(appname));
    if (SStrChrR(appname, '.'))
      *SStrChrR(appname, '.') = 0;
  }

  errorstr[0] = 0;
  SErrGetErrorStr(errorcode, errorstr, sizeof(errorstr));
  if (!errorstr[0])
    SStrPrintf(errorstr, sizeof(errorstr), GetErrorString(0), errorcode & 0xFFFF, errorcode);

  localnameptr = filename;
  if (filename && *filename && (linenumber == SERR_LINECODE_OBJECT || linenumber == SERR_LINECODE_HANDLE)) {
    if (UndecorateObjectName(filename, localnamebuffer, sizeof(localnamebuffer)))
      localnameptr = localnamebuffer;
  }

  char *cursor = outstr;
  if (localnameptr)
    cursor += SStrCopy(outstr, localnameptr, sizeof(outstr));
  if (linenumber > 0) {
    SStrPrintf(cursor, sizeof(outstr) - (cursor - outstr), "(%u)", linenumber);
    cursor += SStrLen(cursor);
  }
  SStrPrintf(cursor, sizeof(outstr) - (cursor - outstr), " : error %u: ", errorcode & 0xFFFF);
  cursor += SStrLen(cursor);
  SStrCopy(cursor, errorstr, sizeof(outstr) - (cursor - outstr));
  OutputDebugStringA(outstr);

  SStrPrintf(outstr, sizeof(outstr), GetErrorString(1), errorstr);
  cursor = outstr + SStrLen(outstr);
  SStrPrintf(cursor, sizeof(outstr) - (cursor - outstr), GetErrorString(2), appfilename);
  cursor += SStrLen(cursor);
  if (localnameptr && *localnameptr) {
    switch (linenumber) {
      case SERR_LINECODE_FUNCTION:
        SStrPrintf(cursor, sizeof(outstr) - (cursor - outstr), GetErrorString(4), localnameptr);
        break;
      case SERR_LINECODE_OBJECT:
        SStrPrintf(cursor, sizeof(outstr) - (cursor - outstr), GetErrorString(5), localnameptr);
        break;
      case SERR_LINECODE_HANDLE:
        SStrPrintf(cursor, sizeof(outstr) - (cursor - outstr), GetErrorString(6), localnameptr);
        break;
      case SERR_LINECODE_FILE:
        SStrPrintf(cursor, sizeof(outstr) - (cursor - outstr), GetErrorString(11), localnameptr);
        break;
      case SERR_LINECODE_EXCEPTION:
        SStrPrintf(cursor, sizeof(outstr) - (cursor - outstr), GetErrorString(14), localnameptr);
        break;
      default:
        SStrPrintf(cursor, sizeof(outstr) - (cursor - outstr), GetErrorString(3), localnameptr, linenumber);
        break;
    }
    cursor += SStrLen(cursor);
  }

  if (errorcode == STORM_ERROR_ASSERTION)
    SStrPrintf(cursor, sizeof(outstr) - (cursor - outstr), GetErrorString(7), description ? description : "");
  else
    SStrPrintf(cursor, sizeof(outstr) - (cursor - outstr), GetErrorString(8), description ? description : "");
  cursor += SStrLen(cursor);

  if (!g_opt.serrsuppresslogs) {
    LPCSTR              suffix;
    SYSTEMTIME          time;
    EXCEPTION_POINTERS *exceptionPointers;

    LoadMachineStateSymbols();

    SErrExceptionEnter();
    exceptionPointers = s_exceptionPointers;
    s_exceptionPointers = NULL;
    SErrExceptionLeave();

    CONTEXT *context = exceptionPointers ? exceptionPointers->ContextRecord : NULL;

    GetLocalTime(&time);

    suffix = context ? "Crash" : "Error";
    HANDLE logfile = CreateErrorLog(suffix, outstr, &time);
    if (logfile != INVALID_HANDLE_VALUE) {
      LogContext(logfile, context ? 0 : 2, context);
      CloseErrorLog(logfile);
    }

    if (LogMiniDumpIsAvailable()
        && !s_noMiniDumps) {
      char logpath[MAX_PATH];
      logfile = CreateErrorLogFile(suffix, "dmp", logpath, sizeof(logpath), time);
      if (logfile != INVALID_HANDLE_VALUE) {
        char *userStrings[1] = {s_logtitle};
        LogMiniDump(logfile, exceptionPointers, 1, userStrings);
        CloseErrorLog(logfile);
      }
    }

    UnloadMachineStateSymbols();
  }

  UINT icon;
  UINT buttons;
  int  result;
  if (CanBreakToDebugger()) {
    if (recoverable) {
      SStrCopy(cursor, GetErrorString(13), sizeof(outstr) - (cursor - outstr));
      icon = MB_ICONEXCLAMATION;
      buttons = MB_YESNOCANCEL | MB_DEFBUTTON2;
      result = IDNO;
      continueresult = IDCANCEL;
      debuggerresult = IDYES;
    } else {
      SStrCopy(cursor, GetErrorString(12), sizeof(outstr) - (cursor - outstr));
      icon = MB_ICONHAND;
      buttons = MB_YESNO | MB_DEFBUTTON2;
      result = IDNO;
      continueresult = 0;
      debuggerresult = IDYES;
    }
  } else if (recoverable) {
    SStrCopy(cursor, GetErrorString(10), sizeof(outstr) - (cursor - outstr));
    icon = MB_ICONEXCLAMATION;
    buttons = MB_OKCANCEL | MB_DEFBUTTON2;
    result = IDCANCEL;
    continueresult = IDOK;
    debuggerresult = 0;
  } else {
    SStrCopy(cursor, GetErrorString(9), sizeof(outstr) - (cursor - outstr));
    icon = MB_ICONHAND;
    buttons = MB_OK;
    result = IDOK;
    continueresult = 0;
    debuggerresult = 0;
  }

  SErrEnter();

  messageboxflags = buttons | icon | 0x52000;

  if (continueresult)
    result = continueresult;

  BOOL handled = FALSE;
  for (HANDLER *handler = s_handlerlist.Head(); (LONG)handler > 0; handler = s_handlerlist.RawNext(handler)) {
    if (!handler->handler(errorcode, errorstr, localnameptr, linenumber, description ? description : "")) {
      handled = TRUE;
      break;
    }
  }
  if (!handled) {
    HWND window = SMsgGetDefaultWindow();
    if (window && (!IsWindow(window) || !IsWindowVisible(window)))
      window = NULL;
    result = MessageBoxA(window, outstr, appname, messageboxflags);
  }
  SErrLeave();

  if (result == debuggerresult) {
    Breakpoint();
    s_displaying = FALSE;
    return TRUE;
  }

  if (result == continueresult) {
    s_displaying = FALSE;
    return TRUE;
  }

  SErrSuppressErrors(TRUE);

  {
    DWORD terminationcode;
    if (!(GetVersion() & 0x80000000) || !GetExitCodeProcess(GetCurrentProcess(), &terminationcode) || terminationcode == STILL_ACTIVE)
      TerminateProcess(GetCurrentProcess(), exitcode);
  }

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
  VALIDATEBEGIN;
  VALIDATE(buffer);
  VALIDATE(bufferchars);
  VALIDATEEND;

  SErrEnter();
  if (!s_msgsrcinit) {
    s_msgsrcinit = TRUE;
    AddStormMessages();
  }

  MSGSRC   *source = s_msgsrchead;
  WORD      facility = (errorcode >> 16) & 0x0FFF;
  HINSTANCE module = NULL;
  while (source && source->facility != facility)
    source = source->next;
  if (source)
    module = source->module;
  SErrLeave();

  *buffer = 0;
  return FormatMessageA(module ? FORMAT_MESSAGE_FROM_HMODULE : FORMAT_MESSAGE_FROM_SYSTEM, module, errorcode, 0x400, buffer, bufferchars, NULL) != 0;
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
    SRegLoadValue("Internal", "Debug Memory", 0, (LPDWORD)&debugmode);
  }

  if (!debugmode) {
    return;
  }

  if (resourcename && *resourcename) {
    SStrPrintf(errormessage, sizeof(errormessage), "%s (%s)", handlename, resourcename);
  } else {
    SStrPrintf(errormessage, sizeof(errormessage), "%s", handlename);
  }

  if (!g_opt.serrleaksilentwarning) {
    SErrDisplayError(STORM_ERROR_HANDLE_NEVER_RELEASED, errormessage, SERR_LINECODE_HANDLE, NULL, TRUE, 1);
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
  SErrEnter();

  BOOL result = FALSE;
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
  SErrEnter();
  ITERATELIST(HANDLER, s_handlerlist, node) {
    if (node->handler == handler) {
      ITERATE_DELETE;
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
  int    i;

  SErrThreadsEnter();
  for (i = 0; i < s_numthreads; i++) {
    if (s_threadids[i] == threadid) {
      break;
    }
  }
  if (i == s_numthreads && s_numthreads < MAX_ERR_THREADS) {
    process = GetCurrentProcess();
    if (DuplicateHandle(process, thread, process, &s_threads[s_numthreads], 0, FALSE, DUPLICATE_SAME_ACCESS)) {
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
      break;
    }
  }

  if (i < s_numthreads) {
    CloseHandle(s_threads[i]);
    s_numthreads--;
    if (i < s_numthreads) {
      s_threads[i] = s_threads[s_numthreads];
      s_threadids[i] = s_threadids[s_numthreads];
    }
  }
  SErrThreadsLeave();
}

static void LogThreads(HANDLE *threads, LPDWORD threadids, int numthreads, LPCSTR description, LPCSTR suffix) {
  if (numthreads > 0) {
    SErrThreadsEnter();
    HANDLE logfile = CreateErrorLog(suffix, description, NULL);
    if (logfile != INVALID_HANDLE_VALUE) {
      LoadMachineStateSymbols();

      BOOL  suspended[MAX_ERR_THREADS];
      char  msg[0x100];
      DWORD currentid = GetCurrentThreadId();
      int   suspendedCount = 0;
      int   currentIndex = -1;
      int   i;
      for (i = 0; i < numthreads; i++) {
        if (threadids[i] == currentid) {
          suspended[i] = FALSE;
          currentIndex = i;
          ++suspendedCount;
        } else {
          suspended[i] = SuspendThread(threads[i]) != (DWORD)-1;
          if (suspended[i])
            ++suspendedCount;
        }
      }

      SStrPrintf(msg, sizeof(msg), "================================================================\n\nLogging %d threads\n\n", suspendedCount);
      WriteMessageToLog(logfile, msg);

      int loggedIndex = 0;
      if (currentIndex != -1) {
        SStrPrintf(
            msg, sizeof(msg),
            "================================================================\n================================================================"
            "\n\nLogging thread %d of %d (the current thread), thread id = 0x%08X, thread handle = 0x%08X\n\n",
            ++loggedIndex, numthreads, threadids[currentIndex], threads[currentIndex]
        );
        WriteMessageToLog(logfile, msg);
        LogContext(logfile, 1, NULL);
      }

      for (i = 0; i < numthreads; i++) {
        if (suspended[i]) {
          SStrPrintf(
              msg, sizeof(msg),
              "================================================================\n================================================================"
              "\n\nLogging thread %d of %d, thread id = 0x%08X, thread handle = 0x%08X\n\n",
              ++loggedIndex, numthreads, threadids[i], threads[i]
          );
          WriteMessageToLog(logfile, msg);

          CONTEXT context;
          ZeroMemory(&context, sizeof(context));
          context.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER | CONTEXT_SEGMENTS;
          if (GetThreadContext(threads[i], &context))
            LogContext(logfile, 0, &context);
        }
      }

      for (i = 0; i < numthreads; i++) {
        if (suspended[i])
          ResumeThread(threads[i]);
      }

      UnloadMachineStateSymbols();
      CloseErrorLog(logfile);
    }
    SErrThreadsLeave();
  }
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
    chordDown = FALSE;
  }

  if (chordDown && !s_keysweredown) {
    InternalEnterCriticalSection(&s_threadscritsec);
    LogThreads(s_threads, s_threadids, s_numthreads, "User initiated log -- The user held down Ctrl-Alt-Shift-PageDown", "Log");
    InternalLeaveCriticalSection(&s_threadscritsec);
  }

  s_keysweredown = chordDown;
}

static DWORD WINAPI WatchdogThreadProc(LPVOID) {
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
          s_secondsfrozen = 0;
          lastFreezePingCount = pingAfter;

          char description[0x100];
          SStrPrintf(description, sizeof(description), "Freeze Log: The game was frozen for at least %d seconds", s_freezeperiod);
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
