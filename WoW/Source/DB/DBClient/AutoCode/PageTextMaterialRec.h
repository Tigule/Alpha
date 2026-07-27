#pragma once

#include <DB/WowClientDB.h>

class PageTextMaterialRec {
 public:
  PageTextMaterialRec();
  ~PageTextMaterialRec();

  static const char *GetFilename();

  static unsigned int GetNumColumns() {
    return 2;
  }

  static unsigned int GetRowSize() {
    return 8;
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
  const char *m_name;
};

extern WowClientDB<PageTextMaterialRec> g_pageTextMaterialDB;
