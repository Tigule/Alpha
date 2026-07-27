#pragma once

#include <DB/WowClientDB.h>

class CharHairGeosetsRec {
 public:
  CharHairGeosetsRec();
  ~CharHairGeosetsRec();

  static const char *GetFilename();

  static unsigned int GetNumColumns() {
    return 6;
  }

  static unsigned int GetRowSize() {
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

  bool Read(SFile *f, const char *stringBuffer);

  int m_ID;
  int m_RaceID;
  int m_SexID;
  int m_VariationID;
  int m_GeosetID;
  int m_Showscalp;
};

extern WowClientDB<CharHairGeosetsRec> g_charHairGeosetsDB;
