#include "TSet.h"
#include "MDLStatus.h"

#include <storm.h>

void TSet::Add(unsigned int token, int needed, int allowDuplicates) {
  FATALASSERT(m_count != 64);
  m_set[m_count].token = token;
  m_set[m_count].needed = needed;
  m_set[m_count].dupsOk = allowDuplicates;
  m_set[m_count++].seen = 0;
}

int TSet::Check(unsigned int token) {
  for (int i = 0; i < m_count; ++i) {
    if (m_set[i].token == token) {
      if (m_set[i].seen && !m_set[i].dupsOk) {
        return 0;
      }
      m_set[i].seen = 1;
      break;
    }
  }
  return 1;
}

int TSet::Found(unsigned int token) {
  for (int i = 0; i < m_count; ++i) {
    if (m_set[i].token == token) {
      return m_set[i].seen;
    }
  }
  SErrPrepareAppFatal(__FILE__, __LINE__);
  SErrDisplayAppFatal("TSet::Found: found unregistered token 0x%x", token);
  return 0;
}

int TSet::NotFound(unsigned int token) {
  for (int i = 0; i < m_count; ++i) {
    if (m_set[i].token == token) {
      return !m_set[i].seen;
    }
  }
  SErrPrepareAppFatal(__FILE__, __LINE__);
  SErrDisplayAppFatal("TSet::NotFound: found unregistered token 0x%x", token);
  return 0;
}

void TSet::Complete(CMDLStatus *status) {
  FATALASSERT(status);
  for (int i = 0; i < m_count; ++i) {
    if (m_set[i].needed && !m_set[i].seen) {
      status->FatalNotFound(m_set[i].token, -1);
    }
  }
}
