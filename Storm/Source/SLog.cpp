#include <storm.h>

#include <stdarg.h>

#define BUFFERSIZE 0x10000
#define FLUSHMARK  0xC000
#define SLOTS      4

#define SLOG_FLAG_OPENNOW    0x00000001
#define SLOG_FLAG_MEMORYONLY 0x00000002
#define SLOG_FLAG_APPEND     0x00000004

typedef struct _LOG {
  HSLOG  log;
  _LOG  *next;
  char   filename[MAX_PATH];
  HANDLE file;
  DWORD  flags;
  DWORD  bufferused;
  DWORD  pendpoint;
  long   indent;
  int    timeStamp;
  char   buffer[BUFFERSIZE];
} LOGREC, *LOGPTR;

DECLARE_STRICT_HANDLE(HLOCKEDLOG);

static DWORD            lasttime;
static DWORD            timestrlen;
static char             timestr[64];
static CRITICAL_SECTION s_critsect[SLOTS];
static LOGPTR           s_loghead[SLOTS];
static HSLOG            s_sequence;
static CRITICAL_SECTION s_defaultdir_critsect;
static char             s_defaultdir[MAX_PATH];
static BOOL             s_logsysteminit;

static DWORD PathGetRootChars(LPCSTR path) {
  LPCSTR cursor;
  DWORD  count;
  DWORD  len;

  if (path[0] == '/') {
    return 1;
  }

  len = SStrLen(path);
  if (len < 2) {
    return 0;
  }

  if (path[1] == ':') {
    return 2 + (path[2] == '\\');
  }

  if (path[0] == '\\' && path[1] == '\\') {
    cursor = path + 2;
    count = 2;
    while (count--) {
      cursor = SStrChr(cursor, '\\');
      if (cursor) {
        cursor++;
      }
    }
    if (cursor) {
      return (DWORD)(cursor - path);
    }
    return len;
  }

  return 0;
}

static void PathStripFilename(char *path) {
  char *slash;
  char *forwardSlash;
  char *root;

  slash = SStrChrR(path, '\\');
  forwardSlash = SStrChrR(path, '/');
  if (!slash || slash <= forwardSlash) {
    slash = forwardSlash;
  }

  if (slash) {
    root = path + PathGetRootChars(path);
    if (slash < root) {
      *root = 0;
    } else {
      slash[1] = 0;
    }
  }
}

static void PathConvertSlashes(char *path) {
  for (path = SStrChr(path, '/'); path; path = SStrChr(path + 1, '/')) {
    *path = '\\';
  }
}

static BOOL CreateFileDirectory(LPCSTR path) {
  char  buffer[MAX_PATH];
  char *cursor;
  char *slash;

  FATALASSERT(path);

  SStrCopy(buffer, path, sizeof(buffer));
  PathStripFilename(buffer);
  cursor = buffer + PathGetRootChars(buffer);
  PathConvertSlashes(cursor);

  for (slash = SStrChr(cursor, '\\'); slash; slash = SStrChr(slash + 1, '\\')) {
    *slash = 0;
    CreateDirectoryA(buffer, NULL);
    *slash = '\\';
  }

  return CreateDirectoryA(buffer, NULL);
}

static void FlushLog(LOGPTR logptr) {
  DWORD byteswritten;

  if (!logptr->bufferused) {
    return;
  }

  if (logptr->file == INVALID_HANDLE_VALUE) {
    SErrDisplayError(0x85100000, __FILE__, __LINE__, "logptr->file != ((HANDLE)(LONG_PTR)-1)", FALSE, 1);
  }

  WriteFile(logptr->file, logptr->buffer, logptr->bufferused, &byteswritten, NULL);
  FlushFileBuffers(logptr->file);
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

  bucket = ((DWORD)log) & (SLOTS - 1);
  EnterCriticalSection(&s_critsect[bucket]);
  *lockedhandle = (HLOCKEDLOG)bucket;

  link = &s_loghead[bucket];
  while (*link) {
    rec = *link;
    if (rec->log == log) {
      return rec;
    }
    link = &rec->next;
  }

  if (!createifnecessary) {
    LeaveCriticalSection(&s_critsect[bucket]);
    *lockedhandle = (HLOCKEDLOG)-1;
    return NULL;
  }

  rec = (LOGPTR)VirtualAlloc(NULL, sizeof(LOGREC), MEM_COMMIT, PAGE_READWRITE);
  *link = rec;
  if (!rec) {
    LeaveCriticalSection(&s_critsect[bucket]);
    *lockedhandle = (HLOCKEDLOG)-1;
    return NULL;
  }

  rec->log = log;
  rec->next = NULL;
  rec->filename[0] = 0;
  rec->file = INVALID_HANDLE_VALUE;
  rec->bufferused = 0;
  rec->pendpoint = 0;
  return rec;
}

