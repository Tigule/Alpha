#include <Base/CDataStore.h>

#include "Object/NPCText.h"

void NPCText::Pack(CDataStore *msg) {
  msg->PutString(m_text);
  msg->Put(m_soundID);
}

void NPCText::Unpack(CDataStore *msg) {
  char text[0x400];

  msg->GetString(text, 0x400);
  FREEIFUSED(m_text);
  m_text = SStrDupA(text, __FILE__, __LINE__);
  msg->Get(m_soundID);
}
