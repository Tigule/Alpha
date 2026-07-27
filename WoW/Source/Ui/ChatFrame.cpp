#include "ChatFrame.h"

#include <Base/CDataStore.h>
#include <Console/ConsoleClient.h>
#include <FrameScript/FrameScript.h>
#include <storm.h>
#include <stpl.h>

#include "DB/DBClient/DBCacheInstances.h"
#include "DB/DBClient/AutoCode/ChrRacesRec.h"
#include "DB/DBClient/AutoCode/EmotesTextRec.h"
#include "DB/DBClient/AutoCode/LanguageWordsRec.h"
#include "DB/DBClient/AutoCode/LanguagesRec.h"
#include "Object/ObjectClient/Player_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "WowSvcs/WowSvcsClient/ClientServices.h"

#include <Net/NetClient/NetClient.h>

#include <ctype.h>
#include <lauxlib.h>
#include <lua.h>
#include <string.h>

struct PENDINGUSERLIST : public TSLinkedNode<PENDINGUSERLIST> {
  unsigned __int64 guid;
  unsigned int     flags;
};

struct ChatChannel {
  ChatChannel() : localID(0), channelFlags(0) {
    name[0] = 0;
  }

  int                                                  localID;
  char                                                 name[128];
  TSList<PENDINGUSERLIST, TSGetLink<PENDINGUSERLIST> > pendingNames;
  unsigned int                                         channelFlags;
};

struct PENDINGCHAT : public TSLinkedNode<PENDINGCHAT> {
  int              slashCmd;
  unsigned __int64 guid;
  char            *text;
  unsigned int     language;
  int              waitingForUI;
  int              parse;
  char             channel[128];
  unsigned __int64 guid2;
  char             specialFlag[5];
};

struct PENDINGTEXTEMOTE : public TSLinkedNode<PENDINGTEXTEMOTE> {
  unsigned __int64 sender;
  int              textEmoteID;
  char            *target;
  int              waitingForUI;
};

class HASHKEY_LANGUAGE {
 public:
  HASHKEY_LANGUAGE() : m_languageID(0), m_length(0) {
  }

  HASHKEY_LANGUAGE(unsigned int languageID, unsigned int length) : m_languageID(languageID), m_length(length) {
  }

  unsigned int operator==(const HASHKEY_LANGUAGE &key) const {
    return m_languageID == key.m_languageID && m_length == key.m_length;
  }

  HASHKEY_LANGUAGE &operator=(const HASHKEY_LANGUAGE &key) {
    m_languageID = key.m_languageID;
    m_length = key.m_length;
    return *this;
  }

 private:
  unsigned int m_languageID;
  unsigned int m_length;
};

struct WORDLIST : public TSHashObject<WORDLIST, HASHKEY_LANGUAGE> {
  TSGrowableArray<const LanguageWordsRec *> m_words;
};

static const unsigned int                           s_events[30] = {217, 218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231,
                                                                    232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 338, 339, 340, 341, 342};
static TSList<PENDINGCHAT, TSGetLink<PENDINGCHAT> > s_pendingChat;
static TSList<PENDINGTEXTEMOTE, TSGetLink<PENDINGTEXTEMOTE> > s_pendingTextEmote;
static TSGrowableArray<ChatChannel>                           s_channels;
static TSHashTable<WORDLIST, HASHKEY_LANGUAGE>                s_wordLists;
static int                                                    s_loggingEnabled;
static HSLOG                                                  s_logHandle;

unsigned __int64 Script_GetGUIDFromName(const char *name);

int CGChat::m_paused = 1;
int CGChat::m_filterChat = 1;

void CGChat::InitializeGame() {
  unsigned int numEntries = g_languageWordsDB.GetNumRecords();
  for (unsigned int i = 0; i < numEntries; ++i) {
    LanguageWordsRec *wordRec = g_languageWordsDB.GetRecordByIndex(i);
    unsigned int      len = SStrLen(wordRec->m_word);
    HASHKEY_LANGUAGE  key(wordRec->m_languageID, len);
    unsigned int      hash = wordRec->m_languageID ^ (len << 16);
    WORDLIST         *wordList = s_wordLists.Ptr(hash, key);
    if (!wordList) {
      wordList = s_wordLists.New(hash, key, 0, 0);
    }
    *wordList->m_words.New() = wordRec;
  }
}

void CGChat::ShutdownGame() {
  s_wordLists.Destroy();

  PENDINGCHAT *pending;
  while ((pending = s_pendingChat.Head()) != 0) {
    SMemFree(pending->text, __FILE__, __LINE__, 0);
    s_pendingChat.UnlinkNode(pending);
    s_pendingChat.DeleteNode(pending);
  }

  PENDINGTEXTEMOTE *pendingEmote;
  while ((pendingEmote = s_pendingTextEmote.Head()) != 0) {
    SMemFree(pendingEmote->target, __FILE__, __LINE__, 0);
    s_pendingTextEmote.UnlinkNode(pendingEmote);
    s_pendingTextEmote.DeleteNode(pendingEmote);
  }

  for (unsigned int i = 0; i < s_channels.Count(); ++i) {
    s_channels[i].pendingNames.Clear();
  }
  s_channels.Clear();
}

void CGChat::EnterWorld() {
  m_paused = 0;
  GetPendingChatMessages();
}

void CGChat::LeaveWorld() {
  m_paused = 1;
}

