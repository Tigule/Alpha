#ifndef WOW_SOURCE_GAME_GAMECLIENT_NAMECACHE_H
#define WOW_SOURCE_GAME_GAMECLIENT_NAMECACHE_H

#include <storm.h>

class CDataStore;

class NameCache {
 public:
  NameCache() : m_guid(0) {
    m_name[0] = 0;
  }

  static int Version() {
    return 4;
  }
  void Pack(CDataStore *msg);
  void Unpack(CDataStore *msg);

  NameCache &operator=(const NameCache &rhs) {
    m_guid = rhs.m_guid;
    SStrCopy(m_name, rhs.m_name, 0x30);
    m_race = rhs.m_race;
    m_sex = rhs.m_sex;
    m_class = rhs.m_class;
    m_temp = rhs.m_temp;
    return *this;
  }

  char      m_name[0x30];
  DWORDLONG m_guid;
  UINT      m_race;
  UINT      m_sex;
  BYTE      m_temp;
  UINT      m_class;
};

#endif
