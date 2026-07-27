#include <Event/EvtApi.h>
#include <Base/Base.h>
#include <Base/CDataStore.h>
#include <Base/CmdLine.h>
#include <Base/Status.h>
#include <Gx/Gx.h>
#include <Gxu/IGxuLight.h>
#include <FrameScript/FrameScript.h>
#include <Os/W32/Debugging.h>
#include <Os/W32/OsGui.h>
#include <Os/W32/OsFile.h>
#include <Os/W32/OsMemory.h>
#include <Os/OsTime.h>
#include <Scrn/Scrn.h>
#include <Services/AsyncFileRead.h>
#include <Services/SysMessage.h>
#include <Tempest/c3vector.h>
#include <Tempest/crandom.h>
#include <storm.h>

#include "Console/ConsoleClient.h"
#include "Console/ConsoleVar.h"
#include "Client.h"
#include "DB/DBClient/DBCacheInstances.h"
#include "DB/DBClient/AutoCode/MapRec.h"
#include "Game/GameTime.h"
#include "Glue/CGlueMgr.h"
#include "Object/MovementData.h"
#include "Object/ObjectClient/Item_C.h"
#include "Object/ObjectClient/GameObject_C.h"
#include "Object/ObjectClient/Object_C.h"
#include "Object/ObjectClient/Player_C.h"
#include "Object/ObjectClient/Unit_C.h"
#include "Object/ObjectClient/ZoneDebug.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "UIUtil/Tooltip.h"
#include "SoundInterface/SoundInterface.h"
#include "Ui/ChatFrame.h"
#include "Ui/GameUI.h"
#include "UIUtil/InputControl.h"
#include "WowSvcs/WowSvcsClient/ClientConnection.h"
#include "WowSvcs/WowSvcsClient/ClientServices.h"
#include "WowSvcs/WowSvcsClient/FriendList.h"
#include "WorldClient/AreaList.h"
#include "WorldClient/World.h"

#include <stdio.h>
#include <string.h>

int CDataStore::IsRead() const {
  return m_read == m_size;
}

void CDataStore::Reset() {
  if (m_alloc == static_cast<unsigned int>(-1)) {
    m_data = 0;
    m_alloc = 0;
  }

  FetchWrite(0, 0, 0, 0);
  m_size = 0;
  m_read = static_cast<unsigned int>(-1);
}

void CDataStore::Finalize() {
  ASSERT(!IsFinal());
  m_read = 0;
}

void CStatus::Display() const {
}

#if defined(_MSC_VER) && _MSC_VER == 1200
void __cdecl operator delete(void *ptr) {
  if (ptr) {
    SMemFree(ptr, "delete", -1, 0);
  }
}

void *__cdecl operator new(size_t bytes) {
  return SMemAlloc(bytes, "new", -1, 0);
}
#endif

typedef void(*SPROCESSCOMPLETIONPROC)(void *);

unsigned int ClientSetTimer(unsigned int timeout, CLIENTTIMERHANDLER handler, void *param) {
  return EventSetTimer(timeout, handler, param);
}

unsigned int ClientSetTimer(unsigned int timeout, CLIENTGUIDTIMERHANDLER handler, unsigned __int64 guid, void *param) {
  return EventSetTimer(timeout, handler, guid, param);
}

static int ReceiveObjectRotation(void *__formal, NETMESSAGE msgId, unsigned long time, CDataStore *msg) {
  float facing;
  float anchorfacing;

  msg->Get(facing);
  msg->Get(anchorfacing);
  ConsoleWriteA("facing: %g degrees, anchor: %g degrees", DEFAULT_COLOR, facing * 57.29578f, anchorfacing * 57.29578f);
  return 1;
}

void OsIMEInitialize();
void OsIMEDestroy();
void TextureInitialize();
void TextureDestroy();
void ModelInitialize();
void ModelDestroy();
void ObjectAllocInitialize();
void ObjectAllocDestroy();
void ClientDBInitialize();
void ClientDBShutdown();
void ComponentInitialize();
void ComponentShutdown();
int FrameXML_RegisterDefault();
void FrameXML_ClearFactories();
void GlueScriptEventsInitialize();
void ScriptEventsInitialize();
void CharCustomizationInitialize();
void CharCustomizationShutdown();
void CameraInitialize();
void CameraDestroy();
void SpellVisualsInitialize();
void SpellVisualsShutdown();
void SpellTableInitialize();
void SpellTableDestroy();
void PlayerNameInitialize();
void PlayerNameShutdown();
void WeaponTrailsInitialize();
void WeaponTrailsShutdown();
void ShadowInit();
void ShadowDestroy();
void LootInitialize();
void LootDestroy();
void TaxiMapInitialize();
void TaxiMapShutdown();
void Trade_C_Initialize();
void Trade_C_Destroy();
void SmartScreenRectClearAllGrids();
void ValidateNameInitialize();
void ValidateNameDestroy();
void ViolenceLevelsInitialize();
void ViolenceLevelsShutdown();
void InstallGameConsoleCommands();
void UninstallGameConsoleCommands();
void WorldTextClearStrings();
int SCreateProcess(const char *applicationName, char *commandLine, SPROCESSCOMPLETIONPROC completionProc, void *completionParam);
int ModelCacheUpdate(DWORD currentTime, CStatus *status);
void TextureCacheUpdate(DWORD currentTime, CStatus *status);
void OsGetExePath(char *buffer, DWORD chars);
NTempest::CRndSeed g_rndSeed;
CVar              *g_realmNameVar;
CVar              *g_realmAddressVar;
HEVENTCONTEXT      g_clientEventContext;

const char *const WOW_ERROR_LOG_TITLE = "World of WarCraft: Assertions Enabled Build (build 3368)";

static const ARGLIST s_wowArgList[17] = {
    {SCMD_TYPE_BOOL, 15,     "640x480", 0},
    {SCMD_TYPE_BOOL, 16,     "800x600", 0},
    {SCMD_TYPE_BOOL, 17,    "1024x768", 0},
    {SCMD_TYPE_BOOL, 18,    "1280x960", 0},
    {SCMD_TYPE_BOOL, 19,   "1280x1024", 0},
    {SCMD_TYPE_BOOL, 20,   "1600x1200", 0},
    {SCMD_TYPE_BOOL, 22,       "16bit", 0},
    {SCMD_TYPE_BOOL, 21,    "uptodate", 0},
    {SCMD_TYPE_BOOL, 26,     "nosound", 0},
    {SCMD_TYPE_BOOL, 24,    "nofixlag", 0},
    {SCMD_TYPE_BOOL, 28,         "d16", 0},
    {SCMD_TYPE_BOOL, 29,         "d24", 0},
    {SCMD_TYPE_BOOL, 30,         "d32", 0},
    {SCMD_TYPE_BOOL, 31,    "windowed", 0},
    {SCMD_TYPE_BOOL, 36,    "hwdetect", 0},
    {SCMD_TYPE_BOOL, 34,     "skiptos", 0},
    {SCMD_TYPE_BOOL, 35, "keepsession", 0}
};

static const char *const s_archiveNames[8] = {"Data\\model.MPQ",     "Data\\texture.MPQ", "Data\\sound.MPQ",  "Data\\misc.MPQ",
                                              "Data\\interface.MPQ", "Data\\fonts.MPQ",   "Data\\speech.MPQ", "Data\\dbc.MPQ"};

