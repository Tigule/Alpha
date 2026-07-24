#ifndef WOW_SOURCE_WOWSVCS_WOWSVCSCLIENT_FRIENDLIST_H
#define WOW_SOURCE_WOWSVCS_WOWSVCSCLIENT_FRIENDLIST_H

#include <storm.h>

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
  static void __fastcall Initialize();
  static void __fastcall RegisterScriptFunctions();
  static void __fastcall UnregisterScriptFunctions();
  static void __fastcall Destroy();
  unsigned int           GetNumFriends() const;
  Friend                *GetFriend(unsigned int index);
  void                   SetFriendSelectionIndex(unsigned int index);
  int                    GetFriendSelectionIndex() const;
  void                   AddFriend(const char *name);
  void                   RemoveFriend(unsigned int index);
  void                   ShowFriends();
  unsigned int           GetNumIgnores() const;
  unsigned __int64       GetIgnore(unsigned int index) const;
  void                   SetIgnoreSelectionIndex(unsigned int index);
  int                    GetIgnoreSelectionIndex() const;
  void                   AddOrDelIgnore(const char *name);
  void                   AddIgnore(const char *name);
  void                   DelIgnore(const char *name);
  void                   SendWho(const char *str);

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
