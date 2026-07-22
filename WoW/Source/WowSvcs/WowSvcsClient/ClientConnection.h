#ifndef WOW_SOURCE_WOWSVCS_WOWSVCSCLIENT_CLIENTCONNECTION_H
#define WOW_SOURCE_WOWSVCS_WOWSVCSCLIENT_CLIENTCONNECTION_H

#include <stpl.h>

#include "Net/NetClient/NetClient.h"
#include "Tempest/c3vector.h"
#include "WowSvcs/WowSvcsClient/ClientServices.h"

class CDataStore;

struct CHARACTER_INFO {
  unsigned __int64   guid;
  char               name[48];
  unsigned int       mapID;
  unsigned int       zoneID;
  unsigned int       guildID;
  NTempest::C3Vector position;
  unsigned int       inventoryItemDisplayID[20];
  unsigned int       inventoryItemType[20];
  unsigned int       petDisplayInfoID;
  unsigned int       petExperienceLevel;
  unsigned int       petCreatureFamilyID;
  unsigned char      raceID;
  unsigned char      classID;
  unsigned char      sexID;
  unsigned char      skinID;
  unsigned char      faceID;
  unsigned char      hairStyleID;
  unsigned char      hairColorID;
  unsigned char      facialHairStyleID;
  unsigned char      experienceLevel;
};

struct REALM_INFO {
  unsigned char id;
  char          name[256];
  char          address[32];
  unsigned int  players;
};

class ClientConnection : public NetClient {
 public:
  ClientConnection();
  ClientConnection(const ClientConnection &connection);
  virtual ~ClientConnection();

  virtual int  Initialize(LoginData *loginData);
  virtual void Destroy();

  void              Cancel(int errorCode);
  void              Cleanup();
  void              Connect();
  void              AccountLogin(const char *name, const char *password, int region, WOW_LOCALE locale);
  void              AccountLogout();
  void              GetRealmList();
  int               GetRealmListCount();
  int               EnumerateRealms(void(__fastcall *fcn)(REALM_INFO &info, void *param), void *param);
  const REALM_INFO *GetRealmInfoByIndex(int index);
  void              GetCharacterList();
  int               GetCharacterListCount();
  int               EnumerateCharacters(void(__fastcall *fcn)(CHARACTER_INFO &info, void *param), void *param);
  void              CharacterCreate(const CHARACTER_CREATE_INFO &info);
  void              CharacterDelete(unsigned __int64 guid);
  void              CharacterLogin(unsigned __int64 id);
  int               Disconnect();
  void              CharacterSetInGame(int state);
  void              CharacterLogout(unsigned int exitAfterLogout, unsigned int instant);
  void              CharacterRemoveFromGame();
  void              CharacterAbortLogout();
  void              CharacterForceLogout();
  int               CharacterLoggingOut();
  void              SetPlaying(int value);
  int               PollStatus(WOWCS_OPS &op, int &errorCode, int &result);
  void              RealmEnumCallback(CDataStore *data);

  unsigned int GetWaitCount() {
    return m_waitCount;
  }

  int IsInGame() {
    return m_inGame;
  }

  int IsConnected() {
    return m_connected;
  }

  virtual int HandleConnect();
  virtual int HandleDisconnect();
  virtual int HandleCantConnect();

  int HandleAuthChallenge(NETMESSAGE msgId, unsigned long __formal, CDataStore *msg);
  int HandleAuthResponse(NETMESSAGE msgId, unsigned long __formal, CDataStore *msg);
  int HandleCharEnum(NETMESSAGE msgId, unsigned long time, CDataStore *msg);
  int HandleCharacterCreate(NETMESSAGE msgId, unsigned long time, CDataStore *msg);
  int HandleCharacterDelete(NETMESSAGE msgId, unsigned long time, CDataStore *msg);
  int HandleCharacterLoginFailed(NETMESSAGE msgId, unsigned long time, CDataStore *msg);
  int HandleLogoutComplete(NETMESSAGE msgId, unsigned long time, CDataStore *msg);
  int HandleLogoutAbortAck(NETMESSAGE msgId, unsigned long time, CDataStore *msg);
  int HandleLogoutResponse(NETMESSAGE msgId, unsigned long time, CDataStore *msg);

 private:
  ClientConnection &operator=(const ClientConnection &connection);
  void              AccountLogin_Finish(int reason);
  void              ConnectToSelectedServer();

  int                          m_initialized;
  int                          m_connected;
  int                          m_playing;
  int                          m_statusComplete;
  int                          m_statusResult;
  WOWCS_OPS                    m_statusCop;
  int                          m_errorCode;
  int                          m_inGame;
  unsigned char                m_exitAfterLogout;
  unsigned char                m_loggingOut;
  LoginData                    m_loginData;
  TSFixedArray<CHARACTER_INFO> m_characterList;
  TSFixedArray<REALM_INFO>     m_realmList;
  int                          m_isBot;
  unsigned int                 m_waitCount;
  void (ClientConnection::*m_cleanup)();
};

#endif
