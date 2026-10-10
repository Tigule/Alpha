#include <Base/Base.h>
#include <Frame/CSimpleTop.h>
#include <Frame/CSimpleModel.h>
#include <WowConst.h>
#include <MapDefs.h>
#include <WorldClient/World.h>
#include "Net/NetClient/NetClient.h"
#include "Object/ObjectClient/Unit_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "SoundInterface/SoundInterface.h"
#include "UIUtil/InputControl.h"
#include "WorldFrame.h"
#include "GameUI.h"

#include "ChatFrame.h"
#include "GameUI.h"

#include <Base/CDataStore.h>
#include <Console/ConsoleClient.h>
#include <Console/ConsoleVar.h>
#include <FrameScript/FrameScript.h>
#include <storm.h>
#include <stpl.h>

#include "DB/DBClient/DBCacheInstances.h"
#include "DB/DBClient/AutoCode/ChrRacesRec.h"
#include "DB/DBClient/AutoCode/EmotesTextDataRec.h"
#include "DB/DBClient/AutoCode/EmotesTextRec.h"
#include "DB/DBClient/AutoCode/LanguageWordsRec.h"
#include "DB/DBClient/AutoCode/LanguagesRec.h"
#include "Object/ObjectClient/Player_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "Ui/Tutorial.h"
#include "WowSvcs/WowSvcsClient/ClientServices.h"

#include <Net/NetClient/NetClient.h>
#include <regex/regex.h>

#include <ctype.h>
#include <lauxlib.h>
#include <lua.h>
#include <string.h>

NODEDECL(PENDINGUSERLIST) {
  DWORDLONG guid;
  BYTE      flags;
};

struct ChatChannel {
  ~ChatChannel() {
    pendingNames.Clear();
  }
  int  localID;
  char name[128];
  LISTDECL(PENDINGUSERLIST, pendingNames);
  BYTE channelFlags;
};


NODEDECL(PENDINGCHAT) {
  PENDINGCHAT() : text(0) {
  }

  PENDINGCHAT(const PENDINGCHAT &);

  ~PENDINGCHAT() {
    FREEIFUSED(text);
  }

  int       slashCmd;
  DWORDLONG guid;
  char     *text;
  UINT      language;
  int       waitingForUI;
  int       parse;
  char      channel[128];
  DWORDLONG guid2;
  char      specialFlag[5];
};

NODEDECL(PENDINGTEXTEMOTE) {
  PENDINGTEXTEMOTE() : target(0) {
  }

  PENDINGTEXTEMOTE(const PENDINGTEXTEMOTE &);

  ~PENDINGTEXTEMOTE() {
    FREEIFUSED(target);
  }

  DWORDLONG sender;
  int       textEmoteID;
  char     *target;
  int       waitingForUI;
};

class HASHKEY_LANGUAGE {
 private:
  UINT m_languageID;
  UINT m_length;

 public:
  HASHKEY_LANGUAGE(UINT languageID = 0, UINT length = 0);

  HASHKEY_LANGUAGE(const HASHKEY_LANGUAGE &key) : m_languageID(key.m_languageID), m_length(key.m_length) {
  }

  BYTE operator==(const HASHKEY_LANGUAGE &key) const {
    return m_languageID == key.m_languageID && m_length == key.m_length;
  }

  HASHKEY_LANGUAGE &operator=(const HASHKEY_LANGUAGE &key) {
    m_languageID = key.m_languageID;
    m_length = key.m_length;
    return *this;
  }
};

inline HASHKEY_LANGUAGE::HASHKEY_LANGUAGE(UINT languageID, UINT length) : m_languageID(languageID), m_length(length) {
}

struct WORDLIST : public TSHashObject<WORDLIST, HASHKEY_LANGUAGE> {
  TSGrowableArray<const LanguageWordsRec *> m_words;
};

static const char s_msgGetHeaderTokens[30][32] = {"CHAT_SAY_GET",    "CHAT_PARTY_GET",          "CHAT_GUILD_GET", "CHAT_OFFICER_GET",
                                                  "CHAT_YELL_GET",   "CHAT_WHISPER_GET",        "CHAT_WHISPER_INFORM_GET",
                                                  "CHAT_EMOTE_GET",  "CHAT_SYSTEM_GET",         "CHAT_SAY_GET",   "CHAT_YELL_GET"};
static const UINT s_events[30] = {217, 218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231,
                                  232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 338, 339, 340, 341, 342};
static const char s_replacementChars[9] = "!@#$%^&*";
static UINT       s_replacementIndex;
static TSGrowableArray<ChatChannel> s_channels;
static LISTDECL(PENDINGCHAT, s_pendingChat);
static LISTDECL(PENDINGTEXTEMOTE, s_pendingTextEmote);
TSHashTable<WORDLIST, HASHKEY_LANGUAGE> s_wordList;
static int                                     s_loggingEnabled;
static HSLOG                                   s_logHandle;

DWORDLONG Script_GetGUIDFromName(LPCSTR name);
extern TSFixedArray<regex_t> g_profanityTokens;

int CGChat::m_paused = 1;
int CGChat::m_filterChat = 1;

void CGChat::InitializeGame() {
  UINT numEntries = g_languageWordsDB.GetNumRecords();
  for (UINT i = 0; i < numEntries; ++i) {
    const LanguageWordsRec *wordRec = g_languageWordsDB.GetRecordByIndex(i);
    if (wordRec && wordRec->m_word && *wordRec->m_word) {
      UINT      len = SStrLen(wordRec->m_word);
      WORDLIST *wordList = s_wordList.Ptr(wordRec->m_languageID, HASHKEY_LANGUAGE(len));
      if (!wordList) {
        wordList = s_wordList.New(wordRec->m_languageID, HASHKEY_LANGUAGE(len), 0, 0);
      }
      *wordList->m_words.New() = wordRec;
    }
  }

  CVar *cvar = CVar::Lookup("profanityFilter");
  if (cvar && !cvar->GetInt()) {
    m_filterChat = 0;
  }
}

void CGChat::ShutdownGame() {
  if (s_logHandle) {
    SLogClose(s_logHandle);
    s_logHandle = 0;
  }
  s_wordList.Clear();
  s_pendingChat.Clear();
  s_channels.SetCount(0);
}

void CGChat::EnterWorld() {
  m_paused = 0;
  GetPendingChatMessages();
}

void CGChat::LeaveWorld() {
  m_paused = 1;
}

