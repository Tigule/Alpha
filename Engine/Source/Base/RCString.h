#ifndef ENGINE_SOURCE_BASE_RCSTRING_H
#define ENGINE_SOURCE_BASE_RCSTRING_H

#include "RefCount.h"

#include <stpl.h>

typedef short RCStringIndex;

enum {
  MAX_RCSTRING_LENGTH = 0x400
};

class CStringManager;
class RCString;

class CStringRep : public TRefCnt, public TSHashObject<CStringRep, HASHKEY_STR> {
  friend class CStringManager;
  friend class RCString;

 private:
  static CStringRep s_nullRep;

 public:
  CStringRep() {
  }

  CStringRep(const CStringRep &);

  virtual void DecrRef();

  BOOL IsString(LPCSTR str) const;

  int operator==(LPCSTR str) const {
    return IsString(str);
  }

  int operator==(const CStringRep &r) const;
};

class CStringManager : public TSHashTable<CStringRep, HASHKEY_STR> {
  friend class CStringRep;
  friend class RCString;

 protected:
  static CStringManager *Get();
  static CStringManager *s_stringManager;

 public:
  CStringManager() {
  }

  CStringManager(const CStringManager &);

  virtual ~CStringManager();

  CStringRep &Add(LPCSTR str);
  CStringRep &Find(LPCSTR str);

  static void DestroyManager();
};

class RCString : public TRefCnt {
 private:
  static RCString s_nullString;

 protected:
  void     Copy(LPCSTR source);
  void     Copy(const RCString &source);
  void     Free();
  RCString Cat(LPCSTR lstr, LPCSTR rstr);

 public:
  RCString(LPCSTR str = 0) {
    Copy(str);
  }

  RCString(const RCString &str) {
    Copy(str);
  }

  virtual ~RCString();

  RCString &operator=(RCString *str);

  RCString &operator=(const RCString &str) {
    Copy(str);
    return *this;
  }

  RCString &operator=(LPCSTR str) {
    Copy(str);
    return *this;
  }

  int operator==(LPCSTR str) const;
  int operator==(const RCString &r) const;

  RCString &operator+=(LPCSTR rstr);
  RCString &operator+=(const RCString &r);

  RCString SubString(RCStringIndex start, RCStringIndex end) const;
  LPCSTR   GetString() const;

  operator LPCSTR() const;

  void Get(char *buf, RCStringIndex bufSize) const;

 private:
  TRefCntPtr<CStringRep> m_rep;
};

class RCStaticString : public RCString {
 public:
  RCStaticString &operator=(LPCSTR str) {
    Copy(str);
    return *this;
  }

  operator LPCSTR() const {
    LPCSTR itemstring = GetString();
    return itemstring ? itemstring : "";
  }
};

#endif
