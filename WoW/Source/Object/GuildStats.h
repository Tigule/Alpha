#ifndef WOW_SOURCE_OBJECT_GUILDSTATS_H
#define WOW_SOURCE_OBJECT_GUILDSTATS_H

class CDataStore;

class GuildStats {
 public:
  GuildStats() {
  }

  int Version() {
    return 1;
  }
  void Pack(CDataStore *msg);

  unsigned int m_guildID;
  char         m_guildName[0x18];
  int          m_emblemStyle;
  int          m_emblemColor;
  int          m_borderStyle;
  int          m_borderColor;
  int          m_backgroundColor;
};

class GuildStats_C : public GuildStats {
 public:
  void Unpack(CDataStore *msg);
};

#endif
