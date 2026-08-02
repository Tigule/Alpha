#include "SysMessage.h"

#include <Base/Status.h>
#include <Os/W32/OsFile.h>
#include <storm.h>
#include <stpl.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <typeinfo>

NODEDECL(MSGBUFFER) {
  MSGBUFFER() : string(0), timeVisible(0.0f), severity(SYSMSG_INFO), categoryMask(0) {
  }

  ~MSGBUFFER() {
    FREEIFUSED(string);
  }

  void SetInfo(const char *newString, SYSMSG_TYPE newSeverity, unsigned int categories);

  char        *string;
  float        timeVisible;
  SYSMSG_TYPE  severity;
  unsigned int categoryMask;
};

static LISTDECL(MSGBUFFER, s_msgBuffer);

static const char s_categoryMaskLetters[7] = {'G', 'W', 'U', 'A', 'M', 'O', 'S'};

static const char *s_severityStrings[SYSMSG_NUMTYPES] = {"INFO", "WARNING", "ERROR", "FATAL"};

static struct {
  unsigned char red;
  unsigned char green;
  unsigned char blue;
  unsigned char unused;
  float         timeVisible;
} s_severityDisplay[SYSMSG_NUMTYPES] = {
    {255, 255, 255, 0, 10.0f},
    {255, 255, 127, 0, 15.0f},
    {255, 127, 127, 0, 15.0f},
    {127, 255, 255, 0, 20.0f}
};

static int            s_enabled = 1;
static SYSMSG_TYPE    s_minSeverity = SYSMSG_INFO;
static SYSMSG_TYPE    s_maxSeverity = SYSMSG_FATAL;
static unsigned int   s_categoryFilter = 0xFFFFFFFF;
static HOSFILE        s_osFile;
static SYSMSGCALLBACK s_callback;

static void GenerateMaskString(char *buffer, unsigned int size, unsigned int maskString);
static int DetermineFileName(const char *curDir, char *buffer, unsigned int size);

void MSGBUFFER::SetInfo(const char *newString, SYSMSG_TYPE newSeverity, unsigned int categories) {
  FREEIFUSED(string);

  if (newString) {
    string = SStrDupA(newString, __FILE__, __LINE__);
  } else {
    string = 0;
  }

  severity = newSeverity;
  categoryMask = categories;
}

static void GenerateMaskString(char *buffer, unsigned int size, unsigned int maskString) {
  char         maskLetters[7];
  unsigned int i;

  for (i = 0; i < 7; ++i) {
    maskLetters[i] = maskString & (1 << i) ? s_categoryMaskLetters[i] : '-';
  }

  SStrPrintf(
      buffer, size, "%c%c%c%c%c%c%c", maskLetters[0], maskLetters[1], maskLetters[2], maskLetters[3], maskLetters[4], maskLetters[5], maskLetters[6]
  );
}

static int DetermineFileName(const char *curDir, char *buffer, unsigned int size) {
  unsigned int index;

  for (index = 0; index < 1000; ++index) {
    ASSERT(curDir);

    SStrPrintf(buffer, size, "%sSysMsgLog%03d.txt", curDir, index);
    if (!OsFileExists(buffer)) {
      return 1;
    }
  }

  return 0;
}

int SysMsgAdd(const char *msg, SYSMSG_TYPE severity, unsigned int categoryMask) {
  char          string[512];
  char          maskString[32] = "";

  FATALASSERT(msg);

  FATALASSERT(severity < SYSMSG_NUMTYPES);

  if (severity >= s_minSeverity && severity <= s_maxSeverity && (categoryMask & s_categoryFilter) && s_enabled && msg[0]) {
    GenerateMaskString(maskString, sizeof(maskString), categoryMask);
    SStrPrintf(string, sizeof(string), "%s|%s|%s\r\n", maskString, s_severityStrings[severity], msg);

    if (s_osFile) {
      OsWriteFile(s_osFile, string, SStrLen(string), reinterpret_cast<unsigned long *>(&categoryMask));
    }

    if (s_callback) {
      s_callback(msg, severity);
    }
  }

  return 1;
}

