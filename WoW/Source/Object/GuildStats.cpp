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
  msg->Put(static_cast<UINT>(m_emblemStyle));
  msg->Put(static_cast<UINT>(m_emblemColor));
  msg->Put(static_cast<UINT>(m_borderStyle));
  msg->Put(static_cast<UINT>(m_borderColor));
  msg->Put(static_cast<UINT>(m_backgroundColor));
}
