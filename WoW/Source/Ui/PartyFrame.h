#ifndef WOW_SOURCE_UI_PARTYFRAME_H
#define WOW_SOURCE_UI_PARTYFRAME_H

#include <storm.h>
#include <Tempest/c3vector.h>
#include "Object/Unit.h"

enum LOOT_METHOD {
  LOOT_METHOD_FREEFORALL = 0,
  LOOT_METHOD_ROUNDROBIN = 1,
  LOOT_METHOD_MASTERLOOTER = 2,
  LOOT_METHOD_MAX = 3
};

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
  static BOOL      IsMember(const DWORDLONG &guid);
  static DWORDLONG GetMemberByName(LPCSTR name);
  static DWORDLONG GetLeader() {
    return m_leader;
  }
  static int GetLeaderIndex() {
    return m_leaderIndex;
  }
  static BOOL InParty() {
    return NumMembers() != 0;
  }
  static UINT         NumMembers();
  static RemoteStats *GetRemoteStats(DWORDLONG guid);
  static RemoteStats *GetRemoteStatsByIndex(int index);
  static void         OnNameCacheCallback();
  static DWORDLONG    GetMember(UINT index) {
    FATALASSERT(index < 4);
    return m_members[index];
  }
  static void        SetLeader(DWORDLONG guid);
  static void        AddMember(DWORDLONG guid, int connected);
  static void        EnableMember(DWORDLONG guid, int enable);
  static void        RemoveActivePlayer(DWORDLONG guid);
  static void        RemoveAll();
  static void        SetLootMethod(LOOT_METHOD method, DWORDLONG master);
  static void        SetLookingForGroup(int looking);
  static LOOT_METHOD GetLootMethod() {
    return m_lootMethod;
  }
  static DWORDLONG GetMasterLooter() {
    return m_lootMaster;
  }
  static BOOL IsLookingForGroup() {
    return m_lookingForGroup;
  }

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
