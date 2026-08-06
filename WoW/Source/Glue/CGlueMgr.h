#ifndef WOW_SOURCE_GLUE_CGLUEMGR_H
#define WOW_SOURCE_GLUE_CGLUEMGR_H

#include "DB/WowClientDB.h"

#include <Model/IModel.h>

#include <string.h>

class CSimpleTop;
struct CHARACTER_INFO;
struct CHARACTER_CREATE_INFO;
struct REALM_INFO;

class CGlueMgr {
 public:
  enum GLUE_IDLE_STATE {
    IDLE_NONE = 0,
    IDLE_ACCOUNT_LOGIN = 1,
    IDLE_CHARACTER_LIST = 2,
    IDLE_REALM_LIST = 3,
    IDLE_CREATE_CHARACTER = 4,
    IDLE_DELETE_CHARACTER = 5,
    IDLE_ENTER_WORLD = 6,
    IDLE_WORLD_LOGIN = 7
  };

  static void Initialize();
  static void Suspend();
  static void Resume();
  static void Shutdown();

  static int Initialized() {
    return m_initialized;
  }

  static int Suspended() {
    return m_suspended;
  }

  static void ClearWaitQueue() {
    memset(m_queueTime, 0, sizeof(m_queueTime));
    memset(m_queuePosition, 0, sizeof(m_queuePosition));
    m_estimatedWaitTime = 0;
  }

  static void UpdateWaitQueue(UINT wait);

  static void ExpectDisconnect(int reconnect) {
    m_disconnectPending = 1;
    m_reconnect = reconnect;
  }

  static void SetScreen(LPCSTR screen);
  static void UpdateCurrentScreen(LPCSTR screen);

  static void Reload() {
    m_reload = 1;
  }

  static void DefaultServerLogin();
  static void ChangeRealm(const REALM_INFO *info);
  static void CreateCharacter(const CHARACTER_CREATE_INFO *info);
  static void DeleteCharacter(DWORDLONG guid);
  static void QuitGame();
  static void EnterWorld();
  static void WorldLoginFailed();
  static void StatusDialogClick();

  static LPCSTR GetCurrentAccount() {
    return m_accountName;
  }

  static void GetCharacterList();
  static void GetRealmList();
  static int  NetDisconnectHandler(LPCVOID eventData, LPVOID);

 private:
  friend void ClientDestroyGame(int connected, int resumeUI, int loginError);

  static int Idle(LPCVOID eventData, LPVOID param);

 protected:
  static void InitCursor();
  static void DestroyCursor();
  static void RegisterConsoleCommands();
  static void UnregisterConsoleCommands();

 private:
  static CSimpleTop     *m_simpleTop;
  static HMODEL          m_cursorModel;
  static char            m_currentScreen[64];
  static int             m_reload;
  static int             m_initialized;
  static int             m_suspended;
  static int             m_disconnectPending;
  static int             m_reconnect;
  static GLUE_IDLE_STATE m_idleState;
  static int             m_region;
  static WOW_LOCALE      m_locale;
  static char            m_accountName[64];
  static char            m_password[64];
  static UINT            m_queuePosition[3];
  static DWORD           m_queueTime[3];
  static int             m_estimatedWaitTime;
  static CHARACTER_INFO *m_characterInfo;
};

#endif
