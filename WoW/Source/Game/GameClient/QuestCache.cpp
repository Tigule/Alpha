#include <Base/CDataStore.h>

#include "Game/GameClient/QuestCache.h"

void QuestCache::Pack(CDataStore *msg) {
  int count;
  int i;

  msg->Put(m_questId);
  msg->Put(m_questType);
  msg->Put(m_questLevel);
  msg->Put(m_questSortID);
  msg->Put(m_questInfoID);
  msg->Put(m_rewardNextQuest);
  msg->Put(m_rewardMoney);
  msg->Put(m_startItem);

  count = 0;
  for (i = 0; i < 4; ++i) {
    if (m_rewardItems[i]) {
      msg->Put(m_rewardItems[i]);
      msg->Put(m_rewardAmount[i]);
      ++count;
    }
  }

  while (count < 4) {
    msg->Put(0);
    msg->Put(0);
    ++count;
  }

  count = 0;
  for (i = 0; i < 6; ++i) {
    if (m_rewardChoiceItems[i]) {
      msg->Put(m_rewardChoiceItems[i]);
      msg->Put(m_rewardChoiceAmount[i]);
      ++count;
    }
  }

  while (count < 6) {
    msg->Put(0);
    msg->Put(0);
    ++count;
  }

  msg->Put(m_POIContinent);
  msg->Put(m_POIx);
  msg->Put(m_POIy);
  msg->Put(m_POIPriority);
  msg->PutString(m_logTitle);
  msg->PutString(m_logDescription);
  msg->PutString(m_questDescription);
  msg->PutString(m_areaDescription);

  for (i = 0; i < 4; ++i) {
    msg->Put(m_monsterToKill[i]);
    msg->Put(m_monsterToKillQuantity[i]);
    msg->Put(m_itemToGet[i]);
    msg->Put(m_itemToGetQuantity[i]);
    msg->PutString(m_getDescription[i]);
  }
}

void QuestCache::Unpack(CDataStore *msg) {
  int i;

  msg->Get(m_questId);
  msg->Get(m_questType);
  msg->Get(m_questLevel);
  msg->Get(m_questSortID);
  msg->Get(m_questInfoID);
  msg->Get(m_rewardNextQuest);
  msg->Get(m_rewardMoney);
  msg->Get(m_startItem);

  for (i = 0; i < 4; ++i) {
    msg->Get(m_rewardItems[i]);
    msg->Get(m_rewardAmount[i]);
  }

  for (i = 0; i < 6; ++i) {
    msg->Get(m_rewardChoiceItems[i]);
    msg->Get(m_rewardChoiceAmount[i]);
  }

  msg->Get(m_POIContinent);
  msg->Get(m_POIx);
  msg->Get(m_POIy);
  msg->Get(m_POIPriority);
  msg->GetString(m_logTitle, 0x7FFFFFFF);
  msg->GetString(m_logDescription, 0x7FFFFFFF);
  msg->GetString(m_questDescription, 0x7FFFFFFF);
  msg->GetString(m_areaDescription, 0x7FFFFFFF);

  for (i = 0; i < 4; ++i) {
    msg->Get(m_monsterToKill[i]);
    msg->Get(m_monsterToKillQuantity[i]);
    msg->Get(m_itemToGet[i]);
    msg->Get(m_itemToGetQuantity[i]);
    msg->GetString(m_getDescription[i], 0x7FFFFFFF);
  }
}
