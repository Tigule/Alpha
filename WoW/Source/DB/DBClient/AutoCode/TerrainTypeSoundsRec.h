#pragma once

#include <DB/WowClientDB.h>

class TerrainTypeSoundsRec {
 public:
  TerrainTypeSoundsRec();
  ~TerrainTypeSoundsRec();

  static LPCSTR GetFilename();

  static UINT GetNumColumns() {
    return 1;
  }

  static UINT GetRowSize() {
    return 4;
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

  int m_ID;
};

extern WowClientDB<TerrainTypeSoundsRec> g_terrainTypeSoundsDB;