static void SendChatEvent(LPCSTR text, SLASH_COMMAND_ID type, LPCSTR player, UINT language, LPCSTR channel, LPCSTR player2, LPCSTR specialFlag) {
  if (type < NUM_SLASH_CMDS) {
    LPCSTR              name = "";
    const LanguagesRec *rec = g_languagesDB.GetRecord(language);
    if (rec) {
      name = rec->m_name_lang[CURRENT_LANGUAGE];
    }
    if (type == SLASH_CMD_WHISPER) {
      CGTutorial::TriggerTutorial(TUTORIAL_TELLS);
    }
    FrameScript_SignalEvent(
        s_events[type],
        "%s%s%s%s%s%s",
        text ? text : "",
        player ? player : "",
        name,
        channel ? channel : "",
        player2 ? player2 : "",
        specialFlag ? specialFlag : ""
    );
  }
}

void CGChat::AddChatMessage(LPCSTR text, SLASH_COMMAND_ID type, LPCSTR player, UINT language, LPCSTR channel, LPCSTR player2, LPCSTR specialFlag) {
  char translated[256];
  char msg[352];

  FATALASSERT(type < NUM_SLASH_CMDS);
  if (type == SLASH_CMD_EMOTE || type == SLASH_CMD_MONSTER_EMOTE || type == SLASH_CMD_SYSTEM) {
    language = 0;
  }
  if (type == SLASH_CMD_EMOTE && !*text) {
    text = FrameScript_GetText("CHAT_EMOTE_UNKNOWN", -1, GENDER_NOT_APPLICABLE);
  }

  CGPlayer_C *playerPtr = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (language && player) {
    UINT skill;
    playerPtr->GetLanguageSkill(language, skill);
    TranslateMessage(language, skill, text, translated, sizeof(translated), 0);
  } else {
    SStrCopy(translated, text, sizeof(translated));
  }

  char channelWithNumber[138] = "";
  if (channel && *channel) {
    if (GetChannelID(channel)) {
      SStrPrintf(channelWithNumber, sizeof(channelWithNumber), "%d. %s", GetChannelID(channel), channel);
    } else {
      SStrCopy(channelWithNumber, channel, sizeof(channelWithNumber));
    }
  }

  if (m_filterChat) {
    UINT count = g_profanityTokens.Count();
    for (UINT i = 0; i < count; ++i) {
      char      *str = translated;
      regmatch_t match;
      while (!regexec(&g_profanityTokens[i], str, 1, &match, 0) && match.rm_so >= 0) {
        str += match.rm_so;
        for (int len = match.rm_eo - match.rm_so; len > 0; --len) {
          *str = s_replacementChars[s_replacementIndex];
          if (++s_replacementIndex >= sizeof(s_replacementChars) - 1) {
            s_replacementIndex = 0;
          }
          ++str;
        }
      }
    }
  }

  SendChatEvent(translated, type, player, language, channelWithNumber, player2, specialFlag);

  if (s_loggingEnabled) {
    char token[32];
    LPCSTR format = FrameScript_GetText(s_msgGetHeaderTokens[type], -1, GENDER_NOT_APPLICABLE);
    SStrCopy(token, format, sizeof(token));

    char header[112];
    if (type != SLASH_CMD_SYSTEM) {
      SStrPrintf(header, sizeof(header), token, player ? player : "");
    } else {
      SStrCopy(header, token, sizeof(header));
    }

    if (language && playerPtr && language != playerPtr->GetDefaultLanguage()) {
      char langBuf[32];
      SStrPrintf(langBuf, sizeof(langBuf), "[%s] ", g_languagesDB.GetRecord(language)->m_name_lang[CURRENT_LANGUAGE]);
      SStrPack(header, langBuf, sizeof(header));
    }

    SStrPrintf(msg, sizeof(msg), "%s%s", header, translated);
    SLogWrite(s_logHandle, msg);
  }
}

void CGChat::AddTextEmoteMessage(const DWORDLONG &senderGUID, int textEmoteID, LPCSTR target) {
  VALIDATEBEGIN;
  VALIDATE(target);
  VALIDATEENDVOID;

  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player) {
    return;
  }
  const NameCache *nc = g_nameDBCache.GetRecord(senderGUID, 0, 0, 0);
  if (!nc) {
    return;
  }
  const EmotesTextRec *rec = g_emotesTextDB.GetRecord(textEmoteID);
  if (!rec) {
    return;
  }

  int senderName = 1;
  int targetName = 1;
  int index = 0;
  if (senderGUID == ClntObjMgrGetActivePlayer()) {
    index = 2;
    senderName = 0;
  } else if (!SStrCmp(target, player->GetUnitName(), INT_MAX)) {
    index = 1;
    targetName = 0;
  }
  if (!*target) {
    index |= 4;
  }

  const EmotesTextDataRec *data = g_emotesTextDataDB.GetRecord(rec->m_emoteText[(nc->m_sex == 1 ? 8 : 0) | index]);
  if (!data || !*data->m_text_lang[CURRENT_LANGUAGE]) {
    data = g_emotesTextDataDB.GetRecord(rec->m_emoteText[index]);
  }
  if (!data || !*data->m_text_lang[CURRENT_LANGUAGE]) {
    index |= 4;
    data = g_emotesTextDataDB.GetRecord(rec->m_emoteText[(nc->m_sex == 1 ? 8 : 0) | index]);
  }
  if (!data || !*data->m_text_lang[CURRENT_LANGUAGE]) {
    data = g_emotesTextDataDB.GetRecord(rec->m_emoteText[index]);
  }
  if (!data || !*data->m_text_lang[CURRENT_LANGUAGE]) {
    return;
  }

  char buffer[256];
  if (senderName) {
    if (targetName) {
      SStrPrintf(buffer, sizeof(buffer), data->m_text_lang[CURRENT_LANGUAGE], nc->m_name, target);
    } else {
      SStrPrintf(buffer, sizeof(buffer), data->m_text_lang[CURRENT_LANGUAGE], nc->m_name);
    }
  } else if (targetName) {
    SStrPrintf(buffer, sizeof(buffer), data->m_text_lang[CURRENT_LANGUAGE], target);
  } else {
    SStrPrintf(buffer, sizeof(buffer), data->m_text_lang[CURRENT_LANGUAGE]);
  }
  SendChatEvent(buffer, SLASH_CMD_TEXT_EMOTE, nc->m_name, 0, 0, 0, 0);
}

