#pragma once

#include "Object/ObjectClient/Unit_C.h"

#define MAX_REPUTATION_FACTIONS 64

class CDataStore;

class CGReputationInfo {
 public:
  static void EnterWorld();
  static void LeaveWorld();
  static void ShutdownGame();
  static int  FactionToIndex(int faction);
  static int  IndexToFaction(int index);
  static void OnInitializeFactions(CDataStore *msg);
  static void OnSetFactionVisible(CDataStore *msg);
  static void OnSetFactionStanding(CDataStore *msg);
  static UINT GetNumFactions() {
    return m_numFactions;
  }
  static int           GetFactionFromSortIndex(UINT index);
  static void          SetFactionFlags(int index, BYTE flags);
  static void          SetAtWar(int faction, bool state);
  static bool          IsAtWar(int faction);
  static BYTE          IsVisible(int faction);
  static void          SetFactionStanding(int factionIndex, int standing);
  static int           GetFactionStanding(int faction);
  static UNIT_REACTION GetFactionStandingReaction(int faction);

 private:
  static void SortFactions();

 protected:
  static UINT m_numFactions;
  static BYTE m_factionFlags[MAX_REPUTATION_FACTIONS];
  static int  m_factionBase[MAX_REPUTATION_FACTIONS];
  static int  m_factionStandings[MAX_REPUTATION_FACTIONS];
  static int  m_factionMap[MAX_REPUTATION_FACTIONS];
  static int  m_factionSorting[MAX_REPUTATION_FACTIONS];
};
