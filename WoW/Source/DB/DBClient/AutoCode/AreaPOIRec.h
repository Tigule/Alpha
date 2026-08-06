#pragma once

#include <DB/WowClientDB.h>

class AreaPOIRec {
 public:
  AreaPOIRec();
  ~AreaPOIRec();

  static LPCSTR GetFilename();

  static UINT GetNumColumns() {
    return 18;
  }

  static UINT GetRowSize() {
    return 72;
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
  int    m_importance;
  int    m_icon;
  int    m_factionID;
  float  m_x;
  float  m_y;
  float  m_z;
  int    m_continentID;
  int    m_flags;
  LPCSTR m_name_lang[8];
  int    m_name_flag;
};

extern WowClientDB<AreaPOIRec> g_areaPOIDB;
