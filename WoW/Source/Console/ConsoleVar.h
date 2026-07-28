#pragma once

#include <stpl.h>

struct CVar : public TSHashObject<CVar, HASHKEY_STRI> {
  typedef bool(*CVARCALLBACKFCN)(CVar *, const char *, const char *, void *);

  enum {
    ARCHIVE = 0x1,
    LATCH = 0x2
  };

  CVar();
  CVar(const CVar &);
  ~CVar();

  static void Initialize(const char *filename);
  static void Destroy();

  static CVar *Register(
      const char  *name,
      const char  *help,
      unsigned int flags,
      const char  *value,
      CVARCALLBACKFCN fcn,
      unsigned int category,
      bool         setCommand,
      void        *arg
  );
  static CVar *Lookup(const char *name);

  const char *GetString() const {
    return m_stringValue;
  }
  float GetFloat() const {
    return m_floatValue;
  }
  int GetInt() const {
    return m_intValue;
  }
  const char *GetName() const {
    return m_name;
  }
  const char *GetLatchedValue() const {
    return m_latchedValue;
  }
  const char *GetDefaultValue() const {
    return m_defaultValue;
  }
  const char *GetResetValue() const {
    return m_resetValue;
  }
  int Modified() const {
    return m_modified;
  }
  bool IsArchived() const {
    return (m_flags & ARCHIVE) != 0;
  }

  bool Set(const char *value, bool setValue, bool setReset, bool setDefault);
  void Reset();
  void Default();
  bool Update();

 private:
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
  CVARCALLBACKFCN m_callback;
  void        *m_arg;

  void InternalSet(const char *value, bool setValue, bool setReset, bool setDefault);
};