static void SendChatEvent(
    const char      *text,
    SLASH_COMMAND_ID type,
    const char      *player,
    unsigned int     language,
    const char      *channel,
    const char      *player2,
    const char      *specialFlag
) {
  if (type >= 30) {
    return;
  }

  if (!text)
    text = "";
  if (!player)
    player = "";
  if (!channel)
    channel = "";
  if (!player2)
    player2 = "";
  if (!specialFlag)
    specialFlag = "";

  FrameScript_SignalEvent(s_events[type], "%s%s%s%s%s%s", text, player, "", channel, player2, specialFlag);
}

void CGChat::AddChatMessage(
    const char      *text,
    SLASH_COMMAND_ID type,
    const char      *player,
    unsigned int     language,
    const char      *channel,
    const char      *player2,
    const char      *specialFlag
) {
  if (s_loggingEnabled && s_logHandle) {
    SLogWrite(s_logHandle, "%s", text ? text : "");
  }
  SendChatEvent(text, type, player, language, channel, player2, specialFlag);
}

void CGChat::AddTextEmoteMessage(const unsigned __int64 &senderGUID, int textEmoteID, const char *target) {
  NameCache *nc = const_cast<NameCache *>(g_nameDBCache.GetRecord(senderGUID, senderGUID, 0, 0));
  if (!nc) {
    return;
  }

  char buffer[256];
  if (target && *target) {
    SStrPrintf(buffer, sizeof(buffer), "%s %s", nc->m_name, target);
  } else {
    SStrCopy(buffer, nc->m_name, sizeof(buffer));
  }
  AddChatMessage(buffer, static_cast<SLASH_COMMAND_ID>(8), nc->m_name, 0, 0, 0, 0);
}

static void CopyWordCase(char *buffer, const char *token, const char *word, unsigned int maxlen) {
  unsigned int i;
  for (i = 0; word[i] && i + 1 < maxlen; ++i) {
    if (token[i] && isupper(static_cast<unsigned char>(token[i]))) {
      buffer[i] = static_cast<char>(toupper(static_cast<unsigned char>(word[i])));
    } else {
      buffer[i] = static_cast<char>(tolower(static_cast<unsigned char>(word[i])));
    }
  }
  buffer[i] = 0;
}

void CGChat::TranslateMessage(unsigned int language, unsigned int skill, const char *text, char *buffer, unsigned int size, int passXML) {
  if (!size) {
    return;
  }
  buffer[0] = 0;
  if (!text || skill >= 300) {
    SStrCopy(buffer, text ? text : "", size);
    return;
  }

  while (*text && SStrLen(buffer) + 1 < size) {
    if (isspace(static_cast<unsigned char>(*text)) || (passXML && *text == '<')) {
      char separator[2] = {*text++, 0};
      SStrPack(buffer, separator, size);
      continue;
    }

    char         token[256];
    unsigned int len = 0;
    while (text[len] && !isspace(static_cast<unsigned char>(text[len])) && len + 1 < sizeof(token)) {
      token[len] = text[len];
      ++len;
    }
    token[len] = 0;
    text += len;

    unsigned int            hash = SStrHash(token, 0, 0);
    unsigned int            count = 0;
    const LanguageWordsRec *replacement = 0;
    for (unsigned int i = 0; i < g_languageWordsDB.GetNumRecords(); ++i) {
      const LanguageWordsRec *word = g_languageWordsDB.GetRecordByIndex(i);
      if (word && word->m_languageID == static_cast<int>(language) && SStrLen(word->m_word) == len) {
        ++count;
        if (hash % count == 0) {
          replacement = word;
        }
      }
    }

    if (replacement && hash % 300 >= skill) {
      char translated[256];
      CopyWordCase(translated, token, replacement->m_word, sizeof(translated));
      SStrPack(buffer, translated, size);
    } else {
      SStrPack(buffer, token, size);
    }
  }
}

void CGChat::UpdateLanguages() {
  FrameScript_SignalEvent(242);
}

void CGChat::AddChannel(const char *name) {
  unsigned int index;
  for (index = 0; index < s_channels.Count(); ++index) {
    if (!s_channels[index].localID) {
      break;
    }
  }

  ChatChannel *channel = index < s_channels.Count() ? &s_channels[index] : s_channels.New();
  channel->localID = index + 1;
  SStrCopy(channel->name, name, sizeof(channel->name));
}

void CGChat::RemoveChannel(const char *name) {
  for (unsigned int index = 0; index < s_channels.Count(); ++index) {
    ChatChannel &channel = s_channels[index];
    if (!SStrCmpI(channel.name, name, 0x7FFFFFFF)) {
      channel.localID = 0;
      channel.name[0] = 0;
      return;
    }
  }
}

int CGChat::GetChannelID(const char *name) {
  ChatChannel *channel = GetChannel(name);
  return channel ? channel->localID : 0;
}

const char *CGChat::GetChannelName(int localID) {
  if (localID < 1 || static_cast<unsigned int>(localID) > s_channels.Count()) {
    return 0;
  }
  ChatChannel &channel = s_channels[localID - 1];
  return channel.localID == localID ? channel.name : 0;
}

ChatChannel *CGChat::GetChannel(const char *name) {
  for (unsigned int index = 0; index < s_channels.Count(); ++index) {
    if (!SStrCmpI(s_channels[index].name, name, 0x7FFFFFFF)) {
      return &s_channels[index];
    }
  }
  return 0;
}

