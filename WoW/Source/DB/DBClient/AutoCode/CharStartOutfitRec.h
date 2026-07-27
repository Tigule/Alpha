#pragma once

#include <DB/WowClientDB.h>

class CharStartOutfitRec {
 public:
  CharStartOutfitRec();
  ~CharStartOutfitRec();

  static const char *GetFilename();

  static unsigned int GetNumColumns() {
    return 41;
  }

  static unsigned int GetRowSize() {
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

  bool Read(SFile *f, const char *stringBuffer);

  int           m_ID;
  unsigned char m_raceID;
  unsigned char m_classID;
  unsigned char m_sexID;
  unsigned char m_outfitID;
  int           m_ItemID[12];
  int           m_DisplayItemID[12];
  int           m_InventoryType[12];
};

extern WowClientDB<CharStartOutfitRec> g_charStartOutfitDB;
