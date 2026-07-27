#pragma once

#include <DB/WowClientDB.h>

class CreatureDisplayInfoExtraRec {
 public:
  CreatureDisplayInfoExtraRec();
  ~CreatureDisplayInfoExtraRec();

  static const char *GetFilename();

  static unsigned int GetNumColumns() {
    return 19;
  }

  static unsigned int GetRowSize() {
    return 76;
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
  int         m_DisplayRaceID;
  int         m_DisplaySexID;
  int         m_SkinID;
  int         m_FaceID;
  int         m_HairStyleID;
  int         m_HairColorID;
  int         m_FacialHairID;
  int         m_NPCItemDisplay[10];
  const char *m_BakeName;
};

extern WowClientDB<CreatureDisplayInfoExtraRec> g_creatureDisplayInfoExtraDB;
