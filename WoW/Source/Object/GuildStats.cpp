#include <Base/CDataStore.h>

#include "Object/GuildStats.h"

void GuildStats::Pack(CDataStore *msg) {
  ASSERT(msg);
  msg->Put(m_guildID);
  msg->PutString(m_guildName[0] ? m_guildName : "");
  msg->Put(m_emblemStyle);
  msg->Put(m_emblemColor);
  msg->Put(m_borderStyle);
  msg->Put(m_borderColor);
  msg->Put(m_backgroundColor);
}
