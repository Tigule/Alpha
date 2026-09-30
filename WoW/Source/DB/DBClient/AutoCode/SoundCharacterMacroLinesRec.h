#pragma once

#include <DB/WowClientDB.h>

class SoundCharacterMacroLinesRec {
 public:
  int m_ID;
  int m_Category;
  int m_Sex;
  int m_Race;
  int m_SoundID;

  SoundCharacterMacroLinesRec();
  ~SoundCharacterMacroLinesRec();

  static LPCSTR GetFilename();

  static UINT GetNumColumns() {
    return 5;
  }

  static UINT GetRowSize() {
    return 20;
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

extern WowClientDB<SoundCharacterMacroLinesRec> g_soundCharacterMacroLinesDB;
