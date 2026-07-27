#pragma once

#include <DB/WowClientDB.h>

class AreaTriggerRec {
 public:
  AreaTriggerRec();
  ~AreaTriggerRec();

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
  int   m_ContinentID;
  float m_x;
  float m_y;
  float m_z;
  float m_radius;
};

extern WowClientDB<AreaTriggerRec> g_areaTriggerDB;
