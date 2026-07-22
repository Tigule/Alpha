#ifndef WOW_SOURCE_GAME_GAMECLIENT_PETNAMECACHE_H
#define WOW_SOURCE_GAME_GAMECLIENT_PETNAMECACHE_H

class CDataStore;

class PetNameCache {
 public:
  PetNameCache() {
    m_ID = 0;
    m_name[0] = 0;
    m_timestamp = 0;
  }

  int Version() {
    return 1;
  }
  void Pack(CDataStore *msg);
  void Unpack(CDataStore *msg);

  char         m_name[0x30];
  unsigned int m_ID;
  unsigned int m_timestamp;
};

#endif