void CGChat::ChannelNotify(CDataStore *msg) {
  char             namebuffer[256] = "";
  char             buffer[512] = "";
  char             channel[128];
  const char      *name[2] = {0, 0};
  unsigned __int64 guid2 = 0;
  unsigned int     oldFlags = 0;
  unsigned int     newFlags = 0;
  unsigned __int64 guid = 0;
  unsigned int     type;
  unsigned char    byteType;
  SLASH_COMMAND_ID eventType = static_cast<SLASH_COMMAND_ID>(9);

  msg->Get(byteType);
  type = byteType;
  msg->GetString(channel, sizeof(channel));

  switch (type) {
    case 0:
      eventType = static_cast<SLASH_COMMAND_ID>(14);
      msg->Get(guid);
      if (!GetChannelID(channel))
        return;
      break;
    case 1:
      eventType = static_cast<SLASH_COMMAND_ID>(15);
      msg->Get(guid);
      if (!GetChannelID(channel))
        return;
      break;
    case 2:
      eventType = static_cast<SLASH_COMMAND_ID>(17);
      if (msg->IsRead())
        namebuffer[0] = 0;
      else
        msg->GetString(namebuffer, sizeof(namebuffer));
      AddChannel(channel);
      SStrCopy(buffer, "YOU_JOINED", sizeof(buffer));
      break;
    case 3:
      eventType = static_cast<SLASH_COMMAND_ID>(17);
      SStrCopy(buffer, "YOU_LEFT", sizeof(buffer));
      break;
    case 4:
      eventType = static_cast<SLASH_COMMAND_ID>(17);
      SStrCopy(buffer, "WRONG_PASSWORD", sizeof(buffer));
      break;
    case 5:
      eventType = static_cast<SLASH_COMMAND_ID>(17);
      SStrCopy(buffer, "NOT_MEMBER", sizeof(buffer));
      break;
    case 6:
      eventType = static_cast<SLASH_COMMAND_ID>(17);
      SStrCopy(buffer, "NOT_MODERATOR", sizeof(buffer));
      break;
    case 7:
      eventType = static_cast<SLASH_COMMAND_ID>(18);
      msg->Get(guid);
      SStrCopy(buffer, "PASSWORD_CHANGED", sizeof(buffer));
      break;
    case 8:
      eventType = static_cast<SLASH_COMMAND_ID>(18);
      msg->Get(guid);
      SStrCopy(buffer, "OWNER_CHANGED", sizeof(buffer));
      break;
    case 9:
      eventType = static_cast<SLASH_COMMAND_ID>(18);
      msg->GetString(namebuffer, sizeof(namebuffer));
      name[0] = namebuffer;
      SStrCopy(buffer, "PLAYER_NOT_FOUND", sizeof(buffer));
      break;
    case 10:
      eventType = static_cast<SLASH_COMMAND_ID>(17);
      SStrCopy(buffer, "NOT_OWNER", sizeof(buffer));
      break;
    case 11:
      eventType = static_cast<SLASH_COMMAND_ID>(18);
      msg->GetString(namebuffer, sizeof(namebuffer));
      name[0] = namebuffer;
      SStrCopy(buffer, "CHANNEL_OWNER", sizeof(buffer));
      break;
    case 12: {
      unsigned char oldByte;
      unsigned char newByte;
      msg->Get(guid);
      msg->Get(oldByte);
      msg->Get(newByte);
      oldFlags = oldByte;
      newFlags = newByte;
      HandleFlagsChanged(guid, oldFlags, newFlags, channel);
      return;
    }
    case 13:
      eventType = static_cast<SLASH_COMMAND_ID>(18);
      msg->Get(guid);
      SStrCopy(buffer, "ANNOUNCEMENTS_ON", sizeof(buffer));
      break;
    case 14:
      eventType = static_cast<SLASH_COMMAND_ID>(18);
      msg->Get(guid);
      SStrCopy(buffer, "ANNOUNCEMENTS_OFF", sizeof(buffer));
      break;
    case 15:
      eventType = static_cast<SLASH_COMMAND_ID>(18);
      msg->Get(guid);
      SStrCopy(buffer, "MODERATION_ON", sizeof(buffer));
      break;
    case 16:
      eventType = static_cast<SLASH_COMMAND_ID>(18);
      msg->Get(guid);
      SStrCopy(buffer, "MODERATION_OFF", sizeof(buffer));
      break;
    case 17:
      eventType = static_cast<SLASH_COMMAND_ID>(17);
      SStrCopy(buffer, "MUTED", sizeof(buffer));
      break;
    case 18:
      eventType = static_cast<SLASH_COMMAND_ID>(18);
      msg->Get(guid);
      msg->Get(guid2);
      SStrCopy(buffer, "PLAYER_KICKED", sizeof(buffer));
      break;
    case 19:
      eventType = static_cast<SLASH_COMMAND_ID>(17);
      SStrCopy(buffer, "BANNED", sizeof(buffer));
      break;
    case 20:
      eventType = static_cast<SLASH_COMMAND_ID>(18);
      msg->Get(guid);
      msg->Get(guid2);
      SStrCopy(buffer, "PLAYER_BANNED", sizeof(buffer));
      break;
    case 21:
      eventType = static_cast<SLASH_COMMAND_ID>(18);
      msg->Get(guid);
      msg->Get(guid2);
      SStrCopy(buffer, "PLAYER_UNBANNED", sizeof(buffer));
      break;
    case 22:
      eventType = static_cast<SLASH_COMMAND_ID>(18);
      msg->GetString(namebuffer, sizeof(namebuffer));
      name[0] = namebuffer;
      SStrCopy(buffer, "PLAYER_NOT_BANNED", sizeof(buffer));
      break;
    case 23:
      eventType = static_cast<SLASH_COMMAND_ID>(18);
      msg->Get(guid);
      SStrCopy(buffer, "PLAYER_ALREADY_MEMBER", sizeof(buffer));
      break;
    case 24:
      eventType = static_cast<SLASH_COMMAND_ID>(18);
      msg->Get(guid);
      SStrCopy(buffer, "INVITE", sizeof(buffer));
      break;
    case 25:
      eventType = static_cast<SLASH_COMMAND_ID>(18);
      SStrCopy(buffer, "INVITE_WRONG_FACTION", sizeof(buffer));
      break;
    case 26:
      eventType = static_cast<SLASH_COMMAND_ID>(18);
      SStrCopy(buffer, "WRONG_FACTION", sizeof(buffer));
      break;
    default:
      break;
  }

  if (guid) {
    NameCache *nc = const_cast<NameCache *>(g_nameDBCache.GetRecord(guid, guid, NameQueryCallback, 0));
    if (!nc) {
      QueueChatText(eventType, guid, buffer, 0, 0, 0, channel, guid2, "");
      return;
    }
    name[0] = nc->m_name;
  }
  if (guid2) {
    NameCache *nc = const_cast<NameCache *>(g_nameDBCache.GetRecord(guid2, guid2, NameQueryCallback, 0));
    if (!nc) {
      QueueChatText(eventType, guid, buffer, 0, 0, 0, channel, guid2, "");
      return;
    }
    name[1] = nc->m_name;
  }

  AddChatMessage(buffer, eventType, name[0], 0, channel, name[1], 0);
  if (type == 3)
    RemoveChannel(channel);
  if (type == 2 && namebuffer[0]) {
    AddChatMessage(namebuffer, eventType, 0, 0, channel, 0, 0);
  }
}

