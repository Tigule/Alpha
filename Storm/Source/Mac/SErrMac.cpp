#include <storm.h>
#include <stpl.h>

#include <Os/OsTime.h>
#include <Os/W32/OsFile.h>

#include <execinfo.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <CoreFoundation/CoreFoundation.h>

#define SERR_OPTION_CARBONALERT 0x00000001
#define SERR_OPTION_COCOAALERT  0x00000002
#define SERR_OPTION_STDERR      0x00000004
#define SERR_OPTION_ERRORFILE   0x00000008
#define SERR_OPTION_ABORT       0x00000010

#define SERR_STACKCRAWL_SIZE 0x4000

NODEDECL(HANDLER) {
  SERRHANDLER handler;
};

static DWORD s_assertOptions = SERR_OPTION_STDERR | SERR_OPTION_ERRORFILE;

static pthread_mutex_t s_critsect = PTHREAD_MUTEX_INITIALIZER;
static LISTDECL(HANDLER, s_handlers);

static DWORD           s_lastError;
static int             s_displaying;
static SERRLOGCALLBACK s_logCallback;
static char            s_logTitle[0x100];
static char            s_appCommand[0x104] = "GenericBlizzardApp";
static char            s_lastLogPath[0x104];

static const char *s_fatalFileName;
static int         s_fatalLineNumber;
static int         s_fatalProcessId;

static void StackCrawl(char *buffer, DWORD bufferchars, const char *prefix, const char *suffix) {
  void  *frames[40];
  int    count;
  char **symbols;
  int    index;
  DWORD  used = 0;

  buffer[0] = 0;

  count = backtrace(frames, 40);
  symbols = backtrace_symbols(frames, count);

  if (!symbols) {
    return;
  }

  for (index = 0; index < count && used < bufferchars; ++index) {
    used += SStrPrintf(buffer + used, bufferchars - used, "%s%s%s", prefix, symbols[index], suffix);
  }

  free(symbols);
}

extern "C" DWORD APIENTRY SErrGetLastError() {
  return s_lastError;
}

extern "C" void APIENTRY SErrSetLastError(DWORD errorcode) {
  s_lastError = errorcode;
}

extern "C" BOOL APIENTRY SErrIsDisplayingError() {
  return s_displaying;
}

void SErrInitialize() {
}

extern "C" BOOL APIENTRY SErrDestroy() {
  HANDLER *handler;

  pthread_mutex_lock(&s_critsect);

  while ((handler = s_handlers.Head()) != 0) {
    s_handlers.DeleteNode(handler);
  }

  pthread_mutex_unlock(&s_critsect);
  return TRUE;
}

extern "C" void APIENTRY SErrRegisterHandler(SERRHANDLER handler) {
  HANDLER *node;

  pthread_mutex_lock(&s_critsect);

  node = s_handlers.NewNode(LIST_HEAD, 0, 0);
  node->handler = handler;

  pthread_mutex_unlock(&s_critsect);
}

extern "C" void APIENTRY SErrUnregisterHandler(SERRHANDLER handler) {
  pthread_mutex_lock(&s_critsect);

  ITERATELIST(HANDLER, s_handlers, node) {
    if (node->handler == handler) {
      s_handlers.DeleteNode(node);
      break;
    }
  }

  pthread_mutex_unlock(&s_critsect);
}

extern "C" void APIENTRY SErrSetLogCallback(SERRLOGCALLBACK cb) {
  s_logCallback = cb;
}

extern "C" void APIENTRY SErrSetLogTitleString(LPCSTR title) {
  SStrCopy(s_logTitle, title, sizeof(s_logTitle));
}

extern "C" void APIENTRY SErrSetAppCommand(LPCSTR command) {
  SStrCopy(s_appCommand, command, sizeof(s_appCommand));
}

extern "C" BOOL APIENTRY SErrGetLogLastPath(char *buf, int size) {
  if (!s_lastLogPath[0]) {
    return FALSE;
  }

  SStrCopy(buf, s_lastLogPath, size);
  return TRUE;
}