static SArchive          *s_archive[8];
static LoginData          s_loginData;
static CVar              *s_errorFileCvar;
static CVar              *s_errorsCvar;
static CVar              *s_minErrorLevelCvar;
static CVar              *s_maxErrorLevelCvar;
static CVar              *s_errorFilterCvar;
static CVar              *s_debugTargetInfoCvar;
static CVar              *s_debugShowGUIDsCvar;
static CVar              *s_desktopGammaCvar;
static CVar              *s_gammaCvar;
static CVar              *s_profanityFilterCvar;
static unsigned int       s_cacheUpdateTimerHandle;
static int                clientGameInitialized;
static unsigned int       s_newZoneID;
static NTempest::C3Vector s_newPosition;
static float              s_newFacing;
static const char        *s_newMapname;

static int CacheUpdateHandler(const void *eventData, void *arg);
static void CacheUpdateInitialize();
static void CacheUpdateShutdown();
static void ClientRegisterConsoleCommands();
static void ClientUnregisterConsoleCommands();
static int ReceiveObjectRotation(void *__formal, NETMESSAGE msgId, unsigned long time, CDataStore *msg);
static int ClientChatHandler(void *__formal, NETMESSAGE msgID, unsigned long timestamp, CDataStore *msg);
static int ChannelNotifyHandler(void *param, NETMESSAGE msgId, unsigned long timestamp, CDataStore *msg);
static int ClientTextEmoteHandler(void *__formal, NETMESSAGE msgID, unsigned long timestamp, CDataStore *msg);
static int ClientChannelListHandler(void *param, NETMESSAGE msgId, unsigned long timestamp, CDataStore *msg);
static int MovementLoggingHandler(void *param, NETMESSAGE msgID, unsigned long time, CDataStore *msg);
static int MovementFallLoggingHandler(void *param, NETMESSAGE msgId, unsigned long time, CDataStore *msg);
static int LookupResultsHandler(void *__formal, NETMESSAGE msgID, unsigned long timestamp, CDataStore *msg);
static int ReceiveObjectPosition(void *__formal, NETMESSAGE msgId, unsigned long time, CDataStore *msg);
static int PlayedTimeHandler(void *param, NETMESSAGE msgId, unsigned long timestamp, CDataStore *msg);
static void FormatTime(char *buf, int len, int secs);
static int NotifyHandler(void *__formal, NETMESSAGE msgID, unsigned long timestamp, CDataStore *msg);
static int TransferAbortedHandler(void *param, NETMESSAGE msgId, unsigned long timestamp, CDataStore *msg);
static int TransferPendingHandler(void *param, NETMESSAGE msgId, unsigned long timestamp, CDataStore *msg);
void MovementInit();
static int LoadNewWorld(const void *eventData, void *param);
static int NewWorldHandler(void *__formal, NETMESSAGE msgID, unsigned long timestamp, CDataStore *msg);
static int ClientIdle(const void *data, void *__formal);
static int ClientFocus(const void *packetData, void *__formal);

typedef unsigned __int64(*GETLOCALTARGETPROC)(CGPlayer_C *);

static bool ErrorDisplayCallback(CVar *h, const char *oldValue, const char *newValue, void *arg);
static bool ErrorDisplayMinLevelCallback(CVar *h, const char *oldValue, const char *newValue, void *arg);
static bool ErrorDisplayMaxLevelCallback(CVar *h, const char *oldValue, const char *newValue, void *arg);
static bool ErrorDisplayFilterCallback(CVar *h, const char *oldValue, const char *newValue, void *arg);
static bool DebugTargetInfoCallback(CVar *h, const char *oldValue, const char *newValue, void *arg);
static bool DebugShowGUIDsCallback(CVar *h, const char *oldValue, const char *newValue, void *arg);
static bool GammaCallback(CVar *h, const char *oldValue, const char *newValue, void *arg);
static bool DesktopGammaCallback(CVar *h, const char *oldValue, const char *newValue, void *arg);
static bool ProfanityFilterCallback(CVar *h, const char *oldValue, const char *newValue, void *arg);

static void DisplayErrorLevelStatus();
static void PrintFilterMask();
static int SetFilterMask(const char *filterString);

static int CCommand_ReloadUI(const char *command, const char *arguments);
static int CCommand_ToggleLighting(const char *command, const char *arguments);
static int CCommand_ToggleFog(const char *command, const char *arguments);
static int CCommand_ToggleDepthTesting(const char *command, const char *arguments);
static int CCommand_ToggleDepthSetting(const char *command, const char *arguments);
static int CCommand_ToggleCulling(const char *command, const char *arguments);
static int CCommand_ToggleDblBuffer(const char *command, const char *arguments);
static int CCommand_SetResolutionXY(const char *command, const char *arguments);
static int CCommand_SetResolutionMode(const char *command, const char *arguments);
static int CCommand_SetColorDepth(const char *command, const char *arguments);
static int CCommand_SetAPI(const char *command, const char *arguments);
static int CCommand_Bug(const char *command, const char *args);

static int ClientChatHandler(void *__formal, NETMESSAGE msgID, unsigned long timestamp, CDataStore *msg) {
  return CGChat::ChatHandler(msg);
}

static int ChannelNotifyHandler(void *, NETMESSAGE, unsigned long, CDataStore *msg) {
  CGChat::ChannelNotify(msg);
  return 1;
}

static int ClientTextEmoteHandler(void *__formal, NETMESSAGE msgID, unsigned long timestamp, CDataStore *msg) {
  return CGChat::HandleTextEmote(msg);
}

static int ClientChannelListHandler(void *, NETMESSAGE, unsigned long, CDataStore *msg) {
  CGChat::ChannelList(msg);
  return 1;
}

static int DebugAIStateHandler(void *, NETMESSAGE msgID, unsigned long timestamp, CDataStore *msg) {
  CGTooltip *tooltip = CGGameUI::m_gameTooltip;
  unsigned __int64 unit;
  msg->Get(unit);
  if (tooltip && tooltip->GetDebugUnit() != unit) {
    tooltip = 0;
  }

  int count;
  msg->Get(count);
  for (int i = 0; i < count; ++i) {
    char string[128];
    msg->GetString(string, sizeof(string));
    if (tooltip) {
      tooltip->AddLine(string, 0, 0);
    }
  }

  if (tooltip) {
    tooltip->CalculateSize();
    tooltip->SetDebugUnit(0);
  }
  return 1;
}

static int MovementFallLoggingHandler(void *param, NETMESSAGE msgId, unsigned long time, CDataStore *msg) {
  if (CMovement::ToggleFallLogging()) {
    SysMsgAdd("MOVEMENT|Movement fall logging started", SYSMSG_INFO, 1);
    if (CGUnit_C::m_activeMover) {
      CMovement::FallLogWrite("Local mover guid (0x%016I64X)\n", CGUnit_C::m_activeMover);
    }

    unsigned __int64 guid = ClntObjMgrGetActivePlayer();
    CGObject_C      *object = ClntObjMgrObjectPtr(guid, __FILE__, __LINE__);
    if (object) {
      unsigned __int64 target = reinterpret_cast<CGPlayer_C *>(object)->CGPlayer_C::GetLocalTarget();
      CMovement::FallLogWrite("Local target guid (0x%016I64X)\n", target);
    }
  } else {
    SysMsgAdd("MOVEMENT|Movement fall logging stopped", SYSMSG_INFO, 1);
  }

  return 1;
}

