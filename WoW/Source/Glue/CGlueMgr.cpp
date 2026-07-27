#include "CGlueMgr.h"

#include <Base/Status.h>
#include <Event/EvtApi.h>
#include <Frame/CSimpleTop.h>
#include <FrameXML/FrameXML.h>
#include <FrameScript/FrameScript.h>
#include <Os/OsTime.h>
#include <Services/Profile.h>
#include <Services/AsyncFileRead.h>
#include <Services/SysMessage.h>
#include <Services/Texture.h>

#include "Client.h"
#include "Console/ConsoleCommand.h"
#include "DB/DBClient/DBClient.h"
#include "Glue/CharCreateInfo.h"
#include "Glue/CharSelectInfo.h"
#include "Glue/GlueScriptEvents.h"
#include "SoundInterface/SoundInterface.h"
#include "WowSvcs/WowSvcsClient/ClientServices.h"

static void LoadScriptFunctions();
static void UnloadScriptFunctions();
static int CCommand_Script(const char *command, const char *arguments);
void EnableLoadingScreen();

static const char REGKEY[11] = "WoW\\Client";
static const char REGVAL_ACCOUNTNAME[12] = "AccountName";
static const char REGVAL_LASTCHARACTER[14] = "LastCharacter";
static const char REGVAL_LASTACCOUNT[12] = "LastAccount";
static const char REGVAL_LASTREALM[10] = "LastRealm";

const char *g_glueBgObjNames[2] = {"CharacterAttachment", "PetAttachment"};

CSimpleTop               *CGlueMgr::m_simpleTop;
HMODEL                    CGlueMgr::m_cursorModel;
char                      CGlueMgr::m_currentScreen[64];
int                       CGlueMgr::m_reload;
int                       CGlueMgr::m_initialized;
int                       CGlueMgr::m_suspended;
int                       CGlueMgr::m_disconnectPending;
int                       CGlueMgr::m_reconnect;
CGlueMgr::GLUE_IDLE_STATE CGlueMgr::m_idleState;
int                       CGlueMgr::m_region;
WOW_LOCALE                CGlueMgr::m_locale;
char                      CGlueMgr::m_accountName[64];
char                      CGlueMgr::m_password[64];
unsigned int              CGlueMgr::m_queuePosition[3];
unsigned long             CGlueMgr::m_queueTime[3];
int                       CGlueMgr::m_estimatedWaitTime;
CHARACTER_INFO           *CGlueMgr::m_characterInfo;
static unsigned __int64   s_loginGUID;

static void LoadScriptFunctions() {
  RegisterSimpleFrameScriptMethods();
  GlueScriptEventsRegisterFunctions();
  CharSelectRegisterScriptFunctions();
  CharCreateRegisterScriptFunctions();
  SoundRegisterScriptFunctions();
}

static void UnloadScriptFunctions() {
  UnregisterSimpleFrameScriptMethods();
  GlueScriptEventsUnregisterFunctions();
  CharSelectUnregisterScriptFunctions();
  CharCreateUnregisterScriptFunctions();
  SoundUnregisterScriptFunctions();
}

void CGlueMgr::Initialize() {
  HPROFILE profile;
  char     language[16];
  char     country[16];
  char     locale[5];

  m_initialized = 1;
  Resume();

  profile = ProfileCreate();
  if (ProfileReadFile(profile, "Wow.ini")) {
    ProfileGetValue(profile, "WoW Config", "Region", &m_region, 0);

    if (ProfileGetValue(profile, "WoW Config", "Language", language, sizeof(language), 0) &&
        ProfileGetValue(profile, "WoW Config", "Country", country, sizeof(country), 0))
    {
      SStrPrintf(locale, sizeof(locale), "%s%s", language, country);

      for (WOW_LOCALE candidate = LOCALE_en_US; candidate < NUM_LOCALES; candidate = static_cast<WOW_LOCALE>(candidate + 1)) {
        if (!SStrCmpI(locale, g_localeID[candidate], 0x7FFFFFFF)) {
          m_locale = candidate;
          break;
        }
      }
    }
  }

  ProfileClose(profile);
  CURRENT_LANGUAGE = m_locale;
  EventRegisterEx(EVENT_ID_IDLE, Idle, 0, EVENT_PRIORITY_NORMAL);
}

