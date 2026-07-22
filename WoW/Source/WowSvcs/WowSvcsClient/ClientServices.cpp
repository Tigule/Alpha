#include <ctype.h>
#include <new>

#include "ClientServices.h"

#include <storm.h>

#include "Base/CDataStore.h"
#include "Client.h"
#include "Console/ConsoleCommand.h"
#include "Console/ConsoleVar.h"
#include "DB/DBClient/AutoCode/ChrClassesRec.h"
#include "DB/DBClient/AutoCode/ChrRacesRec.h"
#include "DB/DBClient/AutoCode/MapRec.h"
#include "DB/DBClient/AutoCode/SpellRec.h"
#include "FrameScript/FrameScript.h"
#include "Game/ValidateName.h"
#include "Glue/CGlueMgr.h"
#include "Object/ObjectClient/Player_C.h"
#include "Object/ObjectClient/Unit_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "Os/OsTime.h"
#include "Os/W32/OSSystem.h"
#include "Ui/GameUI.h"
#include "WowServices/WDataStore.h"
#include "WowSvcs/WowSvcsClient/ClientConnection.h"

void __fastcall BotClientSetAccount(const char *accountName, const char *password);

static char                   text[0x100];
static const char            *s_vendors[4] = {"Unknown", "intel", "AMD", "PPC"};
static const unsigned __int64 mhzCutoff = 990000000ui64;
static const char             verstr[0x43] = "WoW [Release Assertions Enabled] Build 3368 (Dec 11 2003 18:01:27)";
static const char            *s_sexNames[3] = {"Male", "Female", "Neuter"};
static unsigned int           s_accountNameValid;
static char                   s_accountName[64];
static char                   s_redirectServer[64];
ClientConnection             *g_clientConnection;
static ClientConnection      *s_currentConnection;
static CVar                  *s_realmListVar;
static const char            *s_errorCodeTokens[0x42] = {
    "RESPONSE_SUCCESS",
    "RESPONSE_FAILURE",
    "RESPONSE_CANCELLED",
    "RESPONSE_DISCONNECTED",
    "RESPONSE_FAILED_TO_CONNECT",
    "RESPONSE_CONNECTED",
    "RESPONSE_VERSION_MISMATCH",
    "CSTATUS_CONNECTING",
    "CSTATUS_NEGOTIATING_SECURITY",
    "CSTATUS_NEGOTIATION_COMPLETE",
    "CSTATUS_NEGOTIATION_FAILED",
    "CSTATUS_AUTHENTICATING",
    "AUTH_OK",
    "AUTH_FAILED",
    "AUTH_REJECT",
    "AUTH_BAD_SERVER_PROOF",
    "AUTH_UNAVAILABLE",
    "AUTH_SYSTEM_ERROR",
    "AUTH_BILLING_ERROR",
    "AUTH_BILLING_EXPIRED",
    "AUTH_VERSION_MISMATCH",
    "AUTH_UNKNOWN_ACCOUNT",
    "AUTH_INCORRECT_PASSWORD",
    "AUTH_SESSION_EXPIRED",
    "AUTH_SERVER_SHUTTING_DOWN",
    "AUTH_ALREADY_LOGGING_IN",
    "AUTH_LOGIN_SERVER_NOT_FOUND",
    "AUTH_WAIT_QUEUE",
    "REALM_LIST_IN_PROGRESS",
    "REALM_LIST_SUCCESS",
    "REALM_LIST_FAILED",
    "REALM_LIST_INVALID",
    "REALM_LIST_REALM_NOT_FOUND",
    "ACCOUNT_CREATE_IN_PROGRESS",
    "ACCOUNT_CREATE_SUCCESS",
    "ACCOUNT_CREATE_FAILED",
    "CHAR_LIST_RETRIEVING",
    "CHAR_LIST_RETRIEVED",
    "CHAR_LIST_FAILED",
    "CHAR_CREATE_IN_PROGRESS",
    "CHAR_CREATE_SUCCESS",
    "CHAR_CREATE_ERROR",
    "CHAR_CREATE_FAILED",
    "CHAR_CREATE_NAME_IN_USE",
    "CHAR_CREATE_DISABLED",
    "CHAR_DELETE_IN_PROGRESS",
    "CHAR_DELETE_SUCCESS",
    "CHAR_DELETE_FAILED",
    "CHAR_LOGIN_IN_PROGRESS",
    "CHAR_LOGIN_SUCCESS",
    "CHAR_LOGIN_NO_WORLD",
    "CHAR_LOGIN_DUPLICATE_CHARACTER",
    "CHAR_LOGIN_NO_INSTANCES",
    "CHAR_LOGIN_FAILED",
    "CHAR_LOGIN_DISABLED",
    "CHAR_NAME_NO_NAME",
    "CHAR_NAME_TOO_SHORT",
    "CHAR_NAME_TOO_LONG",
    "CHAR_NAME_STARTS_WITH_GRAVE",
    "CHAR_NAME_TWO_GRAVES",
    "CHAR_NAME_INVALID_CHARACTER",
    "CHAR_NAME_MIXED_LANGUAGES",
    "CHAR_NAME_PROFANE",
    "CHAR_NAME_RESERVED",
    "CHAR_NAME_FAILURE",
    "CHAR_NAME_SUCCESS"
};
extern CVar           *g_realmNameVar;
static void __fastcall RealmEnum_InternalCallback(CDataStore *data, void *param);

