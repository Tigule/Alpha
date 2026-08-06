#pragma once

#include "InstanceId.h"

enum STATUS_TYPE {
  STATUS_INFO = 0,
  STATUS_WARNING = 1,
  STATUS_ERROR = 2,
  STATUS_FATAL = 3,
  STATUS_NUMTYPES = 4
};

class CStatus {
 public:
  struct STATUSENTRY;

  virtual ~CStatus();

  virtual void Display() const;
  virtual void Add(STATUS_TYPE severity, LPCSTR format, ...);
  virtual void Add(const CStatus &source);
  virtual void Prepend(STATUS_TYPE severity, LPCSTR format, ...);

  int         IsEmpty() const;
  void        Clear();
  void        GetErrorStr(char *buffer, DWORD bufchars, STATUS_TYPE minSeverity) const;
  UINT        GetErrorStrLen(STATUS_TYPE minSeverity) const;
  char       *GetErrorStrAlloc(STATUS_TYPE minSeverity) const;
  STATUS_TYPE GetHighestSeverity() const;

 protected:
  TSExplicitList<STATUSENTRY, 8> statusList;
};

struct CNullStatus : public CStatus {
  void Add(int, LPCSTR, ...) {
  }
};

struct CStatus::STATUSENTRY {
  ~STATUSENTRY() {
    FREEIFUSED(text);
  }

  char       *text;
  STATUS_TYPE severity;
  LINKDECLEX(STATUSENTRY, link);
};

CStatus &GetGlobalStatusObj();
