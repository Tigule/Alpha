#pragma once

#include <DB/WowClientDB.h>

class ItemDisplayInfoRec {
 public:
  ItemDisplayInfoRec();
  ~ItemDisplayInfoRec();

  static const char *GetFilename();

  static unsigned int GetNumColumns() {
    return 25;
  }

  static unsigned int GetRowSize() {
    return 100;
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
  const char *m_modelName[2];
  const char *m_modelTexture[2];
  const char *m_inventoryIcon;
  const char *m_groundModel;
  int         m_geosetGroup[4];
  int         m_flags;
  int         m_spellVisualID;
  int         m_groupSoundIndex;
  int         m_itemSize;
  int         m_helmetGeosetVisID;
  const char *m_texture[8];
  int         m_itemVisual;
};

extern WowClientDB<ItemDisplayInfoRec> g_itemDisplayInfoDB;