void CGlueMgr::Resume() {
  m_disconnectPending = 0;
  m_reconnect = 0;
  m_idleState = IDLE_NONE;
  m_characterInfo = 0;
  m_suspended = 0;

  m_simpleTop = NEW(CSimpleTop);
  InitCursor();
  m_simpleTop->SetCursor(m_cursorModel);

  FrameScript_Flush();
  FrameScript_LoadTextTables("Interface\\GlueXML\\GlueStrings.lua");
  LoadScriptFunctions();
  FrameScript_CreateEvents(g_glueScriptEvents, 11);
  CCharSelectInfo::Initialize();
  CCharCreateInfo::Initialize();

  CWOWClientStatus status("GlueXML.log");
  FrameXML_CreateFrames("Interface\\GlueXML\\GlueXML.toc", &status);
  RegisterConsoleCommands();
}

void CGlueMgr::Suspend() {
  m_suspended = 1;
  CCharSelectInfo::Shutdown();
  CCharCreateInfo::Shutdown();

  if (m_simpleTop) {
    DEL(m_simpleTop);
    m_simpleTop = 0;
  }

  DestroyCursor();
  UnloadScriptFunctions();
  UnregisterConsoleCommands();
}

void CGlueMgr::Shutdown() {
  Suspend();
  m_initialized = 0;
  EventUnregister(EVENT_ID_IDLE, Idle);
}

void CGlueMgr::InitCursor() {
  CStatus status;

  m_cursorModel = ModelCreate(ClientDBStringLookup(SLOOKUP_DEFAULTCURSOR), 0, &status);
  SysMsgAdd(status, 4);
}

void CGlueMgr::DestroyCursor() {
  if (m_cursorModel) {
    HandleClose(m_cursorModel);
    m_cursorModel = 0;
  }
}

void CGlueMgr::UpdateWaitQueue(unsigned int wait) {
  if (wait != m_queuePosition[0] || m_queueTime[2] <= 0) {
    for (unsigned int i = 1; i > 0; --i) {
      m_queuePosition[i + 1] = m_queuePosition[i];
      m_queueTime[i + 1] = m_queueTime[i];
    }

    m_queuePosition[0] = wait;
  }

  m_queueTime[0] = OsGetAsyncTimeMs();

  if (m_queueTime[2] && static_cast<int>(m_queuePosition[0] - m_queuePosition[2]) < 0) {
    m_estimatedWaitTime = -(static_cast<int>(m_queueTime[0] - m_queueTime[2]) / static_cast<int>(m_queuePosition[0] - m_queuePosition[2]) * wait);
  }
}

void CGlueMgr::DefaultServerLogin() {
  if (m_idleState != IDLE_NONE) {
    return;
  }

  SStrCopy(m_accountName, ClientServices_GetAccountName(), sizeof(m_accountName));

  char lastAccount[64] = "";
  SRegLoadString(REGKEY, REGVAL_ACCOUNTNAME, 0, lastAccount, sizeof(lastAccount));
  m_idleState = IDLE_ACCOUNT_LOGIN;
  FrameScript_SignalEvent(3, "%s", "CANCEL");
  ClientServices_Connect();
}

void CGlueMgr::ChangeRealm(const REALM_INFO *info) {
  if (!info) {
    return;
  }

  ClientServices_SelectRealm(info->name, info->address);
  if (ClientServices_IsConnected()) {
    m_disconnectPending = 1;
    m_reconnect = 1;
    ClientServices_Disconnect();
  } else {
    m_idleState = IDLE_ACCOUNT_LOGIN;
    FrameScript_SignalEvent(3, "%s", "CANCEL");
    ClientServices_Connect();
  }
}

void CGlueMgr::CreateCharacter(const CHARACTER_CREATE_INFO *info) {
  if (!info) {
    return;
  }

  m_idleState = IDLE_CREATE_CHARACTER;
  FrameScript_SignalEvent(3, "%s", "CANCEL");
  ClientServices_CharacterCreate(*info);
}

void CGlueMgr::DeleteCharacter(unsigned __int64 guid) {
  if (guid) {
    m_idleState = IDLE_DELETE_CHARACTER;
    FrameScript_SignalEvent(3, "%s", "CANCEL");
    ClientServices_CharacterDelete(guid);
  }
}

void CGlueMgr::EnterWorld() {
  m_characterInfo = CCharSelectInfo::GetSelectedCharacterInfo();
  if (!m_characterInfo || !ClientServices_IsConnected()) {
    return;
  }

  SRegSaveValue(REGKEY, REGVAL_LASTCHARACTER, 0, CCharSelectInfo::m_selectionIndex);
  SRegSaveString(REGKEY, REGVAL_LASTACCOUNT, 0, m_accountName);
  SRegSaveString(REGKEY, REGVAL_LASTREALM, 0, ClientServices_GetSelectedRealmAddress());
  ModelCacheFlush();
  TextureCacheFlush();
  TextureGxCacheFlush();
  EnableLoadingScreen();
  m_idleState = IDLE_ENTER_WORLD;
}

