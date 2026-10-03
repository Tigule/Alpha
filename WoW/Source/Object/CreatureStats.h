#ifndef WOW_SOURCE_OBJECT_CREATURESTATS_H
#define WOW_SOURCE_OBJECT_CREATURESTATS_H

#include <string.h>

class CDataStore;

class CreatureStats {
 public:
  CreatureStats() : m_title(0) {
    int i;

    for (i = 0; i < 4; ++i) {
      m_name[i] = 0;
    }
  }

  ~CreatureStats() {
    int i;

    for (i = 0; i < 4; ++i) {
      FREEIFUSED(m_name[i]);
    }

    FREEIFUSED(m_title);
  }

  char *m_name[4];
  char *m_title;
  int   m_flags;
  int   m_creatureType;
  int   m_creatureFamily;

  static int Version() {
    return 1;
  }
  void Pack(CDataStore *msg);
};

class CreatureStats_C : public CreatureStats {
 public:
  void Unpack(CDataStore *msg);
};

#endif