static void OutputIndent(LOGPTR logptr) {
  long count;

  if (logptr->indent <= 0) {
    return;
  }

  count = logptr->indent >= 0x80 ? 0x80 : logptr->indent;
  memset(logptr->buffer + logptr->bufferused, ' ', count);
  logptr->buffer[logptr->bufferused + count] = 0;
  logptr->bufferused += count;
}

static void OutputReturn(LOGPTR logptr) {
  memcpy(logptr->buffer + logptr->bufferused, "\r\n", 3);
  logptr->bufferused += 2;
}

static void OutputTime(LOGPTR logptr, int show) {
  SYSTEMTIME     systime;
  register DWORD tick;
  register char *output;

  if (!logptr->timeStamp) {
    return;
  }

  tick = GetTickCount();
  if (tick != lasttime) {
    lasttime = tick;
    GetLocalTime(&systime);
    wsprintfA(
        timestr, "%u/%u %02u:%02u:%02u.%03u  ", systime.wMonth, systime.wDay, systime.wHour, systime.wMinute, systime.wSecond, systime.wMilliseconds
    );
    timestrlen = SStrLen(timestr);
  }

  output = logptr->buffer + logptr->bufferused;
  if (show) {
    memcpy(output, timestr, timestrlen + 1);
  } else {
    memset(output, ' ', timestrlen);
    output[timestrlen] = 0;
  }

  logptr->bufferused += timestrlen;
}

static void UnlockDeleteLog(LOGPTR logptr, HLOCKEDLOG lockedhandle) {
  DWORD   bucket;
  LOGPTR *link;

  bucket = (DWORD)lockedhandle;
  link = &s_loghead[bucket];
  while (*link) {
    if (*link == logptr) {
      *link = logptr->next;
      VirtualFree(logptr, 0, MEM_RELEASE);
      break;
    }
    link = &(*link)->next;
  }

  LeaveCriticalSection(&s_critsect[bucket]);
}

static void UnlockLog(HLOCKEDLOG lockedhandle) {
  DWORD bucket = (DWORD)lockedhandle;
  LeaveCriticalSection(&s_critsect[bucket]);
}

static LPCSTR PrependDefaultDir(char *newfilename, DWORD newfilenamesize, LPCSTR filename) {
  char *slash;

  if (!filename || !filename[0] || filename[1] == ':' || SStrChr(filename, '\\')) {
    return filename;
  }

  EnterCriticalSection(&s_defaultdir_critsect);
  if (s_defaultdir[0]) {
    newfilenamesize -= SStrCopy(newfilename, s_defaultdir, newfilenamesize);
    SStrCopy(newfilename + SStrLen(newfilename), filename, newfilenamesize);
  } else {
    GetModuleFileNameA(GetModuleHandleA(NULL), newfilename, newfilenamesize);
    slash = SStrChrR(newfilename, '\\');
    if (slash) {
      *slash = 0;
    }
    SStrPack(newfilename, "\\", newfilenamesize);
    SStrPack(newfilename, filename, newfilenamesize);
  }
  LeaveCriticalSection(&s_defaultdir_critsect);

  return newfilename;
}

static BOOL OpenLogFile(LPCSTR filename, LPVOID *file, DWORD flags) {
  char   newfilename[MAX_PATH];
  LPCSTR openPath;
  DWORD  disposition;

  if (!filename || !filename[0]) {
    *file = INVALID_HANDLE_VALUE;
    return FALSE;
  }

  openPath = PrependDefaultDir(newfilename, sizeof(newfilename), filename);
  CreateFileDirectory(openPath);

  disposition = (flags & SLOG_FLAG_APPEND) ? OPEN_ALWAYS : CREATE_ALWAYS;
  *file = CreateFileA(openPath, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, disposition, FILE_ATTRIBUTE_NORMAL, NULL);
  if (*file == INVALID_HANDLE_VALUE) {
    return FALSE;
  }

  if (flags & SLOG_FLAG_APPEND) {
    SetFilePointer((HANDLE)*file, 0, NULL, FILE_END);
  }

  return TRUE;
}