static void CopyWordCase(char *buffer, LPCSTR token, LPCSTR word, UINT maxlen) {
  FATALASSERT(maxlen);
  --maxlen;
  while (*buffer && maxlen) {
    ++buffer;
    --maxlen;
  }
  while (*word && *token && maxlen) {
    if (isupper(*token)) {
      *buffer++ = toupper(*word);
    } else {
      *buffer++ = tolower(*word);
    }
    ++word;
    ++token;
    --maxlen;
  }
  *buffer = 0;
}

void CGChat::TranslateMessage(UINT language, UINT skill, LPCSTR text, char *buffer, UINT size, int passXML) {
  char token[256];

  FATALASSERT(text);
  FATALASSERT(buffer);
  if (!language) {
    SStrCopy(buffer, text, size);
    return;
  }
  if (!ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__)) {
    SStrCopy(buffer, text, size);
    return;
  }
  if (skill >= 300) {
    SStrCopy(buffer, text, size);
    return;
  }

  *buffer = 0;
  for (;;) {
    while (*text && !isalpha(*text)) {
      if (*text == '<' && passXML) {
        do {
          token[0] = *text;
          token[1] = 0;
          SStrPack(buffer, token, size);
          if (*text++ == '>') {
            break;
          }
        } while (*text);
      } else {
        token[0] = *text++;
        token[1] = 0;
        SStrPack(buffer, token, size);
      }
    }
    if (!*text) {
      break;
    }

    char *space = (char *)text;
    while (*space && isalpha(*space)) {
      ++space;
    }

    UINT len = min((UINT)(space - text), 256);
    SStrCopy(token, text, len + 1);
    UINT hash = SStrHash(token, 0, 0);
    if (hash % 300 >= skill) {
      len = min(len, 18);
      WORDLIST *wordList = s_wordList.Ptr(language, HASHKEY_LANGUAGE(len));
      while (!wordList && len != 1) {
        --len;
        wordList = s_wordList.Ptr(language, HASHKEY_LANGUAGE(len));
      }
      if (wordList) {
        UINT numWords = wordList->m_words.Count();
        FATALASSERT(numWords);
        const LanguageWordsRec *wordRec = wordList->m_words[hash % numWords];
        CopyWordCase(buffer, token, wordRec->m_word, size);
      } else {
        SStrPack(buffer, token, size);
      }
    } else {
      SStrPack(buffer, token, size);
    }
    text = space;
  }
}

void CGChat::UpdateLanguages() {
  FrameScript_SignalEvent(242);
}

void CGChat::AddChannel(LPCSTR name) {
  for (UINT i = 0; i < s_channels.Count(); ++i) {
    if (!s_channels[i].localID) {
      s_channels[i].localID = i + 1;
      SStrCopy(s_channels[i].name, name, sizeof(s_channels[i].name));
      return;
    }
  }

  int index = s_channels.Count();
  s_channels.SetCount(index + 1);
  s_channels[index].localID = index + 1;
  SStrCopy(s_channels[index].name, name, sizeof(s_channels[index].name));
}

void CGChat::RemoveChannel(LPCSTR name) {
  for (UINT i = 0; i < s_channels.Count(); ++i) {
    if (!SStrCmpI(s_channels[i].name, name, INT_MAX)) {
      s_channels[i].localID = 0;
      s_channels[i].name[0] = 0;
      return;
    }
  }
}

int CGChat::GetChannelID(LPCSTR name) {
  for (UINT i = 0; i < s_channels.Count(); ++i) {
    if (!SStrCmpI(s_channels[i].name, name, INT_MAX)) {
      return s_channels[i].localID;
    }
  }
  return 0;
}

LPCSTR CGChat::GetChannelName(int localID) {
  if (localID >= 1 && localID <= (int)s_channels.Count() && s_channels[localID - 1].localID == localID) {
    return s_channels[localID - 1].name;
  }
  return 0;
}

ChatChannel *CGChat::GetChannel(LPCSTR name) {
  for (UINT index = 0; index < s_channels.Count(); ++index) {
    if (!SStrCmpI(s_channels[index].name, name, 0x7FFFFFFF)) {
      return &s_channels[index];
    }
  }
  return 0;
}

