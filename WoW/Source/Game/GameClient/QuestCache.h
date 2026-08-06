#ifndef WOW_SOURCE_GAME_GAMECLIENT_QUESTCACHE_H
#define WOW_SOURCE_GAME_GAMECLIENT_QUESTCACHE_H

#include <storm.h>

class CDataStore;

class QuestCache {
 public:
  QuestCache() {
    UINT index;

    m_questId = 0;
    m_questType = 0;
    m_questLevel = 0;
    m_questSortID = 0;
    m_questInfoID = 0;
    m_rewardNextQuest = 0;
    m_rewardMoney = 0;
    m_startItem = 0;

    for (index = 0; index < 4; ++index) {
      m_rewardItems[index] = 0;
      m_rewardAmount[index] = 1;
    }

    for (index = 0; index < 6; ++index) {
      m_rewardChoiceItems[index] = 0;
      m_rewardChoiceAmount[index] = 1;
    }

    m_POIContinent = 0;
    m_POIx = 0.0f;
    m_POIy = 0.0f;
    m_POIPriority = 0;
    m_logTitle[0] = 0;
    m_logDescription[0] = 0;
    m_questDescription[0] = 0;
    m_areaDescription[0] = 0;

    for (index = 0; index < 4; ++index) {
      m_monsterToKill[index] = 0;
      m_monsterToKillQuantity[index] = 0;
      m_itemToGet[index] = 0;
      m_itemToGetQuantity[index] = 0;
      m_getDescription[index][0] = 0;
    }
  }

  static int Version() {
    return 3;
  }
  void Pack(CDataStore *msg);
  void Unpack(CDataStore *msg);

  QuestCache &operator=(const QuestCache &rhs) {
    UINT index;

    m_questId = rhs.m_questId;
    m_questType = rhs.m_questType;
    m_questLevel = rhs.m_questLevel;
    m_questSortID = rhs.m_questSortID;
    m_questInfoID = rhs.m_questInfoID;
    m_rewardNextQuest = rhs.m_rewardNextQuest;
    m_rewardMoney = rhs.m_rewardMoney;
    m_startItem = rhs.m_startItem;

    for (index = 0; index < 4; ++index) {
      m_rewardItems[index] = rhs.m_rewardItems[index];
      m_rewardAmount[index] = rhs.m_rewardAmount[index];
    }

    for (index = 0; index < 6; ++index) {
      m_rewardChoiceItems[index] = rhs.m_rewardChoiceItems[index];
      m_rewardChoiceAmount[index] = rhs.m_rewardChoiceAmount[index];
    }

    m_POIContinent = rhs.m_POIContinent;
    m_POIx = rhs.m_POIx;
    m_POIy = rhs.m_POIy;
    m_POIPriority = rhs.m_POIPriority;
    SStrCopy(m_logTitle, rhs.m_logTitle, 0x80);
    SStrCopy(m_logDescription, rhs.m_logDescription, 0x400);
    SStrCopy(m_questDescription, rhs.m_questDescription, 0x400);
    SStrCopy(m_areaDescription, rhs.m_areaDescription, 0x80);

    for (index = 0; index < 4; ++index) {
      m_monsterToKill[index] = rhs.m_monsterToKill[index];
      m_monsterToKillQuantity[index] = rhs.m_monsterToKillQuantity[index];
      m_itemToGet[index] = rhs.m_itemToGet[index];
      m_itemToGetQuantity[index] = rhs.m_itemToGetQuantity[index];
      SStrCopy(m_getDescription[index], rhs.m_getDescription[index], 0x40);
    }

    return *this;
  }

  int   m_questId;
  int   m_questType;
  int   m_questLevel;
  int   m_questSortID;
  int   m_questInfoID;
  int   m_rewardNextQuest;
  int   m_rewardMoney;
  int   m_startItem;
  int   m_rewardItems[4];
  int   m_rewardAmount[4];
  int   m_rewardChoiceItems[6];
  int   m_rewardChoiceAmount[6];
  int   m_POIContinent;
  float m_POIx;
  float m_POIy;
  int   m_POIPriority;
  char  m_logTitle[0x80];
  char  m_logDescription[0x400];
  char  m_questDescription[0x400];
  char  m_areaDescription[0x80];
  int   m_monsterToKill[4];
  int   m_monsterToKillQuantity[4];
  int   m_itemToGet[4];
  int   m_itemToGetQuantity[4];
  char  m_getDescription[4][0x40];
};

#endif
