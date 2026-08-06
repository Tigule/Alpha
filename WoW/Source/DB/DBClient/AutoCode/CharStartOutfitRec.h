#pragma once

#include <DB/WowClientDB.h>

class CharStartOutfitRec {
 public:
  CharStartOutfitRec();
  ~CharStartOutfitRec();

  static LPCSTR GetFilename();

  static UINT GetNumColumns() {
    return 41;
  }

  static UINT GetRowSize() {
    return 152;
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

  int  m_ID;
  BYTE m_raceID;
  BYTE m_classID;
  BYTE m_sexID;
  BYTE m_outfitID;
  int  m_ItemID[12];
  int  m_DisplayItemID[12];
  int  m_InventoryType[12];
};

extern WowClientDB<CharStartOutfitRec> g_charStartOutfitDB;