void CGlueMgr::StatusDialogClick() {
  switch (m_idleState) {
    case IDLE_NONE:
      ClientServices_Cleanup();
      break;

    case IDLE_CHARACTER_LIST:
      ClientServices_Cancel();
      m_idleState = IDLE_NONE;
      SetScreen("login");
      break;

    case IDLE_ACCOUNT_LOGIN:
    case IDLE_CREATE_CHARACTER:
    case IDLE_DELETE_CHARACTER:
    case IDLE_ENTER_WORLD:
      ClientServices_Cancel();
      m_idleState = IDLE_NONE;
      break;

    case IDLE_WORLD_LOGIN:
      m_idleState = IDLE_NONE;
      GetCharacterList();
      break;
  }
}

void CGlueMgr::SetScreen(const char *screen) {
  FrameScript_SignalEvent(0, "%s", screen);
}

void CGlueMgr::UpdateCurrentScreen(const char *screen) {
  SStrCopy(m_currentScreen, screen, 64);
}

void CGlueMgr::QuitGame() {
  EventPostClose();
}

void CGlueMgr::GetCharacterList() {
  if (m_idleState == IDLE_WORLD_LOGIN) {
    return;
  }

  m_idleState = IDLE_CHARACTER_LIST;
  FrameScript_SignalEvent(3, "%s%s", "CANCEL", FrameScript_GetText("CHAR_LIST_RETRIEVING", -1, GENDER_NOT_APPLICABLE));
  ClientServices_GetCharacterList();
}

void CGlueMgr::GetRealmList() {
  m_idleState = IDLE_REALM_LIST;
  FrameScript_SignalEvent(3, "%s%s", "CANCEL", FrameScript_GetText("REALM_LIST_IN_PROGRESS", -1, GENDER_NOT_APPLICABLE));
  ClientServices_GetRealmList();
}

