#pragma once

#include <DB/WowClientDB.h>

class TaxiNodesRec {
 public:
  TaxiNodesRec();
  ~TaxiNodesRec();

  static LPCSTR GetFilename();

  static UINT GetNumColumns() {
    return 14;
  }

  static UINT GetRowSize() {
    return 56;
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
  int    m_ContinentID;
  float  m_X;
  float  m_Y;
  float  m_Z;
  LPCSTR m_Name_lang[8];
  int    m_Name_flag;
};

extern WowClientDB<TaxiNodesRec> g_taxiNodesDB;
