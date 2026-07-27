#pragma once

#include "Object/ObjectClient/Unit_C.h"

class CDataStore;

class CGReputationInfo {
 public:
  static void EnterWorld();
  static void LeaveWorld();
  static void ShutdownGame();
  static void OnInitializeFactions(CDataStore *msg);
  static void OnSetFactionVisible(CDataStore *msg);
  static void OnSetFactionStanding(CDataStore *msg);
  static int FactionToIndex(int faction);
  static int IndexToFaction(int index);
  static unsigned int GetNumFactions();
  static int GetFactionFromSortIndex(unsigned int index);
  static void SetAtWar(int faction, unsigned int state);
  static bool IsAtWar(int faction);
  static int GetFactionStanding(int faction);
  static UNIT_REACTION GetFactionStandingReaction(int faction);
  static void SetFactionFlags(int index, unsigned char flags);
  static void SetFactionStanding(int factionIndex, int standing);

 protected:
  static unsigned int m_numFactions;
  static unsigned int m_factionFlags[64];
  static int          m_factionBase[64];
  static int          m_factionStandings[64];
  static int          m_factionMap[64];
  static int          m_factionSorting[64];

 private:
  static void SortFactions();
};