int CGlueMgr::Idle(const void *, void *) {
  NTempest::C3Vector position;
  WOWCS_OPS          op;
  const char        *msg;
  int                result;
  int                errorCode;

  if (m_idleState == IDLE_NONE) {
    if (m_reload) {
      if (!m_suspended) {
        Suspend();
        Resume();
        SetScreen(m_currentScreen);
      }

      m_reload = 0;
    }

    return 1;
  }

  int statusComplete = ClientServices_PollStatus(op, msg, result, errorCode);

  if (m_idleState == IDLE_ACCOUNT_LOGIN) {
    if (errorCode == 27 && m_estimatedWaitTime > 0) {
      char msgBuf[512];
      char timeBuf[256];
      int  remaining = m_queueTime[0] + m_estimatedWaitTime - OsGetAsyncTimeMs();

      if (remaining && (remaining = m_queueTime[0] + m_estimatedWaitTime - OsGetAsyncTimeMs()) >= 60000) {
        SStrPrintf(timeBuf, sizeof(timeBuf), FrameScript_GetText("QUEUE_TIME_LEFT", -1, GENDER_NOT_APPLICABLE), remaining / 60000);
      } else {
        SStrPrintf(timeBuf, sizeof(timeBuf), FrameScript_GetText("QUEUE_TIME_LEFT_SECONDS", -1, GENDER_NOT_APPLICABLE));
      }

      SStrPrintf(msgBuf, sizeof(msgBuf), "%s\n%s", msg, timeBuf);
      FrameScript_SignalEvent(4, "%s", msgBuf);
    } else {
      FrameScript_SignalEvent(4, "%s", msg);
    }

    if (!statusComplete) {
      return 1;
    }

    if (!result) {
      FrameScript_SignalEvent(3, "%s%s", "OKAY", msg);
      m_idleState = IDLE_NONE;
      SetScreen("login");
      return 1;
    }

    if (op == COP_CONNECT) {
      ClientServices_AccountLogin(m_accountName, "", m_region, m_locale);
      return 1;
    }

    ASSERT(op == COP_AUTHENTICATE || op == COP_WAIT_QUEUE);

    SRegSaveString(REGKEY, REGVAL_ACCOUNTNAME, 0, m_accountName);

    char lastAccount[64] = "";
    SRegLoadString(REGKEY, REGVAL_LASTACCOUNT, 0, lastAccount, sizeof(lastAccount));

    if (SStrCmpI(lastAccount, m_accountName, 0x7FFFFFFF)) {
      SRegSaveValue(REGKEY, REGVAL_LASTCHARACTER, 0, 0);
    }

    m_idleState = IDLE_NONE;
    FrameScript_SignalEvent(5);
    CCharSelectInfo::SelectCharacter(-1);
    SetScreen("charselect");
    return 1;
  }

  if (m_idleState == IDLE_CHARACTER_LIST) {
    ASSERT(op == COP_GET_CHARACTERS);
    FrameScript_SignalEvent(4, "%s", msg);

    if (!statusComplete) {
      return 1;
    }

    if (!result) {
      FrameScript_SignalEvent(3, "%s%s", "OKAY", msg);
    } else {
      m_idleState = IDLE_NONE;
      FrameScript_SignalEvent(5);
      CCharSelectInfo::UpdateCharacterList();
      return 1;
    }
  } else if (m_idleState == IDLE_REALM_LIST) {
    FrameScript_SignalEvent(4, "%s", msg);

    if (!statusComplete) {
      return 1;
    }

    if (!result) {
      FrameScript_SignalEvent(3, "%s%s", "OKAY", msg);
    } else {
      ASSERT(op == COP_GET_REALMS);
      m_idleState = IDLE_NONE;
      FrameScript_SignalEvent(5);
      FrameScript_SignalEvent(8);
      return 1;
    }
  } else if (m_idleState == IDLE_CREATE_CHARACTER) {
    FrameScript_SignalEvent(4, "%s", msg);

    if (!statusComplete) {
      return 1;
    }

    if (!result) {
      FrameScript_SignalEvent(3, "%s%s", "OKAY", msg);
    } else {
      ASSERT(op == COP_CREATE_CHARACTER);
      m_idleState = IDLE_NONE;
      FrameScript_SignalEvent(5);
      FrameScript_SignalEvent(10);
      SetScreen("charselect");
      return 1;
    }
  } else {
    if (m_idleState == IDLE_DELETE_CHARACTER) {
      FrameScript_SignalEvent(4, "%s", msg);

      if (!statusComplete) {
        return 1;
      }

      CCharSelectInfo::SelectCharacter(0);
      GetCharacterList();
      return 1;
    }

    if (m_idleState == IDLE_ENTER_WORLD) {
      if (!DrawingLoadingScreen()) {
        return 1;
      }

      if (!m_suspended) {
        s_loginGUID = m_characterInfo->guid;
        unsigned int mapID = m_characterInfo->mapID;
        position = m_characterInfo->position;
        Suspend();
        ClientInitializeGame(mapID, position);
        return 1;
      }

      if (AsyncFileReadIsReading()) {
        return 1;
      }

      m_idleState = IDLE_NONE;
      SndInterfaceSetGlueMusic(0);
      ClientServices_CharacterLogin(s_loginGUID, 0, 0.0f);
      return 1;
    }

    if (m_idleState == IDLE_WORLD_LOGIN) {
      FrameScript_SignalEvent(3, "%s%s", "OKAY", msg);
    }

    return 1;
  }

  m_idleState = IDLE_NONE;
  return 1;
}

int CGlueMgr::NetDisconnectHandler(const void *eventData, void *__formal) {
  WOWCS_OPS   op;
  int         errorCode;
  const char *msg;
  int         result;
  bool        notAccountLogin = m_idleState != IDLE_ACCOUNT_LOGIN;

  m_idleState = IDLE_NONE;

  if (m_disconnectPending) {
    m_disconnectPending = 0;

    if (m_reconnect) {
      m_reconnect = 0;
      m_idleState = IDLE_ACCOUNT_LOGIN;
      FrameScript_SignalEvent(3, "%s", "CANCEL");
      ClientServices_Connect();
      return 1;
    }
  } else if (ClientServices_ValidDisconnect(eventData)) {
    ClientServices_Disconnected();
    ClientDestroyGame(0, 1, 0);
    EventSetMouseMode(MOUSE_MODE_NORMAL, 0);

    if (m_suspended) {
      Resume();
    }

    if (!notAccountLogin && ClientServices_PollStatus(op, msg, result, errorCode) && !result) {
      FrameScript_SignalEvent(3, "%s%s", "OKAY", msg);
      return 1;
    }

    FrameScript_SignalEvent(2);
  }

  return 1;
}

static int CCommand_Script(const char *command, const char *arguments) {
  FrameScript_Execute(arguments, arguments);
  return 1;
}

void CGlueMgr::RegisterConsoleCommands() {
  ConsoleCommandRegister("script", CCommand_Script, DEFAULT, 0);
}

void CGlueMgr::UnregisterConsoleCommands() {
  ConsoleCommandUnregister("script");
}
