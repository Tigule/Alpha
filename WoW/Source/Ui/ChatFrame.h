#ifndef WOW_SOURCE_UI_CHATFRAME_H
#define WOW_SOURCE_UI_CHATFRAME_H

class CDataStore;
class NameCache;
struct ChatChannel;

enum SLASH_COMMAND_ID {
  SLASH_COMMAND_SAY = 0
};

class CGChat {
 public:
  static void InitializeGame();
  static void ShutdownGame();
  static void EnterWorld();
  static void LeaveWorld();
  static int IsPaused();
  static int ChatHandler(CDataStore *msg);
  static void ChannelList(CDataStore *msg);
  static void ChannelNotify(CDataStore *msg);
  static int HandleTextEmote(CDataStore *msg);

  static void FilterChat(int filter) {
    m_filterChat = filter;
  }

  static void AddChatMessage(
      const char      *text,
      SLASH_COMMAND_ID type,
      const char      *player,
      unsigned int     language,
      const char      *channel,
      const char      *player2,
      const char      *specialFlag
  );
  static void AddTextEmoteMessage(const unsigned __int64 &senderGUID, int textEmoteID, const char *target);
  static void AddChannel(const char *name);
  static void RemoveChannel(const char *name);
  static int GetChannelID(const char *name);
  static const char *GetChannelName(int localID);
  static ChatChannel *GetChannel(const char *name);
  static void DisplayPendingUserList(ChatChannel *channel);
  static void QueueChatText(
      int              slashCmd,
      unsigned __int64 guid,
      char            *text,
      unsigned int     language,
      int              waitingForUI,
      int              parse,
      const char      *channel,
      unsigned __int64 guid2,
      const char      *specialFlag
  );
  static void QueueTextEmote(const unsigned __int64 &sender, int textEmoteID, const char *target, int waitingForUI);
  static void NameQueryCallback(int id, const unsigned __int64 &guid, void *arg, bool granted);
  static void TextEmoteNameQueryCallback(int id, const unsigned __int64 &guid, void *arg, bool granted);
  static void GetPendingChatMessages();
  static void UpdateLanguages();
  static void TranslateMessage(unsigned int language, unsigned int skill, const char *text, char *buffer, unsigned int size, int passXML);
  static const char *GetChannelString(const char *commandString);
  static void CheckFlagChanged(
      unsigned __int64 guid,
      const NameCache *nc,
      unsigned char    oldFlags,
      unsigned char    newFlags,
      const char      *channel,
      unsigned char    flagToCheck,
      const char      *setText,
      const char      *unsetText
  );
  static void HandleFlagsChanged(unsigned __int64 guid, unsigned char oldFlags, unsigned char newFlags, const char *channel);

 private:
  static int m_paused;
  static int m_filterChat;
};

#endif
