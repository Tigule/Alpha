#define BOOL OBJC_BOOL
#import <AppKit/AppKit.h>
#undef BOOL

#include <storm.h>
#include <stpl.h>

#include <Os/OsTime.h>
#include <Os/W32/OsFile.h>

#include <cxxabi.h>
#include <dlfcn.h>
#include <execinfo.h>
#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SERR_OPTION_CARBONALERT 0x00000001
#define SERR_OPTION_COCOAALERT  0x00000002
#define SERR_OPTION_STDERR      0x00000004
#define SERR_OPTION_ERRORFILE   0x00000008
#define SERR_OPTION_ABORT       0x00000010

#define SERR_STACKCRAWL_SIZE 0x4000

NODEDECL(HANDLER) {
  SERRHANDLER handler;
};

@interface SErrAssertion : NSObject {
  NSArray *args;
  int      result;
}
- (id)initWithArgs:(NSArray *)arguments;
- (void)runCriticalAlert:(id)arguments;
- (void)setResult:(int)value;
- (int)result;
@end

@implementation SErrAssertion

- (id)initWithArgs:(NSArray *)arguments {
  self = [super init];
  if (self) {
    args = arguments;
  }
  return self;
}

- (void)runCriticalAlert:(id)arguments {
  NSUInteger count = [args count];
  id         other = count > 4 ? [arguments objectAtIndex:4] : nil;
  id         alternate = count > 3 ? [arguments objectAtIndex:3] : nil;
  id         defaultButton = count > 2 ? [arguments objectAtIndex:2] : nil;
  id         message = count > 1 ? [arguments objectAtIndex:1] : nil;
  id         title = count > 0 ? [arguments objectAtIndex:0] : nil;

  [self setResult:(int)NSRunCriticalAlertPanel(title, @"%@", defaultButton, alternate, other, message)];
}

- (void)setResult:(int)value {
  result = value;
}

- (int)result {
  return result;
}

@end

static DWORD s_assertOptions = SERR_OPTION_CARBONALERT | SERR_OPTION_STDERR | SERR_OPTION_ABORT;

static pthread_mutex_t s_critsect = PTHREAD_MUTEX_INITIALIZER;
static LISTDECL(HANDLER, s_handlers);

static DWORD           s_lastError;
static int             s_displaying;
static SERRLOGCALLBACK s_logCallback;
static char            s_logTitle[0x80];
static char            s_appCommand[0x400] = "GenericBlizzardApp";
static char            s_lastLogPath[0x104];

static LPCSTR s_fatalFileName;
static int    s_fatalLineNumber;
static int    s_fatalProcessId;

static void UnmangleNames(char **names, int count) {
  int index;

  for (index = 0; index < count; ++index) {
    char *unmangled;
    int   status;

    if (!names[index]) {
      continue;
    }

    unmangled = abi::__cxa_demangle(names[index], 0, 0, &status);
    if (unmangled) {
      free(names[index]);
      names[index] = unmangled;
    }
  }
}

static void OutputFrames(
    char *buffer, unsigned int bufferSize, void *const *frames, unsigned long count, unsigned char unmangle, const char *prefix, const char *suffix,
    long startIndex
) {
  char        **names;
  unsigned long index;

  names = (char **)malloc(count * sizeof(char *));
  if (names) {
    for (index = 0; index < count; ++index) {
      Dl_info info;

      names[index] = dladdr(frames[index], &info) && info.dli_sname ? strdup(info.dli_sname) : 0;
    }

    if (unmangle) {
      UnmangleNames(names, (int)count);
    }
  }

  for (index = 0; index < count; ++index) {
    int written = snprintf(
        buffer, bufferSize, "%s%3d  0x%08lx %s%s", prefix ? prefix : "", (int)(index + startIndex), (unsigned long)frames[index],
        names && names[index] ? names[index] : "", suffix ? suffix : ""
    );

    if (written < 0 || (unsigned int)written >= bufferSize) {
      break;
    }

    buffer += written;
    bufferSize -= written;
  }

  if (names) {
    for (index = 0; index < count; ++index) {
      free(names[index]);
    }

    free(names);
  }
}

