#ifndef WOW_SOURCE_WOWSVCS_WOWSVCSCLIENT_CLIENTSERVICES_H
#define WOW_SOURCE_WOWSVCS_WOWSVCSCLIENT_CLIENTSERVICES_H

#include "../../../Common/WowConst.h"
#include "Net/NetClient/NetClient.h"
#include "DB/WowClientDB.h"

namespace NTempest {
  class C3Vector;
}

class ClientConnection;
class CDataStore;
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
  char name[48];
  BYTE raceID;
  BYTE classID;
  BYTE sexID;
  BYTE skinID;
  BYTE faceID;
  BYTE hairStyleID;
  BYTE hairColorID;
  BYTE facialHairStyleID;
  BYTE outfitID;
};

enum {
  REALM_LIST_IN_PROGRESS = 28,
  REALM_LIST_SUCCESS = 29,
  REALM_LIST_FAILED = 30,
  REALM_LIST_INVALID = 31,
  REALM_LIST_REALM_NOT_FOUND = 32,
  LAST_REALM_LIST_RESULT = 33
};

enum {
  ACCOUNT_CREATE_IN_PROGRESS = 33,
  ACCOUNT_CREATE_SUCCESS = 34,
  ACCOUNT_CREATE_FAILED = 35,
  LAST_ACCOUNT_CREATE_RESULT = 36
};

enum {
  CHAR_LIST_RETRIEVING = 36,
  CHAR_LIST_RETRIEVED = 37,
  CHAR_LIST_FAILED = 38,
  LAST_CHAR_LIST_RESULT = 39
};

enum {
  CHAR_CREATE_IN_PROGRESS = 39,
  CHAR_CREATE_SUCCESS = 40,
  CHAR_CREATE_ERROR = 41,
  CHAR_CREATE_FAILED = 42,
  CHAR_CREATE_NAME_IN_USE = 43,
  CHAR_CREATE_DISABLED = 44,
  LAST_CHAR_CREATE_RESULT = 45
};

enum {
  CHAR_DELETE_IN_PROGRESS = 45,
  CHAR_DELETE_SUCCESS = 46,
  CHAR_DELETE_FAILED = 47,
  LAST_CHAR_DELETE_RESULT = 48
};

enum {
  CHAR_LOGIN_IN_PROGRESS = 48,
  CHAR_LOGIN_SUCCESS = 49,
  CHAR_LOGIN_NO_WORLD = 50,
  CHAR_LOGIN_DUPLICATE_CHARACTER = 51,
  CHAR_LOGIN_NO_INSTANCES = 52,
  CHAR_LOGIN_FAILED = 53,
  CHAR_LOGIN_DISABLED = 54,
  LAST_CHAR_LOGIN_RESULT = 55
};

struct LoginData {
  char m_account[64];
  int  m_loginServerID;
  BYTE m_sessionKey[40];
};

extern ClientConnection *g_clientConnection;

ClientConnection *ClientServices_SetCurrent(ClientConnection *conn);
ClientConnection *ClientServices_GetCurrent();
void              ClientServices_Initialize(LoginData *loginData);
void              ClientServices_Destroy();
void              ClientServices_PollEventQueue();
void              ClientServices_SetMessageHandler(NETMESSAGE msgId, BOOL (*handler)(LPVOID, NETMESSAGE, DWORD, CDataStore *), LPVOID param);
void              ClientServices_ClearMessageHandler(NETMESSAGE msgId);
void              ClientServices_Send(CDataStore *msg);
void              ClientServices_SetAccountName(LPCSTR accountName);
LPCSTR            ClientServices_GetSelectedRealmName();
LPCSTR            ClientServices_GetSelectedRealmAddress();
LPCSTR            ClientServices_GetAccountName();
void              ClientServices_AccountLogin(LPCSTR name, LPCSTR password, int region, WOW_LOCALE locale);
void              ClientServices_AccountLogout();
void              ClientServices_Connect();
void              ClientServices_Cancel();
void              ClientServices_Cleanup();
BOOL              ClientServices_IsConnected();
BOOL              ClientServices_Disconnect();
void              ClientServices_Disconnected();
BOOL              ClientServices_ValidDisconnect(LPCVOID message);
int               ClientServices_PollStatus(WOWCS_OPS &op, LPCSTR &msg, int &result, int &errorCode);
LPCSTR            ClientServices_GetErrorToken(int errorCode);
UINT              ClientServices_GetWaitCount();
int               ClientServices_GetCharacterListCount();
void              ClientServices_CharacterCreate(const CHARACTER_CREATE_INFO &info);
CHAR_NAME_RESULT  ClientServices_CharacterValidateName(LPCSTR name);
BOOL              ClientServices_AccountValidateName(LPCSTR name);
void              ClientServices_CharacterLogout(bool instant);
void              ClientServices_CharacterSetInGame(int state);
BOOL              ClientServices_CharacterIsInGame();
void              ClientServices_CharacterAbortLogout();
void              ClientServices_CharacterForceLogout();
BOOL              ClientServices_CharacterLoggingOut();
void              ClientServices_Exit();
void              ClientServices_CharacterLogin(DWORDLONG id, UINT continentID, NTempest::C3Vector position);
int               ClientServices_EnumerateCharacters(void (*fcn)(CHARACTER_INFO &info, LPVOID param), LPVOID param);
int               ClientServices_EnumerateRealms(void (*fcn)(REALM_INFO &info, LPVOID param), LPVOID param);
void              ClientServices_CharacterDelete(DWORDLONG guid);
void              ClientServices_GetRealmList();
int               ClientServices_GetRealmListCount();
const REALM_INFO *ClientServices_GetRealmInfoByIndex(int index);
void              ClientServices_SelectRealm(LPCSTR realmName, LPCSTR redirectServerAddress);
void              ClientServices_GetCharacterList();
void              ClientServices_CharacterRemoveFromGame();
bool              ClientServices_Report(UINT reportType, LPCSTR text, LPCSTR category);
bool              ClientServices_ReportScreenshot();
void              ClientServices_GetNetStats(float &bandwidthIn, float &bandwidthOut, DWORD &latency);

#endif
