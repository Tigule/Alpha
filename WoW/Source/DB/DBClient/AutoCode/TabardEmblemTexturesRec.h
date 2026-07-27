#pragma once

#include <DB/WowClientDB.h>

class TabardEmblemTexturesRec {
 public:
  TabardEmblemTexturesRec();
  ~TabardEmblemTexturesRec();

  static const char *GetFilename();

  static unsigned int GetNumColumns() {
    return 3;
  }

  static unsigned int GetRowSize() {
    return 12;
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

  int         m_ID;
  const char *m_TorsoTexture[2];
};

extern WowClientDB<TabardEmblemTexturesRec> g_tabardEmblemTexturesDB;
