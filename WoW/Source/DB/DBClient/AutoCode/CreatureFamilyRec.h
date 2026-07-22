#pragma once

#include <DB/WowClientDB.h>

class CreatureFamilyRec {
 public:
  CreatureFamilyRec();
  ~CreatureFamilyRec();

  static const char *__fastcall GetFilename();

  static unsigned int GetNumColumns() {
    return 7;
  }

  static unsigned int GetRowSize() {
    return 28;
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

  int   m_ID;
  float m_minScale;
  int   m_minScaleLevel;
  float m_maxScale;
  int   m_maxScaleLevel;
  int   m_skillLine[2];
};

extern WowClientDB<CreatureFamilyRec> g_creatureFamilyDB;
