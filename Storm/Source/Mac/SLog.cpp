#include <storm.h>

#include <Os/OsTime.h>
#include <Os/W32/OsFile.h>

#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define BUFFERSIZE 0x10000
#define FLUSHMARK  0xC000
#define SLOTS      4

#define SLOG_FLAG_OPENNOW    0x00000001
#define SLOG_FLAG_MEMORYONLY 0x00000002
#define SLOG_FLAG_APPEND     0x00000004

typedef struct _LOG {
  HSLOG   log;
  _LOG   *next;
  char    filename[0x100];
  HOSFILE file;
  DWORD   bufferused;
  DWORD   pendpoint;
  LONG    indent;
  int     timeStamp;
  char    buffer[BUFFERSIZE];
} LOGREC, *LOGPTR;

DECLARE_STRICT_HANDLE(HLOCKEDLOG);

static DWORD           lasttime;
static DWORD           timestrlen;
static char            timestr[64];
static pthread_mutex_t s_critsect[SLOTS] = {PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER};
static LOGPTR          s_loghead[SLOTS];
static HSLOG           s_sequence;
static pthread_mutex_t s_defaultdir_critsect = PTHREAD_MUTEX_INITIALIZER;
static char            s_defaultdir[0x100];
static BOOL            s_logsysteminit;

static void FlushLog(LOGPTR logptr) {
  DWORD byteswritten;

  if (!logptr->bufferused) {
    return;
  }

  if (logptr->file == HOSFILE_INVALID) {
    SErrDisplayError(STORM_ERROR_ASSERTION, __FILE__, __LINE__, "logptr->file != HOSFILE_INVALID", FALSE, 1);
  }

  OsWriteFile(logptr->file, logptr->buffer, logptr->bufferused, &byteswritten);
  logptr->bufferused = 0;
  logptr->pendpoint = 0;
}

static LOGPTR LockLog(HSLOG log, HLOCKEDLOG *lockedhandle, int createifnecessary) {
  DWORD   bucket;
  LOGPTR *link;
  LOGPTR  rec;

  if (!log) {
    *lockedhandle = (HLOCKEDLOG)-1;
    return NULL;
  }

  bucket = ((DWORD)(DWORD_PTR)log) & (SLOTS - 1);
  pthread_mutex_lock(&s_critsect[bucket]);
  *lockedhandle = (HLOCKEDLOG)(DWORD_PTR)bucket;

  link = &s_loghead[bucket];
  while (*link) {
    rec = *link;
    if (rec->log == log) {
      return rec;
    }
    link = &rec->next;
  }

  if (!createifnecessary) {
    pthread_mutex_unlock(&s_critsect[bucket]);
    *lockedhandle = (HLOCKEDLOG)-1;
    return NULL;
  }

  rec = (LOGPTR)SMemAlloc(sizeof(LOGREC), __FILE__, __LINE__, 0);
  *link = rec;
  if (!rec) {
    pthread_mutex_unlock(&s_critsect[bucket]);
    *lockedhandle = (HLOCKEDLOG)-1;
    return NULL;
  }

  rec->log = log;
  rec->next = NULL;
  rec->filename[0] = 0;
  rec->file = HOSFILE_INVALID;
  rec->bufferused = 0;
  rec->pendpoint = 0;
  return rec;
}

static void OutputIndent(LOGPTR logptr) {
  LONG count;

  if (logptr->indent <= 0) {
    return;
  }

  count = logptr->indent >= 0x80 ? 0x80 : logptr->indent;
  memset(logptr->buffer + logptr->bufferused, ' ', count);
  logptr->buffer[logptr->bufferused + count] = 0;
  logptr->bufferused += count;
}

static void OutputReturn(LOGPTR logptr) {
  memcpy(logptr->buffer + logptr->bufferused, "\r", 2);
  logptr->bufferused += strlen("\r");
}

static void OutputTime(LOGPTR logptr, int show) {
  time_t         now;
  struct tm     *systime;
  register DWORD tick;

  if (!logptr->timeStamp) {
    return;
  }

  tick = OsGetAsyncTimeMsPrecise();
  if (tick != lasttime) {
    lasttime = tick;
    time(&now);
    systime = localtime(&now);
    SStrPrintf(
        timestr, sizeof(timestr), "%u/%u %02u:%02u:%02u.%03u  ", systime->tm_mon, systime->tm_mday, systime->tm_hour, systime->tm_min, systime->tm_sec,
        tick % 1000
    );
    timestrlen = SStrLen(timestr);
  }

  if (show) {
    memcpy(logptr->buffer + logptr->bufferused, timestr, timestrlen + 1);
  } else {
    memset(logptr->buffer + logptr->bufferused, ' ', timestrlen);
    logptr->buffer[logptr->bufferused + timestrlen] = 0;
  }

  logptr->bufferused += timestrlen;

  OutputIndent(logptr);
}

static void UnlockDeleteLog(LOGPTR logptr, HLOCKEDLOG lockedhandle) {
  DWORD   bucket;
  LOGPTR *link;

  LOGPTR curr;

  bucket = (DWORD)(DWORD_PTR)lockedhandle;
  link = &s_loghead[bucket];
  while ((curr = *link) != NULL) {
    if (curr == logptr) {
      *link = curr->next;
      SMemFree(curr, __FILE__, __LINE__, 0);
      break;
    }
    link = &curr->next;
  }

  pthread_mutex_unlock(&s_critsect[bucket]);
}

static void UnlockLog(HLOCKEDLOG lockedhandle) {
  DWORD bucket = (DWORD)(DWORD_PTR)lockedhandle;
  pthread_mutex_unlock(&s_critsect[bucket]);
}

