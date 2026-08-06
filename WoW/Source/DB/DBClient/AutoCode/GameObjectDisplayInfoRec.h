#pragma once

#include <DB/WowClientDB.h>

class GameObjectDisplayInfoRec {
 public:
  GameObjectDisplayInfoRec();
  ~GameObjectDisplayInfoRec();

  static LPCSTR GetFilename();

  static UINT GetNumColumns() {
    return 12;
  }

  static UINT GetRowSize() {
    return 48;
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
  LPCSTR m_modelName;
  int    m_Sound[10];
};

extern WowClientDB<GameObjectDisplayInfoRec> g_gameObjectDisplayInfoDB;
