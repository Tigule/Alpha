#ifndef WOW_SOURCE_WOWSVCS_WOWSVCSCLIENT_CLIENTCONNECTION_H
#define WOW_SOURCE_WOWSVCS_WOWSVCSCLIENT_CLIENTCONNECTION_H

#include <stpl.h>

#include "Net/NetClient/NetClient.h"
#include "Tempest/c3vector.h"
#include "WowSvcs/WowSvcsClient/ClientServices.h"

class CDataStore;

struct CHARACTER_INFO {
  DWORDLONG          guid;
  char               name[48];
  UINT               mapID;
  UINT               zoneID;
  UINT               guildID;
  NTempest::C3Vector position;
  UINT               inventoryItemDisplayID[20];
  UINT               inventoryItemType[20];
  UINT               petDisplayInfoID;
  UINT               petExperienceLevel;
  UINT               petCreatureFamilyID;
  BYTE               raceID;
  BYTE               classID;
  BYTE               sexID;
  BYTE               skinID;
  BYTE               faceID;
  BYTE               hairStyleID;
  BYTE               hairColorID;
  BYTE               facialHairStyleID;
  BYTE               experienceLevel;
};

struct REALM_INFO {
  BYTE id;
  char name[256];
  char address[32];
  UINT players;
};

class ClientConnection : public NetClient {
 public:
  ClientConnection();
  ClientConnection(const ClientConnection &connection);
  virtual ~ClientConnection();

  virtual BOOL Initialize(LoginData *loginData);
  virtual void Destroy();

  int               PollStatus(WOWCS_OPS &op, int &errorCode, int &result);
  void              Cancel(int errorCode);
  void              Cleanup();
  void              Connect();
  void              AccountLogin(LPCSTR name, LPCSTR password, int region, WOW_LOCALE locale);
  void              AccountLogout();
  void              GetCharacterList();
  int               GetCharacterListCount();
  int               EnumerateCharacters(void (*fcn)(CHARACTER_INFO &info, LPVOID param), LPVOID param);
  void              CharacterCreate(const CHARACTER_CREATE_INFO &info);
  void              CharacterLogin(DWORDLONG id);
  void              CharacterSetInGame(int state);
  void              CharacterLogout(bool exitAfterLogout, bool instant);
  BOOL              CharacterLoggingOut();
  void              CharacterDelete(DWORDLONG guid);
  void              CharacterRemoveFromGame();
  void              CharacterAbortLogout();
  void              CharacterForceLogout();
  void              SetPlaying(int value);
  void              SetIsBot(int value);
  BOOL              IsBot();
  BOOL              Disconnect();

  BOOL IsInGame() {
    return m_inGame;
  }

  BOOL IsConnected() {
    return m_connected;
  }

  void              RealmEnumCallback(CDataStore *data);
  void              GetRealmList();
  int               GetRealmListCount();
  int               EnumerateRealms(void (*fcn)(REALM_INFO &info, LPVOID param), LPVOID param);
  const REALM_INFO *GetRealmInfoByIndex(int index);
  LPCSTR            GetCharacterName();

  virtual BOOL HandleConnect();
  virtual BOOL HandleDisconnect();
  virtual BOOL HandleCantConnect();

  BOOL HandleAuthChallenge(NETMESSAGE msgId, DWORD, CDataStore *msg);
  BOOL HandleAuthResponse(NETMESSAGE msgId, DWORD, CDataStore *msg);
  BOOL HandleCharEnum(NETMESSAGE msgId, DWORD time, CDataStore *msg);
  BOOL HandleCharacterCreate(NETMESSAGE msgId, DWORD time, CDataStore *msg);
  BOOL HandleCharacterDelete(NETMESSAGE msgId, DWORD time, CDataStore *msg);
  BOOL HandleCharacterLoginFailed(NETMESSAGE msgId, DWORD time, CDataStore *msg);
  BOOL HandleLogoutComplete(NETMESSAGE msgId, DWORD time, CDataStore *msg);
  BOOL HandleLogoutAbortAck(NETMESSAGE msgId, DWORD time, CDataStore *msg);
  BOOL HandleLogoutResponse(NETMESSAGE msgId, DWORD time, CDataStore *msg);

  UINT GetWaitCount() {
    return m_waitCount;
  }

 private:
  void              AccountLogin_Finish(int reason);

  int                          m_initialized;
  int                          m_connected;
  int                          m_playing;
  BOOL                         m_statusComplete;
  int                          m_statusResult;
  WOWCS_OPS                    m_statusCop;
  int                          m_errorCode;
  BOOL                         m_inGame;
  BYTE                         m_exitAfterLogout;
  BYTE                         m_loggingOut;
  LoginData                    m_loginData;
  TSFixedArray<CHARACTER_INFO> m_characterList;
  TSFixedArray<REALM_INFO>     m_realmList;
  BOOL                         m_isBot;
  UINT                         m_waitCount;

  void              Initiate(WOWCS_OPS op, int errorCode, void (ClientConnection::*cleanup)());
  void              Complete(int result, int errorCode) {
    Cleanup();
    m_statusResult = result;
    m_errorCode = errorCode;
    m_statusComplete = 1;
  }
  void              Abort();
  void (ClientConnection::*m_cleanup)();
  void              AccountLogin_Cleanup();
  void              GetCharacterList_Cleanup();
  void              CharacterLogin_Cleanup();
  void              CharacterCreate_Cleanup();
  void              ConnectToSelectedServer();
  ClientConnection &operator=(const ClientConnection &connection);
};

#endif