extern "C" void APIENTRY SErrCatchUnhandledExceptions() {
}

DWORD SErrMacGetAssertOptions() {
  return s_assertOptions;
}

void SErrMacSetAssertOptions(DWORD options) {
  s_assertOptions = options;
}

extern "C" void APIENTRY SErrPrepareAppFatal(LPCSTR filename, int linenumber) {
  pthread_mutex_lock(&s_critsect);

  s_fatalFileName = filename;
  s_fatalLineNumber = linenumber;
  s_fatalProcessId = getpid();

  pthread_mutex_unlock(&s_critsect);
}

static FILE *OpenErrorFile() {
  OSSYSTEMTIME sysTime;
  char         path[0x104];
  char         name[0x104];
  char        *lastSlash;
  FILE        *file;

  OsGetExePath(path, sizeof(path));

  lastSlash = SStrChrR(path, '/');
  if (lastSlash) {
    lastSlash[1] = 0;
  }

  SStrPack(path, "Errors/", sizeof(path));
  OsCreateDirectory(path, 0);

  OsGetSystemTime(&sysTime);
  SStrPrintf(
      name, sizeof(name), "%04d-%02d-%02d %02d.%02d.%02d %s.%s", sysTime.year, sysTime.month, sysTime.day, sysTime.hour,
      sysTime.minute, sysTime.second, "Error", "txt"
  );
  SStrPack(path, name, sizeof(path));

  file = fopen(path, "w");
  if (file) {
    if (s_logCallback) {
      char header[0x1450];

      header[0] = 0;
      s_logCallback(header, sizeof(header));

      if (strlen(header)) {
        fprintf(file, "%s", header);
      }
    }

    SStrCopy(s_lastLogPath, path, sizeof(s_lastLogPath));
  }

  return file;
}

extern "C" BOOL APIENTRY
SErrDisplayError(DWORD errorcode, LPCSTR filename, int linenumber, LPCSTR description, BOOL recoverable, UINT exitcode) {
  if (s_assertOptions & (SERR_OPTION_STDERR | SERR_OPTION_ERRORFILE)) {
    FILE        *destination[2];
    int          count = 0;
    FILE        *errorFile = 0;
    OSSYSTEMTIME localTime;
    CFTimeZoneRef timeZone;
    CFStringRef  abbreviation;
    char         zone[0x40];
    char         crawl[SERR_STACKCRAWL_SIZE];
    int          index;

    StackCrawl(crawl, sizeof(crawl), "", "\n");

    if (s_assertOptions & SERR_OPTION_STDERR) {
      destination[count++] = stderr;
    }

    if (s_assertOptions & SERR_OPTION_ERRORFILE) {
      errorFile = OpenErrorFile();
      if (errorFile) {
        destination[count++] = errorFile;
      }
    }

    OsGetLocalTime(&localTime);

    timeZone = CFTimeZoneCopySystem();
    abbreviation = CFTimeZoneCopyAbbreviation(timeZone, CFAbsoluteTimeGetCurrent());

    if (!abbreviation || !CFStringGetCString(abbreviation, zone, sizeof(zone), kCFStringEncodingUTF8)) {
      SStrCopy(zone, "GMT", sizeof(zone));
    }

    for (index = 0; index < count; ++index) {
      FILE *file = destination[index];

      fprintf(file, "\n=========================================================\n");

      if (linenumber == SERR_LINECODE_EXCEPTION) {
        fprintf(file, "Exception Raised!\n\n");
        fprintf(file, " App:       %s\n", s_appCommand);
        fprintf(file, " Exception: %s\n", filename);
      } else {
        fprintf(file, "Assertion Failed!\n\n");
        fprintf(file, " App:       %s\n", s_appCommand);
        fprintf(file, " File:      %s\n", filename);
        fprintf(file, " Line:      %d\n", linenumber);
      }

      fprintf(
          file, " Time:      %04d-%02d-%02d %02d.%02d.%02d %s\n", localTime.year, localTime.month, localTime.day,
          localTime.hour, localTime.minute, localTime.second, zone
      );
      fprintf(file, "\n");

      if (linenumber == SERR_LINECODE_EXCEPTION) {
        fprintf(file, " Error: %s\n\n", description);
      } else {
        fprintf(file, " Assertion: %s\n\n", description);
      }

      fprintf(file, "---------------------------------------------------------\n");
      fprintf(file, "%s", crawl);
      fprintf(file, "=========================================================\n");
    }

    if (errorFile) {
      fclose(errorFile);
    }

    if (abbreviation) {
      CFRelease(abbreviation);
    }

    CFRelease(timeZone);
  }

  if (s_displaying) {
    return TRUE;
  }

  s_displaying = 1;

  pthread_mutex_lock(&s_critsect);

  ITERATELIST(HANDLER, s_handlers, node) {
    if (!node->handler(errorcode, s_appCommand, filename, linenumber, description ? description : "")) {
      break;
    }
  }

  pthread_mutex_unlock(&s_critsect);

  s_displaying = 0;

  if (!recoverable) {
    if (s_assertOptions & SERR_OPTION_ABORT) {
      abort();
    }

    exit(exitcode);
  }

  return TRUE;
}

