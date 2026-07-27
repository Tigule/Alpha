#pragma once

#include <DB/WowClientDB.h>

class FactionTemplateRec {
 public:
  FactionTemplateRec();
  ~FactionTemplateRec();

  static const char *GetFilename();

  static unsigned int GetNumColumns() {
    return 13;
  }

  static unsigned int GetRowSize() {
    return 52;
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

  int m_ID;
  int m_faction;
  int m_factionGroup;
  int m_friendGroup;
  int m_enemyGroup;
  int m_enemies[4];
  int m_friend[4];
};

extern WowClientDB<FactionTemplateRec> g_factionTemplateDB;