void CGChat::ChannelList(CDataStore *msg) {
  char         channelName[128];
  int          count;
  int          i;
  ChatChannel *channel;
  int          pending;

  msg->GetString(channelName, sizeof(channelName));
  channel = GetChannel(channelName);
  if (!channel) {
    msg->Seek(msg->Size());
    return;
  }

  unsigned char channelFlags;
  msg->Get(channelFlags);
  channel->channelFlags = channelFlags;
  msg->Get(count);
  pending = 0;
  for (i = 0; i < count; ++i) {
    PENDINGUSERLIST *user = channel->pendingNames.NewNode(LIST_TAIL, 0, 0);
    unsigned char    flags;
    msg->Get(user->guid);
    msg->Get(flags);
    user->flags = flags;
    if (!g_nameDBCache.GetRecord(user->guid, user->guid, NameQueryCallback, 0)) {
      ++pending;
    }
  }
  if (!pending)
    DisplayPendingUserList(channel);
}

void CGChat::DisplayPendingUserList(ChatChannel *channel) {
  char line[256] = "";
  char buffer[58];
  int  namesThisLine = 0;

  for (PENDINGUSERLIST *user = channel->pendingNames.Head(); user; user = channel->pendingNames.Next(user)) {
    NameCache *nc = const_cast<NameCache *>(g_nameDBCache.GetRecord(user->guid, user->guid, 0, 0));
    if (!nc)
      return;

    char marker = ' ';
    if (user->flags & 1)
      marker = '*';
    else if (user->flags & 2)
      marker = '@';
    else if ((channel->channelFlags & 2) && !(user->flags & 4))
      marker = '#';
    SStrPrintf(buffer, sizeof(buffer), "%c%s", marker, nc->m_name);
    if (SStrLen(line) + SStrLen(buffer) >= sizeof(line)) {
      AddChatMessage(line, SLASH_COMMAND_SAY, 0, 0, channel->name, 0, 0);
      line[0] = 0;
      namesThisLine = 0;
    }
    if (namesThisLine++)
      SStrPack(line + SStrLen(line), ",", sizeof(line) - SStrLen(line));
    SStrPack(line + SStrLen(line), buffer, sizeof(line) - SStrLen(line));
  }
  if (namesThisLine)
    AddChatMessage(line, SLASH_COMMAND_SAY, 0, 0, channel->name, 0, 0);
  channel->pendingNames.Clear();
}

void CGChat::QueueChatText(
    int              slashCmd,
    unsigned __int64 guid,
    char            *text,
    unsigned int     language,
    int              waitingForUI,
    int              parse,
    const char      *channel,
    unsigned __int64 guid2,
    const char      *specialFlag
) {
  PENDINGCHAT *pending = s_pendingChat.NewNode(LIST_TAIL, 0, 0);
  pending->slashCmd = slashCmd;
  pending->guid = guid;
  pending->text = SStrDupA(text, __FILE__, __LINE__);
  pending->language = language;
  pending->waitingForUI = waitingForUI;
  pending->parse = parse;
  SStrCopy(pending->channel, channel ? channel : "", sizeof(pending->channel));
  pending->guid2 = guid2;
  SStrCopy(pending->specialFlag, specialFlag ? specialFlag : "", sizeof(pending->specialFlag));
}

void CGChat::QueueTextEmote(const unsigned __int64 &sender, int textEmoteID, const char *target, int waitingForUI) {
  PENDINGTEXTEMOTE *pending = s_pendingTextEmote.NewNode(LIST_TAIL, 0, 0);
  pending->sender = sender;
  pending->textEmoteID = textEmoteID;
  pending->target = SStrDupA(target, __FILE__, __LINE__);
  pending->waitingForUI = waitingForUI;
}

