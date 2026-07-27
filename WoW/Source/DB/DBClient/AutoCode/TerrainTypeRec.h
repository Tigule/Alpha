#pragma once

#include <DB/WowClientDB.h>

class TerrainTypeRec {
 public:
  TerrainTypeRec();
  ~TerrainTypeRec();

  static const char *GetFilename();

  static unsigned int GetNumColumns() {
    return 6;
  }

  static unsigned int GetRowSize() {
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

  bool Read(SFile *f, const char *stringBuffer);

  int         m_TerrainID;
  const char *m_TerrainDesc;
  int         m_FootstepSprayRun;
  int         m_FootstepSprayWalk;
  int         m_SoundID;
  int         m_Flags;
  int         m_generatedID;
};

extern WowClientDB<TerrainTypeRec> g_terrainTypeDB;
