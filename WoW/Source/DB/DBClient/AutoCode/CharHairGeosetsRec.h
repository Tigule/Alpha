#pragma once

#include <DB/WowClientDB.h>

class CharHairGeosetsRec {
 public:
  CharHairGeosetsRec();
  ~CharHairGeosetsRec();

  static LPCSTR GetFilename();

  static UINT GetNumColumns() {
    return 6;
  }

  static UINT GetRowSize() {
    return 24;
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
  int m_RaceID;
  int m_SexID;
  int m_VariationID;
  int m_GeosetID;
  int m_Showscalp;
};

extern WowClientDB<CharHairGeosetsRec> g_charHairGeosetsDB;