void CGChat::NameQueryCallback(int, const unsigned __int64 &, void *, bool) {
  for (unsigned int i = 0; i < s_channels.Count(); ++i) {
    if (!s_channels[i].pendingNames.IsEmpty()) {
      DisplayPendingUserList(&s_channels[i]);
    }
  }
  GetPendingChatMessages();
}

void CGChat::TextEmoteNameQueryCallback(int, const unsigned __int64 &, void *, bool) {
  for (PENDINGTEXTEMOTE *pending = s_pendingTextEmote.Head(); pending;) {
    PENDINGTEXTEMOTE *next = s_pendingTextEmote.Next(pending);
    if (g_nameDBCache.GetRecord(pending->sender, pending->sender, 0, 0)) {
      if (!m_paused) {
        AddTextEmoteMessage(pending->sender, pending->textEmoteID, pending->target);
      }
      SMemFree(pending->target, __FILE__, __LINE__, 0);
      s_pendingTextEmote.UnlinkNode(pending);
      s_pendingTextEmote.DeleteNode(pending);
    }
    pending = next;
  }
}

void CGChat::GetPendingChatMessages() {
  if (m_paused)
    return;
  for (PENDINGCHAT *pending = s_pendingChat.Head(); pending;) {
    PENDINGCHAT *next = s_pendingChat.Next(pending);
    NameCache   *nc = pending->guid ? const_cast<NameCache *>(g_nameDBCache.GetRecord(pending->guid, pending->guid, 0, 0)) : 0;
    NameCache   *nc2 = pending->guid2 ? const_cast<NameCache *>(g_nameDBCache.GetRecord(pending->guid2, pending->guid2, 0, 0)) : 0;
    if ((!pending->guid || nc) && (!pending->guid2 || nc2)) {
      AddChatMessage(
          pending->text, static_cast<SLASH_COMMAND_ID>(pending->slashCmd), nc ? nc->m_name : 0, pending->language, pending->channel,
          nc2 ? nc2->m_name : 0, pending->specialFlag
      );
      SMemFree(pending->text, __FILE__, __LINE__, 0);
      s_pendingChat.UnlinkNode(pending);
      s_pendingChat.DeleteNode(pending);
    }
    pending = next;
  }
}

extern bool QuestParserParseText(const char *text, char *buf, unsigned int size, const unsigned __int64 &target, int restoreToken);

int CGChat::ChatHandler(CDataStore *msg) {
  char             message[512] = "";
  char             buffer[512] = "";
  char             name[48] = "";
  char             channel[128] = "";
  NameCache       *nc = 0;
  unsigned int     language;
  const char      *specialFlag = "";
  unsigned __int64 guid = 0;
  unsigned int     afkDND;
  unsigned int     slashCmd;
  unsigned char    byteValue;

  msg->Get(byteValue);
  slashCmd = byteValue;
  msg->Get(language);
  if (slashCmd == 10 || slashCmd == 11 || slashCmd == 12) {
    msg->GetString(name, sizeof(name));
    msg->Get(guid);
    msg->GetString(message, sizeof(message));
    msg->Get(byteValue);
    afkDND = byteValue;
    if (!QuestParserParseText(message, buffer, sizeof(buffer), guid, 0)) {
      if (guid && !g_nameDBCache.GetRecord(guid, guid, NameQueryCallback, 0)) {
        QueueChatText(slashCmd, guid, message, language, 0, 1, channel, 0, "");
      }
      return 1;
    }
  } else {
    if (slashCmd == 13)
      msg->GetString(channel, sizeof(channel));
    msg->Get(guid);
    msg->GetString(message, sizeof(message));
    msg->Get(byteValue);
    afkDND = byteValue;
    if (afkDND == 2)
      specialFlag = "DND";
    else if (afkDND == 1)
      specialFlag = "AFK";
    else if (afkDND == 3)
      specialFlag = "GM";

    if (guid) {
      nc = const_cast<NameCache *>(g_nameDBCache.GetRecord(guid, guid, NameQueryCallback, 0));
      if (!nc) {
        QueueChatText(slashCmd, guid, message, language, 0, 0, channel, 0, specialFlag);
        return 1;
      }
    }
  }

  if (!msg->IsRead()) {
    msg->Reset();
    return 1;
  }
  if (m_paused) {
    QueueChatText(slashCmd, guid, slashCmd >= 10 && slashCmd <= 12 ? buffer : message, language, 1, 0, channel, 0, specialFlag);
    return 1;
  }
  AddChatMessage(
      slashCmd >= 10 && slashCmd <= 12 ? buffer : message, static_cast<SLASH_COMMAND_ID>(slashCmd),
      slashCmd >= 10 && slashCmd <= 12 ? name : (nc ? nc->m_name : 0), language, channel, 0, specialFlag
  );
  return 1;
}

int CGChat::HandleTextEmote(CDataStore *msg) {
  char             target[128];
  unsigned __int64 sender;
  int              textEmoteID;

  msg->Get(sender);
  msg->Get(textEmoteID);
  msg->GetString(target, sizeof(target));
  if (g_nameDBCache.GetRecord(sender, sender, TextEmoteNameQueryCallback, 0)) {
    if (m_paused)
      QueueTextEmote(sender, textEmoteID, target, 1);
    else
      AddTextEmoteMessage(sender, textEmoteID, target);
  } else {
    QueueTextEmote(sender, textEmoteID, target, 0);
  }
  return 1;
}

const char *CGChat::GetChannelString(const char *commandString) {
  unsigned int localID = SStrToUnsigned(commandString);
  return localID ? GetChannelName(localID) : commandString;
}

