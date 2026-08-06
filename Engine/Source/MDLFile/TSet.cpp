#include "TSet.h"
#include "MDLStatus.h"

#include <storm.h>

void TSet::Add(UINT token, int needed, int allowDuplicates) {
  FATALASSERT(count != 64);
  set[count].token = token;
  set[count].needed = needed;
  set[count].dupsOk = allowDuplicates;
  set[count++].seen = 0;
}

int TSet::Check(UINT token) {
  for (int i = 0; i < count; ++i) {
    if (set[i].token == token) {
      if (set[i].seen && !set[i].dupsOk) {
        return 0;
      }
      set[i].seen = 1;
      break;
    }
  }
  return 1;
}

int TSet::Found(UINT token) {
  for (int i = 0; i < count; ++i) {
    if (set[i].token == token) {
      return set[i].seen;
    }
  }
  SErrPrepareAppFatal(__FILE__, __LINE__);
  SErrDisplayAppFatal("TSet::Found: found unregistered token 0x%x", token);
  return 0;
}

int TSet::NotFound(UINT token) {
  for (int i = 0; i < count; ++i) {
    if (set[i].token == token) {
      return !set[i].seen;
    }
  }
  SErrPrepareAppFatal(__FILE__, __LINE__);
  SErrDisplayAppFatal("TSet::NotFound: found unregistered token 0x%x", token);
  return 0;
}

void TSet::Complete(CMDLStatus *status) {
  FATALASSERT(status);
  for (int i = 0; i < count; ++i) {
    if (set[i].needed && !set[i].seen) {
      status->FatalNotFound(set[i].token, -1);
    }
  }
}
