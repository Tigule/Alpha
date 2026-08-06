#ifndef ENGINE_SOURCE_MDLFILE_TSET_H
#define ENGINE_SOURCE_MDLFILE_TSET_H

#include <Base/Base.h>

class CMDLStatus;

class TSet {
 public:
  TSet() : count(0) {
  }

  void Add(UINT token, int needed, int allowDuplicates);
  int  Check(UINT token);
  int  Found(UINT token);
  int  NotFound(UINT token);
  void Complete(CMDLStatus *status);

 private:
  struct {
    UINT token;
    int  needed;
    int  dupsOk;
    int  seen;
  } set[64];

  int count;
};

#endif
