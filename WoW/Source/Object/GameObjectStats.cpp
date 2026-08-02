#include <Base/Base.h>
#include <WowConst.h>
#include <MapDefs.h>

#include <Base/CDataStore.h>

#include "Object/GameObjectStats.h"

void GameObjectStats::Pack(CDataStore *msg) {
  int i;

  msg->Put(m_typeID);
  msg->Put(m_displayID);

  for (i = 0; i < 4; ++i) {
    msg->PutString(m_name[i]);
  }

  for (i = 0; i < 10; ++i) {
    msg->Put(m_propValue[i]);
  }
}

void GameObjectStats_C::Unpack(CDataStore *msg) {
  int  i;
  char string[0x100];

  msg->Get(m_typeID);
  msg->Get(m_displayID);

  for (i = 0; i < 4; ++i) {
    msg->GetString(string, 0x100);

    if (!string[0] && i > 0) {
      m_name[i] = SStrDupA(m_name[i - 1], __FILE__, __LINE__);
    } else {
      m_name[i] = SStrDupA(string, __FILE__, __LINE__);
    }
  }

  for (i = 0; i < 10; ++i) {
    msg->Get(m_propValue[i]);
  }
}
