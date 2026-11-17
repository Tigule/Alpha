#include <storm.h>
#include <stpl.h>
#include <SThread.h>

#include <execinfo.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

LPCSTR OsGetWritePath();

NODEDECL(HANDLER) {
  SERRHANDLER handler;
};

struct FRAMEDATA {
  DWORD length;
  char  data[10000];
};

static SCritSect s_critsect;

static LPCSTR s_fatalFileName;
static int    s_fatalLineNumber;
static int    s_fatalProcessId;

static SERRLOGCALLBACK s_logCallback;

static FILE *s_logFile;
static char  s_debugger[0x100];
static char  s_crashMail[0x100];
static char  s_crashFileName[0x1000];
static pid_t s_parentPid;
static char  s_commandLine[0x1000];
static char  s_appCommand[0x1000] = "GenericBlizzardApp";
static int   s_logFd = -1;

static pthread_key_t s_frameKey;
static LISTDECL(HANDLER, s_handlers);

static char  *s_stackBuffer;
static int    s_stackBufferSize;
static SEvent s_stackEvent;
static pid_t  s_stackPid;
static SCritSect s_stackCritsect;
static LONG   s_debugging;

static void SErrEnter() {
  s_critsect.Enter();
}

static void SErrLeave() {
  s_critsect.Leave();
}

static void GetStackCrawl(char *buffer, int bufferSize) {
  void  *frames[64];
  char **symbols;
  int    count;
  int    written;
  int    index;

  written = snprintf(buffer, bufferSize, "Stack:\n");
  buffer += written;
  bufferSize -= written;

  count = backtrace(frames, 64);
  symbols = backtrace_symbols(frames, count);

  for (index = 1; index < count && bufferSize > 0; ++index) {
    written = snprintf(buffer, bufferSize, "\t%s\n", symbols[index]);
    buffer += written;
    bufferSize -= written;
  }

  free(symbols);
}

static void WriteStackCrawl(int fd) {
  void *frames[64];
  int   count;

  count = backtrace(frames, 64);
  backtrace_symbols_fd(frames, count, fd);
}

static void GetAddr2LineCommand(char *buffer, int bufferSize) {
  void *frames[64];
  char  address[16];
  int   count;
  int   index;

  snprintf(buffer, bufferSize, "addr2line -f -C -e %s", s_appCommand);

  count = backtrace(frames, 64);
  for (index = 0; index < count; ++index) {
    snprintf(address, sizeof(address), " %x", (DWORD)(DWORD_PTR)frames[index]);
    SStrPack(buffer, address, bufferSize);
  }
}

extern "C" DWORD APIENTRY SErrGetLastError() {
  return 0;
}

extern "C" void APIENTRY SErrSetLastError(DWORD errorcode) {
}

extern "C" BOOL APIENTRY SErrIsDisplayingError() {
  return FALSE;
}

extern "C" void APIENTRY SErrCatchUnhandledExceptions() {
}

extern "C" void APIENTRY SErrPrepareAppFatal(LPCSTR filename, int linenumber) {
  SErrEnter();

  s_fatalFileName = filename;
  s_fatalLineNumber = linenumber;
  s_fatalProcessId = getpid();

  SErrLeave();
}

extern "C" void APIENTRY SErrSetLogCallback(SERRLOGCALLBACK cb) {
  SErrEnter();
  s_logCallback = cb;
  SErrLeave();
}

extern "C" void APIENTRY SErrSetLogTitleString(LPCSTR title) {
}

extern "C" void __cdecl SErrDisplayAppFatal(LPCSTR format, ...) {
  LPCSTR  filename = 0;
  int     linenumber = 0;
  char    description[0x400];
  va_list args;

  SErrEnter();

  if (s_fatalProcessId == getpid()) {
    filename = s_fatalFileName;
    linenumber = s_fatalLineNumber;

    s_fatalFileName = 0;
    s_fatalLineNumber = 0;
    s_fatalProcessId = 0;
  }

  SErrLeave();

  va_start(args, format);
  vsnprintf(description, sizeof(description) - 1, format, args);
  va_end(args);
  description[sizeof(description) - 1] = 0;

  SErrDisplayError(STORMERROR(132), filename, linenumber, description, FALSE, 1);
}

extern "C" BOOL APIENTRY SErrDisplayError(DWORD errorcode, LPCSTR filename, int linenumber, LPCSTR description, BOOL recoverable, UINT exitcode) {
  fprintf(stderr, "%s:%d: %s\n", filename, linenumber, description);

  if (s_logFile) {
    fprintf(s_logFile, "ASSERT:\n%s:%d: %s\n", filename, linenumber, description);
    fprintf(s_logFile, "**********************************************************************\n");
    fflush(s_logFile);
  }

  abort();
}

extern "C" BOOL __cdecl SErrDisplayErrorFmt(DWORD errorcode, LPCSTR filename, int linenumber, BOOL recoverable, UINT exitcode, LPCSTR format, ...) {
  char    description[0x400];
  va_list args;

  va_start(args, format);
  vsnprintf(description, sizeof(description) - 1, format, args);
  va_end(args);
  description[sizeof(description) - 1] = 0;

  return SErrDisplayError(errorcode, filename, linenumber, description, recoverable, exitcode);
}

