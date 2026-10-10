#include <Base/Base.h>
#include <Gx/Gx.h>
#include <WowConst.h>
#include <MapDefs.h>

#include <Base/CDataStore.h>

#include "Object/GuildStats.h"

void GuildStats::Pack(CDataStore *msg) {
  ASSERT(msg);
  msg->Put(m_guildID);
  if (m_guildName[0]) {
    msg->PutString(m_guildName);
  } else {
    msg->PutString("");
  }
  msg->Put((UINT)m_emblemStyle);
  msg->Put((UINT)m_emblemColor);
  msg->Put((UINT)m_borderStyle);
  msg->Put((UINT)m_borderColor);
  msg->Put((UINT)m_backgroundColor);
}
