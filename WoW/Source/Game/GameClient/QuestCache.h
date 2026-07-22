#ifndef WOW_SOURCE_GAME_GAMECLIENT_QUESTCACHE_H
#define WOW_SOURCE_GAME_GAMECLIENT_QUESTCACHE_H

class CDataStore;

class QuestCache {
 public:
  QuestCache() {
    unsigned int index;

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

  int Version() {
    return 1;
  }
  void Pack(CDataStore *msg);
  void Unpack(CDataStore *msg);

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