static int __fastcall ConsoleCommand_Logout(const char *command, const char *arguments) {
  ASSERT(s_currentConnection);

  if (s_currentConnection->IsInGame()) {
    ClientServices_CharacterLogout(true);
  }

  return 1;
}

static int __fastcall ClientServices_MessageHandler(void *param, NETMESSAGE msgId, unsigned long time, CDataStore *msg) {
  ASSERT(param);

  ClientConnection *connection = static_cast<ClientConnection *>(param);
  switch (msgId) {
    case SMSG_AUTH_CHALLENGE:
      return connection->HandleAuthChallenge(msgId, time, msg);
    case SMSG_AUTH_RESPONSE:
      return connection->HandleAuthResponse(msgId, time, msg);
    case SMSG_CHAR_ENUM:
      return connection->HandleCharEnum(msgId, time, msg);
    case SMSG_CHAR_CREATE:
      return connection->HandleCharacterCreate(msgId, time, msg);
    case SMSG_CHARACTER_LOGIN_FAILED:
      return connection->HandleCharacterLoginFailed(msgId, time, msg);
    case SMSG_LOGOUT_COMPLETE:
      return connection->HandleLogoutComplete(msgId, time, msg);
    case SMSG_LOGOUT_CANCEL_ACK:
      return connection->HandleLogoutAbortAck(msgId, time, msg);
    case SMSG_LOGOUT_RESPONSE:
      return connection->HandleLogoutResponse(msgId, time, msg);
    case SMSG_CHAR_DELETE:
      return connection->HandleCharacterDelete(msgId, time, msg);
  }

  return 0;
}
ClientConnection::ClientConnection() {
  m_initialized = 0;
  m_connected = 0;
  m_playing = 0;
  m_inGame = 0;
  m_statusCop = COP_NONE;
  m_errorCode = 0;
  m_cleanup = 0;
  m_isBot = 0;
  m_exitAfterLogout = 0;
  m_loggingOut = 0;
  m_statusComplete = 1;
  m_statusResult = 1;
}

ClientConnection::~ClientConnection() {
  Destroy();
}

int ClientConnection::Initialize(LoginData *loginData) {
  if (!m_statusComplete) {
    Cancel(1);
  }

  m_statusCop = COP_INIT;
  if (!m_initialized) {
    int result = NetClient::Initialize();
    if (!result) {
      m_statusCop = COP_NONE;
      return result;
    }
  }

  m_statusComplete = 1;
  m_statusResult = 1;
  m_errorCode = 0;
  m_initialized = 1;

  SetMessageHandler(SMSG_AUTH_CHALLENGE, ClientServices_MessageHandler, this);
  SetMessageHandler(SMSG_AUTH_RESPONSE, ClientServices_MessageHandler, this);
  SetMessageHandler(SMSG_CHAR_ENUM, ClientServices_MessageHandler, this);
  SetMessageHandler(SMSG_CHAR_CREATE, ClientServices_MessageHandler, this);
  SetMessageHandler(SMSG_CHARACTER_LOGIN_FAILED, ClientServices_MessageHandler, this);
  SetMessageHandler(SMSG_LOGOUT_COMPLETE, ClientServices_MessageHandler, this);
  SetMessageHandler(SMSG_LOGOUT_CANCEL_ACK, ClientServices_MessageHandler, this);
  SetMessageHandler(SMSG_LOGOUT_RESPONSE, ClientServices_MessageHandler, this);
  SetMessageHandler(SMSG_CHAR_DELETE, ClientServices_MessageHandler, this);

  m_loginData = *loginData;
  return 1;
}

void __fastcall ClientServices_Initialize(LoginData *loginData) {
  if (!g_clientConnection) {
    g_clientConnection = NEW(ClientConnection);
    ASSERT(g_clientConnection);
  }

  if (loginData) {
    g_clientConnection->Initialize(loginData);
  } else {
    LoginData dummy;
    memset(&dummy, 0, sizeof(dummy));
    g_clientConnection->Initialize(&dummy);
  }

  s_currentConnection = g_clientConnection;
  ConsoleCommandRegister("logout", ConsoleCommand_Logout, DEFAULT, 0);

  if (!s_realmListVar) {
    s_realmListVar = CVar::Register("realmList", "Address of realm list server", 0, "wowrealms.battle.net", 0, NET, false, 0);
    ConsoleCommandExecute("run realmlist.wtf", 1);
  }
}
void ClientConnection::Destroy() {
  if (m_initialized) {
    ClearMessageHandler(SMSG_AUTH_CHALLENGE);
    ClearMessageHandler(SMSG_AUTH_RESPONSE);
    ClearMessageHandler(SMSG_CHAR_ENUM);
    ClearMessageHandler(SMSG_CHAR_CREATE);
    ClearMessageHandler(SMSG_CHARACTER_LOGIN_FAILED);
    ClearMessageHandler(SMSG_LOGOUT_COMPLETE);
    ClearMessageHandler(SMSG_LOGOUT_CANCEL_ACK);
    ClearMessageHandler(SMSG_LOGOUT_RESPONSE);
    ClearMessageHandler(SMSG_CHAR_DELETE);

    if (!m_statusComplete) {
      Cancel(1);
    }

    if (m_playing) {
      ClientServices_CharacterRemoveFromGame();
    }

    if (m_connected) {
      Disconnect();
    }

    m_characterList.Clear();
    m_realmList.Clear();
    NetClient::Destroy();
    m_initialized = 0;
  }
}

