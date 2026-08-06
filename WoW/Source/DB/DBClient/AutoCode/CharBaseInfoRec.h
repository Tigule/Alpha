#pragma once

#include <DB/WowClientDB.h>

class CharBaseInfoRec {
 public:
  CharBaseInfoRec();
  ~CharBaseInfoRec();

  static LPCSTR GetFilename();

  static UINT GetNumColumns() {
    return 3;
  }

  static UINT GetRowSize() {
    return 6;
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

  BYTE m_raceID;
  BYTE m_classID;
  int  m_proficiency;
  int  m_generatedID;
};

extern WowClientDB<CharBaseInfoRec> g_charBaseInfoDB;
