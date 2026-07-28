#ifndef WOW_SOURCE_UI_CHATFRAME_H
#define WOW_SOURCE_UI_CHATFRAME_H

class CDataStore;
class NameCache;
struct ChatChannel;

enum SLASH_COMMAND_ID {
  SLASH_CMD_SAY = 0,
  SLASH_CMD_PARTY = 1,
  SLASH_CMD_GUILD = 2,
  SLASH_CMD_OFFICER = 3,
  SLASH_CMD_YELL = 4,
  SLASH_CMD_WHISPER = 5,
  SLASH_CMD_WHISPER_INFORM = 6,
  SLASH_CMD_EMOTE = 7,
  SLASH_CMD_TEXT_EMOTE = 8,
  SLASH_CMD_SYSTEM = 9,
  SLASH_CMD_MONSTER_SAY = 10,
  SLASH_CMD_MONSTER_YELL = 11,
  SLASH_CMD_MONSTER_EMOTE = 12,
  SLASH_CMD_SEND_CHANNEL = 13,
  SLASH_CMD_JOIN_CHANNEL = 14,
  SLASH_CMD_LEAVE_CHANNEL = 15,
  SLASH_CMD_LIST_CHANNEL = 16,
  SLASH_CMD_CHANNEL_NOTICE = 17,
  SLASH_CMD_CHANNEL_NOTICE_USER = 18,
  SLASH_CMD_SEND_AFK = 19,
  SLASH_CMD_SEND_DND = 20,
  SLASH_CMD_COMBAT_LOG = 21,
  SLASH_CMD_IGNORED = 22,
  SLASH_CMD_SKILL = 23,
  SLASH_CMD_LOOT = 24,
  SLASH_CMD_COMBAT_LOG_ENEMY = 25,
  SLASH_CMD_COMBAT_LOG_SELF = 26,
  SLASH_CMD_COMBAT_LOG_PARTY = 27,
  SLASH_CMD_COMBAT_LOG_ERROR = 28,
  SLASH_CMD_COMBAT_LOG_MISC_INFO = 29,
  NUM_SLASH_CMDS = 30
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