int SysMsgAdd(const CStatus &status, unsigned int categoryMask) {
  char *msg;
  int   result;

  if (status.IsEmpty()) {
    return 1;
  }

  msg = status.GetErrorStrAlloc(STATUS_INFO);
  result = SysMsgAdd(msg, static_cast<SYSMSG_TYPE>(status.GetHighestSeverity()), categoryMask);
  FREE(msg);
  return result;
}

int __cdecl SysMsgVPrintf(SYSMSG_TYPE severity, unsigned int categoryMask, const char *format, char *arglist) {
  char buff[256];

  _vsnprintf(buff, sizeof(buff), format, arglist);
  buff[sizeof(buff) - 1] = 0;
  return SysMsgAdd(buff, severity, categoryMask);
}

int __cdecl SysMsgPrintf(SYSMSG_TYPE severity, unsigned int categoryMask, const char *format, ...) {
  va_list arglist;

  FATALASSERT(format);

  va_start(arglist, format);
  return SysMsgVPrintf(severity, categoryMask, format, arglist);
}

void SysMsgEnable(int enable) {
  s_enabled = enable;
}

int SysMsgEnabled() {
  return s_enabled;
}

void SysMsgSetMinDisplayLevel(SYSMSG_TYPE minSeverity) {
  ASSERT(minSeverity < SYSMSG_NUMTYPES);

  s_minSeverity = minSeverity;
  if (s_maxSeverity < minSeverity) {
    s_maxSeverity = minSeverity;
  }
}

void SysMsgSetMaxDisplayLevel(SYSMSG_TYPE maxSeverity) {
  ASSERT(maxSeverity < SYSMSG_NUMTYPES);

  s_maxSeverity = maxSeverity;
  if (s_minSeverity > maxSeverity) {
    s_minSeverity = maxSeverity;
  }
}

SYSMSG_TYPE SysMsgGetMinDisplayLevel() {
  return s_minSeverity;
}

SYSMSG_TYPE SysMsgGetMaxDisplayLevel() {
  return s_maxSeverity;
}

void SysMsgSetFilter(unsigned int categoryFilter) {
  s_categoryFilter = categoryFilter;
}

unsigned int SysMsgGetFilter() {
  return s_categoryFilter;
}

void SysMsgGetSeverityColor(SYSMSG_TYPE severity, float &r, float &g, float &b) {
  r = s_severityDisplay[severity].red * 0.0039215689f;
  g = s_severityDisplay[severity].green * 0.0039215689f;
  b = s_severityDisplay[severity].blue * 0.0039215689f;
}

float SysMsgGetSeverityDuration(SYSMSG_TYPE severity) {
  return s_severityDisplay[severity].timeVisible;
}

void SysMsgInitialize() {
  s_enabled = 1;
  s_minSeverity = SYSMSG_INFO;
  s_maxSeverity = SYSMSG_FATAL;
  s_categoryFilter = 0xFFFFFFFF;
}

void SysMsgShutdown() {
  MSGBUFFER *msg;

  s_enabled = 0;
  while ((msg = s_msgBuffer.Head()) != 0) {
    s_msgBuffer.UnlinkNode(msg);
    msg->~MSGBUFFER();
    SMemFree(msg, typeid(MSGBUFFER).raw_name(), SERR_LINECODE_OBJECT, 0);
  }

  SysMsgDisableFileLog();
}

void SysMsgEnableFileLog(const char *baseDir) {
  char fileName[MAX_PATH];
  char file[MAX_PATH] = "";

  if (baseDir) {
    SStrPrintf(file, sizeof(file), "%s", baseDir);
  }

  if (!OsDirectoryExists(file)) {
    OsCreateDirectory(file, 1);
  }

  if (!OsDirectoryExists(file)) {
    SysMsgPrintf(SYSMSG_ERROR, 1, "Error cannot create directory %s", file);
    return;
  }

  SysMsgDisableFileLog();
  if (DetermineFileName(file, fileName, sizeof(fileName))) {
    s_osFile = OsCreateFile(fileName, 0x40000000, 1, 4, 0x80, 0x3F3F3F3F);
  }

  if (s_osFile == reinterpret_cast<HOSFILE>(-1)) {
    s_osFile = 0;
  }
}

void SysMsgDisableFileLog() {
  if (s_osFile) {
    OsCloseFile(s_osFile);
  }
  s_osFile = 0;
}

void SysMsgSetCallback(SYSMSGCALLBACK callback) {
  s_callback = callback;
}
