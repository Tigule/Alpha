#include <Base/Base.h>
#include <WowConst.h>
#include <MapDefs.h>

#include <Base/CDataStore.h>

#include "Object/CreatureStats.h"

void CreatureStats::Pack(CDataStore *msg) {
  int i;

  for (i = 0; i < 4; ++i) {
    msg->PutString(m_name[i]);
  }

  msg->PutString(m_title);
  msg->Put(m_flags);
  msg->Put(m_creatureType);
  msg->Put(m_creatureFamily);
}

void CreatureStats_C::Unpack(CDataStore *msg) {
  int  i;
  char string[0x100];

  for (i = 0; i < 4; ++i) {
    msg->GetString(string, 0x100);

    if (!string[0] && i > 0) {
      m_name[i] = SStrDupA(m_name[i - 1], __FILE__, __LINE__);
    } else {
      m_name[i] = SStrDupA(string, __FILE__, __LINE__);
    }
  }

  msg->GetString(string, 0x100);
  m_title = SStrDupA(string, __FILE__, __LINE__);
  msg->Get(m_flags);
  msg->Get(m_creatureType);
  msg->Get(m_creatureFamily);
}
