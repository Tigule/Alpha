#include <Base/CDataStore.h>

#include "Object/GuildStats.h"

void GuildStats_C::Unpack(CDataStore *msg) {
  ASSERT(msg);
  msg->Get(m_guildID);
  msg->GetString(m_guildName, 0x18);
  msg->Get(m_emblemStyle);
  msg->Get(m_emblemColor);
  msg->Get(m_borderStyle);
  msg->Get(m_borderColor);
  msg->Get(m_backgroundColor);
}
