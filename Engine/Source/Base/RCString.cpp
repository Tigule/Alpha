#include <Base/Base.h>
#include <time.h>

#include "RCString.h"

#include <storm.h>
#include <stdio.h>

CStringRep      CStringRep::s_nullRep;
RCString        RCString::s_nullString;
CStringManager *CStringManager::s_stringManager;

CStringManager *CStringManager::Get() {
  if (!s_stringManager) {
    s_stringManager = NEW(CStringManager);
  }

  return s_stringManager;
}

void CStringManager::DestroyManager() {
  if (s_stringManager) {
    DEL(s_stringManager);
  }

  s_stringManager = NULL;
}

CStringManager::~CStringManager() {
  Clear();
}

CStringRep &CStringManager::Add(LPCSTR str) {
  if (!str) {
    return CStringRep::s_nullRep;
  }

  CStringRep *pRep = Ptr(str);
  if (!pRep) {
    pRep = New(str, 0, 0);
  }

  return *pRep;
}

CStringRep &CStringManager::Find(LPCSTR str) {
  if (str) {
    CStringRep *rep = Ptr(str);
    if (rep) {
      return *rep;
    }
  }

  return CStringRep::s_nullRep;
}

void CStringRep::DecrRef() {
  if (this != &s_nullRep && !--m_refcnt) {
    CStringManager *pStringManager = CStringManager::Get();

    VALIDATE2VOID(pStringManager);

    pStringManager->Delete(this);
  }
}

BOOL CStringRep::IsString(LPCSTR str) const {
  LPCSTR thisString = GetString();

  if (!thisString) {
    return str == NULL;
  }

  if (!str) {
    return FALSE;
  }

  return SStrCmp(str, thisString, 0x7FFFFFFF) == 0;
}

int CStringRep::operator==(const CStringRep &r) const {
  if (&r == this) {
    return 1;
  }

  return IsString(r.GetString());
}

RCString::~RCString() {
  Free();
}

void RCString::Copy(const RCString &source) {
  m_rep = source.m_rep;
}

void RCString::Copy(LPCSTR source) {
  if (!source) {
    m_rep = &CStringRep::s_nullRep;
    return;
  }

  CStringManager *pManager;

  VALIDATE2VOID(SStrLen(source)<(unsigned)MAX_RCSTRING_LENGTH);
  pManager = CStringManager::Get();
  VALIDATE2VOID(pManager);

  m_rep = &pManager->Add(source);
}

void RCString::Free() {
  if (*m_rep == CStringRep::s_nullRep) {
    m_rep = 0;
  }
}

int RCString::operator==(const RCString &r) const {
  return m_rep == r.m_rep;
}

int RCString::operator==(LPCSTR str) const {
  if (!m_rep) {
    return str == NULL;
  }

  return *m_rep == str;
}

LPCSTR RCString::GetString() const {
  return m_rep ? m_rep->GetString() : 0;
}

RCString::operator LPCSTR() const {
  return GetString();
}

RCString RCString::Cat(LPCSTR lstr, LPCSTR rstr) {
  char buffer[MAX_RCSTRING_LENGTH * 2 + 1];

  SStrCopy(buffer, lstr, sizeof(buffer));
  SStrPack(buffer, rstr, 0x7FFFFFFF);

  RCString rcstr(buffer);
  return rcstr;
}

RCString &RCString::operator+=(LPCSTR rstr) {
  LPCSTR lstr = GetString();

  if (!lstr) {
    Copy(rstr);
  } else if (rstr) {
    Copy(Cat(lstr, rstr));
  }

  return *this;
}

RCString &RCString::operator+=(const RCString &r) {
  LPCSTR lstr = GetString();
  LPCSTR rstr = r.GetString();

  if (!lstr) {
    Copy(rstr);
  } else if (rstr) {
    Copy(Cat(lstr, rstr));
  }

  return *this;
}

RCString RCString::SubString(RCStringIndex start, RCStringIndex end) const {
  char *str = (char *)GetString();

  if (str) {
    UINT len = SStrLen(str);

    VALIDATE2(len<(unsigned short)-1);

    if (len && start <= (RCStringIndex)len) {
      if (end >= (RCStringIndex)len) {
        end = len;
      }

      char save = str[end];
      str[end] = 0;
      RCString rstr(&str[start]);
      str[end] = save;
      return rstr;
    }
  }

  return s_nullString;
}

void RCString::Get(char *buf, RCStringIndex bufSize) const {
  LPCSTR str = GetString();

  if (!str) {
    *buf = 0;
  } else {
    SStrCopy(buf, str, bufSize);
  }
}