extern "C" BOOL __cdecl
SErrDisplayErrorFmt(DWORD errorcode, LPCSTR filename, int linenumber, BOOL recoverable, UINT exitcode, LPCSTR format, ...) {
  char    description[0x400];
  va_list args;

  va_start(args, format);
  SStrVPrintf(description, sizeof(description), format, args);
  va_end(args);

  return SErrDisplayError(errorcode, filename, linenumber, description, recoverable, exitcode);
}

extern "C" void __cdecl SErrDisplayAppFatal(LPCSTR format, ...) {
  const char *filename = 0;
  int         linenumber = 0;
  char        description[0x400];
  va_list     args;

  pthread_mutex_lock(&s_critsect);

  if (s_fatalProcessId == getpid()) {
    filename = s_fatalFileName;
    linenumber = s_fatalLineNumber;

    s_fatalFileName = 0;
    s_fatalLineNumber = 0;
    s_fatalProcessId = 0;
  }

  pthread_mutex_unlock(&s_critsect);

  va_start(args, format);
  SStrVPrintf(description, sizeof(description), format, args);
  va_end(args);

  SErrDisplayError(STORMERROR(68), filename, linenumber, description, FALSE, 1);
}

extern "C" BOOL APIENTRY SErrGetErrorStr(DWORD errorcode, char *buffer, DWORD bufferchars) {
  FATALASSERT(buffer);
  FATALASSERT(bufferchars);

  buffer[0] = 0;
  return FALSE;
}

extern "C" void APIENTRY SErrStartWatchdog(DWORD freezeSeconds, BOOL checkKeyboard) {
}

extern "C" void APIENTRY SErrStopWatchdog() {
}

extern "C" void APIENTRY SErrPingWatchdog() {
}

extern "C" void APIENTRY SErrPauseWatchdog() {
}

extern "C" void APIENTRY SErrResumeWatchdog() {
}

extern "C" void APIENTRY SErrReportResourceLeak(LPCSTR handlename) {
  SErrDisplayErrorFmt(STORMERROR(127), __FILE__, __LINE__, TRUE, 1, "%s handle never released", handlename);
}

extern "C" void APIENTRY SErrReportNamedResourceLeak(LPCSTR handlename, LPCSTR name) {
  SErrDisplayErrorFmt(STORMERROR(127), __FILE__, __LINE__, TRUE, 1, "%s handle never released: %s", handlename, name);
}

extern "C" void APIENTRY SErrRegisterThread(HANDLE thread, DWORD threadid) {
}

extern "C" void APIENTRY SErrUnregisterThread(HANDLE thread, DWORD threadid) {
}
