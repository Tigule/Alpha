#pragma once

#include <DB/WowClientDB.h>

class PaperDollItemFrameRec {
 public:
  PaperDollItemFrameRec();
  ~PaperDollItemFrameRec();

  static const char *__fastcall GetFilename();

  static unsigned int GetNumColumns() {
    return 3;
  }

  static unsigned int GetRowSize() {
    return 12;
  }

  int GetID() {
    return m_generatedID;
  }

  bool NeedIDAssigned() {
    return true;
  }

  void SetID(int id) {
    m_generatedID = id;
  }

  bool Read(SFile *f, const char *stringBuffer);

  const char *m_ItemButtonName;
  const char *m_SlotIcon;
  int         m_SlotNumber;
  int         m_generatedID;
};

extern WowClientDB<PaperDollItemFrameRec> g_paperDollItemFrameDB;
