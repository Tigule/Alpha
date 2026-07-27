#ifndef WOW_SOURCE_OBJECT_NPCTEXT_H
#define WOW_SOURCE_OBJECT_NPCTEXT_H

#include <storm.h>

class CDataStore;

class NPCText {
 public:
  NPCText() : m_text(0), m_soundID(0) {
  }

  ~NPCText() {
    FREEIFUSED(m_text);
  }

  static int Version() {
    return 6;
  }
  void Pack(CDataStore *msg);
  void Unpack(CDataStore *msg);

  char *m_text;
  int   m_soundID;
};

#endif
