#pragma once

#include <DB/WowClientDB.h>

class TerrainTypeSoundsRec {
 public:
  TerrainTypeSoundsRec();
  ~TerrainTypeSoundsRec();

  static const char *GetFilename();

  static unsigned int GetNumColumns() {
    return 1;
  }

  static unsigned int GetRowSize() {
    return 4;
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

  int m_ID;
};

extern WowClientDB<TerrainTypeSoundsRec> g_terrainTypeSoundsDB;
