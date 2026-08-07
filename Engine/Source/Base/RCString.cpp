#include <Base/Base.h>

#include "RCString.h"

#include <storm.h>

void TRefCnt::DeleteSelf() {
  DEL(this);
}

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

  s_stringManager = 0;
}

CStringManager::~CStringManager() {
}

CStringRep &CStringManager::Add(LPCSTR str) {
  if (!str) {
    return CStringRep::s_nullRep;
  }

  CStringRep *rep = Ptr(str);
  if (!rep) {
    rep = New(str, 0, 0);
  }

  return *rep;
}

CStringRep &CStringManager::Find(LPCSTR str) {
  CStringRep *rep = str ? Ptr(str) : 0;

  return rep ? *rep : CStringRep::s_nullRep;
}

void CStringRep::DecrRef() {
  if (this != &s_nullRep && !--m_refcnt) {
    CStringManager *pStringManager = CStringManager::Get();

    FATALASSERT(pStringManager);

    pStringManager->Delete(this);
  }
}

BOOL CStringRep::IsString(LPCSTR str) const {
  LPCSTR thisString = GetString();

  if (!thisString) {
    return str == 0;
  }

  if (!str) {
    return 0;
  }

  return SStrCmp(str, thisString, 0x7FFFFFFF) == 0;
}

int CStringRep::operator==(const CStringRep &r) const {
  if (this == &r) {
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

  FATALASSERT(SStrLen(source) < (unsigned)MAX_RCSTRING_LENGTH);

  CStringManager *pManager = CStringManager::Get();

  FATALASSERT(pManager);

  m_rep = &pManager->Add(source);
}

void RCString::Free() {
  if (*m_rep == CStringRep::s_nullRep) {
    m_rep = static_cast<CStringRep *>(0);
  }
}

int RCString::operator==(const RCString &r) const {
  return m_rep == r.m_rep;
}

int RCString::operator==(LPCSTR str) const {
  return m_rep.m_ptr ? m_rep.m_ptr->IsString(str) : str == 0;
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
  char *str = const_cast<char *>(GetString());

  if (str) {
    UINT len = SStrLen(str);

    FATALASSERT(len < (WORD)-1);

    if (len && start <= static_cast<RCStringIndex>(len)) {
      if (end >= static_cast<RCStringIndex>(len)) {
        end = static_cast<RCStringIndex>(len);
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

  if (str) {
    SStrCopy(buf, str, bufSize);
  } else {
    *buf = 0;
  }
}
