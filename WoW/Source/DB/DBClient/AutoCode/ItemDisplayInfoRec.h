#pragma once

#include <DB/WowClientDB.h>

class ItemDisplayInfoRec {
 public:
  ItemDisplayInfoRec();
  ~ItemDisplayInfoRec();

  static LPCSTR GetFilename();

  static UINT GetNumColumns() {
    return 25;
  }

  static UINT GetRowSize() {
    return 100;
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
  LPCSTR m_modelName[2];
  LPCSTR m_modelTexture[2];
  LPCSTR m_inventoryIcon;
  LPCSTR m_groundModel;
  int    m_geosetGroup[4];
  int    m_flags;
  int    m_spellVisualID;
  int    m_groupSoundIndex;
  int    m_itemSize;
  int    m_helmetGeosetVisID;
  LPCSTR m_texture[8];
  int    m_itemVisual;
};

extern WowClientDB<ItemDisplayInfoRec> g_itemDisplayInfoDB;
