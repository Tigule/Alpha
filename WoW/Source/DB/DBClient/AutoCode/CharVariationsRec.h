#pragma once

#include <DB/WowClientDB.h>

class CharVariationsRec {
 public:
  CharVariationsRec();
  ~CharVariationsRec();

  static LPCSTR GetFilename();

  static UINT GetNumColumns() {
    return 6;
  }

  static UINT GetRowSize() {
    return 24;
  }

  int GetID() const {
    return m_generatedID;
  }

  bool NeedIDAssigned() {
    return true;
  }

  void SetID(int id) {
    m_generatedID = id;
  }

  bool Read(SFile *f, LPCSTR stringBuffer);

  int m_RaceID;
  int m_SexID;
  int m_TextureHoldLayer[4];
  int m_generatedID;
};

extern WowClientDB<CharVariationsRec> g_charVariationsDB;
