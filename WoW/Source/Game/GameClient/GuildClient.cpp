#include "Game/GameClient/GuildClient.h"

#include "DB/DBClient/DBCacheInstances.h"
#include "Object/GuildStats.h"

unsigned int __fastcall GuildGetTabardCost() {
  return 100000;
}

bool __fastcall GuildGetGuildTabard(
    unsigned int guildID,
    void(__fastcall *callback)(int, const unsigned __int64 &, void *, bool),
    int &eStyle,
    int &eColor,
    int &bStyle,
    int &bColor,
    int &background
) {
  unsigned __int64    guid = 0;
  const GuildStats_C *guild = g_guildInfoCache.GetRecord(guildID, guid, callback, 0);

  eStyle = -1;
  eColor = -1;
  bStyle = -1;
  bColor = -1;
  background = -1;

  if (!guild || guild->m_emblemStyle == -1 || guild->m_emblemColor == -1 || guild->m_borderStyle == -1 || guild->m_borderColor == -1 ||
      guild->m_backgroundColor == -1)
  {
    return false;
  }

  eStyle = guild->m_emblemStyle;
  eColor = guild->m_emblemColor;
  bStyle = guild->m_borderStyle;
  bColor = guild->m_borderColor;
  background = guild->m_backgroundColor;
  return true;
}