void __fastcall ClientServices_Destroy() {
  ConsoleCommandUnregister("logout");

  if (g_clientConnection) {
    DEL(g_clientConnection);
    g_clientConnection = 0;
  }
}
int ClientConnection::PollStatus(WOWCS_OPS &op, int &errorCode, int &result) {
  op = m_statusCop;
  errorCode = m_errorCode;
  result = m_statusResult;
  return m_statusComplete;
}

int __fastcall ClientServices_PollStatus(WOWCS_OPS &op, const char *&msg, int &result, int &errorCode) {
  ASSERT(s_currentConnection);

  int status = s_currentConnection->PollStatus(op, errorCode, result);
  if (status) {
    const char *localized = 0;
    if (errorCode >= 0 && errorCode < 0x42) {
      localized = FrameScript_GetText(s_errorCodeTokens[errorCode], -1, GENDER_NOT_APPLICABLE);
    }

    if (!localized) {
      SStrPrintf(text, sizeof(text), "(%i)", errorCode);
      msg = text;
    } else if (errorCode == 27) {
      SStrPrintf(text, sizeof(text), localized, s_currentConnection->GetWaitCount());
      msg = text;
    } else {
      msg = localized;
    }
  } else {
    msg = "";
  }

  return status;
}

const char *__fastcall ClientServices_GetErrorToken(int errorCode) {
  if (errorCode <= 0 || errorCode >= LAST_CHAR_NAME_RESULT) {
    return "";
  }

  return s_errorCodeTokens[errorCode];
}

unsigned int __fastcall ClientServices_GetWaitCount() {
  ASSERT(s_currentConnection);
  return s_currentConnection->GetWaitCount();
}

int __fastcall ClientServices_ValidDisconnect(const void *message) {
  const ClientConnection *client = static_cast<const ClientConnection *>(message);

  ASSERT(client);
  return client == s_currentConnection;
}

void ClientConnection::Cancel(int errorCode) {
  Cleanup();
  m_statusResult = 0;
  m_errorCode = errorCode;
  m_statusComplete = 1;
}

void __fastcall ClientServices_Cancel() {
  ASSERT(s_currentConnection);
  s_currentConnection->Cancel(2);
}

void __fastcall ClientServices_Disconnected() {
  ASSERT(s_currentConnection);
}

void ClientConnection::Cleanup() {
  if (m_cleanup) {
    (this->*m_cleanup)();
    m_cleanup = 0;
  }
}

void __fastcall ClientServices_Cleanup() {
  ASSERT(s_currentConnection);
  s_currentConnection->Cleanup();
}

int ClientConnection::HandleConnect() {
  Cleanup();
  m_statusResult = 1;
  m_errorCode = 5;
  m_statusComplete = 1;
  m_connected = 1;
  return NetClient::HandleConnect();
}

int ClientConnection::HandleDisconnect() {
  Cleanup();
  m_statusComplete = 1;
  m_connected = 0;
  return NetClient::HandleDisconnect();
}

int ClientConnection::HandleCantConnect() {
  Cleanup();
  m_statusResult = 0;
  m_errorCode = 4;
  m_statusComplete = 1;
  m_connected = 0;
  return NetClient::HandleCantConnect();
}

void ClientConnection::Connect() {
  ASSERT(m_statusComplete == 1);

  m_cleanup = 0;
  m_statusCop = COP_CONNECT;
  m_errorCode = 7;
  m_statusComplete = 0;

  if (m_connected) {
    Cleanup();
    m_statusResult = 1;
    m_statusComplete = 1;
    m_errorCode = 5;
  } else {
    if (m_realmList.Count()) {
      m_realmList.Clear();
    }

    ClientNetGetRealms(s_realmListVar->GetString(), RealmEnum_InternalCallback, this);
  }
}

void __fastcall ClientServices_Connect() {
  ASSERT(s_currentConnection);

  CGlueMgr::ClearWaitQueue();
  s_currentConnection->Connect();
}

int __fastcall ClientServices_IsConnected() {
  ASSERT(s_currentConnection);
  return s_currentConnection->IsConnected();
}

void ClientConnection::AccountLogin_Finish(int reason) {
  Cleanup();
  m_errorCode = reason;
  m_statusComplete = 1;
  m_statusResult = reason == 12;
}

int ClientConnection::HandleAuthChallenge(NETMESSAGE msgId, unsigned long __formal, CDataStore *msg) {
  SHA1_CONTEXT ctx;
  unsigned int localDigest[5];
  int          localChallenge;
  unsigned int loginServerID;
  unsigned int challenge;
  unsigned int addr;

  ASSERT(msgId == SMSG_AUTH_CHALLENGE);

  msg->Get(challenge);

  WDataStore resp;
  resp.Put(CMSG_AUTH_SESSION);
  resp.Put(3368);
  resp.Put(m_loginData.m_loginServerID);
  resp.PutString(m_loginData.m_account);

  localChallenge = NTempest::CRandom::uint32_(g_rndSeed);
  resp.Put(localChallenge);

  addr = 0;
  loginServerID = m_loginData.m_loginServerID;

  SHA1_Init(&ctx);
  SHA1_Update(&ctx, reinterpret_cast<const unsigned char *>(m_loginData.m_account), SStrLen(m_loginData.m_account));
  SHA1_Update(&ctx, reinterpret_cast<const unsigned char *>(&addr), sizeof(addr));
  SHA1_Update(&ctx, reinterpret_cast<const unsigned char *>(&localChallenge), sizeof(localChallenge));
  SHA1_Update(&ctx, reinterpret_cast<const unsigned char *>(&loginServerID), sizeof(loginServerID));
  SHA1_Update(&ctx, reinterpret_cast<const unsigned char *>(&challenge), sizeof(challenge));
  SHA1_Update(&ctx, m_loginData.m_sessionKey, sizeof(m_loginData.m_sessionKey));
  SHA1_Final(reinterpret_cast<unsigned char *>(localDigest), &ctx);

  resp.PutData(localDigest, sizeof(localDigest));
  resp.Finalize();
  NetClient::Send(&resp);
  return 1;
}

