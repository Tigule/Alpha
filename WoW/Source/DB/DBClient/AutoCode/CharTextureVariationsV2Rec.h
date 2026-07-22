#pragma once

#include <DB/WowClientDB.h>

class CharTextureVariationsV2Rec {
 public:
  CharTextureVariationsV2Rec();
  ~CharTextureVariationsV2Rec();

  static const char *__fastcall GetFilename();

  static unsigned int GetNumColumns() {
    return 8;
  }

  static unsigned int GetRowSize() {
    return 32;
  }

  int GetID() {
    return m_ID;
  }

  bool NeedIDAssigned() {
    return false;
  }

  void SetID(int id) {
  }

  bool Read(SFile *f, const char *stringBuffer);

  int         m_ID;
  int         m_RaceID;
  int         m_SexID;
  int         m_SectionID;
  int         m_VariationID;
  int         m_ColorID;
  int         m_IsNPC;
  const char *m_TextureName;
};

extern WowClientDB<CharTextureVariationsV2Rec> g_charTextureVariationsV2DB;
