#ifndef WOW_SOURCE_WOWSVCS_WOWSVCSCLIENT_CLIENTSERVICES_H
#define WOW_SOURCE_WOWSVCS_WOWSVCSCLIENT_CLIENTSERVICES_H

#include "Net/NetClient/NetClient.h"
#include "DB/WowClientDB.h"

namespace NTempest {
  class C3Vector;
}

class ClientConnection;
struct CHARACTER_INFO;
struct REALM_INFO;

enum WOWCS_OPS {
  COP_NONE = 0,
  COP_INIT = 1,
  COP_CONNECT = 2,
  COP_AUTHENTICATE = 3,
  COP_CREATE_ACCOUNT = 4,
  COP_CREATE_CHARACTER = 5,
  COP_GET_CHARACTERS = 6,
  COP_DELETE_CHARACTER = 7,
  COP_LOGIN_CHARACTER = 8,
  COP_GET_REALMS = 9,
  COP_WAIT_QUEUE = 10
};

enum CHAR_NAME_RESULT {
  CHAR_NAME_RESULT_START = 55,
  CHAR_NAME_NO_NAME = 55,
  CHAR_NAME_SUCCESS = 65,
  LAST_CHAR_NAME_RESULT = 66
};

struct CHARACTER_CREATE_INFO {
  char          name[48];
  unsigned char raceID;
  unsigned char classID;
  unsigned char sexID;
  unsigned char skinID;
  unsigned char faceID;
  unsigned char hairStyleID;
  unsigned char hairColorID;
  unsigned char facialHairStyleID;
  unsigned char outfitID;
};

struct LoginData {
  char          m_account[64];
  int           m_loginServerID;
  unsigned char m_sessionKey[40];
};

extern ClientConnection *g_clientConnection;

ClientConnection *ClientServices_SetCurrent(ClientConnection *conn);
ClientConnection *ClientServices_GetCurrent();
void ClientServices_Initialize(LoginData *loginData);
void ClientServices_Destroy();
void ClientServices_PollEventQueue();
void
ClientServices_SetMessageHandler(NETMESSAGE msgId, int(*handler)(void *, NETMESSAGE, unsigned long, CDataStore *), void *param);
void ClientServices_ClearMessageHandler(NETMESSAGE msgId);
void ClientServices_Send(CDataStore *msg);
void ClientServices_SetAccountName(const char *accountName);
const char *ClientServices_GetSelectedRealmName();
const char *ClientServices_GetSelectedRealmAddress();
const char *ClientServices_GetAccountName();
void ClientServices_AccountLogin(const char *name, const char *password, int region, WOW_LOCALE locale);
void ClientServices_AccountLogout();
void ClientServices_Connect();
void ClientServices_Cancel();
void ClientServices_Cleanup();
int ClientServices_IsConnected();
int ClientServices_Disconnect();
void ClientServices_Disconnected();
int ClientServices_ValidDisconnect(const void *message);
int ClientServices_PollStatus(WOWCS_OPS &op, const char *&msg, int &result, int &errorCode);
const char *ClientServices_GetErrorToken(int errorCode);
unsigned int ClientServices_GetWaitCount();
int ClientServices_GetCharacterListCount();
void ClientServices_CharacterCreate(const CHARACTER_CREATE_INFO &info);
CHAR_NAME_RESULT ClientServices_CharacterValidateName(const char *name);
int ClientServices_AccountValidateName(const char *name);
void ClientServices_CharacterLogout(bool instant);
void ClientServices_CharacterSetInGame(int state);
int ClientServices_CharacterIsInGame();
void ClientServices_CharacterAbortLogout();
void ClientServices_CharacterForceLogout();
int ClientServices_CharacterLoggingOut();
void ClientServices_Exit();
void ClientServices_CharacterLogin(unsigned __int64 id, unsigned int continentID, NTempest::C3Vector position);
int ClientServices_EnumerateCharacters(void(*fcn)(CHARACTER_INFO &info, void *param), void *param);
int ClientServices_EnumerateRealms(void(*fcn)(REALM_INFO &info, void *param), void *param);
void ClientServices_CharacterDelete(unsigned __int64 guid);
void ClientServices_GetRealmList();
int ClientServices_GetRealmListCount();
const REALM_INFO *ClientServices_GetRealmInfoByIndex(int index);
void ClientServices_SelectRealm(const char *realmName, const char *redirectServerAddress);
void ClientServices_GetCharacterList();
void ClientServices_CharacterRemoveFromGame();
bool ClientServices_Report(unsigned int reportType, const char *text, const char *category);
bool ClientServices_ReportScreenshot();
void ClientServices_GetNetStats(float &bandwidthIn, float &bandwidthOut, unsigned long &latency);

#endif
