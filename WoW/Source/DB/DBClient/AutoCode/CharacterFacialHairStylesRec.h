#pragma once

#include <DB/WowClientDB.h>

class CharacterFacialHairStylesRec {
 public:
  CharacterFacialHairStylesRec();
  ~CharacterFacialHairStylesRec();

  static const char *__fastcall GetFilename();

  static unsigned int GetNumColumns() {
    return 6;
  }

  static unsigned int GetRowSize() {
    return 24;
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

  int m_RaceID;
  int m_SexID;
  int m_VariationID;
  int m_BeardGeoset;
  int m_MoustacheGeoset;
  int m_SideburnGeoset;
  int m_generatedID;
};

extern WowClientDB<CharacterFacialHairStylesRec> g_characterFacialHairStylesDB;
