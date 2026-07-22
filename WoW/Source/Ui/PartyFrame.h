#ifndef WOW_SOURCE_UI_PARTYFRAME_H
#define WOW_SOURCE_UI_PARTYFRAME_H

#include <storm.h>
#include <Tempest/c3vector.h>

enum LOOT_METHOD {
  LOOT_METHOD_FREEFORALL = 0,
  LOOT_METHOD_ROUNDROBIN = 1,
  LOOT_METHOD_MASTERLOOTER = 2,
  LOOT_METHOD_MAX = 3
};

enum POWER_TYPE {
  POWER_TYPE_MANA = 0,
  POWER_TYPE_RAGE = 1,
  POWER_TYPE_FOCUS = 2,
  POWER_TYPE_ENERGY = 3
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

  static void __fastcall             InitializeGame();
  static void __fastcall             EnterWorld();
  static void __fastcall             LeaveWorld();
  static void __fastcall             ShutdownGame();
  static int __fastcall              IsMember(const unsigned __int64 &guid);
  static unsigned __int64 __fastcall GetMemberByName(const char *name);
  static unsigned __int64 __fastcall GetLeader() {
    return m_leader;
  }
  static int __fastcall GetLeaderIndex() {
    return m_leaderIndex;
  }
  static int __fastcall InParty() {
    return NumMembers() != 0;
  }
  static unsigned int __fastcall NumMembers();
  static RemoteStats *__fastcall GetRemoteStats(unsigned __int64 guid);
  static unsigned __int64        GetMember(unsigned int index) {
    FATALASSERT(index < 4);
    return m_members[index];
  }
  static void __fastcall        SetLeader(unsigned __int64 guid);
  static void __fastcall        AddMember(unsigned __int64 guid, int connected);
  static void __fastcall        EnableMember(unsigned __int64 guid, int enable);
  static void __fastcall        RemoveActivePlayer(unsigned __int64 guid);
  static void __fastcall        RemoveAll();
  static void __fastcall        SetLootMethod(LOOT_METHOD method, unsigned __int64 master);
  static void __fastcall        SetLookingForGroup(int looking);
  static LOOT_METHOD __fastcall GetLootMethod() {
    return m_lootMethod;
  }
  static unsigned __int64 __fastcall GetMasterLooter() {
    return m_lootMaster;
  }
  static int __fastcall IsLookingForGroup() {
    return m_lookingForGroup;
  }

 protected:
  static unsigned __int64 m_leader;
  static int              m_leaderIndex;
  static unsigned __int64 m_members[4];
  static RemoteStats      m_remoteStats[4];
  static LOOT_METHOD      m_lootMethod;
  static unsigned __int64 m_lootMaster;
  static int              m_lookingForGroup;
};

#endif
