#ifndef WOW_SOURCE_WOWSVCS_WOWSVCSCLIENT_FRIENDLIST_H
#define WOW_SOURCE_WOWSVCS_WOWSVCSCLIENT_FRIENDLIST_H

#include <storm.h>

class CDataStore;

enum FRIEND_RESULT {
  FRIEND_DB_ERROR,
  FRIEND_LIST_FULL,
  FRIEND_ONLINE,
  FRIEND_OFFLINE,
  FRIEND_NOT_FOUND,
  FRIEND_REMOVED,
  FRIEND_ADDED_ONLINE,
  FRIEND_ADDED_OFFLINE,
  FRIEND_ALREADY,
  FRIEND_SELF,
  FRIEND_ENEMY,
  FRIEND_IGNORE_FULL,
  FRIEND_IGNORE_SELF,
  FRIEND_IGNORE_NOT_FOUND,
  FRIEND_IGNORE_ALREADY,
  FRIEND_IGNORE_ADDED,
  FRIEND_IGNORE_REMOVED
};

class FriendList {
 public:
  class Friend {
   public:
    Friend() {
    }

    BYTE      m_connected;
    char     *m_name;
    DWORDLONG guid;
    int       m_level;
    int       m_class;
    int       m_area;

    ~Friend() {
      FREEIFUSED(m_name);
    }
  };

  ~FriendList();

  FriendList();
  static void   Initialize();
  static void   RegisterScriptFunctions();
  static void   UnregisterScriptFunctions();
  static void   Destroy();
  UINT          GetNumFriends();
  const Friend *GetFriend(UINT index);
  void          SetFriendSelectionIndex(UINT index);
  int           GetFriendSelectionIndex();
  void          AddFriend(LPCSTR name);
  void          RemoveFriend(LPCSTR name);
  void          RemoveFriend(DWORDLONG guid);
  void          RemoveFriend(UINT index);
  void          ShowFriends();
  UINT          GetNumIgnores();
  DWORDLONG     GetIgnore(UINT index);
  void          SetIgnoreSelectionIndex(UINT index);
  int           GetIgnoreSelectionIndex();
  void          AddOrDelIgnore(LPCSTR name);
  void          AddIgnore(LPCSTR name);
  void          DelIgnore(LPCSTR name);
  void          DelIgnore(DWORDLONG guid);
  void          SendWho(LPCSTR str);
  bool          IsIgnored(DWORDLONG guid);
  void          HandleStatus(FRIEND_RESULT result, DWORDLONG guid, CDataStore *msg);
  void          AddFriends(CDataStore *msg);
  void          IgnoreList(CDataStore *msg);
  void          SetName(DWORDLONG guid, LPCSTR name);
  void          DecrementPendingFriendName();
  void          DecrementPendingIgnoreName();
  void          SortFriends();
  void          SortIgnore();
  int           Added(DWORDLONG guid);
  void          Removed(DWORDLONG guid);
  void          SetConnected(DWORDLONG guid, bool connected);
  void          IgnoreAdded(DWORDLONG guid, int sort);
  void          IgnoreRemoved(DWORDLONG guid);

 private:
  Friend    m_friends[50];
  UINT      m_friendNamesPending;
  DWORDLONG m_selectedFriend;
  DWORDLONG m_ignore[25];
  UINT      m_ignoreNamesPending;
  DWORDLONG m_selectedIgnore;
};

extern FriendList *g_friendList;

#endif