static int MovementLoggingHandler(void *param, NETMESSAGE msgID, unsigned long time, CDataStore *msg) {
  if (CMovement::ToggleLogging()) {
    SysMsgAdd("MOVEMENT|Movement logging started", SYSMSG_INFO, 1);
    if (CGUnit_C::m_activeMover) {
      CMovement::LogWrite("Local mover guid (0x%016I64X)\n", CGUnit_C::m_activeMover);
    }
  } else {
    SysMsgAdd("MOVEMENT|Movement logging stopped", SYSMSG_INFO, 1);
  }

  return 1;
}

static int LookupResultsHandler(void *__formal, NETMESSAGE msgID, unsigned long timestamp, CDataStore *msg) {
  unsigned int numResults;

  msg->Get(numResults);
  if (!numResults) {
    ConsoleWrite("No results", DEFAULT_COLOR);
    return 1;
  }

  void *results;
  msg->GetDataInSitu(results, numResults * 0xC4);
  ASSERT(results);

  char *result = static_cast<char *>(results) + 68;
  for (unsigned int i = 0; i < numResults; ++i, result += 0xC4) {
    unsigned int id = *reinterpret_cast<unsigned int *>(result - 68);
    ConsoleWriteA("[%.04d] \"%s\" \"%s\" %s", DEFAULT_COLOR, id, result - 64, result, result + 64);
    OsOutputDebugString("[%.04d] \"%s\" \"%s\" %s\n", id, result - 64, result, result + 64);
  }

  return 1;
}

static int ReceiveObjectPosition(void *__formal, NETMESSAGE msgId, unsigned long time, CDataStore *msg) {
  NTempest::C3Vector position;

  msg->Get(position.x);
  msg->Get(position.y);
  msg->Get(position.z);
  ConsoleWriteA("%g, %g, %g", DEFAULT_COLOR, position.x, position.y, position.z);
  return 1;
}

static void FormatTime(char *buf, int len, int secs) {
  int days = secs / 86400;
  secs -= days * 86400;
  int hours = secs / 3600;
  secs -= hours * 3600;
  int minutes = secs / 60;
  secs -= minutes * 60;

  SStrPrintf(buf, len, "%dd %dh %dm %ds", days, hours, minutes, secs);
}

static int PlayedTimeHandler(void *, NETMESSAGE, unsigned long, CDataStore *msg) {
  int  totalTime;
  int  levelTime;
  char buf[256];

  msg->Get(totalTime);
  msg->Get(levelTime);
  ConsolePrintf("Time played:");
  FormatTime(buf, sizeof(buf), totalTime);
  ConsolePrintf("Total: %s", buf);
  FormatTime(buf, sizeof(buf), levelTime);
  ConsolePrintf("Level: %s", buf);
  FrameScript_SignalEvent(0xF3u, "%d%d", totalTime, levelTime);
  return 1;
}

static int TransferPendingHandler(void *, NETMESSAGE, unsigned long, CDataStore *msg) {
  CGGameUI::ClearClientControls();
  ConsolePrintf("World transfer pending...");
  EnableLoadingScreen();
  return 1;
}

static int TransferAbortedHandler(void *, NETMESSAGE, unsigned long, CDataStore *msg) {
  ConsolePrintf("World transfer aborted...");
  DisableLoadingScreen();
  return 1;
}

void MovementInit() {
  char buffer[260];

  if (!SRegLoadString("Wow\\Client", "MoveLogFile", 0, buffer, sizeof(buffer)) || !buffer[0]) {
    SRegSaveString("Wow\\Client", "MoveLogFile", 0, "ClientMovement.txt");
    SStrCopy(buffer, "ClientMovement.txt", sizeof(buffer));
  }

  MovementInitialize(buffer, true);
  EventRegisterEx(EVENT_ID_IDLE, MovementIdleMoveUnits, 0, 2.0f);
}

static int LoadNewWorld(const void *eventData, void *param) {
  ClientServices_CharacterSetInGame(0);

  CWorld::UnloadMap();
  MovementDestroy();
  EventUnregister(EVENT_ID_IDLE, MovementIdleMoveUnits);
  ClntObjMgrDestroy();
  ClntObjMgrDestruct(ClntObjMgrGetCurrent());

  ClntObjMgr *mgr = ClntObjMgrCreate(PLAYER_NORMAL, 0);
  ClntObjMgrSetCurrent(mgr);
  g_clientConnection->SetObjMgr(mgr);
  ClntObjMgrSetNet(g_clientConnection);
  ClntObjMgrSetCurrent(mgr);
  ClntObjMgrInitialize();
  MovementInit();
  ClntObjMgrSetMapID(s_newZoneID);

  CWorld::LoadMap(s_newMapname, s_newPosition, 1);

  CDataStore msg;
  msg.Put(MSG_MOVE_WORLDPORT_ACK);
  msg.Finalize();
  ClientServices_Send(&msg);
  LoadingScreenRegisterWorldLoaded();
  return 1;
}

static int NewWorldHandler(void *__formal, NETMESSAGE msgID, unsigned long timestamp, CDataStore *msg) {
  ASSERT(msgID == SMSG_NEW_WORLD);

  msg->Get(reinterpret_cast<unsigned char &>(s_newZoneID));
  msg->Get(s_newPosition.x);
  msg->Get(s_newPosition.y);
  msg->Get(s_newPosition.z);
  msg->Get(s_newFacing);

  if (!msg->IsRead()) {
    ConsoleWrite("Bad SMSG_NEW_WORLD\n", DEFAULT_COLOR);
    msg->Reset();
    return 1;
  }

  const MapRec *map = g_mapDB.GetRecord(s_newZoneID);
  if (!map) {
    ConsoleWrite("Bad SMSG_NEW_WORLD zoneID\n", DEFAULT_COLOR);
    return 0;
  }

  s_newMapname = map->m_Directory;
  EnableLoadingScreen();
  EventSetTimer(0u, LoadNewWorld, 0);
  return 1;
}

static bool ErrorDisplayCallback(CVar *h, const char *oldValue, const char *newValue, void *arg) {
  int enabled = SStrToInt(newValue);

  SysMsgEnable(enabled);
  ConsoleWrite(enabled ? "Error display enabled" : "Error display disabled", DEFAULT_COLOR);
  return true;
}

