#pragma once

#include <DB/WowClientDB.h>

class TransportAnimationRec {
 public:
  TransportAnimationRec();
  ~TransportAnimationRec();

  static LPCSTR GetFilename();

  static UINT GetNumColumns() {
    return 6;
  }

  static UINT GetRowSize() {
    return 24;
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
  int   m_TransportID;
  int   m_TimeIndex;
  float m_PosX;
  float m_PosY;
  float m_PosZ;
};

extern WowClientDB<TransportAnimationRec> g_transportAnimationDB;