void CGChat::ChannelNotify(CDataStore *msg) {
  char             namebuffer[256];
  char             buffer[512];
  char             channel[128];
  LPCSTR           name[2];
  DWORDLONG        guid2;
  BYTE             oldFlags;
  BYTE             newFlags;
  DWORDLONG        guid;
  BYTE             type;
  SLASH_COMMAND_ID eventType;

  msg->Get(type);
  msg->GetString(channel, sizeof(channel));
  guid = 0;
  guid2 = 0;
  name[0] = 0;
  name[1] = 0;
  buffer[0] = 0;
  eventType = SLASH_CMD_SYSTEM;

  switch (type) {
    case 0:
      eventType = SLASH_CMD_JOIN_CHANNEL;
      msg->Get(guid);
      if (!GetChannelID(channel))
        return;
      break;
    case 1:
      eventType = SLASH_CMD_LEAVE_CHANNEL;
      msg->Get(guid);
      if (!GetChannelID(channel))
        return;
      break;
    case 2:
      if (!msg->IsRead())
        msg->GetString(namebuffer, 0x7FFFFFFF);
      else
        namebuffer[0] = 0;
      AddChannel(channel);
      eventType = SLASH_CMD_CHANNEL_NOTICE;
      SStrCopy(buffer, "YOU_JOINED", sizeof(buffer));
      break;
    case 3:
      eventType = SLASH_CMD_CHANNEL_NOTICE;
      SStrCopy(buffer, "YOU_LEFT", sizeof(buffer));
      break;
    case 4:
      eventType = SLASH_CMD_CHANNEL_NOTICE;
      SStrCopy(buffer, "WRONG_PASSWORD", sizeof(buffer));
      break;
    case 5:
      eventType = SLASH_CMD_CHANNEL_NOTICE;
      SStrCopy(buffer, "NOT_MEMBER", sizeof(buffer));
      break;
    case 6:
      eventType = SLASH_CMD_CHANNEL_NOTICE;
      SStrCopy(buffer, "NOT_MODERATOR", sizeof(buffer));
      break;
    case 7:
      msg->Get(guid);
      eventType = SLASH_CMD_CHANNEL_NOTICE_USER;
      SStrCopy(buffer, "PASSWORD_CHANGED", sizeof(buffer));
      break;
    case 8:
      msg->Get(guid);
      eventType = SLASH_CMD_CHANNEL_NOTICE_USER;
      SStrCopy(buffer, "OWNER_CHANGED", sizeof(buffer));
      break;
    case 9:
      msg->GetString(namebuffer, sizeof(namebuffer));
      name[0] = namebuffer;
      eventType = SLASH_CMD_CHANNEL_NOTICE_USER;
      SStrCopy(buffer, "PLAYER_NOT_FOUND", sizeof(buffer));
      break;
    case 22:
      msg->GetString(namebuffer, sizeof(namebuffer));
      name[0] = namebuffer;
      eventType = SLASH_CMD_CHANNEL_NOTICE_USER;
      SStrCopy(buffer, "PLAYER_NOT_BANNED", sizeof(buffer));
      break;
    case 11:
      msg->GetString(namebuffer, sizeof(namebuffer));
      name[0] = namebuffer;
      eventType = SLASH_CMD_CHANNEL_NOTICE_USER;
      SStrCopy(buffer, "CHANNEL_OWNER", sizeof(buffer));
      break;
    case 10:
      eventType = SLASH_CMD_CHANNEL_NOTICE;
      SStrCopy(buffer, "NOT_OWNER", sizeof(buffer));
      break;
    case 12:
      msg->Get(guid);
      msg->Get(oldFlags);
      msg->Get(newFlags);
      HandleFlagsChanged(guid, oldFlags, newFlags, channel);
      return;
    case 13:
      msg->Get(guid);
      eventType = SLASH_CMD_CHANNEL_NOTICE_USER;
      SStrCopy(buffer, "ANNOUNCEMENTS_ON", sizeof(buffer));
      break;
    case 14:
      msg->Get(guid);
      eventType = SLASH_CMD_CHANNEL_NOTICE_USER;
      SStrCopy(buffer, "ANNOUNCEMENTS_OFF", sizeof(buffer));
      break;
    case 15:
      msg->Get(guid);
      eventType = SLASH_CMD_CHANNEL_NOTICE_USER;
      SStrCopy(buffer, "MODERATION_ON", sizeof(buffer));
      break;
    case 16:
      msg->Get(guid);
      eventType = SLASH_CMD_CHANNEL_NOTICE_USER;
      SStrCopy(buffer, "MODERATION_OFF", sizeof(buffer));
      break;
    case 17:
      eventType = SLASH_CMD_CHANNEL_NOTICE;
      SStrCopy(buffer, "MUTED", sizeof(buffer));
      break;
    case 18:
      msg->Get(guid);
      msg->Get(guid2);
      eventType = SLASH_CMD_CHANNEL_NOTICE_USER;
      SStrCopy(buffer, "PLAYER_KICKED", sizeof(buffer));
      break;
    case 20:
      msg->Get(guid);
      msg->Get(guid2);
      eventType = SLASH_CMD_CHANNEL_NOTICE_USER;
      SStrCopy(buffer, "PLAYER_BANNED", sizeof(buffer));
      break;
    case 19:
      eventType = SLASH_CMD_CHANNEL_NOTICE;
      SStrCopy(buffer, "BANNED", sizeof(buffer));
      break;
    case 21:
      msg->Get(guid);
      msg->Get(guid2);
      eventType = SLASH_CMD_CHANNEL_NOTICE_USER;
      SStrCopy(buffer, "PLAYER_UNBANNED", sizeof(buffer));
      break;
    case 23:
      msg->Get(guid);
      eventType = SLASH_CMD_CHANNEL_NOTICE_USER;
      SStrCopy(buffer, "PLAYER_ALREADY_MEMBER", sizeof(buffer));
      break;
    case 24:
      msg->Get(guid);
      eventType = SLASH_CMD_CHANNEL_NOTICE_USER;
      SStrCopy(buffer, "INVITE", sizeof(buffer));
      break;
    case 25:
      eventType = SLASH_CMD_CHANNEL_NOTICE_USER;
      SStrCopy(buffer, "INVITE_WRONG_FACTION", sizeof(buffer));
      break;
    case 26:
      eventType = SLASH_CMD_CHANNEL_NOTICE_USER;
      SStrCopy(buffer, "WRONG_FACTION", sizeof(buffer));
      break;
    default:
      break;
  }

  if (guid) {
    const NameCache *nc[2];
    nc[1] = 0;
    nc[0] = g_nameDBCache.GetRecord(guid, guid, NameQueryCallback, 0);
    if (guid2) {
      nc[1] = g_nameDBCache.GetRecord(guid2, guid2, NameQueryCallback, 0);
    }
    if (!nc[0] || (guid2 && !nc[1])) {
      QueueChatText(eventType, guid, buffer, 0, 0, 1, channel, guid2, "");
      return;
    }
    name[0] = nc[0]->m_name;
    if (nc[1]) {
      name[1] = nc[1]->m_name;
    }
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

  msg->Get(channel->channelFlags);
  msg->Get(count);
  pending = 0;
  for (i = 0; i < count; ++i) {
    PENDINGUSERLIST *user = channel->pendingNames.NewNode(LIST_TAIL, 0, 0);
    msg->Get(user->guid);
    msg->Get(user->flags);
    if (!g_nameDBCache.GetRecord(user->guid, user->guid, NameQueryCallback, 0)) {
      ++pending;
    }
  }
  if (!pending)
    DisplayPendingUserList(channel);
}

void CGChat::DisplayPendingUserList(ChatChannel *channel) {
  char line[256];
  char buffer[58];
  int  namesThisLine;

  {
    ITERATELIST(PENDINGUSERLIST, channel->pendingNames, user) {
      if (!g_nameDBCache.GetRecord(user->guid, 0, 0, 0)) {
        return;
      }
    }
  }

  line[0] = 0;
  namesThisLine = 0;
  ITERATELIST(PENDINGUSERLIST, channel->pendingNames, user) {
    const NameCache *nc = g_nameDBCache.GetRecord(user->guid, 0, 0, 0);
    if (nc) {
      char marker = ' ';
      if (user->flags & 1) {
        marker = '*';
      } else if (user->flags & 2) {
        marker = '@';
      } else if ((channel->channelFlags & 2) && !(user->flags & 4)) {
        marker = '#';
      }
      SStrPrintf(buffer, sizeof(buffer), "%c%s", marker, nc->m_name);
      if (SStrLen(buffer) + SStrLen(line) >= sizeof(line)) {
        AddChatMessage(line, SLASH_CMD_LIST_CHANNEL, 0, 0, channel->name, 0, 0);
        line[0] = 0;
        namesThisLine = 0;
      } else {
        SStrPack(line, buffer, 0x7FFFFFFF);
        if (user != channel->pendingNames.Tail()) {
          SStrPack(line, ",", 0x7FFFFFFF);
        }
        ++namesThisLine;
      }
    }
  }
  if (namesThisLine) {
    AddChatMessage(line, SLASH_CMD_LIST_CHANNEL, 0, 0, channel->name, 0, 0);
  }
  channel->pendingNames.Clear();
}

void CGChat::QueueChatText(
    int       slashCmd,
    DWORDLONG guid,
    char     *text,
    UINT      language,
    int       waitingForUI,
    int       parse,
    LPCSTR    channel,
    DWORDLONG guid2,
    LPCSTR    specialFlag
) {
  PENDINGCHAT *pending = s_pendingChat.NewNode(LIST_TAIL, 0, 0);
  pending->slashCmd = slashCmd;
  pending->guid = guid;
  pending->text = SStrDupA(text, __FILE__, __LINE__);
  pending->language = language;
  pending->waitingForUI = waitingForUI;
  pending->parse = parse;
  SStrCopy(pending->channel, channel, sizeof(pending->channel));
  pending->guid2 = guid2;
  SStrCopy(pending->specialFlag, specialFlag, sizeof(pending->specialFlag));
}

void CGChat::QueueTextEmote(const DWORDLONG &sender, int textEmoteID, LPCSTR target, int waitingForUI) {
  PENDINGTEXTEMOTE *pending = s_pendingTextEmote.NewNode(LIST_TAIL, 0, 0);
  pending->sender = sender;
  pending->textEmoteID = textEmoteID;
  pending->target = SStrDupA(target, __FILE__, __LINE__);
  pending->waitingForUI = waitingForUI;
}

extern bool QuestParserParseText(LPCSTR text, char *buf, UINT size, const DWORDLONG &target, int restoreToken);

void CGChat::NameQueryCallback(int id, const DWORDLONG &guid, LPVOID arg, bool granted) {
  SAFEITERATELIST(PENDINGCHAT, s_pendingChat, node)
  {
    if (node->waitingForUI) {
      continue;
    }
    if (node->guid != guid && node->guid2 != guid) {
      continue;
    }

    const NameCache *nc[2];
    nc[1] = 0;
    nc[0] = g_nameDBCache.GetRecord(node->guid, 0, 0, 0);
    if (node->guid2) {
      nc[1] = g_nameDBCache.GetRecord(node->guid2, 0, 0, 0);
    }

    if (nc[0] && (!node->guid2 || nc[1])) {
      CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(node->guid, __FILE__, __LINE__));
      if (unit && (node->slashCmd == SLASH_CMD_SAY || node->slashCmd == SLASH_CMD_YELL || node->slashCmd == SLASH_CMD_PARTY)) {
        char laughToken[32];
        for (int index = 1;;) {
          SStrPrintf(laughToken, sizeof(laughToken), "LAUGH_WORD%d", index++);
          LPCSTR laugh = FrameScript_GetText(laughToken, -1, GENDER_NOT_APPLICABLE);
          if (laugh && *laugh) {
            if (SStrCmpI(laugh, node->text, INT_MAX)) {
              continue;
            }
            unit->RequestTalkEmote(TALKANIM_LAUGH);
            break;
          }
          if (node->slashCmd == SLASH_CMD_SAY || node->slashCmd == SLASH_CMD_YELL) {
            TALKANIMATION talkAnim = TALKANIM_TALK;
            UINT          len = SStrLen(node->text);
            if (node->slashCmd == SLASH_CMD_YELL) {
              talkAnim = TALKANIM_SHOUT;
            } else if (len > 0) {
              if (node->text[len - 1] == '?') {
                talkAnim = TALKANIM_QUESTION;
              } else if (node->text[len - 1] == '!') {
                talkAnim = TALKANIM_EXCLAMATION;
              }
            }
            unit->RequestTalkEmote(talkAnim);
          }
          break;
        }
      }

      if (m_paused) {
        node->waitingForUI = 1;
        continue;
      }

      char buffer[512];
      if (QuestParserParseText(node->text, buffer, sizeof(buffer), node->guid, 0)) {
        AddChatMessage(
            buffer, (SLASH_COMMAND_ID)node->slashCmd, nc[0]->m_name, node->language, node->channel, nc[1]->m_name, node->specialFlag
        );
      }
    } else if (!(node->guid == guid && !nc[0]) && !(node->guid2 == guid && !nc[1])) {
      continue;
    }
    s_pendingChat.DeleteNode(node);
  }

  for (UINT i = 0; i < s_channels.Count(); ++i) {
    if (!s_channels[i].pendingNames.IsEmpty()) {
      DisplayPendingUserList(&s_channels[i]);
    }
  }
}