void CGChat::CheckFlagChanged(
    unsigned __int64,
    const NameCache *nc,
    unsigned char    oldFlags,
    unsigned char    newFlags,
    const char      *channel,
    unsigned char    flagToCheck,
    const char      *setText,
    const char      *unsetText
) {
  if (!(oldFlags & flagToCheck) && (newFlags & flagToCheck)) {
    AddChatMessage(setText, SLASH_COMMAND_SAY, nc->m_name, 0, channel, 0, 0);
  } else if ((oldFlags & flagToCheck) && !(newFlags & flagToCheck)) {
    AddChatMessage(unsetText, SLASH_COMMAND_SAY, nc->m_name, 0, channel, 0, 0);
  }
}

void CGChat::HandleFlagsChanged(unsigned __int64 guid, unsigned char oldFlags, unsigned char newFlags, const char *channel) {
  const NameCache *nc = g_nameDBCache.GetRecord(guid, guid, NameQueryCallback, 0);
  if (!nc)
    return;
  CheckFlagChanged(guid, nc, oldFlags, newFlags, channel, 2, "SET_MODERATOR", "UNSET_MODERATOR");
  CheckFlagChanged(guid, nc, oldFlags, newFlags, channel, 4, "SET_VOICE", "UNSET_VOICE");
}

static int StringToChatType(const char *string, SLASH_COMMAND_ID &slashCmd) {
  static const struct {
    const char *name;
    int         type;
  } chatTypes[10] = {
      {    "SAY",  0},
      {  "PARTY",  1},
      {   "RAID",  2},
      {  "GUILD",  3},
      {"OFFICER",  4},
      {"WHISPER",  5},
      {   "YELL",  6},
      {"CHANNEL", 13},
      {    "AFK", 19},
      {    "DND", 20}
  };
  for (unsigned int i = 0; i < 10; ++i) {
    if (!SStrCmpI(string, chatTypes[i].name, 0x7FFFFFFF)) {
      slashCmd = static_cast<SLASH_COMMAND_ID>(chatTypes[i].type);
      return 1;
    }
  }
  return 0;
}

static int StringToLanguage(const char *string, unsigned int &language) {
  int numEntries = g_languagesDB.GetNumRecords();
  for (int i = 0; i < numEntries; ++i) {
    const LanguagesRec *rec = g_languagesDB.GetRecordByIndex(i);
    if (rec && rec->m_name_lang[CURRENT_LANGUAGE] && !SStrCmpI(string, rec->m_name_lang[CURRENT_LANGUAGE], 0x7FFFFFFF)) {
      language = rec->m_ID;
      return 1;
    }
  }
  return 0;
}

static int Script_SendChatMessage(lua_State *L) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player) {
    return 0;
  }
  if (!lua_isstring(L, 1)) {
    return luaL_error(L, "Usage: SendChatMessage(message [, chatType, language, channel])");
  }
  const char      *text = lua_tostring(L, 1);
  SLASH_COMMAND_ID type = SLASH_COMMAND_SAY;
  if (lua_isstring(L, 2) && !StringToChatType(lua_tostring(L, 2), type)) {
    return luaL_error(L, "Unknown chat type");
  }
  if ((!text || !*text) && type != static_cast<SLASH_COMMAND_ID>(19) && type != static_cast<SLASH_COMMAND_ID>(20)) {
    return 0;
  }

  unsigned int       language = 0;
  const ChrRacesRec *race = g_chrRacesDB.GetRecord(player->GetUnitData()->race);
  if (race) {
    language = race->m_BaseLanguage;
  }
  if (lua_isstring(L, 3) && !StringToLanguage(lua_tostring(L, 3), language)) {
    return luaL_error(L, "Unknown language");
  }

  const char *target = lua_isstring(L, 4) ? lua_tostring(L, 4) : 0;
  if (type == static_cast<SLASH_COMMAND_ID>(13)) {
    if (!target || !*target) {
      return luaL_error(L, "Channel send missing channel name");
    }
    target = CGChat::GetChannelString(target);
    if (!target) {
      return luaL_error(L, "Channel not found");
    }
  } else if (type == static_cast<SLASH_COMMAND_ID>(5) && (!target || !*target)) {
    return luaL_error(L, "Whisper message missing target");
  }

  CDataStore message;
  message.Put(static_cast<unsigned int>(CMSG_MESSAGECHAT));
  message.Put(static_cast<unsigned int>(type));
  message.Put(language);
  if (type == static_cast<SLASH_COMMAND_ID>(5) || type == static_cast<SLASH_COMMAND_ID>(13)) {
    message.PutString(target);
  }
  message.PutString(text);
  message.Finalize();
  ClientServices_Send(&message);
  return 0;
}

static int Script_GetNumLanguages(lua_State *L) {
  CGPlayer_C  *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  unsigned int count = 0;
  if (player) {
    for (int i = 0; i < g_languagesDB.GetNumRecords(); ++i) {
      unsigned int        skill;
      const LanguagesRec *rec = g_languagesDB.GetRecordByIndex(i);
      if (rec && player->GetLanguageSkill(rec->m_ID, skill)) {
        ++count;
      }
    }
  }
  lua_pushnumber(L, static_cast<double>(count));
  return 1;
}

static int Script_GetLanguageByIndex(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetLanguageByIndex(index)");
  }
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player) {
    return 0;
  }
  unsigned int index = static_cast<unsigned int>(lua_tonumber(L, 1));
  unsigned int count = 0;
  for (int i = 0; i < g_languagesDB.GetNumRecords(); ++i) {
    unsigned int        skill;
    const LanguagesRec *rec = g_languagesDB.GetRecordByIndex(i);
    if (rec && player->GetLanguageSkill(rec->m_ID, skill) && ++count == index) {
      lua_pushstring(L, rec->m_name_lang[CURRENT_LANGUAGE]);
      return 1;
    }
  }
  return 0;
}

