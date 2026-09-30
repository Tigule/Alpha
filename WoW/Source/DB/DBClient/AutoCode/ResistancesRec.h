#pragma once

#include <DB/WowClientDB.h>

class ResistancesRec {
 public:
  int    m_ID;
  int    m_Flags;
  int    m_FizzleSoundID;
  LPCSTR m_name_lang[8];
  int    m_name_flag;

  ResistancesRec();
  ~ResistancesRec();

  static LPCSTR GetFilename();

  static UINT GetNumColumns() {
    return 12;
  }

  static UINT GetRowSize() {
    return 48;
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
};

extern WowClientDB<ResistancesRec> g_resistancesDB;
