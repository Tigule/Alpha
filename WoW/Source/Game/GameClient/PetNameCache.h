#ifndef WOW_SOURCE_GAME_GAMECLIENT_PETNAMECACHE_H
#define WOW_SOURCE_GAME_GAMECLIENT_PETNAMECACHE_H

#include <storm.h>

class CDataStore;

class PetNameCache {
 public:
  PetNameCache() {
    m_ID = 0;
    m_name[0] = 0;
    m_timestamp = 0;
  }

  static int Version() {
    return 1;
  }
  void Pack(CDataStore *msg);
  void Unpack(CDataStore *msg);

  PetNameCache &operator=(const PetNameCache &rhs) {
    m_ID = rhs.m_ID;
    SStrCopy(m_name, rhs.m_name, 0x30);
    m_timestamp = rhs.m_timestamp;
    return *this;
  }

  char         m_name[0x30];
  unsigned int m_ID;
  unsigned int m_timestamp;
};

#endif
