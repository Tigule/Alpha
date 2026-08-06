#pragma once

#include <DB/WowClientDB.h>

class CreatureDisplayInfoExtraRec {
 public:
  CreatureDisplayInfoExtraRec();
  ~CreatureDisplayInfoExtraRec();

  static LPCSTR GetFilename();

  static UINT GetNumColumns() {
    return 19;
  }

  static UINT GetRowSize() {
    return 76;
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

  int    m_ID;
  int    m_DisplayRaceID;
  int    m_DisplaySexID;
  int    m_SkinID;
  int    m_FaceID;
  int    m_HairStyleID;
  int    m_HairColorID;
  int    m_FacialHairID;
  int    m_NPCItemDisplay[10];
  LPCSTR m_BakeName;
};

extern WowClientDB<CreatureDisplayInfoExtraRec> g_creatureDisplayInfoExtraDB;
