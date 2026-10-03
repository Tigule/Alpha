#ifndef WOW_SOURCE_OBJECT_GAMEOBJECTSTATS_H
#define WOW_SOURCE_OBJECT_GAMEOBJECTSTATS_H

#include <string.h>

class CDataStore;

class GameObjectStats {
 public:
  GameObjectStats() {
    int i;

    for (i = 0; i < 4; ++i) {
      m_name[i] = 0;
    }
  }

  ~GameObjectStats() {
    int i;

    for (i = 0; i < 4; ++i) {
      FREEIFUSED(m_name[i]);
    }
  }

  int   m_typeID;
  int   m_displayID;
  char *m_name[4];
  int   m_propValue[10];

  static int Version() {
    return 1;
  }
  void Pack(CDataStore *msg);
};

class GameObjectStats_C : public GameObjectStats {
 public:
  void Unpack(CDataStore *msg);
};

#endif
