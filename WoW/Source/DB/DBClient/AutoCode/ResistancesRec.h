#pragma once

#include <DB/WowClientDB.h>

class ResistancesRec {
 public:
  ResistancesRec();
  ~ResistancesRec();

  static const char *GetFilename();

  static unsigned int GetNumColumns() {
    return 12;
  }

  static unsigned int GetRowSize() {
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

  bool Read(SFile *f, const char *stringBuffer);

  int         m_ID;
  int         m_Flags;
  int         m_FizzleSoundID;
  const char *m_name_lang[8];
  int         m_name_flag;
};

extern WowClientDB<ResistancesRec> g_resistancesDB;
