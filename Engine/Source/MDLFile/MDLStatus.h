#pragma once

#include <Base/Status.h>

class CMDLStatus : public CStatus {
 public:
  void FatalBadFileName(const char *path);
  void FatalDuplicate(const char *found, int lineno);
  void FatalUnmatched(const char *item1, unsigned int count1, const char *item2, unsigned int count2, int lineno);
  void FatalNotFound(unsigned int what, int lineno);
  void FatalNotFound(const char *expected, int lineno);
  void FatalUnexpected(const char *found, int lineno);
  void FatalExpected(unsigned int what, const char *found, int lineno);
  void FatalExpected(const char *expected, const char *found, int lineno);
  void FatalEOF(int lineno);
  void WarningCount(const char *item, long expected, long actual, int lineno);
  void FatalOverran(const char *section, int lineno);
  void FatalFlunked(const char *section, int lineno);
};