int ClientConnection::HandleAuthResponse(NETMESSAGE msgId, unsigned long __formal, CDataStore *msg) {
  ASSERT(msgId == SMSG_AUTH_RESPONSE);

  unsigned char result;
  msg->Get(result);

  if (result == 27) {
    msg->Get(m_waitCount);
    m_errorCode = result;
    m_statusCop = COP_WAIT_QUEUE;
    m_statusComplete = 0;
    CGlueMgr::UpdateWaitQueue(m_waitCount);
  } else {
    AccountLogin_Finish(result);
  }

  return 1;
}

void ClientConnection::AccountLogin(const char *name, const char *password, int region, WOW_LOCALE locale) {
  ASSERT(m_initialized);
  ASSERT(m_statusComplete == 1);
  ASSERT(name);
  ASSERT(password);

  m_cleanup = 0;
  m_statusCop = COP_AUTHENTICATE;
  m_errorCode = 11;
  m_statusComplete = 0;

  if (!m_connected) {
    Cancel(4);
  }
}

void __fastcall ClientServices_AccountLogin(const char *name, const char *password, int region, WOW_LOCALE locale) {
  ASSERT(s_currentConnection);
  s_currentConnection->AccountLogin(name, password, region, locale);
}

void __fastcall ClientServices_SetAccountName(const char *accountName) {
  ASSERT(accountName);
  SStrCopy(s_accountName, accountName, sizeof(s_accountName));
  s_accountNameValid = 1;
  BotClientSetAccount(accountName, "");
}

void ClientConnection::AccountLogout() {
  s_accountNameValid = 0;
}

void __fastcall ClientServices_AccountLogout() {
  ASSERT(s_currentConnection);
  s_currentConnection->AccountLogout();
}

int ClientConnection::HandleCharEnum(NETMESSAGE msgId, unsigned long time, CDataStore *msg) {
  ASSERT(msgId == SMSG_CHAR_ENUM);

  unsigned char count;
  msg->Get(count);
  m_characterList.SetCount(count);

  for (unsigned char i = 0; i < count; ++i) {
    CHARACTER_INFO &character = m_characterList[i];

    msg->Get(character.guid);
    msg->GetString(character.name, sizeof(character.name));
    msg->Get(character.raceID);
    msg->Get(character.classID);
    msg->Get(character.sexID);
    msg->Get(character.skinID);
    msg->Get(character.faceID);
    msg->Get(character.hairStyleID);
    msg->Get(character.hairColorID);
    msg->Get(character.facialHairStyleID);
    msg->Get(character.experienceLevel);
    msg->Get(character.zoneID);
    msg->Get(character.mapID);
    msg->Get(character.position.x);
    msg->Get(character.position.y);
    msg->Get(character.position.z);
    msg->Get(character.guildID);
    msg->Get(character.petDisplayInfoID);
    msg->Get(character.petExperienceLevel);
    msg->Get(character.petCreatureFamilyID);

    for (unsigned int item = 0; item < 20; ++item) {
      unsigned char type;
      msg->Get(character.inventoryItemDisplayID[item]);
      msg->Get(type);
      character.inventoryItemType[item] = type;
    }
  }

  if (msg->IsRead()) {
    Cleanup();
    m_statusResult = 1;
    m_errorCode = 37;
    m_statusComplete = 1;
  } else {
    m_characterList.Clear();
    Cleanup();
    m_statusResult = 0;
    m_errorCode = 38;
    m_statusComplete = 1;
  }

  return 1;
}

void ClientConnection::GetCharacterList() {
  m_characterList.Clear();
  m_cleanup = 0;
  m_statusCop = COP_GET_CHARACTERS;
  m_errorCode = 36;
  m_statusComplete = 0;

  if (!m_connected) {
    Cancel(4);
    return;
  }

  CDataStore netMsg;
  netMsg.Put(CMSG_CHAR_ENUM);
  netMsg.Finalize();
  Send(&netMsg);
}

void __fastcall ClientServices_GetCharacterList() {
  ASSERT(s_currentConnection);
  s_currentConnection->GetCharacterList();
}

int ClientConnection::GetCharacterListCount() {
  return m_characterList.Count();
}

int __fastcall ClientServices_GetCharacterListCount() {
  ASSERT(s_currentConnection);
  return s_currentConnection->GetCharacterListCount();
}

int ClientConnection::EnumerateCharacters(void(__fastcall *fcn)(CHARACTER_INFO &info, void *param), void *param) {
  ASSERT(fcn);

  int enumCount = m_characterList.Count();
  for (int index = 0; index < enumCount; ++index) {
    fcn(m_characterList[index], param);
  }

  return enumCount;
}

int __fastcall ClientServices_EnumerateCharacters(void(__fastcall *fcn)(CHARACTER_INFO &info, void *param), void *param) {
  ASSERT(s_currentConnection);
  return s_currentConnection->EnumerateCharacters(fcn, param);
}

