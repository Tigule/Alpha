#pragma once

#include <DB/WowClientDB.h>

class SoundCharacterMacroLinesRec {
 public:
  SoundCharacterMacroLinesRec();
  ~SoundCharacterMacroLinesRec();

  static const char *__fastcall GetFilename();

  static unsigned int GetNumColumns() {
    return 5;
  }

  static unsigned int GetRowSize() {
    return 20;
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
  int m_Category;
  int m_Sex;
  int m_Race;
  int m_SoundID;
};

extern WowClientDB<SoundCharacterMacroLinesRec> g_soundCharacterMacroLinesDB;
