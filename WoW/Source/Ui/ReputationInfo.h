#pragma once

#include "Object/ObjectClient/Unit_C.h"

class CDataStore;

class CGReputationInfo {
 public:
  static void __fastcall          EnterWorld();
  static void __fastcall          LeaveWorld();
  static void __fastcall          ShutdownGame();
  static void __fastcall          OnInitializeFactions(CDataStore *msg);
  static void __fastcall          OnSetFactionVisible(CDataStore *msg);
  static void __fastcall          OnSetFactionStanding(CDataStore *msg);
  static int __fastcall           FactionToIndex(int faction);
  static int __fastcall           IndexToFaction(int index);
  static unsigned int __fastcall  GetNumFactions();
  static int __fastcall           GetFactionFromSortIndex(unsigned int index);
  static void __fastcall          SetAtWar(int faction, unsigned int state);
  static bool __fastcall          IsAtWar(int faction);
  static int __fastcall           GetFactionStanding(int faction);
  static UNIT_REACTION __fastcall GetFactionStandingReaction(int faction);
  static void __fastcall          SetFactionFlags(int index, unsigned char flags);
  static void __fastcall          SetFactionStanding(int factionIndex, int standing);

 protected:
  static unsigned int m_numFactions;
  static unsigned int m_factionFlags[64];
  static int          m_factionBase[64];
  static int          m_factionStandings[64];
  static int          m_factionMap[64];
  static int          m_factionSorting[64];

 private:
  static void __fastcall SortFactions();
};
