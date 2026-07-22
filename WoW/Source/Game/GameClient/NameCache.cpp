#include <Base/CDataStore.h>

#include "Game/GameClient/NameCache.h"

void NameCache::Pack(CDataStore *msg) {
  msg->Put(m_guid);
  msg->PutString(m_name);
  msg->Put(m_race);
  msg->Put(m_sex);
  msg->Put(m_class);
  msg->Put(m_temp);
}

void NameCache::Unpack(CDataStore *msg) {
  msg->Get(m_guid);
  msg->GetString(m_name, 0x30);
  msg->Get(m_race);
  msg->Get(m_sex);
  msg->Get(m_class);
  msg->Get(m_temp);
}
