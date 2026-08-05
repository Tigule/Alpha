#include <storm.h>

#include <Os/OsTime.h>
#include <Os/W32/OsFile.h>

#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define SLOG_FLAG_OPENONCREATE 0x00000001
#define SLOG_FLAG_NOFILE       0x00000002

#define SLOG_BUCKETS      4
#define SLOG_BUFFER_SIZE  0x10000
#define SLOG_FLUSH_SIZE   0xC000
#define SLOG_MAX_INDENT   128
#define SLOG_TIMESTAMP_SIZE 0x40

struct LOGRECORD {
  DWORD      id;
  LOGRECORD *next;
  char       filename[0x100];
  HOSFILE    file;
  DWORD      used;
  DWORD      lineStart;
  LONG       indent;
  int        timestamp;
  char       buffer[SLOG_BUFFER_SIZE];
};

static int             s_initialized;
static DWORD           s_nextId;
static LOGRECORD      *s_buckets[SLOG_BUCKETS];
static pthread_mutex_t s_bucketLock[SLOG_BUCKETS];

static SCritSect s_directoryLock;
static char      s_defaultDirectory[0x100];

static DWORD s_timestampMs = 0xFFFFFFFF;
static DWORD s_timestampLen;
static char  s_timestamp[SLOG_TIMESTAMP_SIZE];

static LOGRECORD *LockLog(HSLOG log, int *bucket, int create) {
  DWORD       id = static_cast<DWORD>(reinterpret_cast<uintptr_t>(log));
  LOGRECORD **cursor;
  LOGRECORD  *record;

  if (!id) {
    *bucket = -1;
    return 0;
  }

  *bucket = id & (SLOG_BUCKETS - 1);
  pthread_mutex_lock(&s_bucketLock[*bucket]);

  for (cursor = &s_buckets[*bucket]; *cursor; cursor = &(*cursor)->next) {
    if ((*cursor)->id == id) {
      return *cursor;
    }
  }

  if (!create) {
    pthread_mutex_unlock(&s_bucketLock[*bucket]);
    *bucket = -1;
    return 0;
  }

  record = static_cast<LOGRECORD *>(SMemAlloc(sizeof(LOGRECORD), __FILE__, __LINE__, 0));
  *cursor = record;

  if (!record) {
    pthread_mutex_unlock(&s_bucketLock[*bucket]);
    *bucket = -1;
    return 0;
  }

  record->id = id;
  record->next = 0;
  record->filename[0] = 0;
  record->used = 0;
  record->lineStart = 0;
  record->indent = 0;

  return record;
}

static void UnlockLog(int bucket) {
  if (bucket >= 0) {
    pthread_mutex_unlock(&s_bucketLock[bucket]);
  }
}

static int OpenLogFile(LOGRECORD *record) {
  const char *filename = record->filename;
  char        combined[0x100];

  if (record->file) {
    return 1;
  }

  if (!filename[0]) {
    record->file = 0;
    return 0;
  }

  if (s_defaultDirectory[0] && filename[1] != ':' && !SStrChr(filename, '\\')) {
    s_directoryLock.Enter();

    if (s_defaultDirectory[0]) {
      DWORD chars = SStrCopy(combined, s_defaultDirectory, sizeof(combined));
      s_directoryLock.Leave();

      SStrCopy(combined + chars, filename, sizeof(combined) - chars);
      filename = combined;
    } else {
      s_directoryLock.Leave();
    }
  }

  record->file = OsCreateFile(filename, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0x3F3F3F3F);

  return record->file != 0;
}

static void FlushLog(LOGRECORD *record) {
  DWORD written;

  if (record->used) {
    ASSERT(record->file != 0);

    OsWriteFile(record->file, record->buffer, record->used, &written);
    record->used = 0;
    record->lineStart = 0;
  }
}

static void WriteLinePrefix(LOGRECORD *record, int stamp) {
  LONG indent;

  if (!record->timestamp) {
    return;
  }

  DWORD ms = OsGetAsyncTimeMsPrecise();

  if (ms != s_timestampMs) {
    time_t     now;
    struct tm *broken;

    s_timestampMs = ms;

    time(&now);
    broken = localtime(&now);

    SStrPrintf(
        s_timestamp, sizeof(s_timestamp), "%u/%u %02u:%02u:%02u.%03u  ", broken->tm_mon, broken->tm_mday, broken->tm_hour,
        broken->tm_min, broken->tm_sec, ms % 1000
    );

    s_timestampLen = SStrLen(s_timestamp);
  }

  if (stamp) {
    memcpy(record->buffer + record->used, s_timestamp, s_timestampLen + 1);
  } else {
    memset(record->buffer + record->used, ' ', s_timestampLen);
    record->buffer[record->used + s_timestampLen] = 0;
  }

  record->used += s_timestampLen;

  indent = record->indent;
  if (indent > 0) {
    if (indent > SLOG_MAX_INDENT) {
      indent = SLOG_MAX_INDENT;
    }

    memset(record->buffer + record->used, ' ', indent);
    record->buffer[record->used + indent] = 0;
    record->used += indent;
  }
}

extern "C" void APIENTRY SLogInitialize() {
  int bucket;

  if (s_initialized) {
    return;
  }

  for (bucket = 0; bucket < SLOG_BUCKETS; ++bucket) {
    pthread_mutex_init(&s_bucketLock[bucket], 0);
    s_buckets[bucket] = 0;
  }

  s_initialized = 1;
}

extern "C" BOOL APIENTRY SLogIsInitialized() {
  return s_initialized;
}

