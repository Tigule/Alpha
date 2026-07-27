#pragma once

#include <DB/WowClientDB.h>

class ChrClassesRec {
 public:
  ChrClassesRec();
  ~ChrClassesRec();

  static const char *GetFilename();

  static unsigned int GetNumColumns() {
    return 14;
  }

  static unsigned int GetRowSize() {
    return 56;
  }

  int GetID() const {
    return m_ID;
  }

  bool NeedIDAssigned() {
    return false;
  }

  void SetID(int id) {
  }

  bool Read(SFile *f, const char *stringBuffer);

  int         m_ID;
  int         m_PlayerClass;
  int         m_DamageBonusStat;
  int         m_DisplayPower;
  const char *m_petNameToken;
  const char *m_name_lang[NUM_LOCALES];
  int         m_name_flag;
};

extern WowClientDB<ChrClassesRec> g_chrClassesDB;