int ClientConnection::HandleCharacterCreate(NETMESSAGE msgId, unsigned long time, CDataStore *msg) {
  ASSERT(msgId == SMSG_CHAR_CREATE);

  unsigned char result;
  msg->Get(result);
  if (!msg->IsRead()) {
    msg->Reset();
  }

  Cleanup();
  m_errorCode = result;
  m_statusResult = result == 40;
  m_statusComplete = 1;
  return 1;
}

void ClientConnection::CharacterCreate(const CHARACTER_CREATE_INFO &info) {
  m_cleanup = 0;
  m_statusCop = COP_CREATE_CHARACTER;
  m_errorCode = 39;
  m_statusComplete = 0;

  if (!m_connected) {
    Cancel(4);
    return;
  }

  CDataStore netMsg;
  netMsg.Put(CMSG_CHAR_CREATE);
  netMsg.PutString(info.name);
  netMsg.Put(info.raceID);
  netMsg.Put(info.classID);
  netMsg.Put(info.sexID);
  netMsg.Put(info.skinID);
  netMsg.Put(info.faceID);
  netMsg.Put(info.hairStyleID);
  netMsg.Put(info.hairColorID);
  netMsg.Put(info.facialHairStyleID);
  netMsg.Put(info.outfitID);
  netMsg.Finalize();
  Send(&netMsg);
}

void __fastcall ClientServices_CharacterCreate(const CHARACTER_CREATE_INFO &info) {
  ASSERT(s_currentConnection);
  s_currentConnection->CharacterCreate(info);
}

int ClientConnection::HandleCharacterLoginFailed(NETMESSAGE msgId, unsigned long time, CDataStore *msg) {
  ASSERT(msgId == SMSG_CHARACTER_LOGIN_FAILED);

  unsigned char reason;
  msg->Get(reason);
  ASSERT(msg->IsRead());

  if (!m_isBot) {
    CharacterRemoveFromGame();
    if (this == s_currentConnection) {
      ClientDestroyGame(1, 1, 1);
    }
  }

  switch (reason) {
    case 1:
      Cancel(50);
      break;
    case 2:
      Cancel(51);
      break;
    case 3:
      Cancel(52);
      break;
    case 4:
      Cancel(54);
      break;
    default:
      Cancel(53);
      break;
  }

  return 1;
}

void ClientConnection::CharacterLogin(unsigned __int64 id) {
  m_cleanup = 0;
  m_statusCop = COP_LOGIN_CHARACTER;
  m_errorCode = 48;
  m_statusComplete = 0;

  if (!m_connected) {
    Cancel(4);
    return;
  }

  CDataStore netMsg;
  netMsg.Put(CMSG_PLAYER_LOGIN);
  netMsg.Put(id);
  netMsg.Finalize();
  Send(&netMsg);
  m_playing = 1;
}

void __fastcall ClientServices_CharacterLogin(unsigned __int64 id, unsigned int continentID, NTempest::C3Vector position) {
  ASSERT(s_currentConnection);
  s_currentConnection->CharacterLogin(id);
}

void ClientConnection::CharacterSetInGame(int state) {
  Cleanup();
  m_statusResult = 1;
  m_statusComplete = 1;
  m_errorCode = 49;
  m_inGame = state;
}

void __fastcall ClientServices_CharacterSetInGame(int state) {
  ASSERT(s_currentConnection);
  s_currentConnection->CharacterSetInGame(state);
}

int __fastcall ClientServices_CharacterIsInGame() {
  return s_currentConnection && s_currentConnection->IsInGame();
}

int ClientConnection::HandleLogoutComplete(NETMESSAGE msgId, unsigned long time, CDataStore *msg) {
  ASSERT(msgId == SMSG_LOGOUT_COMPLETE);
  ASSERT(msg->IsRead());

  CharacterRemoveFromGame();
  if (this == s_currentConnection) {
    ClientDestroyGame(1, 1, 0);
  }

  unsigned char exitAfterLogout = m_exitAfterLogout;
  m_loggingOut = 0;
  if (exitAfterLogout) {
    ClientPostClose();
  }

  return 1;
}

int ClientConnection::HandleLogoutResponse(NETMESSAGE msgId, unsigned long time, CDataStore *msg) {
  ASSERT(msgId == SMSG_LOGOUT_RESPONSE);

  unsigned char result = 0;
  msg->Get(result);
  ASSERT(msg->IsRead());

  if (result) {
    FrameScript_SignalEvent(m_exitAfterLogout ? 258 : 257);
  } else {
    FrameScript_SignalEvent(332);
    m_loggingOut = 0;
  }

  return 1;
}

void ClientConnection::CharacterRemoveFromGame() {
  if (m_inGame) {
    CharacterSetInGame(0);
  }

  m_playing = 0;
}

void __fastcall ClientServices_CharacterRemoveFromGame() {
  ASSERT(s_currentConnection);
  s_currentConnection->CharacterRemoveFromGame();
  ClientDestroyGame(1, 1, 0);
}

int ClientConnection::HandleLogoutAbortAck(NETMESSAGE msgId, unsigned long time, CDataStore *msg) {
  ASSERT(msgId == SMSG_LOGOUT_CANCEL_ACK);
  ASSERT(msg->IsRead());

  m_loggingOut = 0;
  return 1;
}

void ClientConnection::CharacterAbortLogout() {
  CDataStore netMsg;
  netMsg.Put(CMSG_LOGOUT_CANCEL);
  netMsg.Finalize();
  Send(&netMsg);
}