static int Script_GetDefaultLanguage(lua_State *L) {
  CGPlayer_C         *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  const ChrRacesRec  *race = player ? g_chrRacesDB.GetRecord(player->GetUnitData()->race) : 0;
  const LanguagesRec *language = race ? g_languagesDB.GetRecord(race->m_BaseLanguage) : 0;
  if (!language) {
    return 0;
  }
  lua_pushstring(L, language->m_name_lang[CURRENT_LANGUAGE]);
  return 1;
}

static int Script_DoEmote(lua_State *L) {
  if (!lua_isstring(L, 1)) {
    return luaL_error(L, "Usage: DoEmote(emote [, unit])");
  }
  const char          *name = lua_tostring(L, 1);
  const EmotesTextRec *rec = 0;
  for (int i = g_emotesTextDB.GetNumRecords(); i;) {
    const EmotesTextRec *candidate = g_emotesTextDB.GetRecordByIndex(--i);
    if (candidate && !SStrCmpI(name, candidate->m_name, 0x7FFFFFFF)) {
      rec = candidate;
      break;
    }
  }
  if (!rec) {
    return 0;
  }
  const char      *unit = lua_isstring(L, 2) ? lua_tostring(L, 2) : "target";
  unsigned __int64 target = Script_GetGUIDFromName(unit);
  CGPlayer_C      *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    player->SendTextEmote(const_cast<EmotesTextRec *>(rec), target);
  }
  return 0;
}

static int Script_ChatFrameLog(lua_State *L) {
  if (lua_isnumber(L, 1)) {
    s_loggingEnabled = static_cast<int>(lua_tonumber(L, 1));
    if (s_logHandle) {
      SLogClose(s_logHandle);
      s_logHandle = 0;
    }
    if (s_loggingEnabled) {
      if (!SLogCreate("WoWChatLog.txt", 0, &s_logHandle)) {
        ConsoleWrite("Error creating chat log", DEFAULT_COLOR);
      } else {
        ConsoleWrite("Chat logging enabled", DEFAULT_COLOR);
      }
    } else {
      ConsoleWrite("Chat logging disabled", DEFAULT_COLOR);
    }
  }
  if (s_loggingEnabled) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static void ChannelPlayerCommand(lua_State *L, int messageCode, const char *funcName) {
  char buffer[512];
  SStrPrintf(buffer, sizeof(buffer), "Usage: %s(channel, player)", funcName);
  if (!lua_isstring(L, 1) || !lua_isstring(L, 2)) {
    luaL_error(L, buffer);
    return;
  }
  const char *channel = CGChat::GetChannelString(lua_tostring(L, 1));
  if (!channel) {
    return;
  }
  CDataStore msg;
  msg.Put(static_cast<unsigned int>(messageCode));
  msg.PutString(channel);
  msg.PutString(lua_tostring(L, 2));
  msg.Finalize();
  ClientServices_Send(&msg);
}

static void ChannelCommand(lua_State *L, int messageCode, const char *funcname) {
  char buffer[512];
  SStrPrintf(buffer, sizeof(buffer), "Usage: %s(channel)", funcname);
  if (!lua_isstring(L, 1)) {
    luaL_error(L, buffer);
    return;
  }
  const char *channel = CGChat::GetChannelString(lua_tostring(L, 1));
  if (!channel) {
    return;
  }
  CDataStore msg;
  msg.Put(static_cast<unsigned int>(messageCode));
  msg.PutString(channel);
  msg.Finalize();
  ClientServices_Send(&msg);
}

static int Script_JoinChannelByName(lua_State *L) {
  if (!lua_isstring(L, 1)) {
    return luaL_error(L, "Usage: JoinChannelByName(channel [, password])");
  }
  const char *password = lua_isstring(L, 2) ? lua_tostring(L, 2) : "";
  CDataStore  msg;
  msg.Put(static_cast<unsigned int>(CMSG_JOIN_CHANNEL));
  msg.PutString(lua_tostring(L, 1));
  msg.PutString(password);
  msg.Finalize();
  ClientServices_Send(&msg);
  return 0;
}

static int Script_LeaveChannelByName(lua_State *L) {
  ChannelCommand(L, CMSG_LEAVE_CHANNEL, "LeaveChannelByName");
  return 0;
}

static int Script_ListChannelByName(lua_State *L) {
  ChannelCommand(L, CMSG_CHANNEL_LIST, "ListChannelByName");
  return 0;
}

static int Script_ListChannels(lua_State *L) {
  char buffer[138];
  char line[256] = "";
  for (unsigned int i = 0; i < s_channels.Count(); ++i) {
    if (!s_channels[i].localID) {
      continue;
    }
    SStrPrintf(buffer, sizeof(buffer), "[%d. %s] ", s_channels[i].localID, s_channels[i].name);
    if (SStrLen(line) + SStrLen(buffer) >= sizeof(line)) {
      CGChat::AddChatMessage(line, static_cast<SLASH_COMMAND_ID>(16), 0, 0, 0, 0, 0);
      line[0] = 0;
    }
    SStrPack(line, buffer, sizeof(line));
  }
  CGChat::AddChatMessage(line, static_cast<SLASH_COMMAND_ID>(16), 0, 0, 0, 0, 0);
  return 0;
}

static int Script_SetChannelPassword(lua_State *L) {
  if (!lua_isstring(L, 1) || !lua_isstring(L, 2)) {
    return luaL_error(L, "Usage: SetChannelPassword(channel, password)");
  }
  const char *channel = CGChat::GetChannelString(lua_tostring(L, 1));
  if (!channel) {
    return 0;
  }
  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_CHANNEL_PASSWORD));
  msg.PutString(channel);
  msg.PutString(lua_tostring(L, 2));
  msg.Finalize();
  ClientServices_Send(&msg);
  return 0;
}