void CGChat::TextEmoteNameQueryCallback(int id, const DWORDLONG &guid, LPVOID arg, bool granted) {
  SAFEITERATELIST(PENDINGTEXTEMOTE, s_pendingTextEmote, node)
  {
    if (node->waitingForUI) {
      continue;
    }
    if (node->sender != guid) {
      continue;
    }
    if (g_nameDBCache.GetRecord(node->sender, 0, 0, 0)) {
      if (m_paused) {
        node->waitingForUI = 1;
        continue;
      }
      AddTextEmoteMessage(node->sender, node->textEmoteID, node->target);
    }
    s_pendingTextEmote.DeleteNode(node);
  }
}

void CGChat::GetPendingChatMessages() {
  if (m_paused) {
    return;
  }

  SAFEITERATELIST(PENDINGCHAT, s_pendingChat, node)
  {
    if (!node->waitingForUI) {
      continue;
    }
    node->waitingForUI = 0;

    const NameCache *nc;
    if (node->guid) {
      nc = g_nameDBCache.GetRecord(node->guid, node->guid, NameQueryCallback, 0);
    } else {
      nc = 0;
    }
    if (node->guid && !nc) {
      continue;
    }

    CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(node->guid, __FILE__, __LINE__));
    if (unit && (node->slashCmd == SLASH_CMD_SAY || node->slashCmd == SLASH_CMD_YELL || node->slashCmd == SLASH_CMD_PARTY)) {
      char laughToken[32];
      for (int index = 1;;) {
        SStrPrintf(laughToken, sizeof(laughToken), "LAUGH_WORD%d", index++);
        LPCSTR laugh = FrameScript_GetText(laughToken, -1, GENDER_NOT_APPLICABLE);
        if (laugh && *laugh) {
          if (SStrCmpI(laugh, node->text, INT_MAX)) {
            continue;
          }
          unit->RequestTalkEmote(TALKANIM_LAUGH);
          break;
        }
        if (node->slashCmd == SLASH_CMD_SAY || node->slashCmd == SLASH_CMD_YELL) {
          TALKANIMATION talkAnim = TALKANIM_TALK;
          UINT          len = SStrLen(node->text);
          if (node->slashCmd == SLASH_CMD_YELL) {
            talkAnim = TALKANIM_SHOUT;
          } else if (len > 0) {
            if (node->text[len - 1] == '?') {
              talkAnim = TALKANIM_QUESTION;
            } else if (node->text[len - 1] == '!') {
              talkAnim = TALKANIM_EXCLAMATION;
            }
          }
          unit->RequestTalkEmote(talkAnim);
        }
        break;
      }
    }

    AddChatMessage(node->text, (SLASH_COMMAND_ID)node->slashCmd, nc->m_name, node->language, node->channel, 0, node->specialFlag);
    s_pendingChat.DeleteNode(node);
  }
}

