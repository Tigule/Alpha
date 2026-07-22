#pragma once

#include <DB/WowClientDB.h>

class FactionRec {
 public:
  FactionRec();
  ~FactionRec();

  static const char *__fastcall GetFilename();

  static unsigned int GetNumColumns() {
    return 23;
  }

  static unsigned int GetRowSize() {
    return 92;
  }

  int GetID() {
    return m_ID;
  }

  bool NeedIDAssigned() {
    return false;
  }

  void SetID(int id) {
  }

  bool Read(SFile *f, const char *stringBuffer);

  int         m_ID;
  int         m_reputationIndex;
  int         m_reputationRaceMask[4];
  int         m_reputationClassMask[4];
  int         m_reputationBase[4];
  const char *m_name_lang[8];
  int         m_name_flag;
};

extern WowClientDB<FactionRec> g_factionDB;
