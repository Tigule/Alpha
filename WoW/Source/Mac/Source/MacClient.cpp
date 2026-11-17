#define BOOL OBJC_BOOL
#import <AppKit/AppKit.h>
#undef BOOL

#include <storm.h>

#include "MacClient.h"
#include "Console/ConsoleCommand.h"
#include "Console/ConsoleVar.h"

#include <Os/Mac/InputMac.h>

#include <ctype.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

namespace MoreCocoa {
void Initialize();
}

void  CommonMain();
int   OsSetArgs(char *const *argv, bool resolveRealPath, bool writeDataPath);
void  OsLaunchURLSetCallback(void (*callback)());
void  SErrMacSetAssertOptions(DWORD options);
DWORD SErrMacGetAssertOptions();

int main(int argc, char **argv) {
  MacClient::SetupCommandLine(argc, argv);
  CommonMain();
  CFPreferencesAppSynchronize(kCFPreferencesCurrentApplication);

  return 0;
}

void MacClient::SetupCommandLine(int argc, char **argv) {
  static char  s_commandLine[1024];
  static char *s_argv[21];
  char         path[1024];
  CFURLRef     bundleUrl;
  FILE        *file;
  char        *cursor;
  char        *token;
  int          count;

  bundleUrl = CFBundleCopyBundleURL(CFBundleGetMainBundle());
  memset(s_commandLine, 0, sizeof(s_commandLine));

  if (CFURLGetFileSystemRepresentation(bundleUrl, 0, (UInt8 *)path, sizeof(path))) {
    char *slash = SStrChrR(path, '/');
    if (slash) {
      slash[1] = 0;
    }

    SStrPack(path, "commandline.txt", sizeof(path));

    file = fopen(path, "r");
    if (file) {
      fread(s_commandLine, 1, sizeof(s_commandLine) - 1, file);
      fclose(file);

      cursor = s_commandLine;
      count = 0;
      s_argv[count++] = argv[0];

      while (count < 20 && (token = strsep(&cursor, " \t\r\n")) != 0) {
        if (*token) {
          s_argv[count++] = token;
        }
      }

      s_argv[count] = 0;
      argv = s_argv;
    } else {
      NSLog(@"command line file not found!\n");
    }
  }

  OsSetArgs(argv, true, true);
}

void MacClient::Destroy() {
  OsLaunchURLSetCallback(0);
}

static void WoWExceptionHandler(int signo, siginfo_t *info, void *context) {
  char   text[1024];
  LPCSTR name = "";

  if (signo == SIGBUS) {
    name = "BusError";
  }

  SErrMacSetAssertOptions(28);
  SStrPrintf(
      text, sizeof(text), "signo: %d, errno: %d, code: %d, pid: %d, uid: %d, status: %d, addr: 0x%x, value: %d, band: %d", info->si_signo,
      info->si_errno, info->si_code, info->si_pid, info->si_uid, info->si_status, (DWORD)(DWORD_PTR)info->si_addr, info->si_value.sival_int,
      (int)info->si_band
  );
  SErrDisplayError(STORMERROR(132), name, SERR_LINECODE_EXCEPTION, text, FALSE, 1);
}

static OSErr s_DoAEQuit(const AppleEvent *event, AppleEvent *reply, SRefCon refcon) {
  [NSApp postEvent:[NSEvent otherEventWithType:NSEventTypeApplicationDefined
                                      location:NSZeroPoint
                                 modifierFlags:0
                                     timestamp:0
                                  windowNumber:0
                                       context:nil
                                       subtype:kEventCommandProcess
                                         data1:kHICommandQuit
                                         data2:0]
           atStart:NO];
  return noErr;
}

static void s_ForceWindowedMode() {
  CVar *window = CVar::Lookup("gxWindow");

  if (window && !window->GetInt()) {
    window->Set("1", true, false, false);
    ConsoleCommandExecute("gxRestart", 1);
  }
}

void MacClient::Initialize() {
  struct sigaction action;
  OSErr            err;

  OsMacApplicationStart();
  MoreCocoa::Initialize();
  MacClient::InitializeMenus();

  err = AEInstallEventHandler(kCoreEventClass, kAEQuitApplication, NewAEEventHandlerUPP(s_DoAEQuit), 0, false);
  if (!(err == noErr)) {
    SErrDisplayErrorFmt(
        STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1,
        isprint((err >> 24) & 0xFF) && isprint((err >> 16) & 0xFF) && isprint((err >> 8) & 0xFF) && isprint(err & 0xFF)
            ? "\"%s\", %s = %ld (0x%08X, '%c%c%c%c')"
            : "\"%s\", %s = %ld (0x%08X)",
        "err == noErr", "err", (long)err, err, (err >> 24) & 0xFF, (err >> 16) & 0xFF, (err >> 8) & 0xFF, err & 0xFF
    );
  }

  SErrMacSetAssertOptions((SErrMacGetAssertOptions() | 8) & ~3);

  memset(&action, 0, sizeof(action));
  action.sa_sigaction = WoWExceptionHandler;
  action.sa_flags = SA_SIGINFO | SA_RESETHAND | SA_RESTART;
  sigaction(SIGBUS, &action, 0);

  OsLaunchURLSetCallback(s_ForceWindowedMode);
}