long StackCrawl_GenerateCrawlEx(char *buffer, long bufferSize, const char *prefix, const char *suffix, long skip, long count, void *thread) {
  void     *frames[256];
  long      total = skip + count;
  long      found = 0;
  DWORD_PTR pc;
  DWORD_PTR fp;

  if (total > 256) {
    total = 256;
  }

  if (!thread) {
    found = backtrace(frames, (int)total);
  } else {
#if defined(__arm64__)
    arm_thread_state64_t   state;
    mach_msg_type_number_t stateCount = ARM_THREAD_STATE64_COUNT;

    if (thread_get_state((thread_act_t)(DWORD_PTR)thread, ARM_THREAD_STATE64, (thread_state_t)&state, &stateCount) != KERN_SUCCESS) {
      return 0;
    }

    pc = (DWORD_PTR)arm_thread_state64_get_pc(state);
    fp = (DWORD_PTR)arm_thread_state64_get_fp(state);
#elif defined(__x86_64__)
    x86_thread_state64_t   state;
    mach_msg_type_number_t stateCount = x86_THREAD_STATE64_COUNT;

    if (thread_get_state((thread_act_t)(DWORD_PTR)thread, x86_THREAD_STATE64, (thread_state_t)&state, &stateCount) != KERN_SUCCESS) {
      return 0;
    }

    pc = (DWORD_PTR)state.__rip;
    fp = (DWORD_PTR)state.__rbp;
#endif

    frames[found++] = (void *)pc;

    while (found < total && fp) {
      DWORD_PTR      record[2];
      mach_vm_size_t size;

      if (mach_vm_read_overwrite(mach_task_self(), fp, sizeof(record), (mach_vm_address_t)record, &size) != KERN_SUCCESS || !record[1]) {
        break;
      }

      frames[found++] = (void *)record[1];

      if (record[0] <= fp) {
        break;
      }

      fp = record[0];
    }
  }

  if (skip > 0 && skip < found) {
    found -= skip;
  } else {
    skip = 0;
  }

  OutputFrames(buffer, (unsigned int)bufferSize, frames + skip, found, 1, prefix, suffix, skip);
  return found;
}

long StackCrawl_AllThreads(char *buffer, long bufferSize, const char *prefix, const char *suffix, long count) {
  thread_act_array_t     threads = NULL;
  mach_msg_type_number_t threadCount = 0;
  mach_port_t            self = mach_thread_self();
  kern_return_t          err;
  mach_msg_type_number_t index;

  err = task_threads(mach_task_self(), &threads, &threadCount);
  if (err != KERN_SUCCESS) {
    return err;
  }

  for (index = 0; index < threadCount; ++index) {
    if (threads[index] != self) {
      thread_suspend(threads[index]);
    }
  }

  for (index = 0; index < threadCount && bufferSize > 0; ++index) {
    long written = snprintf(buffer, bufferSize, "\n\nThread %d %s\n", index, threads[index] == self ? "(Crashed)" : "");

    if (written >= bufferSize) {
      break;
    }

    buffer += written;
    bufferSize -= written;

    StackCrawl_GenerateCrawlEx(buffer, bufferSize, prefix, suffix, 0, count, threads[index] == self ? 0 : (void *)(DWORD_PTR)threads[index]);

    written = strlen(buffer);
    buffer += written;
    bufferSize -= written;
  }

  for (index = 0; index < threadCount; ++index) {
    if (threads[index] != self) {
      thread_resume(threads[index]);
    }

    mach_port_deallocate(mach_task_self(), threads[index]);
  }

  mach_port_deallocate(mach_task_self(), self);
  vm_deallocate(mach_task_self(), (vm_address_t)threads, threadCount * sizeof(thread_act_t));
  return err;
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

  while ((handler = s_handlers.Head()) != NULL) {
    s_handlers.DeleteNode(handler);
  }

  return TRUE;
}

