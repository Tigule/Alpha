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
  struct Friend {
    unsigned int     m_connected;
    char            *m_name;
    unsigned __int64 guid;
    int              m_level;
    int              m_class;
    int              m_area;

    ~Friend() {
      FREEIFUSED(m_name);
    }
  };

  ~FriendList();

  FriendList();
  static void Initialize();
  static void RegisterScriptFunctions();
  static void UnregisterScriptFunctions();
  static void Destroy();
  unsigned int           GetNumFriends() const;
  Friend                *GetFriend(unsigned int index);
  void                   SetFriendSelectionIndex(unsigned int index);
  int                    GetFriendSelectionIndex() const;
  void                   AddFriend(const char *name);
  void                   RemoveFriend(const char *name);
  void                   RemoveFriend(unsigned __int64 guid);
  void                   RemoveFriend(unsigned int index);
  void                   ShowFriends();
  unsigned int           GetNumIgnores() const;
  unsigned __int64       GetIgnore(unsigned int index) const;
  void                   SetIgnoreSelectionIndex(unsigned int index);
  int                    GetIgnoreSelectionIndex() const;
  void                   AddOrDelIgnore(const char *name);
  void                   AddIgnore(const char *name);
  void                   DelIgnore(const char *name);
  void                   DelIgnore(unsigned __int64 guid);
  void                   SendWho(const char *str);
  bool                   IsIgnored(unsigned __int64 guid);
  void                   HandleStatus(FRIEND_RESULT result, unsigned __int64 guid, CDataStore *msg);
  void                   AddFriends(CDataStore *msg);
  void                   IgnoreList(CDataStore *msg);
  void                   SetName(unsigned __int64 guid, const char *name);
  void                   DecrementPendingFriendName();
  void                   DecrementPendingIgnoreName();
  void                   SortFriends();
  void                   SortIgnore();
  void                   IgnoreAdded(unsigned __int64 guid, int sort);
  void                   IgnoreRemoved(unsigned __int64 guid);

 private:
  Friend           m_friends[50];
  unsigned int     m_friendNamesPending;
  unsigned __int64 m_selectedFriend;
  unsigned __int64 m_ignore[25];
  unsigned int     m_ignoreNamesPending;
  unsigned __int64 m_selectedIgnore;
};

extern FriendList *g_friendList;

#endif
