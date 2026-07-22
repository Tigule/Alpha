#include <Base/CDataStore.h>

#include "Game/GameClient/PetNameCache.h"

void PetNameCache::Pack(CDataStore *msg) {
  msg->Put(m_ID);
  msg->PutString(m_name);
  msg->Put(m_timestamp);
}

void PetNameCache::Unpack(CDataStore *msg) {
  msg->Get(m_ID);
  msg->GetString(m_name, 0x30);
  msg->Get(m_timestamp);
}