BOOL CGChat::ChatHandler(CDataStore *msg) {
  char             message[512];
  char             buffer[512] = "";
  char             name[48];
  char             channel[128] = "";
  const NameCache *nc;
  UINT             language;
  DWORDLONG        guid = 0;
  BYTE             afkDND = 0;
  LPCSTR           specialFlag = "";
  BYTE             slashCmd;
  LPCSTR           player;

  msg->Get(slashCmd);
  msg->Get(language);
  if (slashCmd == SLASH_CMD_MONSTER_SAY || slashCmd == SLASH_CMD_MONSTER_YELL || slashCmd == SLASH_CMD_MONSTER_EMOTE) {
    msg->GetString(name, sizeof(name));
    msg->Get(guid);
    msg->GetString(message, sizeof(message));
    msg->Get(afkDND);
    player = name;
    if (!QuestParserParseText(message, buffer, sizeof(buffer), guid, 0)) {
      if (guid && !g_nameDBCache.GetRecord(guid, guid, NameQueryCallback, 0)) {
        QueueChatText(slashCmd, guid, message, language, 0, 1, channel, 0, "");
      }
      return 1;
    }
  } else {
    if (slashCmd == SLASH_CMD_SEND_CHANNEL)
      msg->GetString(channel, sizeof(channel));
    msg->Get(guid);
    msg->GetString(buffer, sizeof(buffer));
    msg->Get(afkDND);
    if (afkDND == 2)
      specialFlag = "DND";
    else if (afkDND == 1)
      specialFlag = "AFK";
    else if (afkDND == 3)
      specialFlag = "GM";

    if (guid) {
      nc = g_nameDBCache.GetRecord(guid, guid, NameQueryCallback, 0);
    } else {
      nc = 0;
    }
    if (guid && !nc) {
      QueueChatText(slashCmd, guid, buffer, language, 0, 0, channel, 0, specialFlag);
      return 1;
    }

    CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
    if (unit && !(unit->GetUnitFlags() & 0x04000000) && (slashCmd == SLASH_CMD_SAY || slashCmd == SLASH_CMD_YELL || slashCmd == SLASH_CMD_PARTY)) {
      char laughToken[32];
      for (int index = 1;;) {
        SStrPrintf(laughToken, sizeof(laughToken), "LAUGH_WORD%d", index++);
        LPCSTR laugh = FrameScript_GetText(laughToken, -1, GENDER_NOT_APPLICABLE);
        if (laugh && *laugh) {
          if (SStrCmpI(laugh, buffer, INT_MAX)) {
            continue;
          }
          unit->RequestTalkEmote(TALKANIM_LAUGH);
          break;
        }
        if (slashCmd == SLASH_CMD_SAY || slashCmd == SLASH_CMD_YELL) {
          TALKANIMATION talkAnim = TALKANIM_TALK;
          UINT          len = SStrLen(buffer);
          if (slashCmd == SLASH_CMD_YELL) {
            talkAnim = TALKANIM_SHOUT;
          } else if (len > 0) {
            if (buffer[len - 1] == '?') {
              talkAnim = TALKANIM_QUESTION;
            } else if (buffer[len - 1] == '!') {
              talkAnim = TALKANIM_EXCLAMATION;
            }
          }
          unit->RequestTalkEmote(talkAnim);
        }
        break;
      }
    }
    player = nc->m_name;
  }

  if (!msg->IsRead()) {
    msg->Reset();
    return 1;
  }
  if (m_paused) {
    QueueChatText(slashCmd, guid, buffer, language, 1, 0, channel, 0, specialFlag);
    return 1;
  }
  AddChatMessage(buffer, (SLASH_COMMAND_ID)slashCmd, player, language, channel, 0, specialFlag);
  return 1;
}

BOOL CGChat::HandleTextEmote(CDataStore *msg) {
  char      target[128];
  DWORDLONG sender;
  int       textEmoteID;

  msg->Get(sender);
  msg->Get(textEmoteID);
  msg->GetString(target, sizeof(target));
  if (!g_nameDBCache.GetRecord(sender, sender, TextEmoteNameQueryCallback, 0)) {
    QueueTextEmote(sender, textEmoteID, target, 0);
  } else if (m_paused) {
    QueueTextEmote(sender, textEmoteID, target, 1);
  } else {
    AddTextEmoteMessage(sender, textEmoteID, target);
  }
  return 1;
}

LPCSTR CGChat::GetChannelString(LPCSTR commandString) {
  int localID = SStrToInt(commandString);
  return localID ? GetChannelName(localID) : commandString;
}

void CGChat::CheckFlagChanged(
    DWORDLONG,
    const NameCache *nc,
    BYTE             oldFlags,
    BYTE             newFlags,
    LPCSTR           channel,
    BYTE             flagToCheck,
    LPCSTR           setText,
    LPCSTR           unsetText
) {
  if (!(flagToCheck & oldFlags) && (flagToCheck & newFlags)) {
    AddChatMessage(setText, SLASH_CMD_CHANNEL_NOTICE_USER, nc->m_name, 0, channel, 0, 0);
  } else if ((flagToCheck & oldFlags) && !(flagToCheck & newFlags)) {
    AddChatMessage(unsetText, SLASH_CMD_CHANNEL_NOTICE_USER, nc->m_name, 0, channel, 0, 0);
  }
}

