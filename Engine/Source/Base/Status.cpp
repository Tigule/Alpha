#include "Status.h"

#include <stdarg.h>
#include <stdio.h>

static CStatus s_errorList;

static char *FormatStatusMessage(LPCSTR format, va_list argptr);

CStatus &GetGlobalStatusObj() {
  return s_errorList;
}

CStatus::~CStatus() {
  Clear();
}

static char *FormatStatusMessage(LPCSTR format, va_list argptr) {
  static char buffer[0x100];
  int         length = _vsnprintf(buffer, sizeof(buffer), format, argptr);

  if (length == sizeof(buffer)) {
    length = sizeof(buffer) - 1;
    buffer[sizeof(buffer) - 1] = 0;
  }

  return length ? buffer : 0;
}

void CStatus::Prepend(STATUS_TYPE severity, LPCSTR format, ...) {
  STATUSENTRY *entry = 0;
  LPCSTR       text;
  va_list      args;

  VALIDATEBEGIN;
  VALIDATE(format);
  VALIDATEENDVOID;

  va_start(args, format);
  text = FormatStatusMessage(format, args);
  va_end(args);
  if (!text) {
    return;
  }

  entry = statusList.NewNode(LIST_UNLINKED, 0, 0);

  entry->text = SStrDupA(text, __FILE__, __LINE__);
  entry->severity = severity;

  STATUSENTRY *pnextstatus = 0;
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

void CStatus::Add(STATUS_TYPE severity, LPCSTR format, ...) {
  STATUSENTRY *entry = 0;
  LPCSTR       text;
  va_list      args;

  VALIDATEBEGIN;
  VALIDATE(format);
  VALIDATEENDVOID;

  va_start(args, format);
  text = FormatStatusMessage(format, args);
  va_end(args);
  if (!text) {
    return;
  }

  entry = statusList.NewNode(LIST_UNLINKED, 0, 0);

  entry->text = SStrDupA(text, __FILE__, __LINE__);
  entry->severity = severity;

  STATUSENTRY *pnextstatus = 0;
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
  for (const STATUSENTRY *entry = source.statusList.Head(); (int)entry > 0; entry = source.statusList.RawNext(entry)) {
    Add(entry->severity, entry->text);
  }
}

BOOL CStatus::IsEmpty() const {
  return statusList.IsEmpty();
}

void CStatus::Clear() {
  statusList.Clear();
}

void CStatus::GetErrorStr(char *buffer, DWORD bufchars, STATUS_TYPE minSeverity) const {
  VALIDATEBEGIN;
  VALIDATE(buffer);
  VALIDATEENDVOID;

  *buffer = 0;
  for (const STATUSENTRY *entry = statusList.Head(); (int)entry > 0; entry = statusList.RawNext(entry)) {
    if (entry->severity >= minSeverity) {
      DWORD length = SStrLen(entry->text);
      if (length >= bufchars) {
        return;
      }
      SStrCopy(buffer, entry->text, bufchars);
      bufchars -= length;
      buffer += length;
    }
  }
}

UINT CStatus::GetErrorStrLen(STATUS_TYPE minSeverity) const {
  UINT length = 0;

  for (const STATUSENTRY *entry = statusList.Head(); (int)entry > 0; entry = statusList.RawNext(entry)) {
    if (entry->severity >= minSeverity) {
      length += SStrLen(entry->text);
    }
  }
  return length;
}

char *CStatus::GetErrorStrAlloc(STATUS_TYPE minSeverity) const {
  UINT  bufchars = GetErrorStrLen(minSeverity) + 1;
  char *buffer = static_cast<char *>(ALLOC(bufchars));
  GetErrorStr(buffer, bufchars, minSeverity);
  return buffer;
}

STATUS_TYPE CStatus::GetHighestSeverity() const {
  const STATUSENTRY *entry = statusList.Head();

  return entry ? entry->severity : STATUS_INFO;
}