static int Script_SetChannelOwner(lua_State *L) {
  ChannelPlayerCommand(L, CMSG_CHANNEL_SET_OWNER, "SetChannelOwner");
  return 0;
}

static int Script_DisplayChannelOwner(lua_State *L) {
  ChannelCommand(L, CMSG_CHANNEL_OWNER, "DisplayChannelOwner");
  return 0;
}

static int Script_GetChannelName(lua_State *L) {
  int         channel;
  const char *name;
  if (lua_isnumber(L, 1)) {
    channel = static_cast<int>(lua_tonumber(L, 1));
    name = CGChat::GetChannelName(channel);
    if (!name) {
      channel = 0;
    }
  } else {
    if (!lua_isstring(L, 1)) {
      return luaL_error(L, "Usage: GetChannelName(channel)");
    }
    name = lua_tostring(L, 1);
    channel = CGChat::GetChannelID(name);
    if (channel) {
      name = "";
    }
  }
  lua_pushnumber(L, static_cast<double>(channel));
  lua_pushstring(L, name);
  return 2;
}

static int Script_ChannelModerator(lua_State *L) {
  ChannelPlayerCommand(L, CMSG_CHANNEL_MODERATOR, "ChannelModerator");
  return 0;
}

static int Script_ChannelUnmoderator(lua_State *L) {
  ChannelPlayerCommand(L, CMSG_CHANNEL_UNMODERATOR, "ChannelUnmoderator");
  return 0;
}

static int Script_ChannelMute(lua_State *L) {
  ChannelPlayerCommand(L, CMSG_CHANNEL_MUTE, "ChannelMute");
  return 0;
}

static int Script_ChannelUnmute(lua_State *L) {
  ChannelPlayerCommand(L, CMSG_CHANNEL_UNMUTE, "ChannelUnmute");
  return 0;
}

static int Script_ChannelInvite(lua_State *L) {
  ChannelPlayerCommand(L, CMSG_CHANNEL_INVITE, "ChannelInvite");
  return 0;
}

static int Script_ChannelKick(lua_State *L) {
  ChannelPlayerCommand(L, CMSG_CHANNEL_KICK, "ChannelKick");
  return 0;
}

static int Script_ChannelBan(lua_State *L) {
  ChannelPlayerCommand(L, CMSG_CHANNEL_BAN, "ChannelBan");
  return 0;
}

static int Script_ChannelUnban(lua_State *L) {
  ChannelPlayerCommand(L, CMSG_CHANNEL_UNBAN, "ChannelUnban");
  return 0;
}

static int Script_ChannelToggleAnnouncements(lua_State *L) {
  ChannelCommand(L, CMSG_CHANNEL_ANNOUNCEMENTS, "ChannelToggleAnnouncements");
  return 0;
}

static int Script_ChannelModerate(lua_State *L) {
  ChannelCommand(L, CMSG_CHANNEL_MODERATE, "ChannelModerate");
  return 0;
}

static FrameScript_Method s_ScriptFunctions[24] = {
    {           "SendChatMessage",            Script_SendChatMessage},
    {            "GetNumLaguages",            Script_GetNumLanguages},
    {        "GetLanguageByIndex",         Script_GetLanguageByIndex},
    {        "GetDefaultLanguage",         Script_GetDefaultLanguage},
    {                   "DoEmote",                    Script_DoEmote},
    {              "ChatFrameLog",               Script_ChatFrameLog},
    {         "JoinChannelByName",          Script_JoinChannelByName},
    {        "LeaveChannelByName",         Script_LeaveChannelByName},
    {         "ListChannelByName",          Script_ListChannelByName},
    {              "ListChannels",               Script_ListChannels},
    {        "SetChannelPassword",         Script_SetChannelPassword},
    {           "SetChannelOwner",            Script_SetChannelOwner},
    {       "DisplayChannelOwner",        Script_DisplayChannelOwner},
    {            "GetChannelName",             Script_GetChannelName},
    {          "ChannelModerator",           Script_ChannelModerator},
    {        "ChannelUnmoderator",         Script_ChannelUnmoderator},
    {               "ChannelMute",                Script_ChannelMute},
    {             "ChannelUnmute",              Script_ChannelUnmute},
    {             "ChannelInvite",              Script_ChannelInvite},
    {               "ChannelKick",                Script_ChannelKick},
    {                "ChannelBan",                 Script_ChannelBan},
    {              "ChannelUnban",               Script_ChannelUnban},
    {"ChannelToggleAnnouncements", Script_ChannelToggleAnnouncements},
    {           "ChannelModerate",            Script_ChannelModerate}
};

void ChatRegisterScriptFunctions() {
  for (unsigned int i = 0; i < 24; ++i) {
    FrameScript_RegisterFunction(s_ScriptFunctions[i].name, s_ScriptFunctions[i].method);
  }
}

void ChatUnregisterScriptFunctions() {
  for (unsigned int i = 0; i < 24; ++i) {
    FrameScript_UnregisterFunction(s_ScriptFunctions[i].name);
  }
}