static BOOL PrepareLog(LOGPTR logptr) {
  if (logptr->file == INVALID_HANDLE_VALUE && !OpenLogFile(logptr->filename, (LPVOID *)&logptr->file, logptr->flags)) {
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

  if (rec->file != INVALID_HANDLE_VALUE) {
    FlushLog(rec);
    CloseHandle(rec->file);
  }
  UnlockDeleteLog(rec, lockedhandle);
}

extern "C" BOOL APIENTRY SLogCreate(LPCSTR filename, DWORD flags, HSLOG *log) {
  HLOCKEDLOG lockedhandle;
  HANDLE     file;
  LOGPTR     rec;

  FATALASSERT(filename);
  FATALASSERT(*filename);
  FATALASSERT(log);
  *log = NULL;

  if (flags & SLOG_FLAG_MEMORYONLY) {
    flags &= ~SLOG_FLAG_OPENNOW;
    filename = "";
  }

  file = INVALID_HANDLE_VALUE;
  if ((flags & SLOG_FLAG_OPENNOW) && !OpenLogFile(filename, (LPVOID *)&file, flags)) {
    return FALSE;
  }

  s_sequence = (HSLOG)((DWORD)s_sequence + 1);
  *log = s_sequence;

  rec = LockLog(*log, &lockedhandle, TRUE);
  if (!rec) {
    *log = NULL;
    return FALSE;
  }

  SStrCopy(rec->filename, filename, sizeof(rec->filename));
  rec->file = file;
  rec->flags = flags;
  rec->indent = 0;
  rec->timeStamp = TRUE;
  UnlockLog(lockedhandle);

  return TRUE;
}

extern "C" void APIENTRY SLogDestroy() {
  char  fileName[MAX_PATH];
  int   i;
  HSLOG log;

  SLogFlushAll();

  for (i = 0; i < SLOTS; i++) {
    EnterCriticalSection(&s_critsect[i]);
    while (s_loghead[i]) {
      log = s_loghead[i]->log;
      SStrCopy(fileName, s_loghead[i]->filename, 0x7FFFFFFF);
      SLogClose(log);
      SErrReportNamedResourceLeak("HSLOG", fileName);
    }
    LeaveCriticalSection(&s_critsect[i]);
  }

  s_logsysteminit = FALSE;
  for (i = 0; i < SLOTS; i++) {
    DeleteCriticalSection(&s_critsect[i]);
  }

  DeleteCriticalSection(&s_defaultdir_critsect);
}

extern "C" void APIENTRY SLogDump(HSLOG log, LPCVOID data, DWORD bytes) {
  HLOCKEDLOG lockedhandle;
  DWORD      offset;
  DWORD      i;
  DWORD      end;
  LOGPTR     rec;

  rec = LockLog(log, &lockedhandle, FALSE);
  if (!rec) {
    return;
  }

  if (!PrepareLog(rec)) {
    UnlockLog(lockedhandle);
    return;
  }

  offset = 0;
  while (offset < bytes) {
    OutputTime(rec, FALSE);
    OutputIndent(rec);

    wsprintfA(rec->buffer + rec->bufferused, "%04x: ", offset);
    rec->bufferused += SStrLen(rec->buffer + rec->bufferused);

    end = offset + 8;
    for (i = offset; i < end; i++) {
      if (i < bytes) {
        wsprintfA(rec->buffer + rec->bufferused, "%02x ", ((const BYTE *)data)[i]);
        rec->bufferused += SStrLen(rec->buffer + rec->bufferused);
      } else {
        rec->bufferused += SStrCopy(rec->buffer + rec->bufferused, "   ", 0x7FFFFFFF);
      }

      if ((i & 3) == 3) {
        rec->bufferused += SStrCopy(rec->buffer + rec->bufferused, "  ", 0x7FFFFFFF);
      }
    }

    for (i = offset; i < end; i++) {
      if (i < bytes && ((LPCSTR)data)[i] >= 0x20 && ((LPCSTR)data)[i] != 0x7F) {
        wsprintfA(rec->buffer + rec->bufferused, "%c", ((LPCSTR)data)[i]);
        rec->bufferused += SStrLen(rec->buffer + rec->bufferused);
      } else {
        rec->bufferused += SStrCopy(rec->buffer + rec->bufferused, i < bytes && ((LPCSTR)data)[i] ? "." : " ", 0x7FFFFFFF);
      }

      if ((i & 7) == 3) {
        rec->bufferused += SStrCopy(rec->buffer + rec->bufferused, " ", 0x7FFFFFFF);
      }
    }

    OutputReturn(rec);
    offset = end;
  }

  if (g_opt.echotooutputdebugstring) {
    OutputDebugStringA(rec->buffer + rec->pendpoint);
  }

  rec->pendpoint = rec->bufferused;
  if (rec->bufferused >= FLUSHMARK) {
    FlushLog(rec);
  }

  UnlockLog(lockedhandle);
}

extern "C" void APIENTRY SLogFlush(HSLOG log) {
  HLOCKEDLOG lockedhandle;
  LOGPTR     rec;

  rec = LockLog(log, &lockedhandle, FALSE);
  if (!rec) {
    return;
  }
  if (rec->file != INVALID_HANDLE_VALUE) {
    FlushLog(rec);
  }
  UnlockLog(lockedhandle);
}

extern "C" void APIENTRY SLogFlushAll() {
  int    i;
  LOGPTR rec;

  for (i = 0; i < SLOTS; i++) {
    EnterCriticalSection(&s_critsect[i]);
    rec = s_loghead[i];
    while (rec) {
      if (rec->file != INVALID_HANDLE_VALUE) {
        FlushLog(rec);
      }
      rec = rec->next;
    }
    LeaveCriticalSection(&s_critsect[i]);
  }
}

extern "C" void APIENTRY SLogGetDefaultDirectory(char *dirname, DWORD dirnamesize) {
  EnterCriticalSection(&s_defaultdir_critsect);
  SStrCopy(dirname, s_defaultdir, dirnamesize);
  LeaveCriticalSection(&s_defaultdir_critsect);
}

extern "C" void APIENTRY SLogInitialize() {
  int i;

  if (s_logsysteminit) {
    return;
  }

  for (i = 0; i < SLOTS; i++) {
    InitializeCriticalSection(&s_critsect[i]);
  }
  InitializeCriticalSection(&s_defaultdir_critsect);
  s_logsysteminit = TRUE;
}

extern "C" BOOL APIENTRY SLogIsInitialized() {
  return s_logsysteminit;
}

extern "C" void __cdecl SLogPend(HSLOG log, LPCSTR format, ...) {
  HLOCKEDLOG lockedhandle;
  LOGPTR     rec;
  va_list    arglist;

  va_start(arglist, format);
  rec = LockLog(log, &lockedhandle, FALSE);
  if (rec) {
    if (PrepareLog(rec)) {
      rec->bufferused = rec->pendpoint;
      OutputTime(rec, TRUE);
      OutputIndent(rec);
      vsprintf(rec->buffer + rec->bufferused, format, arglist);
      rec->bufferused += SStrLen(rec->buffer + rec->bufferused);
      OutputReturn(rec);
    }
    UnlockLog(lockedhandle);
  }
  va_end(arglist);
}

extern "C" long APIENTRY SLogSetAbsIndent(HSLOG log, long indent) {
  HLOCKEDLOG lockedhandle;
  LOGPTR     rec;
  long       previous;

  rec = LockLog(log, &lockedhandle, FALSE);
  if (!rec) {
    return 0;
  }
  previous = rec->indent;
  rec->indent = indent;
  UnlockLog(lockedhandle);
  return previous;
}

extern "C" void APIENTRY SLogSetDefaultDirectory(LPCSTR dirname) {
  DWORD len;

  EnterCriticalSection(&s_defaultdir_critsect);
  len = SStrCopy(s_defaultdir, dirname, MAX_PATH);
  if (len < MAX_PATH - 1 && s_defaultdir[len - 1] != '\\') {
    s_defaultdir[len] = '\\';
    s_defaultdir[len + 1] = 0;
  }
  LeaveCriticalSection(&s_defaultdir_critsect);
}

extern "C" long APIENTRY SLogSetIndent(HSLOG log, long deltaIndent) {
  HLOCKEDLOG lockedhandle;
  LOGPTR     rec;
  long       previous;

  rec = LockLog(log, &lockedhandle, FALSE);
  if (!rec) {
    return 0;
  }
  previous = rec->indent;
  rec->indent += deltaIndent;
  UnlockLog(lockedhandle);
  return previous;
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

extern "C" void APIENTRY SLogVWrite(HSLOG log, LPCSTR format, char *arglist) {
  HLOCKEDLOG lockedhandle;
  LOGPTR     rec;

  rec = LockLog(log, &lockedhandle, FALSE);
  if (!rec) {
    return;
  }
  if (PrepareLog(rec)) {
    OutputTime(rec, TRUE);
    OutputIndent(rec);
    vsprintf(rec->buffer + rec->bufferused, format, (va_list)arglist);
    rec->bufferused += SStrLen(rec->buffer + rec->bufferused);
    OutputReturn(rec);

    if (g_opt.echotooutputdebugstring) {
      OutputDebugStringA(rec->buffer + rec->pendpoint);
    }

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