void CGChat::HandleFlagsChanged(DWORDLONG guid, BYTE oldFlags, BYTE newFlags, LPCSTR channel) {
  const NameCache *nc = g_nameDBCache.GetRecord(guid, guid, NameQueryCallback, 0);
  if (!nc)
    return;
  CheckFlagChanged(guid, nc, oldFlags, newFlags, channel, 2, "SET_MODERATOR", "UNSET_MODERATOR");
  CheckFlagChanged(guid, nc, oldFlags, newFlags, channel, 4, "SET_VOICE", "UNSET_VOICE");
}

static BOOL StringToChatType(LPCSTR string, SLASH_COMMAND_ID &slashCmd) {
  const struct {
    SLASH_COMMAND_ID id;
    LPCSTR           string;
  } array[10] = {
      { SLASH_CMD_SAY,     "SAY"},
      { SLASH_CMD_PARTY,   "PARTY"},
      { SLASH_CMD_GUILD,   "GUILD"},
      { SLASH_CMD_OFFICER, "OFFICER"},
      { SLASH_CMD_YELL,    "YELL"},
      { SLASH_CMD_WHISPER, "WHISPER"},
      { SLASH_CMD_EMOTE,   "EMOTE"},
      {SLASH_CMD_SEND_CHANNEL, "CHANNEL"},
      {SLASH_CMD_SEND_AFK,     "AFK"},
      {SLASH_CMD_SEND_DND,     "DND"}
  };
  for (UINT i = 0; i < 10; ++i) {
    if (!SStrCmpI(array[i].string, string, 0x7FFFFFFF)) {
      slashCmd = array[i].id;
      return 1;
    }
  }
  return 0;
}

static BOOL StringToLanguage(LPCSTR string, UINT &language) {
  int numEntries = g_languagesDB.GetNumRecords();
  for (int i = 0; i < numEntries; ++i) {
    const LanguagesRec *rec = g_languagesDB.GetRecordByIndex(i);
    if (rec && !SStrCmpI(string, rec->m_name_lang[CURRENT_LANGUAGE], 0x7FFFFFFF)) {
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

  if (lua_isstring(L, 1)) {
    LPCSTR           text = lua_tostring(L, 1);
    SLASH_COMMAND_ID type = SLASH_CMD_SAY;
    if (lua_isstring(L, 2)) {
      if (!StringToChatType(lua_tostring(L, 2), type)) {
        luaL_error(L, "Unknown chat type");
      }
    }
    if ((!text || !*text) && type != SLASH_CMD_SEND_DND && type != SLASH_CMD_SEND_AFK) {
      return 0;
    }

    UINT   language = player->GetDefaultLanguage();
    LPCSTR target = 0;
    if (lua_isstring(L, 3)) {
      if (!StringToLanguage(lua_tostring(L, 3), language)) {
        luaL_error(L, "Unknown language");
      }
    }
    if (lua_isstring(L, 4)) {
      target = lua_tostring(L, 4);
    }

    if (type == SLASH_CMD_WHISPER) {
      if (!target || !*target) {
        luaL_error(L, "Whisper message missing target player!");
        return 0;
      }
    } else if (type == SLASH_CMD_SEND_CHANNEL) {
      if (!target || !*target) {
        luaL_error(L, "Channel send missing channel number");
        return 0;
      }
      target = CGChat::GetChannelName(SStrToInt(target));
      if (!target) {
        luaL_error(L, "Channel not found");
        return 0;
      }
    }

    if (type == SLASH_CMD_SEND_AFK) {
      if (!(player->GetPlayerFlags() & 4) && !*text) {
        text = FrameScript_GetText("DEFAULT_AFK_MESSAGE", -1, GENDER_NOT_APPLICABLE);
      }
      if (*text) {
        CGChat::AddChatMessage(FrameScript_GetText("MARKED_AFK", -1, GENDER_NOT_APPLICABLE), SLASH_CMD_SYSTEM, 0, 0, 0, 0, 0);
      } else {
        CGChat::AddChatMessage(FrameScript_GetText("CLEARED_AFK", -1, GENDER_NOT_APPLICABLE), SLASH_CMD_SYSTEM, 0, 0, 0, 0, 0);
      }
    }
    if (type == SLASH_CMD_SEND_DND) {
      if (!(player->GetPlayerFlags() & 8) && !*text) {
        text = FrameScript_GetText("DEFAULT_DND_MESSAGE", -1, GENDER_NOT_APPLICABLE);
      }
      if (*text) {
        CGChat::AddChatMessage(FrameScript_GetText("MARKED_DND", -1, GENDER_NOT_APPLICABLE), SLASH_CMD_SYSTEM, 0, 0, 0, 0, 0);
      } else {
        CGChat::AddChatMessage(FrameScript_GetText("CLEARED_DND", -1, GENDER_NOT_APPLICABLE), SLASH_CMD_SYSTEM, 0, 0, 0, 0, 0);
      }
    }

    if (player->GetHealth() <= 0 && type != SLASH_CMD_WHISPER && type != SLASH_CMD_PARTY && type != SLASH_CMD_GUILD && type != SLASH_CMD_OFFICER && type != SLASH_CMD_SEND_CHANNEL) {
      CGGameUI::DisplayError(GERR_CHAT_WHILE_DEAD);
      return 0;
    }

    CDataStore message;
    message.Put(CMSG_MESSAGECHAT);
    message.Put(type);
    message.Put(language);
    if (type == SLASH_CMD_WHISPER || type == SLASH_CMD_SEND_CHANNEL) {
      message.PutString(target);
    }
    message.PutString(text);
    message.Finalize();
    ClientServices_Send(&message);
  } else {
    luaL_error(L, "Usage: SendChatMessage(text [,type] [,language] [,targetPlayer])");
  }
  return 0;
}

static int Script_GetNumLanguages(lua_State *L) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player) {
    return 0;
  }

  int  numEntries = g_languagesDB.GetNumRecords();
  UINT count = 0;
  for (int i = 0; i < numEntries; ++i) {
    UINT                skill;
    const LanguagesRec *rec = g_languagesDB.GetRecordByIndex(i);
    if (rec && player->GetLanguageSkill(rec->m_ID, skill)) {
      ++count;
    }
  }
  lua_pushnumber(L, count);
  return 1;
}