static LPCSTR PrependDefaultDir(char *newfilename, DWORD newfilenamesize, LPCSTR filename) {
  if (!s_defaultdir[0] || !filename || !filename[0] || filename[1] == ':' || SStrChr(filename, '\\')) {
    return filename;
  }

  pthread_mutex_lock(&s_defaultdir_critsect);
  if (s_defaultdir[0]) {
    DWORD length = SStrCopy(newfilename, s_defaultdir, newfilenamesize);
    pthread_mutex_unlock(&s_defaultdir_critsect);
    SStrCopy(newfilename + length, filename, newfilenamesize - length);
    return newfilename;
  }
  pthread_mutex_unlock(&s_defaultdir_critsect);

  return filename;
}

static BOOL OpenLogFile(LPCSTR filename, HOSFILE *file, DWORD flags) {
  if (filename && *filename) {
    char newfilename[0x100];
    filename = PrependDefaultDir(newfilename, sizeof(newfilename), filename);
    *file = OsCreateFile(filename, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0x3F3F3F3F);
    return *file != HOSFILE_INVALID;
  }

  *file = HOSFILE_INVALID;
  return FALSE;
}

static BOOL PrepareLog(LOGPTR logptr) {
  if (logptr->file == HOSFILE_INVALID && !OpenLogFile(logptr->filename, &logptr->file, 0)) {
    logptr->filename[0] = 0;
    return FALSE;
  }

  return TRUE;
}

extern "C" void APIENTRY SLogClose(HSLOG log) {
  HLOCKEDLOG lockedhandle;
  LOGPTR     rec;

  if (!s_logsysteminit) {
    return;
  }

  rec = LockLog(log, &lockedhandle, FALSE);
  if (!rec) {
    return;
  }

  if (rec->file != HOSFILE_INVALID) {
    FlushLog(rec);
    OsCloseFile(rec->file);
  }
  UnlockDeleteLog(rec, lockedhandle);
}

extern "C" BOOL APIENTRY SLogCreate(LPCSTR filename, DWORD flags, HSLOG *log) {
  HLOCKEDLOG lockedhandle;
  HOSFILE    file;
  LOGPTR     rec;

  VALIDATEBEGIN;
  VALIDATE(filename);
  VALIDATE(*filename);
  VALIDATEANDBLANK(log);
  VALIDATEEND;

  if (flags & SLOG_FLAG_MEMORYONLY) {
    flags &= ~SLOG_FLAG_OPENNOW;
    filename = "";
  }

  file = HOSFILE_INVALID;
  if ((flags & SLOG_FLAG_OPENNOW) && !OpenLogFile(filename, &file, flags)) {
    return FALSE;
  }

  s_sequence = (HSLOG)((DWORD_PTR)s_sequence + 1);
  *log = s_sequence;

  rec = LockLog(*log, &lockedhandle, TRUE);
  if (!rec) {
    *log = NULL;
    return FALSE;
  }

  rec->file = file;
  SStrCopy(rec->filename, filename, sizeof(rec->filename));
  rec->timeStamp = TRUE;
  rec->indent = 0;
  UnlockLog(lockedhandle);

  return TRUE;
}

extern "C" void APIENTRY SLogDestroy() {
  DWORD i;

  SLogFlushAll();

  for (i = 0; i < SLOTS; i++) {
    while (s_loghead[i]) {
      SLogClose(s_loghead[i]->log);
      SErrReportResourceLeak("HSLOG");
    }
  }

  s_logsysteminit = FALSE;
}

extern "C" void APIENTRY SLogFlushAll() {
  DWORD  i;
  LOGPTR rec;

  for (i = 0; i < SLOTS; i++) {
    pthread_mutex_lock(&s_critsect[i]);
    rec = s_loghead[i];
    while (rec) {
      if (rec->file != HOSFILE_INVALID) {
        FlushLog(rec);
      }
      rec = rec->next;
    }
    pthread_mutex_unlock(&s_critsect[i]);
  }
}

extern "C" void APIENTRY SLogInitialize() {
  if (s_logsysteminit) {
    return;
  }

  s_logsysteminit = TRUE;
}

extern "C" void APIENTRY SLogSetTimestamp(HSLOG log, BOOL timeStamp) {
  HLOCKEDLOG lockedhandle;
  LOGPTR     rec;

  rec = LockLog(log, &lockedhandle, FALSE);
  if (!rec) {
    return;
  }
  rec->timeStamp = timeStamp;
  UnlockLog(lockedhandle);
}

extern "C" void APIENTRY SLogVWrite(HSLOG log, LPCSTR format, va_list arglist) {
  HLOCKEDLOG lockedhandle;
  LOGPTR     rec;

  rec = LockLog(log, &lockedhandle, FALSE);
  if (!rec) {
    return;
  }
  if (PrepareLog(rec)) {
    OutputTime(rec, TRUE);
    vsprintf(rec->buffer + rec->bufferused, format, arglist);
    rec->bufferused += SStrLen(rec->buffer + rec->bufferused);
    OutputReturn(rec);

    rec->pendpoint = rec->bufferused;
    if (rec->bufferused >= FLUSHMARK) {
      FlushLog(rec);
    }
  }
  UnlockLog(lockedhandle);
}

extern "C" void __cdecl SLogWrite(HSLOG log, LPCSTR format, ...) {
  va_list arglist;

  va_start(arglist, format);
  SLogVWrite(log, format, arglist);
  va_end(arglist);
}