extern "C" BOOL APIENTRY SLogCreate(const char *filename, DWORD flags, HSLOG *log) {
  LOGRECORD *record;
  int        bucket;

  FATALASSERT(filename);
  FATALASSERT(*filename);
  FATALASSERT(log);

  *log = 0;

  if (flags & SLOG_FLAG_NOFILE) {
    flags &= ~SLOG_FLAG_OPENONCREATE;
    filename = "";
  }

  *log = reinterpret_cast<HSLOG>(static_cast<uintptr_t>(++s_nextId));

  record = LockLog(*log, &bucket, 1);
  if (!record) {
    *log = 0;
    return FALSE;
  }

  record->file = 0;
  SStrCopy(record->filename, filename, sizeof(record->filename));
  record->timestamp = 1;
  record->indent = 0;

  if (flags & SLOG_FLAG_OPENONCREATE) {
    if (!OpenLogFile(record)) {
      UnlockLog(bucket);
      *log = 0;
      return FALSE;
    }
  }

  UnlockLog(bucket);
  return TRUE;
}

extern "C" void APIENTRY SLogVWrite(HSLOG log, const char *format, char *arglist) {
  LOGRECORD *record;
  int        bucket;

  record = LockLog(log, &bucket, 0);
  if (!record) {
    return;
  }

  if (!OpenLogFile(record)) {
    record->filename[0] = 0;
    UnlockLog(bucket);
    return;
  }

  WriteLinePrefix(record, 1);

  vsprintf(record->buffer + record->used, format, arglist);
  record->used += SStrLen(record->buffer + record->used);

  memcpy(record->buffer + record->used, "\r\n", 3);
  record->used += 2;
  record->lineStart = record->used;

  if (record->used >= SLOG_FLUSH_SIZE) {
    FlushLog(record);
  }

  UnlockLog(bucket);
}

extern "C" void __cdecl SLogWrite(HSLOG log, const char *format, ...) {
  va_list args;

  va_start(args, format);
  SLogVWrite(log, format, args);
  va_end(args);
}

extern "C" void APIENTRY SLogSetTimestamp(HSLOG log, BOOL timeStamp) {
  LOGRECORD *record;
  int        bucket;

  record = LockLog(log, &bucket, 0);
  if (!record) {
    return;
  }

  record->timestamp = timeStamp;
  UnlockLog(bucket);
}

extern "C" LONG APIENTRY SLogSetAbsIndent(HSLOG log, LONG indent) {
  LOGRECORD *record;
  int        bucket;
  LONG       previous;

  record = LockLog(log, &bucket, 0);
  if (!record) {
    return 0;
  }

  previous = record->indent;
  record->indent = indent;
  UnlockLog(bucket);

  return previous;
}

extern "C" LONG APIENTRY SLogSetIndent(HSLOG log, LONG deltaIndent) {
  LOGRECORD *record;
  int        bucket;
  LONG       previous;

  record = LockLog(log, &bucket, 0);
  if (!record) {
    return 0;
  }

  previous = record->indent;
  record->indent += deltaIndent;
  UnlockLog(bucket);

  return previous;
}

extern "C" void APIENTRY SLogFlush(HSLOG log) {
  LOGRECORD *record;
  int        bucket;

  record = LockLog(log, &bucket, 0);
  if (!record) {
    return;
  }

  if (record->file) {
    FlushLog(record);
  }

  UnlockLog(bucket);
}

extern "C" void APIENTRY SLogFlushAll() {
  int bucket;

  for (bucket = 0; bucket < SLOG_BUCKETS; ++bucket) {
    LOGRECORD *record;

    pthread_mutex_lock(&s_bucketLock[bucket]);

    for (record = s_buckets[bucket]; record; record = record->next) {
      if (record->file) {
        FlushLog(record);
      }
    }

    pthread_mutex_unlock(&s_bucketLock[bucket]);
  }
}

extern "C" void APIENTRY SLogClose(HSLOG log) {
  LOGRECORD  *record;
  LOGRECORD **cursor;
  int         bucket;

  if (!s_initialized) {
    return;
  }

  record = LockLog(log, &bucket, 0);
  if (!record) {
    return;
  }

  if (record->file) {
    FlushLog(record);
    OsCloseFile(record->file);
  }

  for (cursor = &s_buckets[bucket]; *cursor; cursor = &(*cursor)->next) {
    if (*cursor == record) {
      *cursor = record->next;
      SMemFree(record, __FILE__, __LINE__, 0);
      break;
    }
  }

  UnlockLog(bucket);
}

extern "C" void APIENTRY SLogDestroy() {
  int bucket;

  if (!s_initialized) {
    return;
  }

  for (bucket = 0; bucket < SLOG_BUCKETS; ++bucket) {
    pthread_mutex_lock(&s_bucketLock[bucket]);

    while (s_buckets[bucket]) {
      LOGRECORD *record = s_buckets[bucket];

      if (record->file) {
        FlushLog(record);
        OsCloseFile(record->file);
      }

      s_buckets[bucket] = record->next;
      SMemFree(record, __FILE__, __LINE__, 0);
    }

    pthread_mutex_unlock(&s_bucketLock[bucket]);
  }

  s_initialized = 0;
}

extern "C" void APIENTRY SLogSetDefaultDirectory(const char *dirname) {
  s_directoryLock.Enter();
  SStrCopy(s_defaultDirectory, dirname, sizeof(s_defaultDirectory));
  s_directoryLock.Leave();
}

extern "C" void APIENTRY SLogGetDefaultDirectory(char *dirname, DWORD dirnamesize) {
  s_directoryLock.Enter();
  SStrCopy(dirname, s_defaultDirectory, dirnamesize);
  s_directoryLock.Leave();
}
