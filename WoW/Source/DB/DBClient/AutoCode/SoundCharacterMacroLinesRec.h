#pragma once

#include <DB/WowClientDB.h>

class SoundCharacterMacroLinesRec {
 public:
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

  int m_ID;
  int m_Category;
  int m_Sex;
  int m_Race;
  int m_SoundID;
};

extern WowClientDB<SoundCharacterMacroLinesRec> g_soundCharacterMacroLinesDB;
