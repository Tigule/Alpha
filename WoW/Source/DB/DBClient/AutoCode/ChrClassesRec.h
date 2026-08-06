#pragma once

#include <DB/WowClientDB.h>

class ChrClassesRec {
 public:
  ChrClassesRec();
  ~ChrClassesRec();

  static LPCSTR GetFilename();

  static UINT GetNumColumns() {
    return 14;
  }

  static UINT GetRowSize() {
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

  bool Read(SFile *f, LPCSTR stringBuffer);

  int    m_ID;
  int    m_PlayerClass;
  int    m_DamageBonusStat;
  int    m_DisplayPower;
  LPCSTR m_petNameToken;
  LPCSTR m_name_lang[NUM_LOCALES];
  int    m_name_flag;
};

extern WowClientDB<ChrClassesRec> g_chrClassesDB;
