#pragma once

#include <DB/WowClientDB.h>

class ChrProficiencyRec {
 public:
  ChrProficiencyRec();
  ~ChrProficiencyRec();

  static const char *GetFilename();

  static unsigned int GetNumColumns() {
    return 65;
  }

  static unsigned int GetRowSize() {
    return 260;
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

  int m_ID;
  int m_proficiency_minLevel[16];
  int m_proficiency_acquireMethod[16];
  int m_proficiency_itemClass[16];
  int m_proficiency_itemSubClassMask[16];
};

extern WowClientDB<ChrProficiencyRec> g_chrProficiencyDB;
