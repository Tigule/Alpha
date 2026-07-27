#pragma once

#include <DB/WowClientDB.h>

class TaxiNodesRec {
 public:
  TaxiNodesRec();
  ~TaxiNodesRec();

  static const char *GetFilename();

  static unsigned int GetNumColumns() {
    return 14;
  }

  static unsigned int GetRowSize() {
    return 56;
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
  int         m_ContinentID;
  float       m_X;
  float       m_Y;
  float       m_Z;
  const char *m_Name_lang[8];
  int         m_Name_flag;
};

extern WowClientDB<TaxiNodesRec> g_taxiNodesDB;
