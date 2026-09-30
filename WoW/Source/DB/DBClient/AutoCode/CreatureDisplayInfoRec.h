#pragma once

#include <DB/WowClientDB.h>

class CreatureDisplayInfoRec {
 public:
  int    m_ID;
  int    m_modelID;
  int    m_soundID;
  int    m_extendedDisplayInfoID;
  float  m_creatureModelScale;
  int    m_creatureModelAlpha;
  LPCSTR m_textureVariation[3];
  int    m_bloodID;

  CreatureDisplayInfoRec();
  ~CreatureDisplayInfoRec();

  static LPCSTR GetFilename();

  static UINT GetNumColumns() {
    return 10;
  }

  static UINT GetRowSize() {
    return 40;
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
};

extern WowClientDB<CreatureDisplayInfoRec> g_creatureDisplayInfoDB;
