#ifndef WOW_SOURCE_UI_TUTORIAL_H
#define WOW_SOURCE_UI_TUTORIAL_H

#include "Net/NetClient/NetClient.h"
#include "WowServices/BitField.h"

class CDataStore;

enum TUTORIAL {
  TUTORIAL_QUESTGIVERS = 0,
  TUTORIAL_MOVEMENT = 1,
  TUTORIAL_CAMERA = 2,
  TUTORIAL_TARGETING = 3,
  TUTORIAL_TARGETING_ENEMY = 4,
  TUTORIAL_COMBAT = 5,
  TUTORIAL_LOOTING = 6,
  TUTORIAL_ITEMS = 7,
  TUTORIAL_USABLE_ITEMS = 8,
  TUTORIAL_BAGS = 9,
  TUTORIAL_FOOD = 10,
  TUTORIAL_DRINK = 11,
  TUTORIAL_TALENTS = 12,
  TUTORIAL_SKILLS = 13,
  TUTORIAL_ABILITIES = 14,
  TUTORIAL_REPUTATION = 15,
  TUTORIAL_TELLS = 16,
  TUTORIAL_GROUPING = 17,
  NUM_TUTORIALS = 18
};

class CGTutorial {
 public:
  static void InitializeGame();
  static void ShutdownGame();
  static void TriggerTutorial(TUTORIAL tutorial);
  static void ClearTutorials();
  static void ResetTutorials();

 protected:
  static FBitField m_tutorialFlags;

 private:
  static int OnTutorialFlags(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg);
};

#endif