static int Script_GetLanguageByIndex(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: GetLanguageByIndex(index)");
    return 0;
  }
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player) {
    return 0;
  }
  UINT index = lua_tonumber(L, 1);
  UINT count = 0;
  int  numEntries = g_languagesDB.GetNumRecords();
  for (int i = 0; i < numEntries; ++i) {
    UINT                skill;
    const LanguagesRec *rec = g_languagesDB.GetRecordByIndex(i);
    if (rec && player->GetLanguageSkill(rec->m_ID, skill) && ++count == index) {
      lua_pushstring(L, rec->m_name_lang[CURRENT_LANGUAGE]);
      return 1;
    }
  }
  return 0;
}

static int Script_GetDefaultLanguage(lua_State *L) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    const LanguagesRec *rec = g_languagesDB.GetRecord(player->GetDefaultLanguage());
    if (rec) {
      lua_pushstring(L, rec->m_name_lang[CURRENT_LANGUAGE]);
      return 1;
    }
  }
  return 0;
}

static int Script_DoEmote(lua_State *L) {
  if (lua_isstring(L, 1)) {
    LPCSTR               name = lua_tostring(L, 1);
    const EmotesTextRec *rec = 0;
    for (int i = g_emotesTextDB.GetNumRecords(); i;) {
      const EmotesTextRec *candidate = g_emotesTextDB.GetRecordByIndex(--i);
      if (!SStrCmpI(name, candidate->m_name, 0x7FFFFFFF)) {
        rec = candidate;
        break;
      }
    }
    if (!rec) {
      return 0;
    }
    DWORDLONG target = CGGameUI::GetLockedTarget();
    LPCSTR    targetName = lua_tostring(L, 2);
    if (targetName && *targetName) {
      target = CGGameUI::ClosestObjectMatch(targetName, TYPE_PLAYER);
    }
    CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
    if (player) {
      player->SendTextEmote(rec, target);
    }
    return 0;
  }
  luaL_error(L, "Usage: DoEmote(\"emote\"[, \"target\"])");
  return 0;
}

static int Script_ChatFrameLog(lua_State *L) {
  if (lua_isnumber(L, 1)) {
    s_loggingEnabled = lua_tonumber(L, 1);
    if (s_logHandle) {
      SLogClose(s_logHandle);
      s_logHandle = 0;
    }
    if (s_loggingEnabled) {
      ConsoleWrite(SLogCreate("WOWChatLog.txt", 0, &s_logHandle) ? "Chat logging enabled" : "Error creating chat log..logging disabled", DEFAULT_COLOR);
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

static void ChannelPlayerCommand(lua_State *L, int messageCode, LPCSTR funcName) {
  if (!lua_isstring(L, 1) || !lua_isstring(L, 2)) {
    char buffer[512];
    SStrPrintf(buffer, sizeof(buffer), "Usage: %s(\"channel\", \"name\")", funcName);
    luaL_error(L, buffer);
    return;
  }
  LPCSTR channel = CGChat::GetChannelString(lua_tostring(L, 1));
  if (!channel) {
    return;
  }
  CDataStore msg;
  msg.Put(messageCode);
  msg.PutString(channel);
  msg.PutString(lua_tostring(L, 2));
  msg.Finalize();
  ClientServices_Send(&msg);
}

static void ChannelCommand(lua_State *L, int messageCode, LPCSTR funcname) {
  if (!lua_isstring(L, 1)) {
    char buffer[512];
    SStrPrintf(buffer, sizeof(buffer), "Usage: %s(\"channel\")", funcname);
    luaL_error(L, buffer);
    return;
  }
  LPCSTR channel = CGChat::GetChannelString(lua_tostring(L, 1));
  if (!channel) {
    return;
  }
  CDataStore msg;
  msg.Put(messageCode);
  msg.PutString(channel);
  msg.Finalize();
  ClientServices_Send(&msg);
}

static int Script_JoinChannelByName(lua_State *L) {
  if (lua_isstring(L, 1)) {
    LPCSTR     password = lua_isstring(L, 2) ? lua_tostring(L, 2) : "";
    CDataStore msg;
    msg.Put(CMSG_JOIN_CHANNEL);
    msg.PutString(lua_tostring(L, 1));
    msg.PutString(password);
    msg.Finalize();
    ClientServices_Send(&msg);
    return 0;
  }
  luaL_error(L, "Usage: JoinChannelByName(\"name\")");
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
  char line[256];
  line[0] = 0;
  for (UINT i = 0; i < s_channels.Count(); ++i) {
    if (!s_channels[i].localID) {
      continue;
    }
    SStrPrintf(buffer, sizeof(buffer), "[%d. %s] ", s_channels[i].localID, s_channels[i].name);
    if (SStrLen(line + SStrLen(buffer)) >= sizeof(line)) {
      CGChat::AddChatMessage(line, SLASH_CMD_LIST_CHANNEL, 0, 0, 0, 0, 0);
      line[0] = 0;
    }
    SStrPack(line, buffer, sizeof(line));
  }
  CGChat::AddChatMessage(line, SLASH_CMD_LIST_CHANNEL, 0, 0, 0, 0, 0);
  return 0;
}

static int Script_SetChannelPassword(lua_State *L) {
  if (!lua_isstring(L, 1) || !lua_isstring(L, 2)) {
    luaL_error(L, "Usage: SetChannelPassword(\"name\", \"password\")");
    return 0;
  }
  LPCSTR channel = CGChat::GetChannelString(lua_tostring(L, 1));
  if (!channel) {
    return 0;
  }
  CDataStore msg;
  msg.Put(CMSG_CHANNEL_PASSWORD);
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
  int    channel;
  LPCSTR name;
  if (lua_isnumber(L, 1)) {
    channel = lua_tonumber(L, 1);
    name = CGChat::GetChannelName(channel);
    if (!name) {
      channel = 0;
    }
  } else if (lua_isstring(L, 1)) {
    name = lua_tostring(L, 1);
    channel = CGChat::GetChannelID(name);
    if (channel) {
      name = 0;
    }
  } else {
    luaL_error(L, "Usage: GetChannelName([channelIndex] or [channelName])");
    return 0;
  }
  lua_pushnumber(L, channel);
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
  for (UINT i = 0; i < 24; ++i) {
    FrameScript_RegisterFunction(s_ScriptFunctions[i].name, s_ScriptFunctions[i].method);
  }
}

void ChatUnregisterScriptFunctions() {
  for (UINT i = 0; i < 24; ++i) {
    FrameScript_UnregisterFunction(s_ScriptFunctions[i].name);
  }
}
