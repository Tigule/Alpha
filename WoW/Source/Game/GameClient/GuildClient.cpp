#include "Game/GameClient/GuildClient.h"

#include "DB/DBClient/DBCacheInstances.h"
#include "Object/GuildStats.h"

bool GuildGetGuildTabard(
    UINT guildID,
    void (*callback)(int, const DWORDLONG &, LPVOID, bool),
    int &eStyle,
    int &eColor,
    int &bStyle,
    int &bColor,
    int &background
) {
  const GuildStats_C *guild = g_guildInfoCache.GetRecord(guildID, 0, callback, 0);

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

UINT GuildGetTabardCost() {
  return 100000;
}
