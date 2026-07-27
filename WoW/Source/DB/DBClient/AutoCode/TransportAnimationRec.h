#pragma once

#include <DB/WowClientDB.h>

class TransportAnimationRec {
 public:
  TransportAnimationRec();
  ~TransportAnimationRec();

  static const char *GetFilename();

  static unsigned int GetNumColumns() {
    return 6;
  }

  static unsigned int GetRowSize() {
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

  bool Read(SFile *f, const char *stringBuffer);

  int   m_ID;
  int   m_TransportID;
  int   m_TimeIndex;
  float m_PosX;
  float m_PosY;
  float m_PosZ;
};

extern WowClientDB<TransportAnimationRec> g_transportAnimationDB;
