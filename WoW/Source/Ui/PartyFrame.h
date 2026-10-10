#ifndef WOW_SOURCE_UI_PARTYFRAME_H
#define WOW_SOURCE_UI_PARTYFRAME_H

#include <storm.h>
#include <Tempest/c3vector.h>
#include "Object/Unit.h"

class CGPartyInfo {
 public:
  struct RemoteStats {
    int                health;
    int                maxHealth;
    POWER_TYPE         powerType;
    int                power;
    int                maxPower;
    int                classID;
    int                level;
    int                mapID;
    int                areaID;
    NTempest::C3Vector pos;
    int                connected;
  };

  static void      InitializeGame();
  static void      EnterWorld();
  static void      LeaveWorld();
  static void      ShutdownGame();
  static BOOL InParty() {
    return NumMembers() != 0;
  }
  static BOOL      IsMember(const DWORDLONG &guid);
  static DWORDLONG GetMemberByName(LPCSTR name);
  static DWORDLONG GetLeader() {
    return m_leader;
  }
  static int GetLeaderIndex() {
    return m_leaderIndex;
  }
  static DWORDLONG    GetMember(UINT index) {
    return m_members[index];
  }
  static void        SetLeader(DWORDLONG guid);
  static void        AddMember(DWORDLONG guid, int connected);
  static void        EnableMember(DWORDLONG guid, int enable);
  static void        RemoveAll();
  static void        RemoveActivePlayer(DWORDLONG guid);
  static UINT         NumMembers();
  static void         OnNameCacheCallback();
  static void        SetLootMethod(LOOT_METHOD method, DWORDLONG master);
  static LOOT_METHOD GetLootMethod() {
    return m_lootMethod;
  }
  static DWORDLONG GetMasterLooter() {
    return m_lootMaster;
  }
  static RemoteStats *GetRemoteStats(DWORDLONG guid);
  static RemoteStats *GetRemoteStatsByIndex(int index) {
    FATALASSERT(index >= 0);
    return m_members[index] ? &m_remoteStats[index] : 0;
  }
  static BOOL IsLookingForGroup() {
    return m_lookingForGroup;
  }
  static void        SetLookingForGroup(int looking);

 protected:
  static DWORDLONG   m_leader;
  static int         m_leaderIndex;
  static DWORDLONG   m_members[4];
  static RemoteStats m_remoteStats[4];
  static LOOT_METHOD m_lootMethod;
  static DWORDLONG   m_lootMaster;
  static int         m_lookingForGroup;
};

#endif
