#pragma once

#include <DB/WowClientDB.h>

class GameObjectDisplayInfoRec {
 public:
  GameObjectDisplayInfoRec();
  ~GameObjectDisplayInfoRec();

  static const char *GetFilename();

  static unsigned int GetNumColumns() {
    return 12;
  }

  static unsigned int GetRowSize() {
    return 48;
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
  const char *m_modelName;
  int         m_Sound[10];
};

extern WowClientDB<GameObjectDisplayInfoRec> g_gameObjectDisplayInfoDB;