void __fastcall ClientServices_CharacterAbortLogout() {
  ASSERT(s_currentConnection);
  s_currentConnection->CharacterAbortLogout();
}

void ClientConnection::CharacterForceLogout() {
  CharacterLogout(false, true);
}

void __fastcall ClientServices_CharacterForceLogout() {
  ASSERT(s_currentConnection);
  s_currentConnection->CharacterForceLogout();
}

void ClientConnection::CharacterLogout(unsigned int exitAfterLogout, unsigned int instant) {
  if (m_loggingOut && !instant) {
    return;
  }

  if (this != s_currentConnection) {
    CharacterRemoveFromGame();
    return;
  }

  m_exitAfterLogout = exitAfterLogout;
  if (!instant) {
    m_loggingOut = 1;
  }

  CDataStore netMsg;
  netMsg.Put(instant ? CMSG_PLAYER_LOGOUT : CMSG_LOGOUT_REQUEST);
  netMsg.Finalize();
  Send(&netMsg);
}

void __fastcall ClientServices_CharacterLogout(unsigned int instant) {
  ASSERT(s_currentConnection);
  s_currentConnection->CharacterLogout(false, instant);
}

int ClientConnection::CharacterLoggingOut() {
  return m_loggingOut;
}

int __fastcall ClientServices_CharacterLoggingOut() {
  ASSERT(s_currentConnection);
  return s_currentConnection->CharacterLoggingOut();
}

void __fastcall ClientServices_Exit() {
  ASSERT(s_currentConnection);
  s_currentConnection->CharacterLogout(true, false);
}

int ClientConnection::HandleCharacterDelete(NETMESSAGE msgId, unsigned long time, CDataStore *msg) {
  ASSERT(msgId == SMSG_CHAR_DELETE);

  unsigned char result;
  msg->Get(result);
  if (!msg->IsRead()) {
    msg->Reset();
  }

  if (!result) {
    Cleanup();
    m_statusResult = 1;
    m_errorCode = 46;
    m_statusComplete = 1;
  } else {
    Cleanup();
    m_statusResult = 0;
    m_errorCode = 47;
    m_statusComplete = 1;
  }

  return 1;
}

void ClientConnection::CharacterDelete(unsigned __int64 guid) {
  m_cleanup = 0;
  m_statusCop = COP_DELETE_CHARACTER;
  m_errorCode = 45;
  m_statusComplete = 0;

  if (!m_connected) {
    Cancel(4);
    return;
  }

  CDataStore netMsg;
  netMsg.Put(CMSG_CHAR_DELETE);
  netMsg.Put(guid);
  netMsg.Finalize();
  Send(&netMsg);
}

void __fastcall ClientServices_CharacterDelete(unsigned __int64 guid) {
  ASSERT(s_currentConnection);
  s_currentConnection->CharacterDelete(guid);
}

int ClientConnection::Disconnect() {
  if (!m_connected) {
    return 0;
  }

  NetClient::Disconnect();
  m_connected = 0;
  return 1;
}

int __fastcall ClientServices_Disconnect() {
  ASSERT(s_currentConnection);
  return s_currentConnection->Disconnect();
}

void ClientConnection::SetPlaying(int value) {
  m_playing = value;
}

void ClientConnection::ConnectToSelectedServer() {
  unsigned int index = 0;

  if (m_realmList.Count() > 0) {
    do {
      if (!SStrCmpI(m_realmList[index].name, g_realmNameVar->GetString(), 0x7FFFFFFF)) {
        goto found;
      }
      ++index;
    } while (index < m_realmList.Count());
  }

  Cleanup();
  m_statusResult = 0;
  m_errorCode = 32;
  m_statusComplete = 1;
  return;

found:
  NetClient::Connect(m_realmList[index].address);
}

void ClientConnection::RealmEnumCallback(CDataStore *data) {
  unsigned char count;
  unsigned char id;

  if (!data) {
    Cleanup();
    m_errorCode = 30;
    goto error;
  }

  data->Get(count);
  m_realmList.SetCount(count);

  for (id = 0; id < count; ++id) {
    REALM_INFO &realm = m_realmList[id];

    realm.id = id;
    data->GetString(realm.name, sizeof(realm.name));
    data->GetString(realm.address, sizeof(realm.address));
    data->Get(realm.players);

    if (!data->IsValid()) {
      break;
    }
  }

  if (!data->IsValid() || !data->IsRead()) {
    if (m_realmList.Count()) {
      m_realmList.Clear();
    }

    Cleanup();
    m_errorCode = 31;
    goto error;
  }

  if (!m_realmList.Count()) {
    s_redirectServer[0] = 0;
  }

  if (m_statusCop == COP_CONNECT) {
    ConnectToSelectedServer();
  } else {
    Cleanup();
    m_statusResult = 1;
    m_errorCode = 29;
    m_statusComplete = 1;
  }

  return;

error:
  m_statusResult = 0;
  m_statusComplete = 1;
}

static void __fastcall RealmEnum_InternalCallback(CDataStore *data, void *param) {
  ClientConnection *client = static_cast<ClientConnection *>(param);

  ASSERT(client);
  client->RealmEnumCallback(data);
}

void ClientConnection::GetRealmList() {
  if (m_realmList.Count()) {
    m_realmList.Clear();
  }

  m_cleanup = 0;
  m_statusCop = COP_GET_REALMS;
  m_errorCode = 28;
  m_statusComplete = 0;
  ClientNetGetRealms(s_realmListVar->GetString(), RealmEnum_InternalCallback, this);
}

