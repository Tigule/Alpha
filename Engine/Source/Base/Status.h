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

  CStatus() {
  }
  CStatus(const CStatus &source);
  virtual ~CStatus();

  CStatus &operator=(const CStatus &source);

  virtual void Display() const;
  virtual void Add(STATUS_TYPE severity, const char *format, ...);
  virtual void Add(const CStatus &source);
  virtual void Prepend(STATUS_TYPE severity, const char *format, ...);

  int          IsEmpty() const;
  void         Clear();
  void         GetErrorStr(char *buffer, unsigned long bufchars, STATUS_TYPE minSeverity) const;
  unsigned int GetErrorStrLen(STATUS_TYPE minSeverity) const;
  char        *GetErrorStrAlloc(STATUS_TYPE minSeverity) const;
  STATUS_TYPE  GetHighestSeverity() const;

 protected:
  TSExplicitList<STATUSENTRY, 8> statusList;
};

class CNullStatus : public CStatus {};

struct CStatus::STATUSENTRY {
  ~STATUSENTRY() {
    FREEIFUSED(text);
  }

  char               *text;
  STATUS_TYPE         severity;
  TSLink<STATUSENTRY> link;
};

CStatus &__fastcall GetGlobalStatusObj();