static void DisplayErrorLevelStatus() {
  char        buffer[80];
  SYSMSG_TYPE minLevel = SysMsgGetMinDisplayLevel();
  SYSMSG_TYPE maxLevel = SysMsgGetMaxDisplayLevel();

  if (minLevel == SYSMSG_INFO && maxLevel == SYSMSG_FATAL) {
    ConsoleWrite("Displaying all system messages", DEFAULT_COLOR);
    return;
  }

  SStrCopy(buffer, "Displaying ", sizeof(buffer));
  if (minLevel == maxLevel) {
    SStrPack(buffer, "only ", 0x7FFFFFFF);
  }

  switch (minLevel) {
    case SYSMSG_INFO:
      SStrPack(buffer, "informational messages", 0x7FFFFFFF);
      break;
    case SYSMSG_WARNING:
      SStrPack(buffer, "warnings", 0x7FFFFFFF);
      break;
    case SYSMSG_ERROR:
      SStrPack(buffer, "errors", 0x7FFFFFFF);
      break;
    case SYSMSG_FATAL:
      SStrPack(buffer, "fatal errors", 0x7FFFFFFF);
      break;
  }

  if (minLevel != maxLevel) {
    switch (maxLevel) {
      case SYSMSG_WARNING:
        SStrPack(buffer, " through warnings", 0x7FFFFFFF);
        break;
      case SYSMSG_ERROR:
        SStrPack(buffer, " through errors", 0x7FFFFFFF);
        break;
      case SYSMSG_FATAL:
        SStrPack(buffer, " through fatal errors", 0x7FFFFFFF);
        break;
    }
  }

  ConsoleWrite(buffer, DEFAULT_COLOR);
}

static bool ErrorDisplayMinLevelCallback(CVar *h, const char *oldValue, const char *newValue, void *arg) {
  int level = SStrToInt(newValue);

  if (level >= 0 && level < SYSMSG_NUMTYPES) {
    SysMsgSetMinDisplayLevel(static_cast<SYSMSG_TYPE>(level));
    DisplayErrorLevelStatus();
    return true;
  }

  ConsoleWriteA("%i is not valid, valid values are 0 - %i", DEFAULT_COLOR, level, SYSMSG_NUMTYPES - 1);
  return false;
}

static bool ErrorDisplayMaxLevelCallback(CVar *h, const char *oldValue, const char *newValue, void *arg) {
  int level = SStrToInt(newValue);

  if (level >= 0 && level < SYSMSG_NUMTYPES) {
    SysMsgSetMaxDisplayLevel(static_cast<SYSMSG_TYPE>(level));
    DisplayErrorLevelStatus();
    return true;
  }

  ConsoleWriteA("%i is not valid, valid values are 0 - %i", DEFAULT_COLOR, level, SYSMSG_NUMTYPES - 1);
  return false;
}

static void PrintFilterMask() {
  char         filters[80] = "";
  unsigned int filter = SysMsgGetFilter();

  if (filter == 0xFFFFFFFF) {
    ConsoleWrite("Now filtering: all messages", DEFAULT_COLOR);
    return;
  }

  if (filter & 0x01) {
    SStrPack(filters, "general ", sizeof(filters));
  }
  if (filter & 0x02) {
    SStrPack(filters, "world ", sizeof(filters));
  }
  if (filter & 0x04) {
    SStrPack(filters, "ui ", sizeof(filters));
  }
  if (filter & 0x08) {
    SStrPack(filters, "animation ", sizeof(filters));
  }
  if (filter & 0x10) {
    SStrPack(filters, "models ", sizeof(filters));
  }
  if (filter & 0x20) {
    SStrPack(filters, "objects ", sizeof(filters));
  }

  ConsolePrintf("Now filtering: %s", filters);
}

static int SetFilterMask(const char *filterString) {
  char         filter[64];
  char         whitespace[] = "\t\r\n\" ";
  const char  *string = filterString;
  int          invert = 0;
  unsigned int categoryFilter = 0;

  SStrTokenize(&string, filter, sizeof(filter), whitespace, 0);
  while (filter[0]) {
    switch (filter[0]) {
      case 'A':
      case 'a':
        if (!SStrCmpI(filter, "all", 0x7FFFFFFF)) {
          categoryFilter = invert ? 0 : 0xFFFFFFFF;
        } else if (!SStrCmpI(filter, "animation", 0x7FFFFFFF)) {
          if (invert) {
            categoryFilter &= ~0x08;
          } else {
            categoryFilter |= 0x08;
          }
        } else {
          goto unknownFilter;
        }
        break;

      case 'E':
      case 'e':
        if (SStrCmpI(filter, "except", 0x7FFFFFFF)) {
          goto unknownFilter;
        }
        invert = 1;
        break;

      case 'G':
      case 'g':
        if (SStrCmpI(filter, "general", 0x7FFFFFFF)) {
          goto unknownFilter;
        }
        if (invert) {
          categoryFilter &= ~0x01;
        } else {
          categoryFilter |= 0x01;
        }
        break;

      case 'W':
      case 'w':
        if (SStrCmpI(filter, "world", 0x7FFFFFFF)) {
          goto unknownFilter;
        }
        if (invert) {
          categoryFilter &= ~0x02;
        } else {
          categoryFilter |= 0x02;
        }
        break;

      case 'U':
      case 'u':
        if (SStrCmpI(filter, "ui", 0x7FFFFFFF)) {
          goto unknownFilter;
        }
        if (invert) {
          categoryFilter &= ~0x04;
        } else {
          categoryFilter |= 0x04;
        }
        break;

      case 'M':
      case 'm':
        if (SStrCmpI(filter, "models", 0x7FFFFFFF)) {
          goto unknownFilter;
        }
        if (invert) {
          categoryFilter &= ~0x10;
        } else {
          categoryFilter |= 0x10;
        }
        break;

      case 'O':
      case 'o':
        if (SStrCmpI(filter, "objects", 0x7FFFFFFF)) {
          goto unknownFilter;
        }
        if (invert) {
          categoryFilter &= ~0x20;
        } else {
          categoryFilter |= 0x20;
        }
        break;

      case 'S':
      case 's':
        if (SStrCmpI(filter, "objects", 0x7FFFFFFF)) {
          goto unknownFilter;
        }
        if (invert) {
          categoryFilter &= ~0x40;
        } else {
          categoryFilter |= 0x40;
        }
        break;

      default:
        goto unknownFilter;
    }

    SStrTokenize(&string, filter, sizeof(filter), whitespace, 0);
  }

  SysMsgSetFilter(categoryFilter);
  return 1;

unknownFilter:
  ConsolePrintf("Unknown filter %s", filter);
  ConsoleWrite("Filters: general world ui animation models objects all", DEFAULT_COLOR);
  ConsoleWrite("         use \"except\" to invert mask", DEFAULT_COLOR);
  ConsoleWrite("         i.e.: all except objects", DEFAULT_COLOR);
  return 0;
}

static bool ErrorDisplayFilterCallback(CVar *h, const char *oldValue, const char *newValue, void *arg) {
  if (!SetFilterMask(newValue)) {
    return false;
  }

  PrintFilterMask();
  return true;
}

static bool DebugTargetInfoCallback(CVar *h, const char *oldValue, const char *newValue, void *arg) {
  int enabled = SStrToInt(newValue);

  ConsoleWrite(enabled ? "Debug target tooltips enabled" : "Debug target tooltips disabled", DEFAULT_COLOR);
  return true;
}

static bool DebugShowGUIDsCallback(CVar *h, const char *oldValue, const char *newValue, void *arg) {
  int enabled = SStrToInt(newValue);

  ConsoleWrite(enabled ? "GUID tooltips enabled" : "GUID tooltips disabled", DEFAULT_COLOR);
  return true;
}

