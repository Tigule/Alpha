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
  virtual void DecrRef();

  int IsString(const char *str) const;

  int operator==(const char *str) const {
    return IsString(str);
  }

  int operator==(const CStringRep &r) const;
};

class CStringManager : public TSHashTable<CStringRep, HASHKEY_STR> {
  friend class CStringRep;
  friend class RCString;

 protected:
  static CStringManager *__fastcall Get();
  static CStringManager            *s_stringManager;

 public:
  virtual ~CStringManager();

  CStringRep &Add(const char *str);
  CStringRep &Find(const char *str);

  static void __fastcall DestroyManager();
};

class RCString : public TRefCnt {
 private:
  static RCString s_nullString;

 protected:
  void     Copy(const char *source);
  void     Copy(const RCString &source);
  void     Free();
  RCString Cat(const char *lstr, const char *rstr);

 public:
  RCString(const char *str = 0) {
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

  RCString &operator=(const char *str) {
    Copy(str);
    return *this;
  }

  int operator==(const char *str) const;
  int operator==(const RCString &r) const;

  RCString &operator+=(const char *rstr);
  RCString &operator+=(const RCString &r);

  RCString    SubString(RCStringIndex start, RCStringIndex end) const;
  const char *GetString() const;

  operator const char *() const;

  void Get(char *buf, RCStringIndex bufSize) const;

  TRefCntPtr<CStringRep> m_rep;
};

class RCStaticString : public RCString {
 public:
  RCStaticString(const char *str = 0) : RCString(str) {
  }

  RCStaticString(const RCString &str) : RCString(str) {
  }

  RCStaticString &operator=(const char *str) {
    Copy(str);
    return *this;
  }

  RCStaticString &operator=(const RCString &str) {
    Copy(str);
    return *this;
  }

  operator const char *() const {
    const char *itemstring = GetString();
    return itemstring ? itemstring : "";
  }
};

#endif
