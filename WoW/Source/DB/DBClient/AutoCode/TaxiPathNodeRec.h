#pragma once

#include <DB/WowClientDB.h>

class TaxiPathNodeRec {
 public:
  TaxiPathNodeRec();
  ~TaxiPathNodeRec();

  static LPCSTR GetFilename();

  static UINT GetNumColumns() {
    return 8;
  }

  static UINT GetRowSize() {
    return 32;
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

  int   m_ID;
  int   m_PathID;
  int   m_NodeIndex;
  int   m_ContinentID;
  float m_LocX;
  float m_LocY;
  float m_LocZ;
  int   m_flags;
};

extern WowClientDB<TaxiPathNodeRec> g_taxiPathNodeDB;
