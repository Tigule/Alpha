#include "Status.h"

#include <stdarg.h>
#include <stdio.h>

static CStatus s_errorList;

static char *FormatStatusMessage(const char *format, va_list argptr);

CStatus &GetGlobalStatusObj() {
  return s_errorList;
}

CStatus::~CStatus() {
  Clear();
}

static char *FormatStatusMessage(const char *format, va_list argptr) {
  static char buffer[0x100];
  int         length = _vsnprintf(buffer, sizeof(buffer), format, argptr);

  if (length == sizeof(buffer)) {
    length = sizeof(buffer) - 1;
    buffer[sizeof(buffer) - 1] = 0;
  }

  return length ? buffer : 0;
}

void CStatus::Prepend(STATUS_TYPE severity, const char *format, ...) {
  STATUSENTRY *entry = 0;
  STATUSENTRY *pnextstatus = 0;
  const char  *text;
  va_list      args;

  FATALASSERT(format);

  va_start(args, format);
  text = FormatStatusMessage(format, args);
  va_end(args);
  if (!text) {
    return;
  }

  entry = statusList.NewNode(LIST_UNLINKED, 0, 0);

  entry->text = SStrDupA(text, __FILE__, __LINE__);
  entry->severity = severity;

  {
    ITERATELIST(STATUSENTRY, statusList, cursor) {
      if (severity >= cursor->severity) {
        pnextstatus = cursor;
        break;
      }
    }
  }

  statusList.LinkNode(entry, LIST_LINK_BEFORE, pnextstatus);
}

void CStatus::Add(STATUS_TYPE severity, const char *format, ...) {
  STATUSENTRY *entry = 0;
  STATUSENTRY *pnextstatus = 0;
  const char  *text;
  va_list      args;

  FATALASSERT(format);

  va_start(args, format);
  text = FormatStatusMessage(format, args);
  va_end(args);
  if (!text) {
    return;
  }

  entry = statusList.NewNode(LIST_UNLINKED, 0, 0);

  entry->text = SStrDupA(text, __FILE__, __LINE__);
  entry->severity = severity;

  {
    ITERATELIST(STATUSENTRY, statusList, cursor) {
      if (severity > cursor->severity) {
        pnextstatus = cursor;
        break;
      }
    }
  }

  statusList.LinkNode(entry, LIST_LINK_BEFORE, pnextstatus);
}

void CStatus::Add(const CStatus &source) {
  const STATUSENTRY *entry = source.statusList.Head();

  while (entry) {
    Add(entry->severity, entry->text);
    entry = source.statusList.Next(entry);
  }
}

int CStatus::IsEmpty() const {
  return statusList.IsEmpty();
}

void CStatus::Clear() {
  statusList.Clear();
}

void CStatus::GetErrorStr(char *buffer, unsigned long bufchars, STATUS_TYPE minSeverity) const {
  const STATUSENTRY *entry;

  FATALASSERT(buffer);

  *buffer = 0;
  entry = statusList.Head();
  while (entry) {
    if (entry->severity >= minSeverity) {
      unsigned long length = SStrLen(entry->text);
      if (length >= bufchars) {
        return;
      }
      SStrCopy(buffer, entry->text, bufchars);
      bufchars -= length;
      buffer += length;
    }
    entry = statusList.Next(entry);
  }
}

unsigned int CStatus::GetErrorStrLen(STATUS_TYPE minSeverity) const {
  unsigned int       length = 0;
  const STATUSENTRY *entry = statusList.Head();

  while (entry) {
    if (entry->severity >= minSeverity) {
      length += SStrLen(entry->text);
    }
    entry = statusList.Next(entry);
  }
  return length;
}

char *CStatus::GetErrorStrAlloc(STATUS_TYPE minSeverity) const {
  unsigned int bufchars = GetErrorStrLen(minSeverity) + 1;
  char        *buffer = static_cast<char *>(ALLOC(bufchars));
  GetErrorStr(buffer, bufchars, minSeverity);
  return buffer;
}

STATUS_TYPE CStatus::GetHighestSeverity() const {
  const STATUSENTRY *entry = statusList.Head();
  if (!entry) {
    return STATUS_INFO;
  }
  return entry->severity;
}