void __fastcall ClientServices_GetRealmList() {
  ASSERT(s_currentConnection);
  s_currentConnection->GetRealmList();
}

int ClientConnection::GetRealmListCount() {
  return m_realmList.Count();
}

int __fastcall ClientServices_GetRealmListCount() {
  ASSERT(s_currentConnection);
  return s_currentConnection->GetRealmListCount();
}

int ClientConnection::EnumerateRealms(void(__fastcall *fcn)(REALM_INFO &info, void *param), void *param) {
  ASSERT(fcn);

  int enumCount = m_realmList.Count();
  for (int index = 0; index < enumCount; ++index) {
    fcn(m_realmList[index], param);
  }

  return enumCount;
}

int __fastcall ClientServices_EnumerateRealms(void(__fastcall *fcn)(REALM_INFO &info, void *param), void *param) {
  ASSERT(s_currentConnection);
  return s_currentConnection->EnumerateRealms(fcn, param);
}

const REALM_INFO *ClientConnection::GetRealmInfoByIndex(int index) {
  if (index >= m_realmList.Count()) {
    return 0;
  }

  return &m_realmList[index];
}

const REALM_INFO *__fastcall ClientServices_GetRealmInfoByIndex(int index) {
  ASSERT(s_currentConnection);
  return s_currentConnection->GetRealmInfoByIndex(index);
}

void __fastcall ClientServices_Send(CDataStore *msg) {
  if (s_currentConnection) {
    s_currentConnection->Send(msg);
  }
}

void __fastcall
ClientServices_SetMessageHandler(NETMESSAGE msgId, int(__fastcall *handler)(void *, NETMESSAGE, unsigned long, CDataStore *), void *param) {
  ASSERT(s_currentConnection);
  ASSERT(handler);

  s_currentConnection->SetMessageHandler(msgId, handler, param);
}

void __fastcall ClientServices_ClearMessageHandler(NETMESSAGE msgId) {
  ASSERT(s_currentConnection);

  s_currentConnection->ClearMessageHandler(msgId);
}

void __fastcall ClientServices_SelectRealm(const char *realmName, const char *redirectServerAddress) {
  g_realmNameVar->Set(realmName, true, false, false);
}

const char *__fastcall ClientServices_GetSelectedRealmName() {
  return g_realmNameVar->GetString();
}

const char *__fastcall ClientServices_GetSelectedRealmAddress() {
  return s_redirectServer;
}

CHAR_NAME_RESULT __fastcall ClientServices_CharacterValidateName(const char *name) {
  return static_cast<CHAR_NAME_RESULT>(ValidateCharacterName(CURRENT_LANGUAGE, name) + CHAR_NAME_RESULT_START);
}

int __fastcall ClientServices_AccountValidateName(const char *name) {
  while (*name) {
    if (!isalnum(*name) && *name != '.' && *name != '-') {
      return 0;
    }
    ++name;
  }

  return 1;
}

const char *__fastcall ClientServices_GetAccountName() {
  return s_accountNameValid ? s_accountName : 0;
}

bool __fastcall ClientServices_ReportScreenshot() {
  CDataStore msg;
  msg.Put(CMSG_SCREENSHOT);
  msg.Finalize();
  ClientServices_Send(&msg);
  return true;
}

