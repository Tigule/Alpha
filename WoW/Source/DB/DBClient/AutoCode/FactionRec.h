#pragma once

#include <DB/WowClientDB.h>

class FactionRec {
 public:
  int    m_ID;
  int    m_reputationIndex;
  int    m_reputationRaceMask[4];
  int    m_reputationClassMask[4];
  int    m_reputationBase[4];
  LPCSTR m_name_lang[8];
  int    m_name_flag;

  FactionRec();
  ~FactionRec();

  static LPCSTR GetFilename();

  static UINT GetNumColumns() {
    return 23;
  }

  static UINT GetRowSize() {
    return 92;
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

extern WowClientDB<FactionRec> g_factionDB;
