#pragma once

#include <Base/Status.h>

class CMDLStatus : public CStatus {
 public:
  void FatalBadFileName(LPCSTR path);
  void FatalDuplicate(LPCSTR found, int lineno);
  void FatalUnmatched(LPCSTR item1, UINT count1, LPCSTR item2, UINT count2, int lineno);
  void FatalNotFound(UINT what, int lineno);
  void FatalNotFound(LPCSTR expected, int lineno);
  void FatalUnexpected(LPCSTR found, int lineno);
  void FatalExpected(UINT what, LPCSTR found, int lineno);
  void FatalExpected(LPCSTR expected, LPCSTR found, int lineno);
  void FatalEOF(int lineno);
  void WarningCount(LPCSTR item, long expected, long actual, int lineno);
  void FatalOverran(LPCSTR section, int lineno);
  void FatalFlunked(LPCSTR section, int lineno);
};
