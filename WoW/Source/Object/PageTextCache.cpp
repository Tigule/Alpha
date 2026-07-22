#include <Base/CDataStore.h>

#include "Object/PageTextCache.h"

void PageTextCache::Pack(CDataStore *msg) {
  msg->PutString(m_text);
  msg->Put(m_nextPage);
}

void PageTextCache_C::Unpack(CDataStore *msg) {
  msg->GetString(m_text, 0x7FFFFFFF);
  msg->Get(m_nextPage);
}
