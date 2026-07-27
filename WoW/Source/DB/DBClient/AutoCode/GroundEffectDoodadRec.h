#pragma once

#include <DB/WowClientDB.h>

class GroundEffectDoodadRec {
 public:
  GroundEffectDoodadRec();
  ~GroundEffectDoodadRec();

  static const char *GetFilename();

  static unsigned int GetNumColumns() {
    return 3;
  }

  static unsigned int GetRowSize() {
    return 12;
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
  int         m_doodadIdTag;
  const char *m_doodadpath;
};

extern WowClientDB<GroundEffectDoodadRec> g_groundEffectDoodadDB;
