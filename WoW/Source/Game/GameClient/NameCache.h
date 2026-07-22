#ifndef WOW_SOURCE_GAME_GAMECLIENT_NAMECACHE_H
#define WOW_SOURCE_GAME_GAMECLIENT_NAMECACHE_H

class CDataStore;

class NameCache {
 public:
  NameCache() : m_guid(0) {
    m_name[0] = 0;
  }

  int Version() {
    return 1;
  }
  void Pack(CDataStore *msg);
  void Unpack(CDataStore *msg);

  char             m_name[0x30];
  unsigned __int64 m_guid;
  unsigned int     m_race;
  unsigned int     m_sex;
  unsigned int     m_temp;
  unsigned int     m_class;
};

#endif
