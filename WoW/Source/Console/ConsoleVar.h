#pragma once

#include <stpl.h>

struct CVar;

typedef bool(__fastcall *CVARCALLBACK)(CVar *, const char *, const char *, void *);

struct CVar : public TSHashObject<CVar, HASHKEY_STRI> {
  enum {
    ARCHIVE = 0x1,
    LATCH = 0x2
  };

  CVar();
  ~CVar();

  static void __fastcall Initialize(const char *filename);
  static void __fastcall Destroy();

  static CVar *__fastcall Register(
      const char  *name,
      const char  *help,
      unsigned int flags,
      const char  *value,
      CVARCALLBACK fcn,
      unsigned int category,
      bool         setCommand,
      void        *arg
  );
  static CVar *__fastcall Lookup(const char *name);

  const char *GetString() {
    return m_stringValue;
  }
  float GetFloat() {
    return m_floatValue;
  }
  int GetInt() {
    return m_intValue;
  }
  const char *GetName() {
    return m_name;
  }
  const char *GetLatchedValue() {
    return m_latchedValue;
  }
  const char *GetDefaultValue() {
    return m_defaultValue;
  }
  const char *GetResetValue() {
    return m_resetValue;
  }
  int Modified() {
    return m_modified;
  }
  bool IsArchived() {
    return (m_flags & ARCHIVE) != 0;
  }

  bool Set(const char *value, bool setValue, bool setReset, bool setDefault);
  void Reset();
  void Default();
  bool Update();

  char         m_name[32];
  unsigned int m_category;
  unsigned int m_flags;
  char        *m_stringValue;
  float        m_floatValue;
  int          m_intValue;
  int          m_modified;
  char        *m_defaultValue;
  char        *m_resetValue;
  char        *m_latchedValue;
  CVARCALLBACK m_callback;
  void        *m_arg;

 private:
  void InternalSet(const char *value, bool setValue, bool setReset, bool setDefault);
};
