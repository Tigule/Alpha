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
  static void __fastcall InitializeGame();
  static void __fastcall ShutdownGame();
  static void __fastcall EnterWorld();
  static void __fastcall LeaveWorld();
  static int __fastcall  ChatHandler(CDataStore *msg);
  static void __fastcall ChannelList(CDataStore *msg);
  static void __fastcall ChannelNotify(CDataStore *msg);
  static int __fastcall  HandleTextEmote(CDataStore *msg);

  static void FilterChat(int filter) {
    m_filterChat = filter;
  }

  static void __fastcall AddChatMessage(
      const char      *text,
      SLASH_COMMAND_ID type,
      const char      *player,
      unsigned int     language,
      const char      *channel,
      const char      *player2,
      const char      *specialFlag
  );
  static void __fastcall         AddTextEmoteMessage(const unsigned __int64 &senderGUID, int textEmoteID, const char *target);
  static void __fastcall         AddChannel(const char *name);
  static void __fastcall         RemoveChannel(const char *name);
  static int __fastcall          GetChannelID(const char *name);
  static const char *__fastcall  GetChannelName(int localID);
  static ChatChannel *__fastcall GetChannel(const char *name);
  static void __fastcall         DisplayPendingUserList(ChatChannel *channel);
  static void __fastcall         QueueChatText(
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
  static void __fastcall QueueTextEmote(const unsigned __int64 &sender, int textEmoteID, const char *target, int waitingForUI);
  static void __fastcall NameQueryCallback(int id, const unsigned __int64 &guid, void *arg, bool granted);
  static void __fastcall TextEmoteNameQueryCallback(int id, const unsigned __int64 &guid, void *arg, bool granted);
  static void __fastcall GetPendingChatMessages();
  static void __fastcall UpdateLanguages();
  static void __fastcall TranslateMessage(unsigned int language, unsigned int skill, const char *text, char *buffer, unsigned int size, int passXML);
  static const char *__fastcall GetChannelString(const char *commandString);
  static void __fastcall        CheckFlagChanged(
      unsigned __int64 guid,
      const NameCache *nc,
      unsigned char    oldFlags,
      unsigned char    newFlags,
      const char      *channel,
      unsigned char    flagToCheck,
      const char      *setText,
      const char      *unsetText
  );
  static void __fastcall HandleFlagsChanged(unsigned __int64 guid, unsigned char oldFlags, unsigned char newFlags, const char *channel);

 private:
  static int m_paused;
  static int m_filterChat;
};

#endif
