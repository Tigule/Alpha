#include <ApplicationServices/ApplicationServices.h>

#include <string.h>

extern "C" OSErr GetApplicationInformation(StringPtr name, FSRef *appRef, OSType *signature, ProcessSerialNumber *psn) {
  static bool                s_haveInfo;
  static ProcessSerialNumber s_psn;
  static OSType              s_signature;
  static Str63               s_name;
  static FSRef               s_appRef;
  ProcessInfoRec             info;
  OSErr                      err;

  if (!s_haveInfo) {
    err = GetCurrentProcess(&s_psn);
    if (err != noErr) {
      return err;
    }

    memset(&info, 0, sizeof(info));
    info.processInfoLength = sizeof(info);
    info.processName = s_name;
    info.processAppRef = &s_appRef;

    err = GetProcessInformation(&s_psn, &info);
    if (err != noErr) {
      return err;
    }

    s_signature = info.processSignature;
    s_haveInfo = true;
  }

  if (signature) {
    *signature = s_signature;
  }

  if (psn) {
    *psn = s_psn;
  }

  if (appRef) {
    *appRef = s_appRef;
  }

  if (name) {
    memcpy(name, s_name, s_name[0] + 1);
  }

  return noErr;
}