static bool ErrorFileLogCallback(CVar *h, const char *oldValue, const char *newValue, void *arg) {
  char        curDir[MAX_PATH];
  const char *dataDirectory;

  if (SStrToInt(newValue)) {
    dataDirectory = CmdLineGetString(CMD_DATA_DIR);
    if (dataDirectory && dataDirectory[0]) {
      SStrCopy(curDir, dataDirectory, 0x7FFFFFFF);
    } else {
      OsGetExePath(curDir, MAX_PATH);
    }

    SStrPack(curDir, "Logs.Client\\", MAX_PATH);
    SysMsgEnableFileLog(curDir);
  } else {
    SysMsgDisableFileLog();
  }

  return true;
}

static bool GammaCallback(CVar *h, const char *oldValue, const char *newValue, void *arg) {
  float v = SStrToFloat(newValue);

  if (s_desktopGammaCvar && !s_desktopGammaCvar->GetInt()) {
    GxDevSetGamma(v);
  }
  return true;
}

static bool DesktopGammaCallback(CVar *h, const char *oldValue, const char *newValue, void *arg) {
  CGxGammaRamp ramp;
  float        gamma;

  if (SStrToInt(newValue)) {
    GxDevSystemGammaRamp(ramp);
    GxDevSetGamma(ramp);
    return true;
  }

  gamma = s_gammaCvar ? s_gammaCvar->GetFloat() : 1.0f;
  GxDevSetGamma(gamma);
  return true;
}

static bool ProfanityFilterCallback(CVar *h, const char *oldValue, const char *newValue, void *arg) {
  int enabled = SStrToInt(newValue);

  ConsoleWrite(enabled ? "Profanity filter enabled" : "Profanity filter disabled", DEFAULT_COLOR);
  CGChat::FilterChat(enabled);
  return true;
}

static int CCommand_ReloadUI(const char *command, const char *arguments) {
  CGlueMgr::Reload();
  CGGameUI::Reload();
  return 1;
}

static int CCommand_ToggleLighting(const char *command, const char *arguments) {
  int enabled = SStrToInt(arguments);

  GxMasterEnableSet(GxMasterEnable_Lighting, enabled);
  ConsoleWrite(enabled ? "Lighting enabled" : "Lighting disabled", DEFAULT_COLOR);
  return 1;
}

static int CCommand_ToggleFog(const char *command, const char *arguments) {
  int enabled = SStrToInt(arguments);

  GxMasterEnableSet(GxMasterEnable_Fog, enabled);
  ConsoleWrite(enabled ? "Fog enabled" : "Fog disabled", DEFAULT_COLOR);
  return 1;
}

static int CCommand_ToggleDepthTesting(const char *command, const char *arguments) {
  int enabled = !GxMasterEnable(GxMasterEnable_DepthTest);

  GxMasterEnableSet(GxMasterEnable_DepthTest, enabled);
  ConsoleWrite(enabled ? "Depth Testing enabled" : "Depth Testing disabled", DEFAULT_COLOR);
  return 1;
}

static int CCommand_ToggleDepthSetting(const char *command, const char *arguments) {
  int enabled = !GxMasterEnable(GxMasterEnable_DepthWrite);

  GxMasterEnableSet(GxMasterEnable_DepthWrite, enabled);
  ConsoleWrite(enabled ? "Depth Writing enabled" : "Depth Writing disabled", DEFAULT_COLOR);
  return 1;
}

static int CCommand_ToggleCulling(const char *command, const char *arguments) {
  int enabled = !GxMasterEnable(GxMasterEnable_Culling);

  GxMasterEnableSet(GxMasterEnable_Culling, enabled);
  ConsoleWrite(enabled ? "Back Face Culling enabled" : "Back Face Culling disabled", DEFAULT_COLOR);
  return 1;
}

static int CCommand_ToggleDblBuffer(const char *command, const char *arguments) {
  int enabled = !GxMasterEnable(GxMasterEnable_DoubleBuffering);

  GxMasterEnableSet(GxMasterEnable_DoubleBuffering, enabled);
  ConsoleWrite(enabled ? "Double Buffering enabled" : "DoubleBuffering disabled", DEFAULT_COLOR);
  return 1;
}

static int CCommand_SetResolutionXY(const char *command, const char *arguments) {
  return 1;
}

static int CCommand_SetResolutionMode(const char *command, const char *arguments) {
  return 1;
}

static int CCommand_SetColorDepth(const char *command, const char *arguments) {
  return 1;
}

static int CCommand_SetAPI(const char *command, const char *arguments) {
  return 1;
}

static int CCommand_Bug(const char *command, const char *args) {
  unsigned int reportType;

  if (!SStrCmpI(command, "bug", 0x7FFFFFFF)) {
    reportType = 0;
  } else if (!SStrCmpI(command, "suggestion", 0x7FFFFFFF)) {
    reportType = 1;
  } else if (!SStrCmpI(command, "note", 0x7FFFFFFF)) {
    reportType = 2;
  } else {
    return 1;
  }

  if (ClientServices_Report(reportType, args, "")) {
    ConsolePrintf("%s submitted", command);
  } else {
    ConsolePrintf("%s submission failed", command);
  }
  return 1;
}

static void ClientRegisterConsoleCommands() {
  ConsoleCommandRegister("reloadUI", CCommand_ReloadUI, GRAPHICS, 0);
  ConsoleCommandRegister("light", CCommand_ToggleLighting, GRAPHICS, 0);
  ConsoleCommandRegister("fog", CCommand_ToggleFog, GRAPHICS, 0);
  ConsoleCommandRegister("DepthTest", CCommand_ToggleDepthTesting, GRAPHICS, 0);
  ConsoleCommandRegister("DepthSet", CCommand_ToggleDepthSetting, GRAPHICS, 0);
  ConsoleCommandRegister("culling", CCommand_ToggleCulling, GRAPHICS, 0);
  ConsoleCommandRegister("dblbuffer", CCommand_ToggleDblBuffer, GRAPHICS, 0);
  ConsoleCommandRegister(
      "resxy", CCommand_SetResolutionXY, GRAPHICS,
      "x y: sets the screen resolution to x by y pixels.  If no parameters "
      "are given, the current resolution will be displayed."
  );
  ConsoleCommandRegister(
      "resmode", CCommand_SetResolutionMode, GRAPHICS,
      "x: sets the screen resolution mode x, which is a number from 0 to 3.  "
      "If no parameter is given, the possible modes will be displayed."
  );
  ConsoleCommandRegister(
      "bitdepth", CCommand_SetColorDepth, GRAPHICS,
      "16 or 32: sets the color depth 16 or 32 bits.  If no parameter is "
      "given, the current color depth will be displayed."
  );
  ConsoleCommandRegister(
      "3dapi", CCommand_SetAPI, GRAPHICS,
      "toggles the 3D API between OpenGL and Direct3D.  Changes won't take "
      "effect until the game is restarted."
  );
  ConsoleCommandRegister("Bug", CCommand_Bug, DEBUG, 0);
  ConsoleCommandRegister("Suggestion", CCommand_Bug, DEBUG, 0);
  ConsoleCommandRegister("Note", CCommand_Bug, DEBUG, 0);

  s_errorsCvar = CVar::Register("Errors", 0, 0, "0", ErrorDisplayCallback, DEBUG, false, 0);
  s_minErrorLevelCvar = CVar::Register("ErrorLevelMin", 0, 0, "1", ErrorDisplayMinLevelCallback, DEBUG, false, 0);
  s_maxErrorLevelCvar = CVar::Register("ErrorLevelMax", 0, 0, "3", ErrorDisplayMaxLevelCallback, DEBUG, false, 0);
  s_errorFilterCvar = CVar::Register("ErrorFilter", 0, 0, "all", ErrorDisplayFilterCallback, DEBUG, false, 0);
  s_debugTargetInfoCvar =
      CVar::Register("debugTargetInfo", "Toggle debug target tooltips on or off", 0, "0", DebugTargetInfoCallback, DEBUG, false, 0);
  s_debugShowGUIDsCvar = CVar::Register("showGUIDs", "Toggle debug GUID tooltips on or off", 0, "0", DebugShowGUIDsCallback, DEBUG, false, 0);
  s_desktopGammaCvar = CVar::Register("DesktopGamma", 0, 0, "0", DesktopGammaCallback, GRAPHICS, false, 0);
  s_gammaCvar = CVar::Register("Gamma", 0, 0, "1.0", GammaCallback, GRAPHICS, false, 0);
  g_realmNameVar = CVar::Register("realmName", "Last realm connected to", 0, "Friends and Family", 0, NET, false, 0);
  g_realmAddressVar = CVar::Register("realmAddress", "Address of last realm", 0, "172.16.9.12", 0, NET, false, 0);
  s_profanityFilterCvar = CVar::Register("profanityFilter", "Toggle profanity filter", 0, "1", ProfanityFilterCallback, GAME, false, 0);
}

