#pragma once

#include <DB/WowClientDB.h>

class CharacterFacialHairStylesRec {
 public:
  CharacterFacialHairStylesRec();
  ~CharacterFacialHairStylesRec();

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
  int m_VariationID;
  int m_BeardGeoset;
  int m_MoustacheGeoset;
  int m_SideburnGeoset;
  int m_generatedID;
};

extern WowClientDB<CharacterFacialHairStylesRec> g_characterFacialHairStylesDB;
