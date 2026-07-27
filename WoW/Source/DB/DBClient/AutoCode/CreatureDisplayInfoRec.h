#pragma once

#include <DB/WowClientDB.h>

class CreatureDisplayInfoRec {
 public:
  CreatureDisplayInfoRec();
  ~CreatureDisplayInfoRec();

  static const char *GetFilename();

  static unsigned int GetNumColumns() {
    return 10;
  }

  static unsigned int GetRowSize() {
    return 40;
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
  int         m_modelID;
  int         m_soundID;
  int         m_extendedDisplayInfoID;
  float       m_creatureModelScale;
  int         m_creatureModelAlpha;
  const char *m_textureVariation[3];
  int         m_bloodID;
};

extern WowClientDB<CreatureDisplayInfoRec> g_creatureDisplayInfoDB;
