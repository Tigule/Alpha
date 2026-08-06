#pragma once

#include <stpl.h>

struct CVar : public TSHashObject<CVar, HASHKEY_STRI> {
  typedef bool (*CVARCALLBACKFCN)(CVar *, LPCSTR, LPCSTR, LPVOID);

  enum {
    ARCHIVE = 0x1,
    LATCH = 0x2
  };

  CVar();
  CVar(const CVar &);
  ~CVar();

  static void Initialize(LPCSTR filename);
  static void Destroy();

  static CVar *Register(LPCSTR name, LPCSTR help, UINT flags, LPCSTR value, CVARCALLBACKFCN fcn, UINT category, bool setCommand, LPVOID arg);
  static CVar *Lookup(LPCSTR name);

  LPCSTR GetString() const {
    return m_stringValue;
  }
  float GetFloat() const {
    return m_floatValue;
  }
  int GetInt() const {
    return m_intValue;
  }
  LPCSTR GetName() const {
    return m_name;
  }
  LPCSTR GetLatchedValue() const {
    return m_latchedValue;
  }
  LPCSTR GetDefaultValue() const {
    return m_defaultValue;
  }
  LPCSTR GetResetValue() const {
    return m_resetValue;
  }
  int Modified() const {
    return m_modified;
  }
  bool IsArchived() const {
    return (m_flags & ARCHIVE) != 0;
  }

  bool Set(LPCSTR value, bool setValue, bool setReset, bool setDefault);
  void Reset();
  void Default();
  bool Update();

 private:
  char            m_name[32];
  UINT            m_category;
  UINT            m_flags;
  char           *m_stringValue;
  float           m_floatValue;
  int             m_intValue;
  int             m_modified;
  char           *m_defaultValue;
  char           *m_resetValue;
  char           *m_latchedValue;
  CVARCALLBACKFCN m_callback;
  LPVOID          m_arg;

  void InternalSet(LPCSTR value, bool setValue, bool setReset, bool setDefault);
};