extern "C" void APIENTRY SErrRegisterHandler(SERRHANDLER handler) {
  HANDLER *node;

  pthread_mutex_lock(&s_critsect);

  node = s_handlers.NewNode(LIST_HEAD, 0, 0x08000000);
  node->handler = handler;

  pthread_mutex_unlock(&s_critsect);
}

extern "C" void APIENTRY SErrSetLogCallback(SERRLOGCALLBACK cb) {
  pthread_mutex_lock(&s_critsect);
  s_logCallback = cb;
  pthread_mutex_unlock(&s_critsect);
}

extern "C" void APIENTRY SErrSetLogTitleString(LPCSTR title) {
  pthread_mutex_lock(&s_critsect);
  SStrCopy(s_logTitle, title, sizeof(s_logTitle));
  pthread_mutex_unlock(&s_critsect);
}

extern "C" void APIENTRY SErrSetAppCommand(LPCSTR command) {
  SStrCopy(s_appCommand, command, sizeof(s_appCommand));
}

extern "C" BOOL APIENTRY SErrGetLogLastPath(char *buf, int size) {
  BOOL result = FALSE;

  pthread_mutex_lock(&s_critsect);

  if (s_lastLogPath[0]) {
    SStrCopy(buf, s_lastLogPath, size);
    result = TRUE;
  }

  pthread_mutex_unlock(&s_critsect);
  return result;
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

extern "C" BOOL APIENTRY SErrDisplayError(DWORD errorcode, LPCSTR filename, int linenumber, LPCSTR description, BOOL recoverable, UINT exitcode) {
  BOOL debug = FALSE;
  BOOL handled = FALSE;

  if (s_assertOptions & (SERR_OPTION_STDERR | SERR_OPTION_ERRORFILE)) {
    FILE         *destination[2];
    int           count = 0;
    FILE         *errorFile = NULL;
    OSSYSTEMTIME  localTime;
    CFTimeZoneRef timeZone;
    CFStringRef   abbreviation;
    const char   *zone;
    char          crawl[SERR_STACKCRAWL_SIZE];
    int           index;

    StackCrawl_AllThreads(crawl, sizeof(crawl), "", "\n", 40);

    if (s_assertOptions & SERR_OPTION_STDERR) {
      destination[count++] = stderr;
    }

    if (s_assertOptions & SERR_OPTION_ERRORFILE) {
      OSSYSTEMTIME sysTime;
      char         path[0x104];
      char         name[0x104];
      char        *lastSlash;

      OsGetExePath(path, sizeof(path));

      lastSlash = SStrChrR(path, '/');
      if (lastSlash) {
        lastSlash[1] = 0;
      }

      SStrPack(path, "Errors/", sizeof(path));
      if (!OsCreateDirectory(path, 0) && lastSlash) {
        lastSlash[1] = 0;
      }

      OsGetLocalTime(&sysTime);
      SStrPrintf(
          name, sizeof(name), "%04d-%02d-%02d %02d.%02d.%02d %s.%s", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute,
          sysTime.second, "Error", "txt"
      );
      SStrPack(path, name, sizeof(path));

      errorFile = fopen(path, "w");
      if (errorFile) {
        if (s_logCallback) {
          char header[0x1450];

          header[0] = 0;
          s_logCallback(header, sizeof(header));

          if (strlen(header)) {
            fprintf(errorFile, "%s", header);
          }
        }

        SStrCopy(s_lastLogPath, path, sizeof(s_lastLogPath));
        destination[count++] = errorFile;
      }
    }

    OsGetLocalTime(&localTime);

    timeZone = CFTimeZoneCopySystem();
    abbreviation = CFTimeZoneCopyAbbreviation(timeZone, CFAbsoluteTimeGetCurrent());

    zone = [(NSString *)abbreviation UTF8String];
    if (!zone) {
      zone = "GMT";
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
          file, " Time:      %04d-%02d-%02d %02d.%02d.%02d %s\n", localTime.year, localTime.month, localTime.day, localTime.hour, localTime.minute,
          localTime.second, zone
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

    [(NSString *)abbreviation release];
    CFRelease(timeZone);
  }

  if (s_displaying) {
    return TRUE;
  }

  s_displaying = 1;

  pthread_mutex_lock(&s_critsect);

  ITERATELIST(HANDLER, s_handlers, node) {
    if (!node->handler(errorcode, "Assertion Failed", filename, linenumber, description ? description : "")) {
      handled = TRUE;
      break;
    }
  }

  pthread_mutex_unlock(&s_critsect);

  if (!handled && (s_assertOptions & (SERR_OPTION_CARBONALERT | SERR_OPTION_COCOAALERT))) {
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    NSString          *message;

    message = description ? [[NSString stringWithUTF8String:description] stringByAppendingString:@"\r"] : @"";
    message = [message stringByAppendingFormat:@"%@:%d", filename ? [NSString stringWithUTF8String:filename] : @"", linenumber];

    if (s_assertOptions & SERR_OPTION_CARBONALERT) {
      CFOptionFlags response = kCFUserNotificationDefaultResponse;

      CFUserNotificationDisplayAlert(
          0, kCFUserNotificationStopAlertLevel, 0, 0, 0, (CFStringRef) @"Assertion Failed!", (CFStringRef)message, 0, 0, 0, &response
      );

      if (response == kCFUserNotificationOtherResponse) {
        debug = TRUE;
      }
    } else if (s_assertOptions & SERR_OPTION_COCOAALERT) {
      NSArray       *arguments = [NSArray arrayWithObjects:@"Assertion Failed!", message, @"OK", nil];
      SErrAssertion *assertion = [[[SErrAssertion alloc] initWithArgs:arguments] autorelease];

      [assertion performSelectorOnMainThread:@selector(runCriticalAlert:) withObject:arguments waitUntilDone:YES];

      if (![assertion result]) {
        debug = TRUE;
      }
    }

    [pool release];
  }

  s_displaying = 0;

  if (debug) {
    __builtin_debugtrap();
  }

  if (!recoverable) {
    if (s_assertOptions & SERR_OPTION_ABORT) {
      abort();
    }

    exit(exitcode);
  }

  return TRUE;
}

extern "C" BOOL __cdecl SErrDisplayErrorFmt(DWORD errorcode, LPCSTR filename, int linenumber, BOOL recoverable, UINT exitcode, LPCSTR format, ...) {
  char    description[0x400];
  va_list args;

  va_start(args, format);
  SStrVPrintf(description, sizeof(description), format, args);
  va_end(args);

  return SErrDisplayError(errorcode, filename, linenumber, description, recoverable, exitcode);
}

extern "C" void __cdecl SErrDisplayAppFatal(LPCSTR format, ...) {
  LPCSTR  filename = NULL;
  int     linenumber = 0;
  char    description[0x400];
  va_list args;

  pthread_mutex_lock(&s_critsect);

  if (s_fatalProcessId == getpid()) {
    filename = s_fatalFileName;
    linenumber = s_fatalLineNumber;

    s_fatalFileName = NULL;
    s_fatalLineNumber = 0;
    s_fatalProcessId = 0;
  }

  pthread_mutex_unlock(&s_critsect);

  va_start(args, format);
  SStrVPrintf(description, sizeof(description), format, args);
  va_end(args);

  SErrDisplayError(STORMERROR(132), filename, linenumber, description, FALSE, 1);
}

extern "C" BOOL APIENTRY SErrGetErrorStr(DWORD errorcode, char *buffer, DWORD bufferchars) {
  VALIDATEBEGIN;
  VALIDATE(buffer);
  VALIDATE(bufferchars);
  VALIDATEEND;

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

extern "C" void APIENTRY SErrRegisterThread(HANDLE thread, DWORD threadid) {
}

extern "C" void APIENTRY SErrUnregisterThread(HANDLE thread, DWORD threadid) {
}
