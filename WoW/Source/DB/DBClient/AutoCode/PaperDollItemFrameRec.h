#pragma once

#include <DB/WowClientDB.h>

class PaperDollItemFrameRec {
 public:
  LPCSTR m_ItemButtonName;
  LPCSTR m_SlotIcon;
  int    m_SlotNumber;
  int    m_generatedID;

  PaperDollItemFrameRec();
  ~PaperDollItemFrameRec();

  static LPCSTR GetFilename();

  static UINT GetNumColumns() {
    return 3;
  }

  static UINT GetRowSize() {
    return 12;
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
};

extern WowClientDB<PaperDollItemFrameRec> g_paperDollItemFrameDB;