static void ClientUnregisterConsoleCommands() {
  ConsoleCommandUnregister("reloadUI");
  ConsoleCommandUnregister("light");
  ConsoleCommandUnregister("fog");
  ConsoleCommandUnregister("DepthTest");
  ConsoleCommandUnregister("DepthSet");
  ConsoleCommandUnregister("culling");
  ConsoleCommandUnregister("dblbuffer");
  ConsoleCommandUnregister("fullscreen");
  ConsoleCommandUnregister("resxy");
  ConsoleCommandUnregister("resmode");
  ConsoleCommandUnregister("bitdepth");
  ConsoleCommandUnregister("3dapi");
  ConsoleCommandUnregister("Bug");
  ConsoleCommandUnregister("Suggestion");
  ConsoleCommandUnregister("Note");
}

static int CacheUpdateHandler(const void *eventData, void *arg) {
  CStatus status;
  DWORD   time = OsGetAsyncTimeMs();

  ModelCacheUpdate(time, &status);
  TextureCacheUpdate(time, &status);
  SysMsgAdd(status, 1);
  s_cacheUpdateTimerHandle = EventSetTimer(250u, CacheUpdateHandler, 0);
  return 1;
}

static void CacheUpdateInitialize() {
  s_cacheUpdateTimerHandle = EventSetTimer(250u, CacheUpdateHandler, 0);
}

static void CacheUpdateShutdown() {
  if (s_cacheUpdateTimerHandle) {
    EventKillTimer(s_cacheUpdateTimerHandle, CacheUpdateHandler, "CacheUpdateHandler");
  }
}

static int PollNet(const void *, void *) {
  ClientServices_PollEventQueue();
  return 1;
}

static void WowClientInit() {
  ObjectAllocInitialize();
  SysMsgInitialize();
  ClientDBInitialize();
  ComponentInitialize();
  ClientRegisterConsoleCommands();
  SndInterfaceInitialize();
  FrameScript_Initialize();
  FrameXML_RegisterDefault();
  CGGameUI::RegisterFrameFactories();
  GlueScriptEventsInitialize();
  ScriptEventsInitialize();
  CharCustomizationInitialize();
  ClientServices_Initialize(&s_loginData);
  ZoneDebugInitialize();
  DBCache_Initialize();
  DBCache_RegisterHandlers();
  CWorld::Initialize();
  GxuLightInitialize();
  GxuLightBucketSizeSet(16.665f);
  CGlueMgr::Initialize();
  CGlueMgr::SetScreen("login");
  InputControlInitialize();
  CameraInitialize();
  SpellVisualsInitialize();
  ValidateNameInitialize();
  EventRegister(EVENT_ID_POLL, PollNet);
}

static int InitializeHandlerPlayer(const void *, void *) {
  ASSERT(EventIsContextInteractive());

  BaseInitializeContext();
  ScrnInitialize(0);
  ConsoleScreenInitialize("World of Warcraft");
  s_errorFileCvar = CVar::Register("ErrorFileLog", 0, 0, "0", ErrorFileLogCallback, 0, false, 0);
  AsyncFileReadInitialize();
  TextureInitialize();
  ModelInitialize();
  CacheUpdateInitialize();
  WowClientInit();
  ConsoleCommandExecute("run autoexec.wtf", 1);
  return 1;
}

static void WowClientDestroy() {
  ValidateNameDestroy();
  SpellVisualsShutdown();
  CameraDestroy();
  InputControlDestroy();
  CGlueMgr::Shutdown();
  DBCache_ClearHandlers();
  DBCache_Destroy();
  ZoneDebugDestroy();
  CharCustomizationShutdown();
  FrameXML_ClearFactories();
  FrameScript_Destroy();
  SndInterfaceDestroy();
  ClientUnregisterConsoleCommands();
  ComponentShutdown();
  ClientDBShutdown();
  SysMsgShutdown();
  ObjectAllocDestroy();
}

static int DestroyHandlerPlayer(const void *, void *) {
  ASSERT(EventIsContextInteractive());

  ClientDestroyGame(0, 0, 0);
  WowClientDestroy();
  ClientServices_Destroy();
  CacheUpdateShutdown();
  ModelDestroy();
  CWorld::Destroy();
  TextureDestroy();
  AsyncFileReadDestroy();
  GxuLightShutdown();
  ConsoleScreenDestroy();
  ScrnDestroy();
  BaseDestroyContext();
  return 1;
}

static int NotifyHandler(void *__formal, NETMESSAGE msgID, unsigned long timestamp, CDataStore *msg) {
  char text[256];

  msg->GetString(text, sizeof(text));
  ConsoleWriteA("Notification received: %s", ERROR_COLOR, text);
  return 1;
}

static void SetPaths() {
  char        buffer[MAX_PATH];
  const char *path;

  SFile::DisableSFileCheckDisk();
  SFile::EnableDirectAccess(3);

  path = CmdLineGetString(CMD_DATA_DIR);
  if (!path[0]) {
    OsGetExePath(buffer, MAX_PATH);
    path = buffer;
  }

  SFile::SetBasePath(path);
  SFile::SetDataPath("Data\\");
  OsSetCurrentDirectory(path);
}

static void OpenArchives() {
  unsigned int i;

  for (i = 0; i < 8; ++i) {
    if (!SFile::OpenArchive(s_archiveNames[i], 0, 0, &s_archive[i])) {
      s_archive[i] = 0;
    }
  }
}

static void ShutdownFileAccess() {
  unsigned int i;

  for (i = 0; i < 8; ++i) {
    if (s_archive[i]) {
      SFile::CloseArchive(s_archive[i]);
    }
  }
}

static void ProcessCommandLine() {
  SCmdRegisterArgList(s_wowArgList, 17);
  CmdLineProcess();
}

