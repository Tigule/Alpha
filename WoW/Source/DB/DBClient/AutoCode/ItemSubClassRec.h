#pragma once

#include <DB/WowClientDB.h>

class ItemSubClassRec {
 public:
  ItemSubClassRec();
  ~ItemSubClassRec();

  static LPCSTR GetFilename();

  static UINT GetNumColumns() {
    return 28;
  }

  static UINT GetRowSize() {
    return 112;
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

  bool Read(SFile *f, LPCSTR stringBuffer);

  int    m_classID;
  int    m_subClassID;
  int    m_prerequisiteProficiency;
  int    m_postrequisiteProficiency;
  int    m_flags;
  int    m_displayFlags;
  int    m_weaponParrySeq;
  int    m_weaponReadySeq;
  int    m_weaponAttackSeq;
  int    m_WeaponSwingSize;
  LPCSTR m_displayName_lang[NUM_LOCALES];
  int    m_displayName_flag;
  LPCSTR m_verboseName_lang[NUM_LOCALES];
  int    m_verboseName_flag;
  int    m_generatedID;
};

extern WowClientDB<ItemSubClassRec> g_itemSubClassDB;
