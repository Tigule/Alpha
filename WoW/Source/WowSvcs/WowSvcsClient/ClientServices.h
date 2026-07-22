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

ClientConnection *__fastcall ClientServices_SetCurrent(ClientConnection *conn);
ClientConnection *__fastcall ClientServices_GetCurrent();
void __fastcall              ClientServices_Initialize(LoginData *loginData);
void __fastcall              ClientServices_Destroy();
void __fastcall              ClientServices_PollEventQueue();
void __fastcall
ClientServices_SetMessageHandler(NETMESSAGE msgId, int(__fastcall *handler)(void *, NETMESSAGE, unsigned long, CDataStore *), void *param);
void __fastcall              ClientServices_ClearMessageHandler(NETMESSAGE msgId);
void __fastcall              ClientServices_Send(CDataStore *msg);
void __fastcall              ClientServices_SetAccountName(const char *accountName);
const char *__fastcall       ClientServices_GetSelectedRealmName();
const char *__fastcall       ClientServices_GetSelectedRealmAddress();
const char *__fastcall       ClientServices_GetAccountName();
void __fastcall              ClientServices_AccountLogin(const char *name, const char *password, int region, WOW_LOCALE locale);
void __fastcall              ClientServices_AccountLogout();
void __fastcall              ClientServices_Connect();
void __fastcall              ClientServices_Cancel();
void __fastcall              ClientServices_Cleanup();
int __fastcall               ClientServices_IsConnected();
int __fastcall               ClientServices_Disconnect();
void __fastcall              ClientServices_Disconnected();
int __fastcall               ClientServices_ValidDisconnect(const void *message);
int __fastcall               ClientServices_PollStatus(WOWCS_OPS &op, const char *&msg, int &result, int &errorCode);
const char *__fastcall       ClientServices_GetErrorToken(int errorCode);
unsigned int __fastcall      ClientServices_GetWaitCount();
int __fastcall               ClientServices_GetCharacterListCount();
void __fastcall              ClientServices_CharacterCreate(const CHARACTER_CREATE_INFO &info);
CHAR_NAME_RESULT __fastcall  ClientServices_CharacterValidateName(const char *name);
int __fastcall               ClientServices_AccountValidateName(const char *name);
void __fastcall              ClientServices_CharacterLogout(unsigned int instant);
void __fastcall              ClientServices_CharacterSetInGame(int state);
int __fastcall               ClientServices_CharacterIsInGame();
void __fastcall              ClientServices_CharacterAbortLogout();
void __fastcall              ClientServices_CharacterForceLogout();
int __fastcall               ClientServices_CharacterLoggingOut();
void __fastcall              ClientServices_Exit();
void __fastcall              ClientServices_CharacterLogin(unsigned __int64 id, unsigned int continentID, NTempest::C3Vector position);
int __fastcall               ClientServices_EnumerateCharacters(void(__fastcall *fcn)(CHARACTER_INFO &info, void *param), void *param);
int __fastcall               ClientServices_EnumerateRealms(void(__fastcall *fcn)(REALM_INFO &info, void *param), void *param);
void __fastcall              ClientServices_CharacterDelete(unsigned __int64 guid);
void __fastcall              ClientServices_GetRealmList();
int __fastcall               ClientServices_GetRealmListCount();
const REALM_INFO *__fastcall ClientServices_GetRealmInfoByIndex(int index);
void __fastcall              ClientServices_SelectRealm(const char *realmName, const char *redirectServerAddress);
void __fastcall              ClientServices_GetCharacterList();
void __fastcall              ClientServices_CharacterRemoveFromGame();
bool __fastcall              ClientServices_Report(unsigned int reportType, const char *text, const char *category);
bool __fastcall              ClientServices_ReportScreenshot();
void __fastcall              ClientServices_GetNetStats(float &bandwidthIn, float &bandwidthOut, unsigned long &latency);

#endif
