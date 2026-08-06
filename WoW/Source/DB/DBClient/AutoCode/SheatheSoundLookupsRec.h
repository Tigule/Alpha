#pragma once

#include <DB/WowClientDB.h>

class SheatheSoundLookupsRec {
 public:
  SheatheSoundLookupsRec();
  ~SheatheSoundLookupsRec();

  static LPCSTR GetFilename();

  static UINT GetNumColumns() {
    return 7;
  }

  static UINT GetRowSize() {
    return 28;
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
  int m_classID;
  int m_subclassID;
  int m_material;
  int m_checkMaterial;
  int m_sheatheSound;
  int m_unsheatheSound;
};

extern WowClientDB<SheatheSoundLookupsRec> g_sheatheSoundLookupsDB;