extern "C" BOOL APIENTRY SErrGetErrorStr(DWORD errorcode, char *buffer, DWORD bufferchars) {
  VALIDATEBEGIN;
  VALIDATE(buffer);
  VALIDATE(bufferchars);
  VALIDATEEND;

  buffer[0] = 0;
  return FALSE;
}

extern "C" void APIENTRY SErrReportResourceLeak(LPCSTR handlename) {
  SErrReportNamedResourceLeak(handlename, NULL);
}

extern "C" void APIENTRY SErrReportNamedResourceLeak(LPCSTR handlename, LPCSTR resourcename) {
  static BOOL  checked;
  static DWORD debugmode;
  char         errormessage[0x100];

  if (!checked) {
    checked = TRUE;
    SRegLoadValue("Internal", "Debug Memory", 0, &debugmode);
  }

  if (!debugmode) {
    return;
  }

  if (resourcename && *resourcename) {
    SStrPrintf(errormessage, sizeof(errormessage), "%s (%s)", handlename, resourcename);
  } else {
    SStrPrintf(errormessage, sizeof(errormessage), "%s", handlename);
  }

  fprintf(stderr, "Storm Error : handle never released -- %s\n", errormessage);
}

static void AbortHandler(int sig) {
  sleep(120);
  signal(SIGABRT, SIG_DFL);
  abort();
}

BOOL SErrGetProcessStack(pid_t pid, char *buffer, int bufferSize) {
  if (pid == getpid()) {
    GetStackCrawl(buffer, bufferSize);
    return TRUE;
  }

  s_stackBuffer = buffer;
  s_stackBufferSize = bufferSize;
  s_stackPid = pid;
  s_stackEvent.Reset();

  if (kill(pid, SIGUSR2)) {
    perror("kill");
    return FALSE;
  }

  if (!s_stackEvent.Wait(1000)) {
    return TRUE;
  }

  s_stackEvent.Reset();
  kill(pid, SIGUSR2);

  return !s_stackEvent.Wait(20000);
}

static void CollectStackSignal(int sig) {
  printf("collect_stack_signal, pid=%d\n", getpid());

  if (getpid() != s_stackPid) {
    return;
  }

  GetStackCrawl(s_stackBuffer, s_stackBufferSize);
  s_stackEvent.Set();
}

static void CloseLog() {
  if (!s_logFile) {
    return;
  }

  if (s_crashFileName[0]) {
    if (!lseek(s_logFd, 0, SEEK_CUR)) {
      unlink(s_crashFileName);
    }

    fclose(s_logFile);
  } else {
    pclose(s_logFile);
  }

  s_logFile = 0;
}

static void SignalHandler(int sig) {
  char       stack[0x1000];
  char       command[0x1000];
  FRAMEDATA *frame;
  BOOL       handled;

  signal(sig, AbortHandler);

  if (!s_logFile) {
    AbortHandler(sig);
    return;
  }

  fprintf(s_logFile, "App: %s, pid=%d\n", s_appCommand, getpid());
  fprintf(s_logFile, "Command Line: %s\n", s_commandLine);
  fprintf(s_logFile, "Signal %d caught\n", sig);
  fprintf(s_logFile, "**********************************************************************\n");

  if (s_logCallback) {
    char header[0x1450];

    header[0] = 0;
    s_logCallback(header, sizeof(header));

    if (strlen(header)) {
      fprintf(s_logFile, "%s\n", header);
      fprintf(s_logFile, "**********************************************************************\n");
    }
  }

  frame = (FRAMEDATA *)pthread_getspecific(s_frameKey);
  if (frame) {
    fprintf(s_logFile, "Current frame data:\n%s\n", frame->data);
    fprintf(s_logFile, "**********************************************************************\n");
  }

  fflush(s_logFile);

  GetAddr2LineCommand(stack, sizeof(stack));
  fprintf(s_logFile, "Stack trace: %s\n", stack);
  fprintf(s_logFile, "**********************************************************************\n");
  fflush(s_logFile);
  WriteStackCrawl(s_logFd);

  SErrEnter();

  handled = FALSE;
  ITERATELIST(HANDLER, s_handlers, node) {
    if (!node->handler(sig, stack, "??", 0, "")) {
      handled = TRUE;
      break;
    }
  }

  SErrLeave();

  if (!handled) {
    if (s_debugger[0]) {
      if (!strcasecmp(s_debugger, "pause")) {
        printf("App paused on error, parent pid=%d, current pid=%d", s_parentPid, getpid());
      } else if (SInterlockedIncrement(&s_debugging) == 1) {
        sprintf(command, "%s %s %d &", s_debugger, s_appCommand, getpid());
        printf("Executing debugger: %s\n", command);
        printf("Parent PID was %d\n", s_parentPid);
        fprintf(s_logFile, "%s\n", command);
        fflush(s_logFile);
        fprintf(s_logFile, "---DONE\n");
        fflush(s_logFile);
        CloseLog();
      } else {
        printf("No debugger started for new error in pid=%d because already debugging\n", getpid());
      }

      sleep(120);
    } else {
      fprintf(s_logFile, "---DONE\n");
      fflush(s_logFile);
      CloseLog();
    }
  }

  if (s_crashFileName[0]) {
    fprintf(stderr, "Crash logged to file: %s\n", s_crashFileName);
  }

  while (getenv("WOWCRASHMAIL")) {
    sleep(20);
  }

  if (sig == SIGABRT) {
    return;
  }

  signal(SIGABRT, SIG_DFL);
  abort();
}