static bool InitializeGlobal() {
  char  windowTitle[MAX_PATH];
  int   active;
  FILE *sessionFile;

  ProcessCommandLine();
  COsSharedMemory shm;

  if (shm.Initialize("WowData", 0, 1)) {
    memset(&s_loginData, 0, sizeof(s_loginData));
  } else {
    memcpy(&s_loginData, shm.Data(), sizeof(s_loginData));
    memset(shm.Data(), 0, sizeof(s_loginData));
    static_cast<BYTE *>(shm.Data())[sizeof(s_loginData)] = 1;
    shm.Destroy();
  }

  SetPaths();

  if (!s_loginData.m_loginServerID) {
    sessionFile = fopen("wow.ses", "rb");
    if (sessionFile) {
      fread(&s_loginData, 1, sizeof(s_loginData), sessionFile);
      fclose(sessionFile);
    }
  }

  if (!s_loginData.m_account[0] || !CmdLineGetBool((CMDOPT)21)) {
    OsGuiMessageBox(0, 0, "This program must be launched by the updater", "Launch error");
    return FALSE;
  }

  ClientServices_SetAccountName(s_loginData.m_account);
  active = 1;
  StormSetOption(10, &active, sizeof(active));
  OpenArchives();
  ConsoleCommandInitialize();
  CVar::Initialize("Config.wtf");
  SStrCopy(windowTitle, "World of Warcraft", 0x7FFFFFFF);
  ConsoleDeviceInitialize(windowTitle, true);
  BaseInitializeGlobal();
  EventInitialize(1, 0);
  OsIMEInitialize();
  g_rndSeed.SetSeed(OsGetAsyncTimeMs());
  g_clientEventContext = EventCreateContextEx(1, InitializeHandlerPlayer, DestroyHandlerPlayer, 10, 0);

  return TRUE;
}

static void DestroyGlobal() {
  SMemSetDebugFlags(0, 8);
  OsIMEDestroy();
  EventDestroy();
  BaseDestroyGlobal();
  ConsoleDeviceDestroy();
  CVar::Destroy();
  ConsoleCommandDestroy();
  ShutdownFileAccess();
}

static int ClientIdle(const void *data, void *__formal) {
  ClientGameTimeTickHandler(data, 0);
  Player_C_ZoneUpdateHandler(data, 0);
  CGPlayer_C::GMIdle();
  return 1;
}

static int ClientFocus(const void *packetData, void *__formal) {
  Player_C_AppFocusMovementHandler(*static_cast<const int *>(packetData));
  return 1;
}

void ClientKillTimer(unsigned int timerId, CLIENTTIMERHANDLER handler, const char *handlerName) {
  EventKillTimer(timerId, handler, handlerName);
}

void ClientPostClose() {
  EventPostClose();
}

void ClientInitializeGame(unsigned int continentID, NTempest::C3Vector position) {
  GxMasterEnableSet(GxMasterEnable_Fog, 1);
  ClntObjMgrInitializeShared();

  ClntObjMgr *mgr = ClntObjMgrCreate(PLAYER_NORMAL, 0);
  ClntObjMgrSetCurrent(mgr);
  g_clientConnection->SetObjMgr(mgr);
  ClntObjMgrSetNet(g_clientConnection);
  ClntObjMgrSetCurrent(mgr);
  ClntObjMgrInitialize();

  SndInterfaceWorldInitialize();
  CGGameUI::InitializeGame();
  PlayerNameInitialize();
  WeaponTrailsInitialize();
  CGObject_C::Initialize();
  SpellTableInitialize();
  CGUnit_C::Initialize();
  CGGameObject_C::Initialize();
  PlayerClientInitialize();
  CGPlayer_C::Initialize();
  CGItem_C::Initialize();
  LootInitialize();
  AreaListInitialize();
  TaxiMapInitialize();
  FriendList::Initialize();
  SmartScreenRectClearAllGrids();
  Trade_C_Initialize();

  MovementInit();
  EventRegister(EVENT_ID_IDLE, ClientIdle);
  EventRegister(EVENT_ID_FOCUS, ClientFocus);
  ClientInitializeGameTime();
  InstallGameConsoleCommands();

  ClientServices_SetMessageHandler(SMSG_QUERY_OBJECT_POSITION, ReceiveObjectPosition, 0);
  ClientServices_SetMessageHandler(SMSG_QUERY_OBJECT_ROTATION, ReceiveObjectRotation, 0);
  ClientServices_SetMessageHandler(SMSG_MESSAGECHAT, ClientChatHandler, 0);
  ClientServices_SetMessageHandler(SMSG_TEXT_EMOTE, ClientTextEmoteHandler, 0);
  ClientServices_SetMessageHandler(SMSG_DEBUG_AISTATE, DebugAIStateHandler, 0);
  ClientServices_SetMessageHandler(SMSG_DBLOOKUP, LookupResultsHandler, 0);
  ClientServices_SetMessageHandler(MSG_MOVE_TOGGLE_LOGGING, MovementLoggingHandler, 0);
  ClientServices_SetMessageHandler(MSG_MOVE_TOGGLE_FALL_LOGGING, MovementFallLoggingHandler, 0);
  ClientServices_SetMessageHandler(SMSG_NEW_WORLD, NewWorldHandler, 0);
  ClientServices_SetMessageHandler(SMSG_NOTIFICATION, NotifyHandler, 0);
  ClientServices_SetMessageHandler(SMSG_PLAYED_TIME, PlayedTimeHandler, 0);
  ClientServices_SetMessageHandler(SMSG_TRANSFER_PENDING, TransferPendingHandler, 0);
  ClientServices_SetMessageHandler(SMSG_TRANSFER_ABORTED, TransferAbortedHandler, 0);
  ClientServices_SetMessageHandler(SMSG_CHANNEL_NOTIFY, ChannelNotifyHandler, 0);
  ClientServices_SetMessageHandler(SMSG_CHANNEL_LIST, ClientChannelListHandler, 0);

  FATALASSERT(g_mapDB.GetRecord(continentID));
  ClntObjMgrSetMapID(continentID);
  ViolenceLevelsInitialize();
  CWorld::LoadMap(g_mapDB.GetRecord(continentID)->m_Directory, position, 1);
  LoadingScreenRegisterWorldLoaded();
  clientGameInitialized = 1;
}