bool __fastcall ClientServices_Report(unsigned int reportType, const char *text, const char *category) {
  if (!text) {
    return false;
  }
  if (!category) {
    category = "";
  }
  if (!s_currentConnection) {
    return false;
  }

  char          message[0x1000];
  char          name[0x100];
  char          line[0x40];
  unsigned long nameLen;

  SStrCopy(message, text, sizeof(message));
  SStrPack(message, "\n\n\n", sizeof(message));

  nameLen = sizeof(name);
  if (OsGetUserName(name, &nameLen)) {
    SStrPrintf(line, sizeof(line), "Username:\t%s\n", name);
    SStrPack(message, line, sizeof(message));
  }

  nameLen = sizeof(name);
  if (OsGetComputerName(name, &nameLen)) {
    SStrPrintf(line, sizeof(line), "Computer:\t%s\n", name);
    SStrPack(message, line, sizeof(message));
  }

  SStrPrintf(line, sizeof(line), "Processors:\t%u\n", OsGetProcessorCount());
  SStrPack(message, line, sizeof(message));

  int vendor;
  OsGetProcessorFeaturesEx(vendor);
  if (vendor > 4) {
    vendor = 0;
  }
  SStrPrintf(line, sizeof(line), "Processor vendor:\t%s\n", s_vendors[vendor]);
  SStrPack(message, line, sizeof(message));

  unsigned __int64 clocksPerSecond = OsGetAsyncClocksPerSecond();
  unsigned __int64 scaledClocks;
  char             clockUnit;
  if (clocksPerSecond >= mhzCutoff) {
    scaledClocks = clocksPerSecond / 1000000ui64;
    clockUnit = 'G';
  } else {
    scaledClocks = clocksPerSecond / 1000ui64;
    clockUnit = 'M';
  }
  SStrPrintf(line, sizeof(line), "Processor speed:\t%6.2f%cHz\n", static_cast<double>(static_cast<__int64>(scaledClocks)) * 0.001, clockUnit);
  SStrPack(message, line, sizeof(message));

  SStrPrintf(line, sizeof(line), "Memory:\t%uMB\n", OsGetPhysicalMemory() >> 20);
  SStrPack(message, line, sizeof(message));

  SStrPack(message, "OS:\t\t", sizeof(message));
  OsGetVersionString(line, sizeof(line));
  SStrPack(message, line, sizeof(message));
  SStrPack(message, "\n", sizeof(message));

  SStrPrintf(line, sizeof(line), "Version:\t%s\n", verstr);
  SStrPack(message, line, sizeof(message));

  const char *accountName = ClientServices_GetAccountName();
  if (accountName) {
    SStrPrintf(line, sizeof(line), "Realm:\t%s (%s)\n", ClientServices_GetSelectedRealmName(), ClientServices_GetSelectedRealmAddress());
    SStrPack(message, line, sizeof(message));

    SStrPrintf(line, sizeof(line), "Account:\t%s\n", accountName);
    SStrPack(message, line, sizeof(message));

    if (s_currentConnection && s_currentConnection->IsInGame()) {
      unsigned __int64 playerGuid = ClntObjMgrGetActivePlayer();
      CGObject_C      *object = ClntObjMgrObjectPtr(playerGuid, __FILE__, __LINE__);
      CGPlayer_C      *player = static_cast<CGPlayer_C *>(object);
      if (player) {
        const CGUnitData *unitData = player->GetUnitData();
        const char       *gender = s_sexNames[unitData->sex];
        if (!gender) {
          gender = "Unknown gender";
        }

        const ChrRacesRec   *race = g_chrRacesDB.GetRecord(unitData->race);
        const ChrClassesRec *unitClass = g_chrClassesDB.GetRecord(unitData->classId);
        const char          *raceName = race ? race->m_name_lang[CURRENT_LANGUAGE] : "Unknown";
        const char          *className = unitClass ? unitClass->m_name_lang[CURRENT_LANGUAGE] : "Unknown";
        SStrPrintf(line, sizeof(line), "Character:\t%s (level %i %s %s %s)\n", player->GetUnitName(), unitData->level, raceName, gender, className);
        SStrPack(message, line, sizeof(message));

        unsigned int  continentID = CGPlayer_C::GetNewContinentID();
        const MapRec *map = g_mapDB.GetRecord(continentID);
        if (map) {
          SStrPrintf(line, sizeof(line), "Map:\t\t%u (%s)\n", continentID, map->m_Directory);
        } else {
          SStrPrintf(line, sizeof(line), "Map:\t\t%u\n", continentID);
        }
        SStrPack(message, line, sizeof(message));

        const char *zoneText = CGGameUI::GetZoneText();
        if (zoneText && *zoneText) {
          SStrPrintf(line, sizeof(line), "Zone:\t\t%s", zoneText);
          SStrPack(message, line, sizeof(message));

          const char *subZoneText = CGGameUI::GetSubZoneText();
          if (subZoneText && *subZoneText) {
            SStrPrintf(line, sizeof(line), " - %s", subZoneText);
            SStrPack(message, line, sizeof(message));
          }
          SStrPack(message, "\n", sizeof(message));
        }

        NTempest::C3Vector v;
        player->GetPosition(v);
        SStrPrintf(line, sizeof(line), "Position:\t%f %f %f, %f\n", v.x, v.y, v.z, player->GetFacing());
        SStrPack(message, line, sizeof(message));

        const unsigned __int64 &lockedTarget = CGGameUI::GetLockedTarget();
        if (lockedTarget) {
          CGObject_C *targetObject = ClntObjMgrObjectPtr(lockedTarget, __FILE__, __LINE__);
          CGUnit_C   *target = static_cast<CGUnit_C *>(targetObject);
          if (target) {
            SStrPrintf(line, sizeof(line), "Target:\t%s\n", target->GetUnitName());
            SStrPack(message, line, sizeof(message));
          }
        }

        SStrPrintf(line, sizeof(line), "Auras:\n");
        SStrPack(message, line, sizeof(message));
        for (unsigned int auraIndex = 0; auraIndex < 56; ++auraIndex) {
          int spellID = unitData->auras[auraIndex];
          if (!spellID) {
            continue;
          }

          const SpellRec *spell = g_spellDB.GetRecord(spellID);
          const char     *spellName = spell ? spell->m_name_lang[0] : "UNKNOWN";
          SStrPrintf(line, sizeof(line), "\t%s(%d)\n", spellName, spellID);
          SStrPack(message, line, sizeof(message));
        }
      }
    }
  }

  unsigned int reportLength = SStrLen(message) + 1;
  unsigned int categoryLength = SStrLen(category) + 1;
  CDataStore   msg;
  msg.Put(static_cast<int>(CMSG_BUG));
  msg.Put(static_cast<int>(reportType));
  msg.Put(static_cast<int>(reportLength));
  msg.PutData(message, reportLength);
  msg.Put(static_cast<int>(categoryLength));
  msg.PutData(category, categoryLength);
  msg.Finalize();
  s_currentConnection->Send(&msg);
  return true;
}

void __fastcall ClientServices_GetNetStats(float &bandwidthIn, float &bandwidthOut, unsigned long &latency) {
  s_currentConnection->GetNetStats(bandwidthIn, bandwidthOut, latency);
}

ClientConnection *__fastcall ClientServices_GetCurrent() {
  return s_currentConnection;
}

ClientConnection *__fastcall ClientServices_SetCurrent(ClientConnection *conn) {
  ClientConnection *previous = s_currentConnection;
  s_currentConnection = conn;
  return previous;
}

void __fastcall ClientServices_PollEventQueue() {
  if (s_currentConnection) {
    s_currentConnection->PollEventQueue();
  }
}