static void BrokenPipeHandler(int sig) {
  puts("Broken pipe handler called, ignoring");
}

static void FloatingPointHandler(int sig) {
  puts("Floating point exception, ignoring");
}

static void ChildHandler(int sig) {
  int status;

  while (waitpid(-1, &status, WNOHANG) > 0) {
  }
}

static void InstallCrashHandler() {
  char command[0x1000];

  s_parentPid = getpid();

  if (getenv("WOWDEBUGGER")) {
    SStrCopy(s_debugger, getenv("WOWDEBUGGER"), sizeof(s_debugger));
  } else {
    s_debugger[0] = 0;
  }

  if (getenv("WOWCRASHMAIL")) {
    SStrCopy(s_crashMail, getenv("WOWCRASHMAIL"), sizeof(s_crashMail));
    SStrPrintf(command, sizeof(command), "ServerCrashMailer %s %s", s_crashMail, s_appCommand);
    printf("Running: %s\n", command);

    s_logFile = popen(command, "w");
    if (!s_logFile) {
      perror("popen");
    }
  } else {
    s_crashMail[0] = 0;
  }

  if (s_logFile) {
    s_logFd = fileno(s_logFile);
  } else {
    LPCSTR appName = SStrChrR(s_appCommand, '/');

    if (appName) {
      ++appName;
    } else {
      appName = s_appCommand;
    }

    snprintf(s_crashFileName, sizeof(s_crashFileName), "%s/%s-%d-crash.XXXXXX", OsGetWritePath(), appName, getpid());

    s_logFd = mkstemp(s_crashFileName);
    if (s_logFd >= 0) {
      s_logFile = fdopen(s_logFd, "wb");
      if (!s_logFile) {
        close(s_logFd);
      }
    }
  }

  signal(SIGABRT, SignalHandler);
  signal(SIGSEGV, SignalHandler);
  signal(SIGINT, SignalHandler);
  signal(SIGILL, SignalHandler);
  signal(SIGFPE, SignalHandler);
  signal(SIGBUS, SignalHandler);
  signal(SIGTRAP, SignalHandler);
  signal(SIGCHLD, ChildHandler);
  signal(SIGUSR2, CollectStackSignal);
  signal(SIGPIPE, BrokenPipeHandler);

  pthread_key_create(&s_frameKey, 0);
}

void SErrInitialize() {
  InstallCrashHandler();
}

extern "C" BOOL APIENTRY SErrDestroy() {
  HANDLER *handler;

  CloseLog();

  while ((handler = s_handlers.Head()) != 0) {
    s_handlers.DeleteNode(handler);
  }

  return TRUE;
}

extern "C" void APIENTRY SErrRegisterHandler(SERRHANDLER handler) {
  HANDLER *node;

  SErrEnter();

  node = s_handlers.NewNode(LIST_HEAD, 0, 0x08000000);
  node->handler = handler;

  SErrLeave();
}

extern "C" void APIENTRY SErrSetAppCommand(LPCSTR command) {
  SStrCopy(s_appCommand, command, sizeof(s_appCommand));
}

void SErrSetCommandLine(LPCSTR commandLine) {
  SStrCopy(s_commandLine, commandLine, sizeof(s_commandLine));
}

void SErrClearFrameData() {
  FRAMEDATA *frame = (FRAMEDATA *)pthread_getspecific(s_frameKey);

  if (!frame) {
    frame = (FRAMEDATA *)ALLOC(sizeof(FRAMEDATA));
    pthread_setspecific(s_frameKey, frame);
  }

  frame->length = 0;
  frame->data[0] = 0;
}

void SErrAddFrameData(LPCSTR format, ...) {
  FRAMEDATA *frame = (FRAMEDATA *)pthread_getspecific(s_frameKey);
  DWORD      length;
  va_list    args;

  if (!frame) {
    return;
  }

  length = frame->length;
  va_start(args, format);

  if (length > sizeof(frame->data) - 3) {
    SErrClearFrameData();
    SStrPack(frame->data, "==>FRAME CLEARED\n", sizeof(frame->data));
    length = SStrLen(frame->data);
  }

  length += SStrVPrintf(frame->data + length, sizeof(frame->data) - length, format, args);
  va_end(args);

  frame->length = length;
}

extern "C" void APIENTRY SErrStartWatchdog(DWORD freezeSeconds, BOOL checkKeyboard) {
}

extern "C" void APIENTRY SErrStopWatchdog() {
}

extern "C" void APIENTRY SErrPingWatchdog() {
}