void ClientDestroyGame(int connected, int resumeUI, int loginError) {
  if (!clientGameInitialized) {
    return;
  }

  EventUnregister(EVENT_ID_IDLE, ClientIdle);
  EventUnregister(EVENT_ID_FOCUS, ClientFocus);
  ClientServices_ClearMessageHandler(SMSG_QUERY_OBJECT_POSITION);
  ClientServices_ClearMessageHandler(SMSG_QUERY_OBJECT_ROTATION);
  ClientServices_ClearMessageHandler(SMSG_MESSAGECHAT);
  ClientServices_ClearMessageHandler(SMSG_TEXT_EMOTE);
  ClientServices_ClearMessageHandler(SMSG_DEBUG_AISTATE);
  ClientServices_ClearMessageHandler(SMSG_DBLOOKUP);
  ClientServices_ClearMessageHandler(MSG_MOVE_TOGGLE_LOGGING);
  ClientServices_ClearMessageHandler(MSG_MOVE_TOGGLE_FALL_LOGGING);
  ClientServices_ClearMessageHandler(SMSG_NEW_WORLD);
  ClientServices_ClearMessageHandler(SMSG_NOTIFICATION);
  ClientServices_ClearMessageHandler(SMSG_PLAYED_TIME);
  ClientServices_ClearMessageHandler(SMSG_TRANSFER_PENDING);
  ClientServices_ClearMessageHandler(SMSG_TRANSFER_ABORTED);
  ClientServices_ClearMessageHandler(SMSG_CHANNEL_NOTIFY);
  ClientServices_ClearMessageHandler(SMSG_CHANNEL_LIST);

  UninstallGameConsoleCommands();
  Trade_C_Destroy();
  FriendList::Destroy();
  TaxiMapShutdown();
  AreaListShutdown();
  WorldTextClearStrings();
  CGItem_C::Shutdown();
  CGPlayer_C::Shutdown();
  PlayerClientShutdown();
  LootDestroy();
  SpellTableDestroy();
  CGUnit_C::Shutdown();
  CGGameObject_C::Shutdown();
  CGObject_C::Shutdown();
  MovementDestroy();
  EventUnregisterEx(EVENT_ID_IDLE, MovementIdleMoveUnits, 0, 0xFFFFFFFF);

  ClntObjMgrDestroy();
  CGUnit_C::PostShutdown();
  ViolenceLevelsShutdown();
  ClientDestroyGameTime();
  CWorld::UnloadMap();
  WeaponTrailsShutdown();
  PlayerNameShutdown();
  CGGameUI::ShutdownGame();
  SndInterfaceWorldDestroy();

  ClntObjMgrDestruct(ClntObjMgrGetCurrent());
  ClntObjMgrDestroyShared();
  clientGameInitialized = 0;
  DisableLoadingScreen();

  if (resumeUI) {
    CGlueMgr::Resume();
    if (loginError) {
      CGlueMgr::m_idleState = CGlueMgr::IDLE_WORLD_LOGIN;
    }
    CGlueMgr::SetScreen(connected ? "charselect" : "login");
  }
}

unsigned int Bot_QueryAreaId(float x, float y) {
  return CWorld::QueryAreaId(x, y);
}

int Bot_GetWanderPoint(const NTempest::C3Vector&, float, const NTempest::C3Vector&, const NTempest::C3Vector&, float, NTempest::C3Vector&) {
  return 0;
}

void BotClientSetAccount(const char *, const char *) {
}

void BotClientAddKnownSpell(CGPlayer_C*, int) {
}

void BotClientLoseTarget(const CGUnit_C*) {
}

static void LogZoneInfo(CGPlayer_C *player, char *log, unsigned long size) {
  NTempest::C3Vector location(reinterpret_cast<CGObject_C *>(player)->GetPosition());
  char               text[MAX_PATH];
  unsigned int       area;

  SStrPack(log, "Local Zone: ", size);
  area = CWorld::QueryAreaId(location.x, location.y);
  AreaListGetName(ClntObjMgrGetMapID(), area >> 16, area & 0xFFFF, text, sizeof(text), 1);
  SStrPack(log, text, size);
  SStrPack(log, "\r\n", size);
}

static int ClientIsValidPointer(const void *address, unsigned long size, int forWriting) {
  return SMemIsValidPointer(address, size, forWriting) != 0;
}

static void LogObjectInfo(const char *label, CGObject_C *object, char *log, unsigned long size) {
  char             text[MAX_PATH];
  unsigned __int64 guid;

  if (!ClientIsValidPointer(object, 0x30, FALSE)) {
    return;
  }

  NTempest::C3Vector position(object->GetPosition());
  guid = object->GetGUID();

  SStrPrintf(text, sizeof(text), "%s: %s, %016I64X, (%g,%g,%g)\r\n", label, object->GetObjectName(), guid, position.x, position.y, position.z);
  SStrPack(log, text, size);
}

static int APIENTRY WowLogHeader(char *log, DWORD size) {
  CGPlayer_C        *player;
  CGObject_C        *object;
  unsigned __int64   guid;

  SStrPrintf(log, size, "WoWBuild: %d\r\n", 3368);

  if (!ClntObjMgrIsValid(0)) {
    return 1;
  }

  guid = ClntObjMgrGetActivePlayer();
  player = reinterpret_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
  if (!player || !ClientIsValidPointer(player, 0x1860, FALSE)) {
    return 1;
  }

  LogZoneInfo(player, log, size);
  LogObjectInfo("Local Player", reinterpret_cast<CGObject_C *>(player), log, size);

  object = ClntObjMgrObjectPtr(player->CGPlayer_C::GetLocalTarget(), __FILE__, __LINE__);
  if (object) {
    LogObjectInfo("Local Target", object, log, size);
  }

  object = ClntObjMgrObjectPtr(CGGameUI::GetCurrentObjectTrack(), __FILE__, __LINE__);
  if (object) {
    LogObjectInfo("Current Object Track", object, log, size);
  }

  object = ClntObjMgrObjectPtr(CGGameUI::GetInteractTarget(), __FILE__, __LINE__);
  if (object) {
    LogObjectInfo("Interact Target", object, log, size);
  }

  object = ClntObjMgrObjectPtr(CGGameUI::GetLockedTarget(), __FILE__, __LINE__);
  if (object) {
    LogObjectInfo("Locked Target", object, log, size);
  }

  object = ClntObjMgrObjectPtr(CGGameUI::GetLastEnemyTarget(), __FILE__, __LINE__);
  if (object) {
    LogObjectInfo("Last Enemy Target", object, log, size);
  }

  object = ClntObjMgrObjectPtr(CGGameUI::GetCursorItem(), __FILE__, __LINE__);
  if (object) {
    LogObjectInfo("Cursor Item", object, log, size);
  }

  return 1;
}

static void LaunchWoWError(const char *logFileName) {
  char cmd[520];

  SStrPrintf(cmd, sizeof(cmd), "%s %s", "WowErrorAE.exe", logFileName);
  SCreateProcess("WowErrorAE.exe", cmd, 0, 0);
}

static BOOL APIENTRY SendErrorLog(DWORD code, const char *msg, const char *file, int line, const char *details) {
  char log[MAX_PATH];

  if (SErrGetLogLastPath(log, sizeof(log))) {
    LaunchWoWError(log);
  }

  return TRUE;
}

#ifdef WOW_STORM_ENTRYPOINT

extern "C" void __cdecl WinMainCRTStartup();

extern "C" void __cdecl StormStaticEntryPoint() {
  StormRtlInitialize();
  WinMainCRTStartup();
  StormRtlDestroy();
}

#endif

int APIENTRY WinMain(HINSTANCE, HINSTANCE, char *, int) {
  DWORD sendErrorLogs = 1;

  StormInitialize();
  SErrCatchUnhandledExceptions();

  if (!SRegLoadValue("Wow\\Client", "SendErrorLogs", 0, &sendErrorLogs)) {
    sendErrorLogs = 1;
    SRegSaveValue("Wow\\Client", "SendErrorLogs", 0, sendErrorLogs);
  }

  SErrSetLogTitleString(WOW_ERROR_LOG_TITLE);
  SErrSetLogCallback(WowLogHeader);

  if (sendErrorLogs) {
    SErrRegisterHandler(SendErrorLog);
  }

  if (InitializeGlobal()) {
    EventDoMessageLoop();
    DestroyGlobal();
  }

  StormDestroy();

  return 0;
}
