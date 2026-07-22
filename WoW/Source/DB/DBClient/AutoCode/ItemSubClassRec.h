#pragma once

#include <DB/WowClientDB.h>

class ItemSubClassRec {
 public:
  ItemSubClassRec();
  ~ItemSubClassRec();

  static const char *__fastcall GetFilename();

  static unsigned int GetNumColumns() {
    return 28;
  }

  static unsigned int GetRowSize() {
    return 112;
  }

  int GetID() {
    return m_generatedID;
  }

  bool NeedIDAssigned() {
    return true;
  }

  void SetID(int id) {
    m_generatedID = id;
  }

  bool Read(SFile *f, const char *stringBuffer);

  int         m_classID;
  int         m_subClassID;
  int         m_prerequisiteProficiency;
  int         m_postrequisiteProficiency;
  int         m_flags;
  int         m_displayFlags;
  int         m_weaponParrySeq;
  int         m_weaponReadySeq;
  int         m_weaponAttackSeq;
  int         m_WeaponSwingSize;
  const char *m_displayName_lang[NUM_LOCALES];
  int         m_displayName_flag;
  const char *m_verboseName_lang[NUM_LOCALES];
  int         m_verboseName_flag;
  int         m_generatedID;
};

extern WowClientDB<ItemSubClassRec> g_itemSubClassDB;
