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

#include "GameUI.h"
#include "Magic/MagicClient/Spell_C.h"
#include "TabardCreationFrame.h"
#include <Os/OsTime.h>
#include "ActionBarFrame.h"
#include "ChatFrame.h"
#include "CharacterModelBase.h"
#include "ClassTrainerFrame.h"
#include "ContainerFrame.h"
#include "GuildRegistrar.h"
#include "ItemTextFrame.h"
#include "LootFrame.h"
#include "MinimapFrame.h"
#include "MerchantFrame.h"
#include "PaperDollInfoFrame.h"
#include "PartyFrame.h"
#include "PetInfo.h"
#include "QuestLog.h"
#include "QuestFrame.h"
#include "ReputationInfo.h"
#include "SpellBookFrame.h"
#include "TabardModelFrame.h"
#include "TaxiMapFrame.h"
#include "TradeFrame.h"
#include "Tutorial.h"
#include "WorldFrame.h"
#include "UIBindings.h"

#include "Client.h"
#include "Game/GameTime.h"
#include "Object/ObjectClient/Unit_C.h"
#include "Object/ObjectClient/GameObject_C.h"
#include "Object/ObjectClient/Item_C.h"
#include "Object/ObjectClient/Player_C.h"
#include "Object/UnitCombat.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "Game/GameClient/PlayerName.h"
#include "DB/DBClient/AutoCode/CinematicCameraRec.h"
#include "DB/DBClient/AutoCode/CinematicSequencesRec.h"
#include "DB/DBClient/AutoCode/ChrClassesRec.h"
#include "DB/DBClient/AutoCode/SpellIconRec.h"
#include "DB/DBClient/AutoCode/SpellRec.h"
#include "DB/DBClient/DBCacheInstances.h"
#include "DB/DBClient/DBClient.h"
#include "SoundInterface/SoundInterface.h"
#include "WowSvcs/WowSvcsClient/ClientServices.h"
#include "WowSvcs/WowSvcsClient/FriendList.h"
#include "Console/ConsoleClient.h"
#include "Console/ConsoleVar.h"
#include "UIUtil/Cursor.h"

#include <Base/CDataStore.h>
#include <Base/Coordinate.h>
#include <Base/Status.h>
#include <Console/ConsoleCommand.h>
#include <Event/CMouseEvent.h>
#include <Event/EvtApi.h>
#include <FrameScript/FrameScript.h>
#include <Frame/SimpleFrameRegistry.h>
#include <Frame/CSimpleStatusBar.h>
#include <Frame/CSimpleTop.h>
#include <FrameXML/FrameXML.h>
#include <FrameXML/LoadXML.h>
#include <Gx/CGxDevice.h>
#include <Gx/Gx.h>
#include <Model/IModel.h>
#include <lauxlib.h>
#include <lua.h>
#include <Os/W32/OsFile.h>
#include <Os/W32/OsSound.h>
#include <Scrn/Scrn.h>
#include <Services/AsyncFileRead.h>
#include <Services/DataMgr.h>
#include <Services/SysMessage.h>
#include <Tempest/c34matrix.h>
#include <UIUtil/Tooltip.h>
#include <UIUtil/InputControl.h>
#include <storm.h>
#include "WorldClient/World.h"
#include <ctype.h>
#include <float.h>
#include <stdlib.h>

namespace {
  extern FrameScript_Method s_ScriptFunctions[126];
}
extern char **Script_GetNamesFromGUID(const DWORDLONG &guid, int &numnames);
void      PortraitInitialize();
void      PortraitShutdown();
void      UpdatePortraitTexture(const DWORDLONG &guid);
void      Trade_C_CancelTrade();
void      Trade_C_BeginTrade();
CGUnit_C *Script_GetUnitFromName(LPCSTR name);
DWORDLONG Script_GetGUIDFromName(LPCSTR name);
void      Script_SendUnitSignal(const DWORDLONG &guid, int signal);
bool      Spell_C_IsTargeting();
bool      Spell_C_WorldObjectHousing();
void      Spell_C_WorldObjectRotate();
void      Spell_C_StopTargeting();

extern LPCSTR g_scriptEvents[0x177];

void InputControlRegisterScriptFunctions();
void InputControlUnregisterScriptFunctions();
void UIBindingsRegisterScriptFunctions();
void UIBindingsUnegisterScriptFunctions();
void CameraRegisterScriptFunctions();
void CameraUnregisterScriptFunctions();
void SpellRegisterScriptFunctions();
void SpellUnregisterScriptFunctions();
void ScriptEventsRegisterFunctions();
void ScriptEventsUnregisterFunctions();
void ActionBarRegisterScriptFunctions();
void ActionBarUnregisterScriptFunctions();
void BuffBarRegisterScriptFunctions();
void BuffBarUnregisterScriptFunctions();
void PartyInfoRegisterScriptFunctions();
void PartyInfoUnregisterScriptFunctions();
void ChatRegisterScriptFunctions();
void ChatUnregisterScriptFunctions();
void SpellBookRegisterScriptFunctions();
void SpellBookUnregisterScriptFunctions();
void CharacterInfoRegisterScriptFunctions();
void CharacterInfoUnregisterScriptFunctions();
void LootInfoRegisterScriptFunctions();
void LootInfoUnregisterScriptFunctions();
void ItemTextRegisterScriptFunctions();
void ItemTextUnregisterScriptFunctions();
void QuestInfoRegisterScriptFunctions();
void QuestInfoUnregisterScriptFunctions();
void QuestLogRegisterScriptFunctions();
void QuestLogUnregisterScriptFunctions();
void ClassTrainerRegisterScriptFunctions();
void ClassTrainerUnregisterScriptFunctions();
void CraftInfoRegisterScriptFunctions();
void CraftInfoUnregisterScriptFunctions();
void MerchantRegisterScriptFunctions();
void MerchantUnregisterScriptFunctions();
void TradeInfoRegisterScriptFunctions();
void TradeInfoUnregisterScriptFunctions();
void ContainerRegisterScriptFunctions();
void ContainerUnregisterScriptFunctions();
void BankRegisterScriptFunctions();
void BankUnregisterScriptFunctions();
void PetInfoRegisterScriptFunctions();
void PetInfoUnregisterScriptFunctions();
void TradeSkillRegisterScriptFunctions();
void TradeSkillUnregisterScriptFunctions();
void WorldMapRegisterScriptFunctions();
void WorldMapUnregisterScriptFunctions();
void ReputationInfoRegisterScriptFunctions();
void ReputationInfoUnregisterScriptFunctions();
void SndInterfaceRegisterVocalScriptFunctions();
void SndInterfaceUnregisterVocalScriptFunctions();
void TabardCreationRegisterScriptFunctions();
void TabardCreationUnregisterScriptFunctions();
void GuildRegistrarRegisterScriptFunctions();
void GuildRegistrarUnregisterScriptFunctions();
void DuelInfoRegisterScriptFunctions();
void DuelInfoUnregisterScriptFunctions();
void TutorialRegisterScriptFunctions();
void TutorialUnregisterScriptFunctions();
void PetitionInfoRegisterScriptFunctions();
void PetitionInfoUnregisterScriptFunctions();

static void LoadScriptFunctions();
static void UnloadScriptFunctions();
static BOOL CCommand_Script(LPCSTR command, LPCSTR arguments);
static BOOL CCommand_ScaleUI(LPCSTR, LPCSTR arguments);
static void LoadPlacedFrames();
static BOOL SavePlacedFrames(CSimpleTop *top);
static BOOL PlacedFrameCallback(CSimpleFrame *frame, LPVOID param);
static void PlaceFrame(CSimpleFrame *frame, int framelevel, int x, int y, int w, int h);

CVar *s_minimapZoomCVar;
CVar *s_minimapInsideZoomCVar;
CVar *s_statusBarCVar;
CVar *s_assistAttackCVar;
CVar *s_combatLogCVar;

struct WorldMapContinentInfo {
  int               continentID;
  int               mapAreaID;
  TSFixedArray<int> zoneList;
  int               chunkZones[128][128];
  NTempest::CRect   hitRect;
};

struct WorldMapLandmarkInfo {
  int   entryID;
  float x;
  float y;
  BOOL  isPortLoc;
};

class CGBuffDesc {
  friend class CGBuffBar;

 public:
  CGBuffDesc();
  __forceinline ~CGBuffDesc() {
  }
  void SetAuraIndex(int index, CGPlayer_C *player);
  int  GetAuraIndex() const {
    return m_auraIndex;
  }
  int GetAuraSpell() const {
    return m_auraSpell;
  }
  BYTE GetAuraFlags() const {
    return m_auraFlags;
  }
  int GetUntilCancelled() const {
    return m_untilCancelled;
  }

 protected:
  int  m_auraIndex;
  int  m_auraSpell;
  BYTE m_auraFlags;
  int  m_untilCancelled;
};
struct TradeSkillInfo;
struct TradeSkillSubClassInfo;
struct CraftInfo;
struct CraftSkillLineInfo;
class AreaPOIRec;
class WorldSafeLocsRec;

struct PetitionSignerInfo {
  DWORDLONG guid;
  int       choice;
};

class CGWorldMap {
 public:
  static void InitializeGame();
  static void EnterWorld();
  static void LeaveWorld();
  static void ShutdownGame();
  static int  GetCurrentContinent() {
    return m_currentContinent;
  }
  static int GetCurrentZone() {
    return m_currentZone;
  }
  static UINT GetNumContinents() {
    return m_continents.Count();
  }
  static LPCSTR GetContinentName(UINT index);
  static UINT   GetNumZones(UINT continent) {
    return continent < m_continents.Count() ? m_continents[continent].zoneList.Count() : 0;
  }
  static LPCSTR GetZoneName(UINT continent, UINT index);
  static void   SetMapToCurrentZone();
  static void   SetMap(int continent, int zone);
  static LPCSTR GetMapFilename();
  static UINT   GetMapHeight();
  static void   ProcessClick(float x, float y);
  static int    GetMapHighlight(float x, float y);
  static void   RunNearestPortLoc(float x, float y);
  static void   GetPOIPosition(const AreaPOIRec *rec, float &x, float &y);
  static void   GetPortLocPosition(const WorldSafeLocsRec *rec, float &x, float &y);
  static void   GetPlayerPosition(DWORDLONG guid, float &x, float &y);
  static void   GetBindPosition(float &x, float &y);
  static UINT   GetNumLandmarks() {
    return m_numLandmarks;
  }
  static const WorldMapLandmarkInfo *GetLandmarkInfo(UINT index) {
    return index < m_numLandmarks ? &m_landmarks[index] : 0;
  }

 private:
  static int  GetMapAreaFromPos(float x, float y);
  static BOOL GetWorldLocFromPos(float x, float y, NTempest::C2Vector &loc, int &mapID);
  static void GetWorldPosition(const NTempest::C2Vector &pos, int mapID, float &x, float &y);

 protected:
  static int                                 m_currentContinent;
  static int                                 m_currentZone;
  static UINT                                m_numLandmarks;
  static TSFixedArray<WorldMapContinentInfo> m_continents;
  static TSFixedArray<WorldMapLandmarkInfo>  m_landmarks;
};

class CGBuffBar {
 public:
  static void              InitializeGame();
  static void              ShutdownGame();
  static void              EnterWorld();
  static void              LeaveWorld();
  static void              UpdateBuffs();
  static void              UpdateDuration(BYTE slot, UINT duration);
  static const CGBuffDesc *GetBuffByFilter(int index, UINT filter, int &buffIndex);
  static const CGBuffDesc *GetBuffByIndex(int buffIndex);
  static UINT              GetBuffTimeLeftByIndex(int buffIndex);

 private:
  static CGBuffDesc m_buffs[56];
  static UINT       m_durations[56];
};

class CGBankInfo {
 public:
  static void      EnterWorld();
  static void      LeaveWorld();
  static void      OpenBank(const DWORDLONG &guid);
  static void      CloseBank();
  static void      OnCloseBank();
  static void      PickupItem(int slot, BOOL isBag, int slotIsButtonID);
  static void      SplitItem(int slot, int split);
  static DWORDLONG GetBanker() {
    return m_unit;
  }
  static DWORDLONG m_unit;
};

class CGTradeSkillInfo {
  friend int __cdecl QSortSkills(LPCVOID a, LPCVOID b);
  friend int __cdecl QSortSubClasses(LPCVOID a, LPCVOID b);

 public:
  static void EnterWorld();
  static void LeaveWorld();
  static void ShutdownGame();
  static void Close();
  static void ClearItemCallbacks();
  static void DecrementPendingItem() {
    if (!m_itemsPending || !--m_itemsPending) {
      RefreshList(1);
    }
  }
  static void SetSelection(int index);
  static int  GetSelectionIndex();
  static int  GetSkillLine() {
    return m_skillLine;
  }
  static int GetNumTradeSkills() {
    return m_filteredSkills;
  }
  static const TradeSkillInfo *GetTradeSkillInfo(UINT index) {
    return index < m_filteredSkills ? m_skills[index] : 0;
  }
  static void SetSkillLine(int id);
  static void RefreshList(int resetFilters);
  static UINT GetNumSubClasses() {
    return m_numSubClasses;
  }
  static TradeSkillSubClassInfo *GetSubClass(UINT index) {
    return index < m_numSubClasses ? m_subClasses[index] : 0;
  }
  static int  GetSubClassIndexFromSkill(UINT index);
  static BOOL IsCollpasedHeader(UINT index);
  static int  GetSubClassFilter() {
    return m_subClassFilter;
  }
  static int GetInvTypeFilter() {
    return m_invTypeFilter;
  }
  static int GetCollapseFilter() {
    return m_collapseFilter;
  }
  static int GetAvailableSlots() {
    return m_availableSlots;
  }
  static void SetSubClassFilter(int filter);
  static void SetInvTypeFilter(int filter);
  static void SetCollapseFilter(int filter);

 protected:
  static void FilterAndSortSkills();

 private:
  static int                                       m_skillLine;
  static int                                       m_currentSelection;
  static UINT                                      m_itemsPending;
  static UINT                                      m_numSkills;
  static UINT                                      m_numSubClasses;
  static UINT                                      m_filteredSkills;
  static int                                       m_subClassFilter;
  static int                                       m_invTypeFilter;
  static int                                       m_collapseFilter;
  static TSGrowableArray<TradeSkillInfo *>         m_skills;
  static TSGrowableArray<TradeSkillSubClassInfo *> m_subClasses;
  static int                                       m_availableSlots;
};

class CGCraftInfo {
 public:
  static void               EnterWorld();
  static void               ShutdownGame();
  static void               Close();
  static void               SetSelection(int index);
  static int                GetSelectionIndex();
  static SPELL_CAST_UI_TYPE GetCraftType() {
    return m_craftType;
  }
  static int GetNumCrafts() {
    return m_filteredSkills;
  }
  static const CraftInfo *GetCraftInfo(UINT index) {
    return index < m_numSkills ? m_skills[index] : 0;
  }
  static UINT GetNumSkillLines() {
    return m_numSkillLines;
  }
  static CraftSkillLineInfo *GetSkillLine(UINT index) {
    return index < m_numSkillLines ? m_skillLines[index] : 0;
  }
  static int  GetSkillLineIndexFromCraft(UINT index);
  static void SetCraftType(SPELL_CAST_UI_TYPE type);
  static void RefreshList();
  static BOOL IsCollpasedHeader(UINT index);
  static int  GetCollapseFilter() {
    return m_collapseFilter;
  }
  static void SetCollapseFilter(int filter);

 private:
  friend int __cdecl QSortSkills(LPCVOID a, LPCVOID b);
  friend int __cdecl QSortPetSkills(LPCVOID a, LPCVOID b);
  friend int __cdecl QSortSkillLines(LPCVOID a, LPCVOID b);

 protected:
  static void FilterAndSortSkills();

 private:
  static SPELL_CAST_UI_TYPE                    m_craftType;
  static int                                   m_currentSelection;
  static UINT                                  m_numSkills;
  static UINT                                  m_numSkillLines;
  static UINT                                  m_filteredSkills;
  static int                                   m_collapseFilter;
  static TSGrowableArray<CraftInfo *>          m_skills;
  static TSGrowableArray<CraftSkillLineInfo *> m_skillLines;
};

class CGDuelInfo {
 public:
  static void InitializeGame();
  static void ShutdownGame();
  static void StartDuel();
  static void AcceptDuel();
  static void CancelDuel();

 private:
  static BOOL OnDuelRequested(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg);
  static BOOL OnDuelOutOfBounds(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg);
  static BOOL OnDuelInBounds(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg);
  static BOOL OnDuelComplete(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg);
  static BOOL OnDuelWinner(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg);

 protected:
  static DWORDLONG m_arbiter;
};

class CGPetitionInfo {
 public:
  static void EnterWorld();
  static void LeaveWorld();
  static void SetPetition(DWORDLONG petition, int petitionID);
  static DWORDLONG GetPetition() {
    return m_petitionGUID;
  }
  static void SetSignatures(BYTE count, DWORDLONG *signers, int *choices);
  static UINT GetNumSignatures() {
    return m_numSignatures;
  }
  static const PetitionSignerInfo *GetSignature(UINT index) {
    return index < m_numSignatures ? &m_signatures[index] : 0;
  }
  static void DecrementPendingName();
  static void SetPetitionStats(int id);
  static const CGPetition *GetPetitionStats() {
    return m_petition;
  }

 private:
  static void ClearSignatures();

 protected:
  static DWORDLONG                           m_petitionGUID;
  static int                                 m_petitionID;
  static TSGrowableArray<PetitionSignerInfo> m_signatures;
  static UINT                                m_numSignatures;
  static UINT                                m_pendingNames;
  static const CGPetition                   *m_petition;
};

void Spell_C_CancelSpell(bool failed, bool notifyServer, SPELL_FAILED_REASON reason);
bool Spell_C_CastSpell(int spellID, const CGItem_C *item);
UINT CurrencyTotal(int coins[3]);
BOOL CursorGrabMoney(UINT amount);
BOOL CursorGrabSpell(LPCSTR filename);
UINT CursorGetCursorMode();
UINT CursorGetCursorType();
void CursorResetCursor(int force);
void CursorSetHeldItem(DWORDLONG itemGuid);
void CursorSetHeldVirtualItem(UINT displayID);
void CursorSetCursorMode(CURSORANIMATIONS mode);

static UINT s_nearestIndex;
struct NearestEnemyData {
  DWORDLONG guid;
  float     distSq;
};
static TSGrowableArray<NearestEnemyData> s_nearestList;
static UINT                              s_nearestListTime;
static UINT                              s_sameTargetTime;

enum ERROR_TEXT_PLACEMENT {
  ERRORTEXT_CHAT = 0,
  ERRORTEXT_UIINFO = 1,
  ERRORTEXT_UIERROR = 2,
  ERRORTEXT_CONSOLE = 3
};

struct GAMEERRORDESC {
  LPCSTR               stringToken;
  ERROR_TEXT_PLACEMENT textPlacement;
  LPCSTR               soundName;
  VOCALUISOUNDS        voiceID;
  int                  supressText;
  SLASH_COMMAND_ID     slashCmd;

  GAMEERRORDESC(
      LPCSTR               _stringToken,
      ERROR_TEXT_PLACEMENT _textPlacement,
      LPCSTR               _soundName,
      VOCALUISOUNDS        _voiceID,
      int                  _supressText,
      SLASH_COMMAND_ID     _slashCmd
  )
      : stringToken(_stringToken),
        textPlacement(_textPlacement),
        soundName(_soundName),
        voiceID(_voiceID),
        supressText(_supressText),
        slashCmd(_slashCmd) {
  }
};

static const GAMEERRORDESC s_gameErrors[GERR_NUM_TYPES] = {
    GAMEERRORDESC(
        "ERR_INV_FULL",
        ERRORTEXT_UIERROR,
        "GAMEERRORUNABLETOEQUIP",
        VUI_INVENTORYFULL,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_CANT_EQUIP_LEVEL_I",
        ERRORTEXT_UIERROR,
        "GAMEERRORUNABLETOEQUIP",
        VUI_NOEQUIP_LEVEL,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_CANT_EQUIP_SKILL",
        ERRORTEXT_UIERROR,
        "GAMEERRORUNABLETOEQUIP",
        VUI_NOEQUIP_LEVEL,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_CANT_EQUIP_EVER",
        ERRORTEXT_UIERROR,
        "GAMEERRORUNABLETOEQUIP",
        VUI_NOEQUIP_EVER,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_PROFICIENCY_NEEDED",
        ERRORTEXT_UIERROR,
        "GAMEERRORUNABLETOEQUIP",
        VUI_PROFICIENCYNEEDED,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_WRONG_SLOT",
        ERRORTEXT_UIERROR,
        "GAMEERRORUNABLETOEQUIP",
        VUI_WRONGSLOT,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_BAG_FULL",
        ERRORTEXT_UIERROR,
        "GAMEERRORUNABLETOEQUIP",
        VUI_BAGFULL,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_DESTROY_NONEMPTY_BAG",
        ERRORTEXT_UIERROR,
        "GAMEERRORUNABLETOEQUIP",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_BAG_IN_BAG",
        ERRORTEXT_UIERROR,
        "GAMEERRORUNABLETOEQUIP",
        VUI_CANTPUTBAG,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_AMMO_ONLY",
        ERRORTEXT_UIERROR,
        "GAMEERRORUNABLETOEQUIP",
        VUI_AMMOONLYINBAG,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_NO_SLOT_AVAILABLE",
        ERRORTEXT_UIERROR,
        "GAMEERRORUNABLETOEQUIP",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_WRONG_BAG_TYPE",
        ERRORTEXT_UIERROR,
        "GAMEERRORUNABLETOEQUIP",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_ITEM_MAX_COUNT",
        ERRORTEXT_UIERROR,
        "GAMEERRORUNABLETOEQUIP",
        VUI_ITEMMAXCOUNT,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_NOT_EQUIPPABLE",
        ERRORTEXT_UIERROR,
        "GAMEERRORUNABLETOEQUIP",
        VUI_NOTEQUIPPABLE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_CANT_STACK",
        ERRORTEXT_UIERROR,
        "GAMEERRORUNABLETOEQUIP",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_CANT_SWAP",
        ERRORTEXT_UIERROR,
        "GAMEERRORUNABLETOEQUIP",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_SLOT_EMPTY",
        ERRORTEXT_UIERROR,
        "GAMEERRORUNABLETOEQUIP",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_ITEM_NOT_FOUND",
        ERRORTEXT_UIERROR,
        "GAMEERRORUNABLETOEQUIP",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_TOO_FEW_TO_SPLIT",
        ERRORTEXT_UIERROR,
        "GAMEERRORUNABLETOEQUIP",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_SPLIT_FAILED",
        ERRORTEXT_UIERROR,
        "GAMEERRORUNABLETOEQUIP",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_NOT_ENOUGH_GOLD",
        ERRORTEXT_UIERROR,
        "GAMEERRORUNABLETOEQUIP",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_NOT_A_BAG",
        ERRORTEXT_UIERROR,
        "GAMEERRORUNABLETOEQUIP",
        VUI_NOTABAG,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_NOT_OWNER",
        ERRORTEXT_UIERROR,
        "GAMEERRORUNABLETOEQUIP",
        VUI_NOTOWNER,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_ONLY_ONE_QUIVER",
        ERRORTEXT_UIERROR,
        "GAMEERRORUNABLETOEQUIP",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_NO_BANK_SLOT",
        ERRORTEXT_UIERROR,
        "GAMEERRORUNABLETOEQUIP",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_NO_BANK_HERE",
        ERRORTEXT_UIERROR,
        "GAMEERRORUNABLETOEQUIP",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_ITEM_LOCKED",
        ERRORTEXT_UIERROR,
        "GAMEERRORUNABLETOEQUIP",
        VUI_ITEMLOCKED,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_2HANDED_EQUIPPED",
        ERRORTEXT_UIERROR,
        "GAMEERRORUNABLETOEQUIP",
        VUI_CANTEQUIP_2HEQUIPPED,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_VENDOR_NOT_INTERESTED",
        ERRORTEXT_UIERROR,
        "GAMEERRORUNABLETOEQUIP",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_VENDOR_HATES_YOU",
        ERRORTEXT_UIERROR,
        "GAMEERRORUNABLETOEQUIP",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_VENDOR_SOLD_OUT",
        ERRORTEXT_UIERROR,
        "GAMEERRORUNABLETOEQUIP",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_VENDOR_TOO_FAR",
        ERRORTEXT_UIERROR,
        "GAMEERRORUNABLETOEQUIP",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_NOT_ENOUGH_MONEY",
        ERRORTEXT_UIERROR,
        "GAMEERRORUNABLETOEQUIP",
        VUI_NOTENOUGHMONEY,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_RECEIVE_ITEM_S",
        ERRORTEXT_CHAT,
        "ITEMGENERICSOUND",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_DROP_BOUND_ITEM",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_BOUND_NODROP,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_TRADE_BOUND_ITEM",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_CANTTRADE_SOULBOUND,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_TRADE_QUEST_ITEM",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_CANTTRADE_SOULBOUND,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_TRADE_GROUND_ITEM",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC("ERR_TRADE_BAG", ERRORTEXT_UIERROR, "NONE", VUI_CANTTRADE_SOULBOUND, 1, SLASH_CMD_SYSTEM),
    GAMEERRORDESC(
        "ERR_SPELL_FAILED_S",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_ITEM_COOLDOWN",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_ITEMCOOLING,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_POTION_COOLDOWN",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_CANTDRINKMORE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_FOOD_COOLDOWN",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_CANTEATMORE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_SPELL_COOLDOWN",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_SPELLCOOLING,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_ABILITY_COOLDOWN",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_ABILITYCOOLING,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_SPELL_ALREADY_KNOWN_S",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_CANTLEARN_LEVEL,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_SKILL_GAINED_S",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SKILL
    ),
    GAMEERRORDESC(
        "ERR_SKILL_UP_SI",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SKILL
    ),
    GAMEERRORDESC(
        "ERR_LEARN_SPELL_S",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_LEARN_ABILITY_S",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_LEARN_RECIPE_S",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_INVITE_PLAYER_S",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_INVITED_TO_GROUP_S",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_ALREADY_IN_GROUP_S",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_CANTINVITE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_PLAYER_BUSY_S",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_NEW_LEADER_S",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_NEW_LEADER_YOU",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_LEFT_GROUP_S",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_LEFT_GROUP_YOU",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_GROUP_DISBANDED",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_DECLINE_GROUP_S",
        ERRORTEXT_CHAT,
        "igPlayerInviteDecline",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_JOINED_GROUP_S",
        ERRORTEXT_CHAT,
        "igPlayerInviteAccept",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_UNINVITE_YOU",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_BAD_PLAYER_NAME_S",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_NOT_IN_GROUP",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_TARGET_NOT_IN_GROUP_S",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_GROUP_FULL",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_NOT_LEADER",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_PLAYER_DIED_S",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_GUILD_CREATE_S",
        ERRORTEXT_CHAT,
        "LEVELUP",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_GUILD_INVITE_S",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_INVITED_TO_GUILD_SS",
        ERRORTEXT_CHAT,
        "LEVELUP",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_ALREADY_IN_GUILD_S",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_ALREADY_INVITED_TO_GUILD_S",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_INVITED_TO_GUILD",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_ALREADY_IN_GUILD",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_GUILD_ACCEPT",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_GUILD_DECLINE_S",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_GUILD_PERMISSIONS",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_GUILDPERMISSIONS,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_GUILD_JOIN_S",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_GUILD_FOUNDER_S",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_GUILD_PROMOTE_SS",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_GUILD_DEMOTE_SS",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_GUILD_QUIT_S",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_GUILD_LEAVE_S",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_GUILD_REMOVE_SS",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_GUILD_REMOVE_SELF",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_GUILD_DISBAND_S",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_GUILD_DISBAND_SELF",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_GUILD_LEADER_S",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_GUILD_LEADER_SELF",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_GUILD_MOTD_S",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_GUILD_PLAYER_NOT_FOUND_S",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_GUILD_PLAYER_NOT_IN_GUILD_S",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_GUILD_PLAYER_NOT_IN_GUILD",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_GUILD_CANT_PROMOTE_S",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_GUILD_CANT_DEMOTE_S",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_GUILD_NOT_IN_A_GUILD",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_GUILD_INTERNAL",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_GUILD_LEADER_IS_S",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_GUILD_LEADER_CHANGED_SS",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_GUILD_DISBANDED",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_GUILD_NOT_ALLIED",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_GUILD_LEADER_LEAVE",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_GUILD_NAME_INVALID",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_GUILD_NAME_EXISTS_S",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_GUILD_ENTER_NAME",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_GUILD_NAME_TOO_SHORT",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_GUILD_NAME_MIXED_LANGUAGES",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_GUILD_NAME_PROFANE",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_GUILD_NAME_RESERVED",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_NO_GUILD_CHARTER",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_OUT_OF_RANGE",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_TARGETTOOFAR,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_PLAYER_DEAD",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_CLIENT_LOCKED_OUT",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_KILLED_BY_S",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_LOOT_LOCKED",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_CANTLOOT_LOCKED,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_LOOT_TOO_FAR",
        ERRORTEXT_UIERROR,
        "GAMEERRORINVALIDTARGET",
        VUI_CANTLOOT_TOOFAR,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_LOOT_DIDNT_KILL",
        ERRORTEXT_UIERROR,
        "GAMEERRORINVALIDTARGET",
        VUI_CANTLOOT_DIDNTKILL,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_LOOT_BAD_FACING",
        ERRORTEXT_UIERROR,
        "GAMEERRORINVALIDTARGET",
        VUI_CANTLOOT_WRONGFACING,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_LOOT_NOTSTANDING",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_CANTLOOT_NOTSTANDING,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_LOOT_STUNNED",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_LOOT_NO_UI",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_QUEST_ACCEPTED_S",
        ERRORTEXT_CHAT,
        "QUESTADDED",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_QUEST_COMPLETE_S",
        ERRORTEXT_CHAT,
        "igQuestListComplete",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_QUEST_FAILED_S",
        ERRORTEXT_CHAT,
        "igQuestFailed",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_QUEST_FAILED_BAG_FULL_S",
        ERRORTEXT_CHAT,
        "igQuestFailed",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_QUEST_FAILED_MAX_COUNT_S",
        ERRORTEXT_CHAT,
        "igQuestFailed",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_QUEST_FAILED_LOW_LEVEL",
        ERRORTEXT_CHAT,
        "igQuestFailed",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_QUEST_FAILED_MISSING_ITEMS",
        ERRORTEXT_CHAT,
        "igQuestFailed",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_QUEST_REWARD_EXP_I",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_QUEST_REWARD_ITEM_S",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_QUEST_REWARD_MONEY_S",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_QUEST_MUST_CHOOSE",
        ERRORTEXT_UIERROR,
        "igQuestFailed",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_QUEST_LOG_FULL",
        ERRORTEXT_UIERROR,
        "igQuestFailed",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_COMBAT_DAMAGE_SSI",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC("ERR_INSPECT_S", ERRORTEXT_CHAT, "NONE", VUI_NONE, 1, SLASH_CMD_SYSTEM),
    GAMEERRORDESC(
        "ERR_CANT_USE_ITEM",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_CANTUSEITEM,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_MUST_EQUIP_ITEM",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_MUSTEQUIPPITEM,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_PASSIVE_ABILITY",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_2HSKILLNOTFOUND",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_CANTEQUIP2H_SKILL,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_NO_ATTACK_TARGET",
        ERRORTEXT_UIERROR,
        "GAMEERROROUTOFRANGE",
        VUI_CANTATTACK_NOTARGET,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_INVALID_ATTACK_TARGET",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_INVALIDTARGET,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_ATTACK_PACIFIED",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_ATTACK_DEAD",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_HUNGER_VERY_LOW",
        ERRORTEXT_UIINFO,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_HUNGER_LOW",
        ERRORTEXT_UIINFO,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_HUNGER_MED",
        ERRORTEXT_UIINFO,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_HUNGER_HIGH",
        ERRORTEXT_UIINFO,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_HUNGER_SATIATED",
        ERRORTEXT_UIINFO,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_THIRST_VERY_LOW",
        ERRORTEXT_UIINFO,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_THIRST_LOW",
        ERRORTEXT_UIINFO,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_THIRST_MED",
        ERRORTEXT_UIINFO,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_THIRST_HIGH",
        ERRORTEXT_UIINFO,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_THIRST_SATIATED",
        ERRORTEXT_UIINFO,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_TAXISAMENODE",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_TAXINOSUCHPATH",
        ERRORTEXT_UIINFO,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_TAXIUNSPECIFIEDSERVERERROR",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_TAXINOTENOUGHMONEY",
        ERRORTEXT_UIINFO,
        "NONE",
        VUI_CANTTAXI_NOMONEY,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_TAXITOOFARAWAY",
        ERRORTEXT_UIINFO,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_TAXINOVENDORNEARBY",
        ERRORTEXT_UIINFO,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_TAXINOTVISITED",
        ERRORTEXT_UIINFO,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_TAXIPLAYERBUSY",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_TAXIPLAYERALREADYMOUNTED",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_TAXIPLAYERSHAPESHIFTED",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_TAXIPLAYERMOVING",
        ERRORTEXT_UIINFO,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_TAXINOPATHS",
        ERRORTEXT_UIINFO,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_NO_REPLY_TARGET",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_GENERIC_NO_TARGET",
        ERRORTEXT_UIERROR,
        "GAMEERRORINVALIDTARGET",
        VUI_GENERICNOTARGET,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_INITIATE_TRADE_S",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_TRADE_REQUEST_S",
        ERRORTEXT_CHAT,
        "LEVELUP",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_TRADE_TOO_FAR",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_TRADE_CANCELLED",
        ERRORTEXT_UIINFO,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_TRADE_COMPLETE",
        ERRORTEXT_UIINFO,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_TRADE_BAG_FULL",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_TRADE_TARGET_BAG_FULL",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_TRADE_MAX_COUNT_EXCEEDED",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_TRADE_TARGET_MAX_COUNT_EXCEEDED",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_MOUNT_INVALIDMOUNTEE",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_MOUNT_TOOFARAWAY",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_MOUNT_ALREADYMOUNTED",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_MOUNT_NOTMOUNTABLE",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_MOUNT_NOTYOURPET",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_MOUNT_OTHER",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_MOUNT_LOOTING",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_MOUNT_RACECANTMOUNT",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_MOUNT_SHAPESHIFTED",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_DISMOUNT_NOPET",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_DISMOUNT_NOTMOUNTED",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_DISMOUNT_NOTYOURPET",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_SPELL_FAILED_TOTEMS",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_SPELL_FAILED_REAGENTS",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_SPELL_FAILED_EQUIPPED_ITEM",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_SPELL_FAILED_EQUIPPED_ITEM_CLASS_S",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_SPELL_FAILED_SHAPESHIFT_FORM_S",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_BADATTACKFACING",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_BADATTACKPOS",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_CHEST_IN_USE",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_CHESTINUSE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_USE_CANT_OPEN",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_USE_LOCKED",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_ITEMLOCKED,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_USE_LOCKED_WITH_ITEM_S",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_USE_LOCKED_WITH_SPELL_S",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_USE_LOCKED_WITH_SPELL_KNOWN_SI",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_USE_TOO_FAR",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_CANTUSETOOFAR,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_USE_BAD_ANGLE",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_USE_OBJECT_MOVING",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_USE_SPELL_FOCUS",
        ERRORTEXT_UIINFO,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_USE_DESTROYED",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_CANTATTACK_NOTSTANDING",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_CANTATTACK_NOTSTANDING,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_SET_LOOT_FREEFORALL",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_SET_LOOT_ROUNDROBIN",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_SET_LOOT_MASTER",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_NEW_LOOT_MASTER_S",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_SPECIFY_MASTER_LOOTER",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_TAME_FAILED",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_CHAT_WHILE_DEAD",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_NEWTAXIPATH",
        ERRORTEXT_UIERROR,
        "TaxiNodeDiscovered",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC("ERR_NO_PET", ERRORTEXT_UIERROR, "NONE", VUI_NONE, 0, SLASH_CMD_SYSTEM),
    GAMEERRORDESC(
        "ERR_NOTYOURPET",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_PET_NOT_RENAMEABLE",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_NULL_PETNAME",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_INVALID_PETNAME",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_QUEST_OBJECTIVE_COMPLETE_S",
        ERRORTEXT_UIINFO,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_QUEST_UNKNOWN_COMPLETE",
        ERRORTEXT_UIINFO,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_QUEST_ADD_KILL_SII",
        ERRORTEXT_UIINFO,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_QUEST_ADD_FOUND_SII",
        ERRORTEXT_UIINFO,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_QUEST_ADD_ITEM_SII",
        ERRORTEXT_UIINFO,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_CANNOTCREATEDIRECTORY",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_CANNOTCREATEFILE",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_PLAYER_WRONG_FACTION",
        ERRORTEXT_UIERROR,
        "GAMEERRORINVALIDTARGET",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_BANKSLOT_FAILED_TOO_MANY",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_BANKSLOT_INSUFFICIENT_FUNDS",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_CANTAFFORDBANKSLOT,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_BANKSLOT_NOTBANKER",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_FRIEND_DB_ERROR",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_FRIEND_LIST_FULL",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_FRIEND_ADDED_S",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_FRIEND_ONLINE_S",
        ERRORTEXT_CHAT,
        "FRIENDJOINGAME",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_FRIEND_OFFLINE_S",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_FRIEND_NOT_FOUND",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_FRIEND_WRONG_FACTION",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_FRIEND_REMOVED_S",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_FRIEND_ERROR",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_FRIEND_ALREADY_S",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_FRIEND_SELF",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_IGNORE_FULL",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_IGNORE_SELF",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_IGNORE_NOT_FOUND",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_IGNORE_ALREADY_S",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_IGNORE_ADDED_S",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_IGNORE_REMOVED_S",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_ONLY_ONE_BOLT",
        ERRORTEXT_UIERROR,
        "GAMEERRORUNABLETOEQUIP",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_ONLY_ONE_AMMO",
        ERRORTEXT_UIERROR,
        "GAMEERRORUNABLETOEQUIP",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_SPELL_FAILED_EQUIPPED_SPECIFIC_ITEM",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_WRONG_BAG_TYPE_SUBCLASS",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_CANT_WRAP_STACKABLE",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_CANT_WRAP_EQUIPPED",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_CANT_WRAP_WRAPPED",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_CANT_WRAP_BOUND",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_CANT_WRAP_UNIQUE",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_CANT_WRAP_BAGS",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_OUT_OF_MANA",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NOMANA,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_OUT_OF_RAGE",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_OUT_OF_FOCUS",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_OUT_OF_ENERGY",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_OUT_OF_HEALTH",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC("ERR_LOOT_GONE", ERRORTEXT_UIERROR, "NONE", VUI_NONE, 1, SLASH_CMD_SYSTEM),
    GAMEERRORDESC(
        "ERR_MOUNT_FORCEDDISMOUNT",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_AUTOFOLLOW_TOO_FAR",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_UNIT_NOT_FOUND",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_INVALID_FOLLOW_TARGET",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_GUILDEMBLEM_SUCCESS",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_GUILDEMBLEM_INVALID_TABARD_COLORS",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_GUILDEMBLEM_NOGUILD",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_GUILDEMBLEM_COLORSPRESENT",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_GUILDEMBLEM_NOTGUILDMASTER",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_GUILDEMBLEM_NOTENOUGHMONEY",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_GUILDEMBLEM_INVALIDVENDOR",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_SPELL_OUT_OF_RANGE",
        ERRORTEXT_UIERROR,
        "GAMEERROROUTOFRANGE",
        VUI_CANTCAST_OUTOFRANGE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_COMMAND_NEEDS_TARGET",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC("ERR_NOAMMO_S", ERRORTEXT_UIERROR, "NONE", VUI_OUTOFAMMO, 1, SLASH_CMD_SYSTEM),
    GAMEERRORDESC(
        "ERR_TOOBUSYTOFOLLOW",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        1,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_DUEL_REQUESTED",
        ERRORTEXT_CHAT,
        "LEVELUP",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_DUEL_CANCELLED",
        ERRORTEXT_UIINFO,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_DEATHBINDALREADYBOUND",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_NOEMOTEWHILERUNNING",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_ZONE_EXPLORED",
        ERRORTEXT_UIINFO,
        "TaxiNodeDiscovered",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_ZONE_EXPLORED_XP",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_INVALID_ITEM_TARGET",
        ERRORTEXT_UIERROR,
        "GAMEERRORINVALIDTARGET",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_IGNORING_YOU_S",
        ERRORTEXT_CHAT,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_FISH_NOT_HOOKED",
        ERRORTEXT_UIINFO,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_FISH_ESCAPED",
        ERRORTEXT_UIINFO,
        "GAMEERRORINVALIDTARGET",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_SPELL_FAILED_NOTUNSHEATHED",
        ERRORTEXT_UIINFO,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_PETITION_SIGNED",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_PETITION_ALREADY_SIGNED",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_PETITION_IN_GUILD",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_PETITION_CREATOR",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    ),
    GAMEERRORDESC(
        "ERR_PETITION_NOT_ENOUGH_SIGNATURES",
        ERRORTEXT_UIERROR,
        "NONE",
        VUI_NONE,
        0,
        SLASH_CMD_SYSTEM
    )
};

bool          CGGameUI::m_reloadUI;
UINT          CGGameUI::m_stackSplit;
DWORDLONG     CGGameUI::m_cursorItem;
DWORDLONG     CGGameUI::m_cursorItemContainer;
UINT          CGGameUI::m_cursorItemSlot;
UINT          CGGameUI::m_cursorMoney;
int           CGGameUI::m_cursorSpell;
UINT          CGGameUI::m_cursorPetAction;
UINT          CGGameUI::m_cursorVirtualID;
UINT          CGGameUI::m_cursorVirtualDisplay;
UINT          CGGameUI::m_cursorVirtualSlot;
int           CGGameUI::m_cursorHasAction;
UICURSORTYPE  CGGameUI::m_cursorItemType;
DWORDLONG     CGGameUI::m_currentObjectTrack;
float         CGGameUI::m_interactMaxDist;
DWORDLONG     CGGameUI::m_interactTarget;
DWORDLONG     CGGameUI::m_lockedTarget;
DWORDLONG     CGGameUI::m_lastEnemyTarget;
char         *CGGameUI::m_zoneText;
char         *CGGameUI::m_subZoneText;
char         *CGGameUI::m_minimapZoneText;
int           CGGameUI::m_areaID;
CSimpleFrame *CGGameUI::m_UISimpleParent;
CSimpleTop   *CGGameUI::m_simpleTop;
CGTooltip    *CGGameUI::m_gameTooltip;
int           CGGameUI::m_screenWidth;
int           CGGameUI::m_hasControl;
CinematicData CGGameUI::m_cinematic;
char          CGGameUI::s_lastErrorString[512];

LPCSTR g_invTypeTokens[27] = {
    "",
    "INVTYPE_HEAD",
    "INVTYPE_NECK",
    "INVTYPE_SHOULDER",
    "INVTYPE_BODY",
    "INVTYPE_CHEST",
    "INVTYPE_WAIST",
    "INVTYPE_LEGS",
    "INVTYPE_FEET",
    "INVTYPE_WRIST",
    "INVTYPE_HAND",
    "INVTYPE_FINGER",
    "INVTYPE_TRINKET",
    "INVTYPE_WEAPON",
    "INVTYPE_SHIELD",
    "INVTYPE_RANGED",
    "INVTYPE_CLOAK",
    "INVTYPE_2HWEAPON",
    "INVTYPE_BAG",
    "INVTYPE_TABARD",
    "INVTYPE_ROBE",
    "INVTYPE_WEAPONMAINHAND",
    "INVTYPE_WEAPONOFFHAND",
    "INVTYPE_HOLDABLE",
    "INVTYPE_AMMO",
    "INVTYPE_THROWN",
    "INVTYPE_RANGEDRIGHT"
};
static LPSTR compasDirStr[8] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
static LPSTR s_spellMissReasons[10] = {"NONE", "PHYSICAL", "RESIST", "IMMUNE", "EVADED", "DODGED", "PARRIED", "BLOCKED", "TEMPIMMUNE", "DEFLECTED"};
static LPSTR s_combatEvent[9] = {"MISS", "WOUND", "DODGE", "PARRY", "INTERRUPT", "BLOCK", "EVADE", "IMMUNE", "DEFLECT"};
static const float s_distCullValues[3] = {350.0f, 550.0f, 750.0f};
static const float s_smallCullValues[3] = {0.07f, 0.04f, 0.01f};
static const float TARGET_NEAREST_MAX_DISTANCE_SQUARED = 400.0f;
static const float CinematicFadeTime = 0.25f;
static LPCSTR      s_screenResolutions[4] = {"800x600", "1024x768", "1280x1024", "1600x1200"};

struct ItemPushInfo {
  DWORDLONG player;
  int       slot;
  int       pushed;
  int       display;
};

bool Spell_C_IsTargeting();
bool Spell_C_HandleSpriteClick(CGObject_C *object);
bool Spell_C_HandleTerrainClick(const CTerrainClickEvent &evt);
void Trade_C_InitiateTrade(DWORDLONG target, int useCursorItem);

static BOOL CCommand_Script(LPCSTR, LPCSTR arguments) {
  FrameScript_Execute(arguments, arguments);
  return 1;
}

void EnableFadingScreen(float fadeTime, void (*fadedCallback)(LPVOID), LPVOID param);
void DisableFadingScreen(float fadeTime, void (*fadedCallback)(LPVOID), LPVOID param);

static BOOL CCommand_ScaleUI(LPCSTR, LPCSTR arguments) {
  float scale = SStrToFloat(arguments);
  if (scale > 0.0f) {
    CGGameUI::ScaleUI(scale, 0);
  }
  return 1;
}

static int Script_FrameXML_Debug(lua_State *L) {
  int level = FrameXML_GetDebugLevel();
  if (lua_isnumber(L, 1)) {
    level = lua_tonumber(L, 1);
    FrameXML_SetDebugLevel(level);
  }
  return 1;
}

static int Script_ReloadUI(lua_State *L) {
  CGGameUI::Reload();
  return 0;
}

static int Script_SetLayoutMode(lua_State *L) {
  int mode = 1;
  if (lua_isnumber(L, 1)) {
    mode = lua_tonumber(L, 1);
  }
  CSimpleTop::GetInstance()->SetLayoutMode(mode);
  return 0;
}

static int Script_IsShiftKeyDown(lua_State *L) {
  if (EventIsKeyDown(KEY_SHIFT)) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int Script_IsControlKeyDown(lua_State *L) {
  if (EventIsKeyDown(KEY_CONTROL)) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int Script_IsAltKeyDown(lua_State *L) {
  if (EventIsKeyDown(KEY_ALT)) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int Script_GetDebugStats(lua_State *L) {
  char               buffer[1024];
  int                counts[32];
  char               tempBuffer[128];
  float              dayProgression;

  SStrPrintf(buffer, sizeof(buffer), "%02d:%02d", g_clientGameTime.m_hour, g_clientGameTime.m_minute);

  CGObject_C *player = ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__);
  if (player) {
    SStrPack(buffer, "\nPlayer position: ", sizeof(buffer));
    NTempest::C3Vector pos = player->GetPosition();
    SStrPrintf(tempBuffer, sizeof(tempBuffer), "%d, %d, %d\n", (int)pos.x, (int)pos.y, (int)pos.z);
    SStrPack(buffer, tempBuffer, sizeof(buffer));
    SStrPack(buffer, "Player facing: ", sizeof(buffer));

    float facing = fmod(360.0f - player->GetFacing() * 57.29578f + 22.5f, 360.0);
    if (facing < 0.0f) {
      facing += 360.0f;
    }
    int direction = facing * 0.022222223f;
    if ((UINT)direction > 7) {
      direction = ~(direction >> 31) & 7;
    }
    SStrPrintf(tempBuffer, sizeof(tempBuffer), "%-2s\n", compasDirStr[direction]);
    SStrPack(buffer, tempBuffer, sizeof(buffer));
  }

  LPCSTR chunkName = CWorld::QueryChunkName();
  if (!chunkName || !*chunkName) {
    chunkName = "none";
  }
  SStrPrintf(tempBuffer, sizeof(tempBuffer), "Chunk %s\n", chunkName);
  SStrPack(buffer, tempBuffer, sizeof(buffer));

  dayProgression = g_clientGameTime.GameTimeGetDayProgression();
  CWorld::GetCounts(counts);
  SStrPrintf(tempBuffer, sizeof(tempBuffer), "A: %04d %04d\n", counts[0], counts[11]);
  SStrPack(buffer, tempBuffer, sizeof(buffer));
  SStrPrintf(tempBuffer, sizeof(tempBuffer), "DD:%04d %04d\n", counts[1], counts[12]);
  SStrPack(buffer, tempBuffer, sizeof(buffer));
  SStrPrintf(tempBuffer, sizeof(tempBuffer), "C: %04d %04d\n", counts[2], counts[13]);
  SStrPack(buffer, tempBuffer, sizeof(buffer));
  SStrPrintf(tempBuffer, sizeof(tempBuffer), "L: %04d %04d\n", counts[3], counts[14]);
  SStrPack(buffer, tempBuffer, sizeof(buffer));
  SStrPrintf(tempBuffer, sizeof(tempBuffer), "T: %04d %04d\n", counts[4], counts[15]);
  SStrPack(buffer, tempBuffer, sizeof(buffer));
  SStrPrintf(tempBuffer, sizeof(tempBuffer), "MD:%04d %04d\n", counts[5], counts[16]);
  SStrPack(buffer, tempBuffer, sizeof(buffer));
  SStrPrintf(tempBuffer, sizeof(tempBuffer), "MG:%04d %04d\n", counts[6], counts[17]);
  SStrPack(buffer, tempBuffer, sizeof(buffer));
  SStrPrintf(tempBuffer, sizeof(tempBuffer), "E: %04d %04d\n", counts[7], counts[18]);
  SStrPack(buffer, tempBuffer, sizeof(buffer));
  SStrPrintf(tempBuffer, sizeof(tempBuffer), "L: %04d %04d\n", counts[8], counts[19]);
  SStrPack(buffer, tempBuffer, sizeof(buffer));
  SStrPrintf(tempBuffer, sizeof(tempBuffer), "BL:%04d %04d\n", counts[9], counts[20]);
  SStrPack(buffer, tempBuffer, sizeof(buffer));
  SStrPrintf(tempBuffer, sizeof(tempBuffer), "CL:%04d %04d\n", counts[10], counts[21]);
  SStrPack(buffer, tempBuffer, sizeof(buffer));
  SStrPrintf(tempBuffer, sizeof(tempBuffer), "Day Progression: %0.06f\n", dayProgression);
  SStrPack(buffer, tempBuffer, sizeof(buffer));
  lua_pushstring(L, buffer);
  return 1;
}

static int Script_GetCVar(lua_State *L) {
  char message[512];
  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: GetCVar(\"cvar\")");
    return 0;
  }

  CVar *cvar = CVar::Lookup(lua_tostring(L, 1));
  if (!cvar) {
    SStrPrintf(message, sizeof(message), "Couldn't find CVar named '%s'", lua_tostring(L, 1));
    luaL_error(L, message);
    return 0;
  }

  lua_pushstring(L, cvar->GetString());
  return 1;
}

static int Script_SetCVar(lua_State *L) {
  char message[512];
  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: SetCVar(\"cvar\", value [, \"scriptCvar\")");
    return 0;
  }

  CVar *cvar = CVar::Lookup(lua_tostring(L, 1));
  if (!cvar) {
    SStrPrintf(message, sizeof(message), "Couldn't find CVar named '%s'", lua_tostring(L, 1));
    luaL_error(L, message);
    return 0;
  }

  LPCSTR value = lua_tostring(L, 2);
  if (!value) {
    value = "0";
  }
  cvar->Set(value, 1, 0, 0);

  if (lua_isstring(L, 3)) {
    FrameScript_SignalEvent(289, "%s%s", lua_tostring(L, 3), value);
  }
  return 0;
}

static int Script_GetWorldDetail(lua_State *L) {
  float distCull = CVar::Lookup("distCull")->GetFloat();
  int   terrainDetail = 2;
  for (int i = 0; i < 3; ++i) {
    if (distCull <= s_distCullValues[i]) {
      terrainDetail = i;
      break;
    }
  }
  lua_pushnumber(L, terrainDetail);
  return 1;
}

static int Script_SetWorldDetail(lua_State *L) {
  char buf[32];
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: SetWorldDetail(value)");
    return 0;
  }

  int terrainDetail = lua_tonumber(L, 1);
  if (terrainDetail >= 0 && terrainDetail < 3) {
    SStrPrintf(buf, sizeof(buf), "%f", s_distCullValues[terrainDetail]);
    CVar *cvar = CVar::Lookup("DistCull");
    cvar->Set(buf, 1, 0, 0);
    SStrPrintf(buf, sizeof(buf), "%f", s_smallCullValues[terrainDetail]);
    cvar = CVar::Lookup("smallCull");
    cvar->Set(buf, 1, 0, 0);
    return 0;
  }
  luaL_error(L, "value must be in the range 0, 2");
  return 0;
}

static int Script_GetWaterDetail(lua_State *L) {
  lua_pushnumber(L, 0.0);
  return 1;
}

static int Script_SetWaterDetail(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: SetWaterDetail(value)");
  }
  return 0;
}

static int Script_GetFarclip(lua_State *L) {
  lua_pushnumber(L, CVar::Lookup("farclip")->GetFloat());
  return 1;
}

static int Script_SetFarclip(lua_State *L) {
  char strVal[16];
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: SetFarclip(value)");
    return 0;
  }

  CVar  *cvar = CVar::Lookup("farclip");
  double value = lua_tonumber(L, 1);
  SStrPrintf(strVal, sizeof(strVal), "%f", value);
  cvar->Set(strVal, 1, 0, 0);
  return 0;
}

static int Script_GetTerrainMip(lua_State *L) {
  lua_pushnumber(L, 1.0 - CVar::Lookup("alphaLevel")->GetInt());
  return 1;
}

static int Script_SetTerrainMip(lua_State *L) {
  char buf[16];
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: SetTerrainMip(value)");
    return 0;
  }

  int value = 1 - (int)lua_tonumber(L, 1);
  SStrPrintf(buf, sizeof(buf), "%d", value);
  CVar *cvar = CVar::Lookup("alphaLevel");
  cvar->Set(buf, 1, 0, 0);
  SStrPrintf(buf, sizeof(buf), "%d", value);
  cvar = CVar::Lookup("shadowLevel");
  cvar->Set(buf, 1, 0, 0);
  return 0;
}

static int Script_GetDoodadAnim(lua_State *L) {
  lua_pushnumber(L, CVar::Lookup("doodadAnim")->GetFloat());
  return 1;
}

static int Script_SetDoodadAnim(lua_State *L) {
  char strVal[16];
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: SetDoodadAnim(value)");
    return 0;
  }

  CVar  *cvar = CVar::Lookup("doodadAnim");
  double value = lua_tonumber(L, 1);
  SStrPrintf(strVal, sizeof(strVal), "%f", value);
  cvar->Set(strVal, 1, 0, 0);
  return 0;
}

static int Script_GetTexLodBias(lua_State *L) {
  lua_pushnumber(L, CVar::Lookup("texLodBias")->GetFloat());
  return 1;
}

static int Script_SetTexLodBias(lua_State *L) {
  char strVal[16];
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: SetTexLodBias(value)");
    return 0;
  }

  CVar  *cvar = CVar::Lookup("texLodBias");
  double value = lua_tonumber(L, 1);
  SStrPrintf(strVal, sizeof(strVal), "%f", value);
  cvar->Set(strVal, 1, 0, 0);
  return 0;
}

static int Script_GetGamma(lua_State *L) {
  lua_pushnumber(L, 1.0 - CVar::Lookup("gamma")->GetFloat());
  return 1;
}

static int Script_SetGamma(lua_State *L) {
  char strVal[16];
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: SetGamma(value)");
    return 0;
  }

  CVar  *cvar = CVar::Lookup("gamma");
  double value = 1.0 - lua_tonumber(L, 1);
  SStrPrintf(strVal, sizeof(strVal), "%f", value);
  cvar->Set(strVal, 1, 0, 0);
  return 0;
}

static int Script_ToggleTris(lua_State *) {
  return 0;
}

static int Script_TogglePortals(lua_State *) {
  return 0;
}

static int Script_ToggleCollision(lua_State *) {
  return 0;
}

static int Script_ToggleCollisionDisplay(lua_State *) {
  return 0;
}

static int Script_Logout(lua_State *) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    if (player->IsClientControlled()) {
      ClientServices_CharacterLogout(false);
    }
  }
  return 0;
}

static int Script_Quit(lua_State *) {
  ClientServices_Exit();
  return 0;
}

static int Script_Screenshot(lua_State *L) {
  ClientServices_ReportScreenshot();
  ScrnScreenshot(CGGameUI::HandleScreenshot);
  return 0;
}

static int Script_GetFramerate(lua_State *L) {
  lua_pushnumber(L, CWorld::GetFramerate());
  return 1;
}

static int Script_TogglePerformanceDisplay(lua_State *) {
  ScrnPerfEnable(!ScrnPerfIsEnabled());
  return 0;
}

static int Script_TogglePerformanceValues(lua_State *) {
  ScrnPerfToggleDisplayedValues();
  return 0;
}

static int Script_ResetPerformanceValues(lua_State *) {
  ScrnPerfResetTimePeaks();
  return 0;
}

static int Script_TogglePlayerBounds(lua_State *) {
  CGPlayer_C::TogglePlayerBounds();
  return 0;
}

static int Script_ShowNameplates(lua_State *) {
  CGUnit_C::NamePlateShow(1);
  PlayerNameShow(0);
  return 0;
}

static int Script_HideNameplates(lua_State *) {
  CGUnit_C::NamePlateShow(0);
  PlayerNameShow(1);
  return 0;
}

static int Script_SetCursor(lua_State *L) {
  struct {
    CURSORANIMATIONS animation;
    LPCSTR           string;
  } array[8] = {
      {       POINT_CURSOR,        "POINT_CURSOR"},
      {        CAST_CURSOR,         "CAST_CURSOR"},
      {         BUY_CURSOR,          "BUY_CURSOR"},
      {      ATTACK_CURSOR,       "ATTACK_CURSOR"},
      { POINT_ERROR_CURSOR,  "POINT_ERROR_CURSOR"},
      {  CAST_ERROR_CURSOR,   "CAST_ERROR_CURSOR"},
      {   BUY_ERROR_CURSOR,    "BUY_ERROR_CURSOR"},
      {ATTACK_ERROR_CURSOR, "ATTACK_ERROR_CURSOR"}
  };

  CURSORANIMATIONS animation = NO_CURSOR;
  if (lua_isstring(L, 1)) {
    LPCSTR cursor = lua_tostring(L, 1);
    for (UINT i = 0; i < 8; ++i) {
      if (!SStrCmpI(array[i].string, cursor, INT_MAX)) {
        animation = array[i].animation;
        break;
      }
    }
  }
  if (animation != NO_CURSOR) {
    CursorModelSetSequence(animation);
  } else {
    luaL_error(L, "Usage: SetCursor(\"cursor\")");
  }
  return 0;
}

static int Script_CursorHasItem(lua_State *L) {
  if (CGGameUI::m_cursorItemType == UICURSOR_ITEM) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int Script_CursorHasSpell(lua_State *L) {
  if (CGGameUI::m_cursorItemType == UICURSOR_SPELL) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int Script_CursorHasMoney(lua_State *L) {
  if (CGGameUI::m_cursorItemType == UICURSOR_MONEY) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int Script_EquipCursorItem(lua_State *L) {
  DWORDLONG cursorItemPack;
  DWORDLONG cursorItem;
  UINT      cursorItemSlot;
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: EquipCursorItem(slot)");
    return 0;
  }

  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player) {
    return 0;
  }
  CGGameUI::GetCursorItem(cursorItem, cursorItemPack, cursorItemSlot);
  if (cursorItem) {
    int slot = lua_tonumber(L, 1);
    if (slot != 0xFF && slot != -1) {
      player->SwapItems(cursorItem, cursorItemPack, cursorItemSlot, player->GetGUID(), slot, 1);
    } else {
      player->AutoEquipCursorItem(1);
    }
  }
  return 0;
}

static int Script_DeleteCursorItem(lua_State *) {
  DWORDLONG  cursorItemPack;
  DWORDLONG  cursorItem;
  UINT       cursorItemSlot;
  BYTE       cursorItemPackIndex;

  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    CGGameUI::GetCursorItem(cursorItem, cursorItemPack, cursorItemSlot);
    if (cursorItem) {
      cursorItemPackIndex = player->FindSlotIndex(cursorItemPack);
      CDataStore msg;
      msg.Put(CMSG_DESTROYITEM);
      msg.Put(cursorItemPackIndex);
      msg.Put((BYTE)cursorItemSlot);
      msg.Put(CGGameUI::m_stackSplit);
      msg.Finalize();
      ClientServices_Send(&msg);
      CGGameUI::ClearCursor(1);
      CGGameUI::LockItem(cursorItem);
    }
  }
  return 0;
}

static int Script_EquipPendingItem(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: EquipPendingItem(index)");
    return 0;
  }
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    player->ClearPendingEquip(lua_tonumber(L, 1), 1);
  }
  return 0;
}

static int Script_CancelPendingEquip(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: CancelPendingEquip(index)");
    return 0;
  }
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    player->ClearPendingEquip(lua_tonumber(L, 1), 0);
  }
  return 0;
}

static int Script_TargetUnit(lua_State *L) {
  if (lua_isstring(L, 1)) {
    DWORDLONG guid = Script_GetGUIDFromName(lua_tostring(L, 1));
    if (ClntObjMgrObjectPtr(guid, __FILE__, __LINE__) || CGPartyInfo::IsMember(guid)) {
      CGGameUI::Target(guid, 0);
    }
  } else {
    luaL_error(L, "Usage: TargetUnit(\"unit\")");
  }
  return 0;
}

static int Script_TargetUnitsPet(lua_State *L) {
  if (lua_isstring(L, 1)) {
    CGUnit_C *unit = Script_GetUnitFromName(lua_tostring(L, 1));
    if (unit) {
      DWORDLONG petGUID = *(unit->GetCharm() ? &unit->GetCharm() : &unit->GetSummon());
      if (petGUID) {
        CGGameUI::Target(petGUID, 0);
      }
    }
  } else {
    luaL_error(L, "Usage: TargetUnitsPet(\"unit\")");
  }
  return 0;
}

static int Script_TargetNearestEnemy(lua_State *L) {
  int reverse = 0;
  if (lua_isnumber(L, 1)) {
    reverse = lua_tonumber(L, 1);
  } else if (lua_isstring(L, 1)) {
    reverse = StringToBOOL(lua_tostring(L, 1));
  }
  CGGameUI::TargetNearestEnemy(reverse);
  return 0;
}

static int Script_TargetLastEnemy(lua_State *) {
  DWORDLONG target = CGGameUI::m_lastEnemyTarget;
  if (target) {
    CGGameUI::Target(target, 0);
  }
  return 0;
}

static int Script_AttackTarget(lua_State *) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    player->OnAttackIconPressed();
  }
  return 0;
}

static int Script_AssistUnit(lua_State *L) {
  if (lua_isstring(L, 1)) {
    CGUnit_C *unit = Script_GetUnitFromName(lua_tostring(L, 1));
    if (unit) {
      DWORDLONG newTarget = 0;
      if (unit->IsA(ID_PLAYER)) {
        newTarget = ((CGPlayer_C *)unit)->GetSelection();
      } else {
        newTarget = unit->GetTarget();
      }
      if (newTarget) {
        CGGameUI::Target(newTarget, 0);
        if (s_assistAttackCVar->GetInt()) {
          CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
          if (player) {
            player->SetCombatMode(1);
          }
        }
      }
    } else {
      CGGameUI::DisplayError(GERR_GENERIC_NO_TARGET);
    }
  } else {
    luaL_error(L, "Usage: AssistUnit(\"unit\")");
  }
  return 0;
}

static int Script_AssistByName(lua_State *L) {
  if (lua_isstring(L, 1)) {
    CGGameUI::AssistByName(lua_tostring(L, 1));
  } else {
    luaL_error(L, "Usage: AssistByName(\"name\")");
  }
  return 0;
}

static int Script_FollowUnit(lua_State *L) {
  if (lua_isstring(L, 1)) {
    CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
    CGUnit_C   *target = Script_GetUnitFromName(lua_tostring(L, 1));
    if (player && target) {
      if (target->IsA(ID_PLAYER) && target->UnitReaction(player) >= UNIT_REACTION_AMIABLE) {
        player->SaveTrackingTarget(target->GetGUID(), TRACKTYPE_FOLLOW, 0);
      } else {
        CGGameUI::DisplayError(GERR_INVALID_FOLLOW_TARGET);
      }
    } else {
      CGGameUI::DisplayError(GERR_GENERIC_NO_TARGET);
    }
  } else {
    luaL_error(L, "Usage: FollowUnit(\"unit\")");
  }
  return 0;
}

static int Script_FollowByName(lua_State *L) {
  if (lua_isstring(L, 1)) {
    CGGameUI::FollowByName(lua_tostring(L, 1));
  } else {
    luaL_error(L, "Usage: FollowByName(\"name\")");
  }
  return 0;
}

static int Script_ClearTarget(lua_State *L) {
  int hadTarget = CGGameUI::GetLockedTarget() != 0;
  CGGameUI::Target(0, 0);
  if (hadTarget) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int Script_AutoEquipCursorItem(lua_State *) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    player->AutoEquipCursorItem(0);
  }
  return 0;
}

static int Script_ToggleSheath(lua_State *) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    player->ToggleSheathe(0);
  }
  return 0;
}

static int Script_ToggleRun(lua_State *L) {
  DWORD     eventTime = lua_isnumber(L, 1) ? (DWORD)lua_tonumber(L, 1) : OsGetAsyncTimeMs();
  CGUnit_C *mover = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(CGUnit_C::GetActiveMover(), __FILE__, __LINE__));
  if (mover) {
    if ((mover->GetMoveFlags() & 0x200) && (int)(eventTime - mover->GetMoveStartTime()) < 0) {
      eventTime = mover->GetMoveStartTime();
    }

    if (mover->GetHealth() > 0 && mover->IsClientControlled() && !mover->IsInStandSitTransition() && !(mover->GetMoveFlags() & 0x2400)) {
      mover->ToggleRunModeLocal(eventTime);
    }
  }
  return 0;
}

inline float CGUnit::LinearDistanceSquared(const NTempest::C3Vector &position) const {
  return (GetPosition() - position).SquaredMag();
}

inline BYTE CGUnit::IsSitting() const {
  UNITSTANDSTATE standState = (UNITSTANDSTATE)GetStandState();
  return standState == UNIT_SITTING || (standState >= UNIT_FIRSTCHAIRSIT && standState <= UNIT_LASTCHAIRSIT);
}

inline BYTE CGUnit::IsSleeping() const {
  return GetStandState() == UNIT_SLEEPING;
}

static int Script_Jump(lua_State *L) {
  DWORD     eventTime = lua_isnumber(L, 1) ? (DWORD)lua_tonumber(L, 1) : OsGetAsyncTimeMs();
  CGUnit_C *mover = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(CGUnit_C::GetActiveMover(), __FILE__, __LINE__));
  if (mover) {
    if ((mover->GetMoveFlags() & 0x200) && (int)(eventTime - mover->GetMoveStartTime()) < 0) {
      eventTime = mover->GetMoveStartTime();
    }

    if (mover->GetHealth() > 0 && mover->IsClientControlled() && !mover->IsInStandSitTransition() && !(mover->GetMoveFlags() & 0x2400)) {
      if (mover->IsSitting() || mover->IsSleeping()) {
        mover->ChangeStandState(UNIT_STANDING);
      } else {
        mover->OnJumpLocal(eventTime);
      }
    }
  }
  return 0;
}

static int Script_GetZoneText(lua_State *L) {
  lua_pushstring(L, CGGameUI::GetZoneText() ? CGGameUI::GetZoneText() : "");
  return 1;
}

static int Script_GetSubZoneText(lua_State *L) {
  lua_pushstring(L, CGGameUI::GetSubZoneText() ? CGGameUI::GetSubZoneText() : "");
  return 1;
}

static int Script_GetMinimapZoneText(lua_State *L) {
  lua_pushstring(L, CGGameUI::GetMinimapZoneText() ? CGGameUI::GetMinimapZoneText() : "");
  return 1;
}

static int Script_InitiateTrade(lua_State *L) {
  if (lua_isstring(L, 1)) {
    DWORDLONG guid = Script_GetGUIDFromName(lua_tostring(L, 1));
    if (guid) {
      Trade_C_InitiateTrade(guid, 0);
    }
  } else {
    luaL_error(L, "Usage: InitiateTrade(\"unit\")");
  }
  return 0;
}

static int Script_NotifyInspect(lua_State *L) {
  if (lua_isstring(L, 1)) {
    CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
    if (player) {
      player->InspectPlayer(Script_GetGUIDFromName(lua_tostring(L, 1)));
    }
  } else {
    luaL_error(L, "Usage: NotifyInspect(unit)");
  }
  return 0;
}

static int Script_InviteToParty(lua_State *L) {
  if (lua_isstring(L, 1)) {
    CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
    DWORDLONG   guid = Script_GetGUIDFromName(lua_tostring(L, 1));
    if (player && guid) {
      player->InviteToGroup(guid);
    }
  } else {
    luaL_error(L, "Usage: InviteToParty(\"unit\")");
  }
  return 0;
}

static int Script_InviteByName(lua_State *L) {
  if (lua_isstring(L, 1)) {
    CDataStore msg;
    msg.Put(CMSG_GROUP_INVITE);
    msg.PutString(lua_tostring(L, 1));
    msg.Finalize();
    ClientServices_Send(&msg);
  } else {
    luaL_error(L, "Usage: InviteByName(\"name\")");
  }
  return 0;
}

static int Script_UninviteFromParty(lua_State *L) {
  if (lua_isstring(L, 1)) {
    CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
    DWORDLONG   guid = Script_GetGUIDFromName(lua_tostring(L, 1));
    if (player && guid) {
      player->Uninvite(guid);
    }
  } else {
    luaL_error(L, "Usage: UninviteFromParty(\"unit\")");
  }
  return 0;
}

static int Script_UninviteByName(lua_State *L) {
  if (lua_isstring(L, 1)) {
    CDataStore msg;
    msg.Put(CMSG_GROUP_UNINVITE);
    msg.PutString(lua_tostring(L, 1));
    msg.Finalize();
    ClientServices_Send(&msg);
  } else {
    luaL_error(L, "Usage: UninviteByName(\"name\")");
  }
  return 0;
}

static int Script_PromoteToPartyLeader(lua_State *L) {
  if (lua_isstring(L, 1)) {
    CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
    DWORDLONG   guid = Script_GetGUIDFromName(lua_tostring(L, 1));
    if (player && guid) {
      player->SetNewLeader(guid);
    }
  } else {
    luaL_error(L, "Usage: PromoteToPartyLeader(\"unit\")");
  }
  return 0;
}

static int Script_PromoteByName(lua_State *L) {
  if (lua_isstring(L, 1)) {
    CDataStore msg;
    msg.Put(CMSG_GROUP_SET_LEADER);
    msg.PutString(lua_tostring(L, 1));
    msg.Finalize();
    ClientServices_Send(&msg);
  } else {
    luaL_error(L, "Usage: PromoteByName(\"name\")");
  }
  return 0;
}

static int Script_RequestTimePlayed(lua_State *) {
  CDataStore msg;
  msg.Put(CMSG_PLAYED_TIME);
  msg.Finalize();
  ClientServices_Send(&msg);
  return 0;
}

static int Script_RepopMe(lua_State *) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    player->HandleRepopRequest();
  }
  return 0;
}

static int Script_AcceptResurrect(lua_State *) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    player->AcceptResurrectRequest(1);
  }
  return 0;
}

static int Script_DeclineResurrect(lua_State *) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    player->AcceptResurrectRequest(0);
  }
  return 0;
}

static int Script_BeginTrade(lua_State *) {
  Trade_C_BeginTrade();
  return 0;
}

static int Script_CancelTrade(lua_State *) {
  Trade_C_CancelTrade();
  return 0;
}

static int Script_AcceptGroup(lua_State *) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    player->AcceptGroup();
  }
  return 0;
}

static int Script_DeclineGroup(lua_State *) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    player->DeclineGroup();
  }
  return 0;
}

static int Script_AcceptGuild(lua_State *) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    player->AcceptGuild();
  }
  return 0;
}

static int Script_DeclineGuild(lua_State *) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    player->DeclineGuild();
  }
  return 0;
}

static int Script_CancelLogout(lua_State *) {
  ClientServices_CharacterAbortLogout();
  return 0;
}

static int Script_ForceLogout(lua_State *) {
  ClientServices_CharacterForceLogout();
  return 0;
}

static int Script_ForceQuit(lua_State *) {
  EventPostClose();
  return 0;
}

static int Script_ReportBug(lua_State *L) {
  bool success = ClientServices_Report(0, lua_tostring(L, 1), lua_tostring(L, 2));
  CGChat::AddChatMessage(success ? "Bug submitted" : "Bug submission failed", SLASH_CMD_SYSTEM, 0, 0, 0, 0, 0);
  return 0;
}

static int Script_ReportSuggestion(lua_State *L) {
  bool success = ClientServices_Report(1, lua_tostring(L, 1), lua_tostring(L, 2));
  CGChat::AddChatMessage(success ? "Suggestion submitted" : "Suggestion submission failed", SLASH_CMD_SYSTEM, 0, 0, 0, 0, 0);
  return 0;
}

static int Script_ReportNote(lua_State *L) {
  bool success = ClientServices_Report(2, lua_tostring(L, 1), lua_tostring(L, 2));
  CGChat::AddChatMessage(success ? "Note submitted" : "Note submission failed", SLASH_CMD_SYSTEM, 0, 0, 0, 0, 0);
  return 0;
}

static int Script_GetCursorMoney(lua_State *L) {
  lua_pushnumber(L, CGGameUI::GetCursorMoney());
  return 1;
}

static int Script_DropCursorMoney(lua_State *) {
  if (CGGameUI::GetCursorMoney()) {
    CGGameUI::SetCursorMoney(0);
  }
  return 0;
}

static int Script_PickupPlayerMoney(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: PickupPlayerMoney(amount)");
    return 0;
  }
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player) {
    return 0;
  }
  UINT amount = lua_tonumber(L, 1);
  if (amount > 0 && amount <= player->GetMoney()) {
    CGGameUI::SetCursorMoney(amount);
  }
  return 0;
}

static int Script_HideSellCursor(lua_State *) {
  if (CursorGetCursorType() == BUY_CURSOR || CursorGetCursorType() == BUY_ERROR_CURSOR) {
    CursorResetCursor(0);
  }
  return 0;
}

static int Script_HasSoulstone(lua_State *L) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player && player->GetSoulstone()) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int Script_UseSoulstone(lua_State *L) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    player->UseSoulstone();
  }
  return 0;
}

static int Script_JoinChannelByName(lua_State *L) {
  if (lua_isstring(L, 1)) {
    CDataStore msg;
    msg.Put(CMSG_JOIN_CHANNEL);
    msg.PutString(lua_tostring(L, 1));
    msg.Finalize();
    ClientServices_Send(&msg);
  } else {
    luaL_error(L, "Usage: JoinChannelByName(\"name\")");
  }
  return 0;
}

static int Script_LeaveChannelByName(lua_State *L) {
  if (lua_isstring(L, 1)) {
    CDataStore msg;
    msg.Put(CMSG_LEAVE_CHANNEL);
    msg.PutString(lua_tostring(L, 1));
    msg.Finalize();
    ClientServices_Send(&msg);
  } else {
    luaL_error(L, "Usage: LeaveChannelByName(\"name\")");
  }
  return 0;
}

static int Script_GuildInviteByName(lua_State *L) {
  LPCSTR name = lua_isstring(L, 1) ? lua_tostring(L, 1) : 0;
  if (!name || !*name) {
    CGGameUI::DisplayError(GERR_COMMAND_NEEDS_TARGET);
    return 0;
  }
  CDataStore msg;
  msg.Put(CMSG_GUILD_INVITE);
  msg.PutString(lua_tostring(L, 1));
  msg.Finalize();
  ClientServices_Send(&msg);
  return 0;
}

static int Script_GuildUninviteByName(lua_State *L) {
  if (lua_isstring(L, 1)) {
    CDataStore msg;
    msg.Put(CMSG_GUILD_REMOVE);
    msg.PutString(lua_tostring(L, 1));
    msg.Finalize();
    ClientServices_Send(&msg);
  } else {
    luaL_error(L, "Usage: GuildUninviteByName(\"name\")");
  }
  return 0;
}

static int Script_GuildPromoteByName(lua_State *L) {
  if (lua_isstring(L, 1)) {
    CDataStore msg;
    msg.Put(CMSG_GUILD_PROMOTE);
    msg.PutString(lua_tostring(L, 1));
    msg.Finalize();
    ClientServices_Send(&msg);
  } else {
    luaL_error(L, "Usage: GuildPromoteByName(\"name\")");
  }
  return 0;
}

static int Script_GuildDemoteByName(lua_State *L) {
  if (lua_isstring(L, 1)) {
    CDataStore msg;
    msg.Put(CMSG_GUILD_DEMOTE);
    msg.PutString(lua_tostring(L, 1));
    msg.Finalize();
    ClientServices_Send(&msg);
  } else {
    luaL_error(L, "Usage: GuildDemoteByName(\"name\")");
  }
  return 0;
}

static int Script_GuildSetLeaderByName(lua_State *L) {
  if (lua_isstring(L, 1)) {
    CDataStore msg;
    msg.Put(CMSG_GUILD_LEADER);
    if (*lua_tostring(L, 1) && SStrCmpI(lua_tostring(L, 1), "target", INT_MAX)) {
      msg.PutString(lua_tostring(L, 1));
    }
    msg.Finalize();
    ClientServices_Send(&msg);
  } else {
    luaL_error(L, "Usage: GuildSetLeaderByName(\"name\")");
  }
  return 0;
}

static int Script_GuildSetMOTD(lua_State *L) {
  if (lua_isstring(L, 1)) {
    CDataStore msg;
    msg.Put(CMSG_GUILD_MOTD);
    if (*lua_tostring(L, 1)) {
      msg.PutString(lua_tostring(L, 1));
    }
    msg.Finalize();
    ClientServices_Send(&msg);
  } else {
    luaL_error(L, "Usage: GuildSetMOTD(\"message\")");
  }
  return 0;
}

static int Script_GuildLeave(lua_State *L) {
  CDataStore msg;
  msg.Put(CMSG_GUILD_LEAVE);
  msg.Finalize();
  ClientServices_Send(&msg);
  return 0;
}

static int Script_GuildDisband(lua_State *L) {
  CDataStore msg;
  msg.Put(CMSG_GUILD_DISBAND);
  msg.Finalize();
  ClientServices_Send(&msg);
  return 0;
}

static int Script_GuildInfo(lua_State *L) {
  CDataStore msg;
  msg.Put(CMSG_GUILD_INFO);
  msg.Finalize();
  ClientServices_Send(&msg);
  return 0;
}

static int Script_GuildRoster(lua_State *L) {
  CDataStore msg;
  msg.Put(CMSG_GUILD_ROSTER);
  msg.Finalize();
  ClientServices_Send(&msg);
  return 0;
}

static int Script_GetScreenWidth(lua_State *L) {
  NTempest::CRect screenRect;
  GxCapsScreenSize(screenRect);
  lua_pushnumber(L, max(1024.0f, screenRect.r - screenRect.l));
  return 1;
}

static int Script_GetScreenHeight(lua_State *L) {
  NTempest::CRect screenRect;
  GxCapsScreenSize(screenRect);
  lua_pushnumber(L, max(768.0f, screenRect.b - screenRect.t));
  return 1;
}

static int Script_PVPPort(lua_State *L) {
  CDataStore msg;
  msg.Put(CMSG_PVP_PORT);
  msg.Finalize();
  ClientServices_Send(&msg);
  return 0;
}

static int Script_GetDamageBonusStat(lua_State *L) {
  CGPlayer_C          *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    const ChrClassesRec *rec = g_chrClassesDB.GetRecord(player->GetClass());
    if (rec) {
      lua_pushnumber(L, rec->m_DamageBonusStat + 1);
      return 1;
    }
  }
  lua_pushnumber(L, 0.0);
  return 1;
}

static int Script_GetReleaseTimeRemaining(lua_State *L) {
  lua_pushnumber(L, 300.0);
  return 1;
}

static int Script_GetBindZone(lua_State *L) {
  CDataStore msg;
  msg.Put(CMSG_GETDEATHBINDZONE);
  msg.Finalize();
  ClientServices_Send(&msg);
  return 0;
}

static int Script_SplitMoney(lua_State *L) {
  int coins[3];
  if (lua_isstring(L, 1)) {
    LPCSTR text = lua_tostring(L, 1);
    int    count;
    for (count = 2; count >= 0; --count) {
      if (!*text || !isdigit(*text)) {
        break;
      }
      coins[count] = SStrToInt(text);
      if (count > 0) {
        while (*text && isdigit(*text)) {
          ++text;
        }
        while (*text && isspace(*text)) {
          ++text;
        }
      }
    }
    if (count >= 0) {
      int i;
      for (i = 0; i < 2 - count; ++i) {
        coins[i] = coins[i + count + 1];
      }
      for (; i < 3; ++i) {
        coins[i] = 0;
      }
    }
    UINT money = CurrencyTotal(coins);
    if (money > 0) {
      CDataStore msg;
      msg.Put(MSG_SPLIT_MONEY);
      msg.Put((int)money);
      msg.Finalize();
      ClientServices_Send(&msg);
      return 0;
    }
  }
  luaL_error(L, "Usage: SplitMoney(\"gold silver copper\")");
  return 0;
}

static int Script_GetDate(lua_State *L) {
  lua_pushstring(L, "Dec 11 2003");
  return 1;
}

static int Script_GetBuildVersion(lua_State *L) {
  char buf[256];
  SStrPrintf(buf, sizeof(buf), FrameScript_GetText("ALPHA_BUILD", -1, GENDER_NOT_APPLICABLE));
  SStrPack(buf, " ", sizeof(buf));
  SStrPack(buf, "5.3", sizeof(buf));
  lua_pushstring(L, buf);
  return 1;
}

static int Script_GetCurrentPosition(lua_State *L) {
  char        buf[256] = "";
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    NTempest::C3Vector pos = player->GetPosition();
    SStrPrintf(buf, sizeof(buf), "%.2f, %.2f, %.2f", pos.x, pos.y, pos.z);
  }
  lua_pushstring(L, buf);
  return 1;
}

static int Script_GetCursorPosition(lua_State *L) {
  NTempest::C2Vector pos;
  CSimpleTop        *top = CSimpleTop::GetInstance();
  top->GetMousePosition(pos);
  lua_pushnumber(L, ((pos.x * 1024.0f) * 1.25f));
  lua_pushnumber(L, ((pos.y * 1024.0f) * 1.25f));
  return 2;
}

static int Script_GetNetStats(lua_State *L) {
  DWORD latency;
  float out;
  float in;
  ClientServices_GetNetStats(in, out, latency);
  lua_pushnumber(L, in);
  lua_pushnumber(L, out);
  lua_pushnumber(L, latency);
  return 3;
}

static int Script_SitOrStand(lua_State *) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    if (player->GetStandState() == UNIT_STANDING) {
      player->ChangeStandState(UNIT_SITTING);
    } else {
      player->ChangeStandState(UNIT_STANDING);
    }
  }
  return 0;
}

static int Script_StopCinematic(lua_State *) {
  CGGameUI::StopCinematic(0);
  return 0;
}

static int Script_RunScript(lua_State *L) {
  if (lua_isstring(L, 1)) {
    LPCSTR script = lua_tostring(L, 1);
    if (script && *script) {
      FrameScript_Execute(script, script);
    }
  }
  return 0;
}

static int Script_CheckInteractDistance(lua_State *L) {
  if (lua_isstring(L, 1) && lua_isnumber(L, 2)) {
    static const float s_interactDistances[3] = {MAX_INSPECT_DISTANCE_SQUARED, MAX_TRADE_DISTANCE_SQUARED, MAX_DUEL_DISTANCE_SQUARED};

    const CGUnit_C *unit = Script_GetUnitFromName(lua_tostring(L, 1));
    CGPlayer_C     *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
    UINT            index = (int)lua_tonumber(L, 2) - 1;
    if (player && unit && index < 3 && (player->GetPosition() - unit->GetPosition()).SquaredMag() < s_interactDistances[index]) {
      lua_pushnumber(L, 1.0);
      return 1;
    }
    lua_pushnil(L);
    return 1;
  }
  luaL_error(L, "Usage: CheckInteractDistance(\"unit\", distIndex)");
  return 0;
}

static int Script_GetScreenResolutions(lua_State *L) {
  for (UINT i = 0; i < 4; ++i) {
    lua_pushstring(L, s_screenResolutions[i]);
  }
  return 4;
}

static int Script_GetCurrentResolution(lua_State *L) {
  CVar *cvar = CVar::Lookup("gxResolution");
  if (cvar) {
    LPCSTR resolution = cvar->GetString();
    for (UINT i = 0; i < 4; ++i) {
      if (!SStrCmpI(resolution, s_screenResolutions[i], INT_MAX)) {
        lua_pushnumber(L, i + 1);
        return 1;
      }
    }
  }
  lua_pushnumber(L, 1.0);
  return 1;
}

static int Script_SetScreenResolution(lua_State *L) {
  int index = 0;
  if (lua_isnumber(L, 1)) {
    index = (UINT)lua_tonumber(L, 1) - 1 > 4 ? 4 : (UINT)lua_tonumber(L, 1) - 1;
  }
  CVar *cvar = CVar::Lookup("gxResolution");
  if (cvar && SStrCmpI(cvar->GetString(), s_screenResolutions[index], INT_MAX)) {
    cvar->Set(s_screenResolutions[index], 1, 0, 0);
    ConsoleCommandExecute("gxRestart", 1);
  }
  return 0;
}

static int Script_Stuck(lua_State *L) {
  Spell_C_CastSpell(CGSpellBook::GetStuckSpell(), 0);
  return 0;
}

static int Script_RandomRoll(lua_State *L) {
  if (lua_isstring(L, 1) && lua_isstring(L, 2)) {
    int min = SStrToInt(lua_tostring(L, 1));
    int max = SStrToInt(lua_tostring(L, 2));
    if (!min && !max) {
      return 0;
    }
    if (min >= 0 && max >= min) {
      CDataStore msg;
      msg.Put(MSG_RANDOM_ROLL);
      msg.Put(min);
      msg.Put(max);
      msg.Finalize();
      ClientServices_Send(&msg);
    }
  } else {
    luaL_error(L, "Usage: RandomRoll(\"max\") or RandomRoll(\"min\", \"max\")");
  }
  return 0;
}

static int Script_OpeningCinematic(lua_State *L) {
  CDataStore msg;
  msg.Put(CMSG_OPENING_CINEMATIC);
  msg.Finalize();
  ClientServices_Send(&msg);
  return 0;
}

static void LoadScriptFunctions() {
  RegisterSimpleFrameScriptMethods();
  CGTooltip::RegisterScriptMethods();
  CGMinimapFrame::RegisterScriptMethods();
  CGCharacterModelBase::RegisterScriptMethods();
  CGTabardModelFrame::RegisterScriptMethods();

  for (UINT i = 0; i < 126; ++i) {
    FrameScript_RegisterFunction(s_ScriptFunctions[i].name, s_ScriptFunctions[i].method);
  }

  UIBindingsRegisterScriptFunctions();
  InputControlRegisterScriptFunctions();
  CameraRegisterScriptFunctions();
  SoundRegisterScriptFunctions();
  SpellRegisterScriptFunctions();
  ScriptEventsRegisterFunctions();
  ActionBarRegisterScriptFunctions();
  BuffBarRegisterScriptFunctions();
  PartyInfoRegisterScriptFunctions();
  ChatRegisterScriptFunctions();
  SpellBookRegisterScriptFunctions();
  CharacterInfoRegisterScriptFunctions();
  LootInfoRegisterScriptFunctions();
  ItemTextRegisterScriptFunctions();
  QuestInfoRegisterScriptFunctions();
  CGTaxiMap::RegisterScriptFunctions();
  QuestLogRegisterScriptFunctions();
  ClassTrainerRegisterScriptFunctions();
  TradeSkillRegisterScriptFunctions();
  MerchantRegisterScriptFunctions();
  TradeInfoRegisterScriptFunctions();
  ContainerRegisterScriptFunctions();
  BankRegisterScriptFunctions();
  FriendList::RegisterScriptFunctions();
  PetInfoRegisterScriptFunctions();
  CraftInfoRegisterScriptFunctions();
  WorldMapRegisterScriptFunctions();
  ReputationInfoRegisterScriptFunctions();
  SndInterfaceRegisterVocalScriptFunctions();
  TabardCreationRegisterScriptFunctions();
  GuildRegistrarRegisterScriptFunctions();
  DuelInfoRegisterScriptFunctions();
  TutorialRegisterScriptFunctions();
  PetitionInfoRegisterScriptFunctions();
}

static void UnloadScriptFunctions() {
  TabardCreationUnregisterScriptFunctions();
  SndInterfaceUnregisterVocalScriptFunctions();
  UnregisterSimpleFrameScriptMethods();
  CGTooltip::UnregisterScriptMethods();
  CGMinimapFrame::UnregisterScriptMethods();
  CGCharacterModelBase::UnregisterScriptMethods();
  CGTabardModelFrame::UnregisterScriptMethods();

  for (UINT i = 0; i < 126; ++i) {
    FrameScript_UnregisterFunction(s_ScriptFunctions[i].name);
  }

  UIBindingsUnegisterScriptFunctions();
  InputControlUnregisterScriptFunctions();
  CameraUnregisterScriptFunctions();
  SoundUnregisterScriptFunctions();
  SpellUnregisterScriptFunctions();
  ScriptEventsUnregisterFunctions();
  ActionBarUnregisterScriptFunctions();
  BuffBarUnregisterScriptFunctions();
  PartyInfoUnregisterScriptFunctions();
  ChatUnregisterScriptFunctions();
  SpellBookUnregisterScriptFunctions();
  CharacterInfoUnregisterScriptFunctions();
  LootInfoUnregisterScriptFunctions();
  ItemTextUnregisterScriptFunctions();
  QuestInfoUnregisterScriptFunctions();
  CGTaxiMap::UnregisterScriptFunctions();
  QuestLogUnregisterScriptFunctions();
  ClassTrainerUnregisterScriptFunctions();
  TradeSkillUnregisterScriptFunctions();
  MerchantUnregisterScriptFunctions();
  TradeInfoUnregisterScriptFunctions();
  ContainerUnregisterScriptFunctions();
  BankUnregisterScriptFunctions();
  FriendList::UnregisterScriptFunctions();
  PetInfoUnregisterScriptFunctions();
  CraftInfoUnregisterScriptFunctions();
  WorldMapUnregisterScriptFunctions();
  ReputationInfoUnregisterScriptFunctions();
  GuildRegistrarUnregisterScriptFunctions();
  DuelInfoUnregisterScriptFunctions();
  TutorialUnregisterScriptFunctions();
  PetitionInfoUnregisterScriptFunctions();
}

static BOOL PlacedFrameCallback(CSimpleFrame *frame, LPVOID param) {
  char   line[128];
  DWORD  count;
  LPCSTR name = frame->GetName();
  if (!name || !*name || !frame->IsUserPlaced() || (!frame->IsMovable() && !frame->IsResizable())) {
    return 1;
  }

  NTempest::CRect rect;
  NTempest::CRect toprect;
  if (!frame->GetRect(&rect)) {
    return 1;
  }

  if (!frame->GetTop()->GetRect(&toprect)) {
    return 1;
  }

  SStrPrintf(line, sizeof(line), "Frame: %s\n", name);
  OsWriteFile((HOSFILE)param, line, SStrLen(line), &count);
  SStrPrintf(line, sizeof(line), "FrameLevel: %d\n", frame->GetFrameLevel());
  OsWriteFile((HOSFILE)param, line, SStrLen(line), &count);

  if (frame->IsMovable()) {
    SStrPrintf(line, sizeof(line), "X: %d\n", (int)(((-(toprect.l - rect.l)) * 1024.0f) * 1.25f));
    OsWriteFile((HOSFILE)param, line, SStrLen(line), &count);
    SStrPrintf(line, sizeof(line), "Y: %d\n", (int)(((-(toprect.b - rect.b)) * 1024.0f) * 1.25f));
    OsWriteFile((HOSFILE)param, line, SStrLen(line), &count);
  }

  if (frame->IsResizable()) {
    SStrPrintf(line, sizeof(line), "W: %d\n", (int)(((rect.r - rect.l) * 1024.0f) * 1.25f));
    OsWriteFile((HOSFILE)param, line, SStrLen(line), &count);
    SStrPrintf(line, sizeof(line), "H: %d\n", (int)(((rect.b - rect.t) * 1024.0f) * 1.25f));
    OsWriteFile((HOSFILE)param, line, SStrLen(line), &count);
  }

  return 1;
}

static BOOL SavePlacedFrames(CSimpleTop *top) {
  if (!top) {
    return 0;
  }

  HOSFILE file = OsCreateFile("PlacedFrames.txt", GENERIC_WRITE, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0x3F3F3F3F);
  if (file == HOSFILE_INVALID) {
    return 0;
  }

  top->EnumerateFrames(PlacedFrameCallback, file);
  OsCloseFile(file);
  return 1;
}

static void PlaceFrame(CSimpleFrame *frame, int framelevel, int x, int y, int w, int h) {
  if (!frame) {
    return;
  }

  if (framelevel >= 0) {
    frame->SetFrameLevel(framelevel, 1);
  }

  if (frame->IsMovable() && x && y) {
    frame->ClearAllPoints(1);
    frame->SetPoint(FRAMEPOINT_TOPLEFT, frame->GetTop(), FRAMEPOINT_TOPLEFT, (x / 1024.0f) * 0.8f, (y / 1024.0f) * 0.8f, 1);
    frame->SetUserPlaced(1);
  }

  if (frame->IsResizable() && (w || h)) {
    if (w) {
      frame->SetWidth((w / 1024.0f) * 0.8f);
    }
    if (h) {
      frame->SetHeight((h / 1024.0f) * 0.8f);
    }
    frame->SetUserPlaced(1);
  }
}

static void LoadPlacedFrames() {
  char   line[128];
  LPVOID buffer;
  LPCSTR readCursor;

  if (!SFile::LoadFile("PlacedFrames.txt", &buffer, 0, 1, 0)) {
    return;
  }

  readCursor = (LPCSTR)buffer;
  CSimpleFrame *frame = 0;
  int           framelevel = -1;
  int           x = 0;
  int           y = 0;
  int           w = 0;
  int           h = 0;
  do {
    SStrTokenize(&readCursor, line, sizeof(line), "\r\n", 0);
    if (!SStrCmpI(line, "Frame: ", SStrLen("Frame: "))) {
      PlaceFrame(frame, framelevel, x, y, w, h);
      frame = SimpleFrameRegistryGetEntry(line + SStrLen("Frame: "), 0);
      framelevel = -1;
      x = 0;
      y = 0;
      w = 0;
    } else if (!SStrCmpI(line, "FrameLevel: ", SStrLen("FrameLevel: "))) {
      framelevel = SStrToInt(line + SStrLen("FrameLevel: "));
    } else if (!SStrCmpI(line, "X: ", SStrLen("X: "))) {
      x = SStrToInt(line + SStrLen("X: "));
    } else if (!SStrCmpI(line, "Y: ", SStrLen("Y: "))) {
      y = SStrToInt(line + SStrLen("Y: "));
    } else if (!SStrCmpI(line, "W: ", SStrLen("W: "))) {
      w = SStrToInt(line + SStrLen("W: "));
    } else if (!SStrCmpI(line, "H: ", SStrLen("H: "))) {
      h = SStrToInt(line + SStrLen("H: "));
    }
  } while (line[0] && *readCursor);

  PlaceFrame(frame, framelevel, x, y, w, h);
  SFile::Unload(buffer);
}

void CGGameUI::ResetCamera() {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  FATALASSERT(player);

  CGWorldFrame *worldFramePtr = CGWorldFrame::GetActive();
  FATALASSERT(worldFramePtr);
  worldFramePtr->SetCameraTarget(player);
}

namespace {
  FrameScript_Method s_ScriptFunctions[126] = {
      {          "FrameXML_Debug",           Script_FrameXML_Debug},
      {                "ReloadUI",                 Script_ReloadUI},
      {           "SetLayoutMode",            Script_SetLayoutMode},
      {          "IsShiftKeyDown",           Script_IsShiftKeyDown},
      {        "IsControlKeyDown",         Script_IsControlKeyDown},
      {            "IsAltKeyDown",             Script_IsAltKeyDown},
      {              "Screenshot",               Script_Screenshot},
      {            "GetFramerate",             Script_GetFramerate},
      {"TogglePerformanceDisplay", Script_TogglePerformanceDisplay},
      { "TogglePerformanceValues",  Script_TogglePerformanceValues},
      {  "ResetPerformanceValues",   Script_ResetPerformanceValues},
      {           "GetDebugStats",            Script_GetDebugStats},
      {                 "GetCVar",                  Script_GetCVar},
      {                 "SetCVar",                  Script_SetCVar},
      {          "GetWorldDetail",           Script_GetWorldDetail},
      {          "SetWorldDetail",           Script_SetWorldDetail},
      {          "GetWaterDetail",           Script_GetWaterDetail},
      {          "SetWaterDetail",           Script_SetWaterDetail},
      {              "GetFarclip",               Script_GetFarclip},
      {              "SetFarclip",               Script_SetFarclip},
      {           "GetTerrainMip",            Script_GetTerrainMip},
      {           "SetTerrainMip",            Script_SetTerrainMip},
      {           "GetDoodadAnim",            Script_GetDoodadAnim},
      {           "SetDoodadAnim",            Script_SetDoodadAnim},
      {           "GetTexLodBias",            Script_GetTexLodBias},
      {           "SetTexLodBias",            Script_SetTexLodBias},
      {                "GetGamma",                 Script_GetGamma},
      {                "SetGamma",                 Script_SetGamma},
      {              "ToggleTris",               Script_ToggleTris},
      {           "TogglePortals",            Script_TogglePortals},
      {         "ToggleCollision",          Script_ToggleCollision},
      {  "ToggleCollisionDisplay",   Script_ToggleCollisionDisplay},
      {      "TogglePlayerBounds",       Script_TogglePlayerBounds},
      {                  "Logout",                   Script_Logout},
      {                    "Quit",                     Script_Quit},
      {          "ShowNameplates",           Script_ShowNameplates},
      {          "HideNameplates",           Script_HideNameplates},
      {               "SetCursor",                Script_SetCursor},
      {           "CursorHasItem",            Script_CursorHasItem},
      {          "CursorHasSpell",           Script_CursorHasSpell},
      {          "CursorHasMoney",           Script_CursorHasMoney},
      {         "EquipCursorItem",          Script_EquipCursorItem},
      {        "DeleteCursorItem",         Script_DeleteCursorItem},
      {        "EquipPendingItem",         Script_EquipPendingItem},
      {      "CancelPendingEquip",       Script_CancelPendingEquip},
      {              "TargetUnit",               Script_TargetUnit},
      {          "TargetUnitsPet",           Script_TargetUnitsPet},
      {      "TargetNearestEnemy",       Script_TargetNearestEnemy},
      {         "TargetLastEnemy",          Script_TargetLastEnemy},
      {            "AttackTarget",             Script_AttackTarget},
      {              "AssistUnit",               Script_AssistUnit},
      {            "AssistByName",             Script_AssistByName},
      {              "FollowUnit",               Script_FollowUnit},
      {            "FollowByName",             Script_FollowByName},
      {             "ClearTarget",              Script_ClearTarget},
      {     "AutoEquipCursorItem",      Script_AutoEquipCursorItem},
      {            "ToggleSheath",             Script_ToggleSheath},
      {               "ToggleRun",                Script_ToggleRun},
      {                    "Jump",                     Script_Jump},
      {             "GetZoneText",              Script_GetZoneText},
      {          "GetSubZoneText",           Script_GetSubZoneText},
      {      "GetMinimapZoneText",       Script_GetMinimapZoneText},
      {           "InitiateTrade",            Script_InitiateTrade},
      {           "NotifyInspect",            Script_NotifyInspect},
      {           "InviteToParty",            Script_InviteToParty},
      {            "InviteByName",             Script_InviteByName},
      {       "UninviteFromParty",        Script_UninviteFromParty},
      {          "UninviteByName",           Script_UninviteByName},
      {    "PromoteToPartyLeader",     Script_PromoteToPartyLeader},
      {           "PromoteByName",            Script_PromoteByName},
      {       "RequestTimePlayed",        Script_RequestTimePlayed},
      {                 "RepopMe",                  Script_RepopMe},
      {         "AcceptResurrect",          Script_AcceptResurrect},
      {        "DeclineResurrect",         Script_DeclineResurrect},
      {              "BeginTrade",               Script_BeginTrade},
      {             "CancelTrade",              Script_CancelTrade},
      {             "AcceptGroup",              Script_AcceptGroup},
      {            "DeclineGroup",             Script_DeclineGroup},
      {             "AcceptGuild",              Script_AcceptGuild},
      {            "DeclineGuild",             Script_DeclineGuild},
      {            "CancelLogout",             Script_CancelLogout},
      {             "ForceLogout",              Script_ForceLogout},
      {               "ForceQuit",                Script_ForceQuit},
      {               "ReportBug",                Script_ReportBug},
      {        "ReportSuggestion",         Script_ReportSuggestion},
      {              "ReportNote",               Script_ReportNote},
      {          "GetCursorMoney",           Script_GetCursorMoney},
      {         "DropCursorMoney",          Script_DropCursorMoney},
      {       "PickupPlayerMoney",        Script_PickupPlayerMoney},
      {          "HideSellCursor",           Script_HideSellCursor},
      {            "HasSoulstone",             Script_HasSoulstone},
      {            "UseSoulstone",             Script_UseSoulstone},
      {       "JoinChannelByName",        Script_JoinChannelByName},
      {      "LeaveChannelByName",       Script_LeaveChannelByName},
      {       "GuildInviteByName",        Script_GuildInviteByName},
      {     "GuildUninviteByName",      Script_GuildUninviteByName},
      {      "GuildPromoteByName",       Script_GuildPromoteByName},
      {       "GuildDemoteByName",        Script_GuildDemoteByName},
      {    "GuildSetLeaderByName",     Script_GuildSetLeaderByName},
      {            "GuildSetMOTD",             Script_GuildSetMOTD},
      {              "GuildLeave",               Script_GuildLeave},
      {            "GuildDisband",             Script_GuildDisband},
      {               "GuildInfo",                Script_GuildInfo},
      {             "GuildRoster",              Script_GuildRoster},
      {          "GetScreenWidth",           Script_GetScreenWidth},
      {         "GetScreenHeight",          Script_GetScreenHeight},
      {                 "PVPPort",                  Script_PVPPort},
      {      "GetDamageBonusStat",       Script_GetDamageBonusStat},
      { "GetReleaseTimeRemaining",  Script_GetReleaseTimeRemaining},
      {             "GetBindZone",              Script_GetBindZone},
      {              "SplitMoney",               Script_SplitMoney},
      {                 "GetDate",                  Script_GetDate},
      {         "GetBuildVersion",          Script_GetBuildVersion},
      {      "GetCurrentPosition",       Script_GetCurrentPosition},
      {       "GetCursorPosition",        Script_GetCursorPosition},
      {             "GetNetStats",              Script_GetNetStats},
      {              "SitOrStand",               Script_SitOrStand},
      {           "StopCinematic",            Script_StopCinematic},
      {               "RunScript",                Script_RunScript},
      {   "CheckInteractDistance",    Script_CheckInteractDistance},
      {    "GetScreenResolutions",     Script_GetScreenResolutions},
      {    "GetCurrentResolution",     Script_GetCurrentResolution},
      {     "SetScreenResolution",      Script_SetScreenResolution},
      {                   "Stuck",                    Script_Stuck},
      {              "RandomRoll",               Script_RandomRoll},
      {        "OpeningCinematic",         Script_OpeningCinematic}
  };
}

void CGGameUI::StartCinematic(int cinematicID) {
  memset(&m_cinematic, 0, sizeof(m_cinematic));
  m_cinematic.sequence = g_cinematicSequencesDB.GetRecord(cinematicID);
  if (m_cinematic.sequence) {
    FATALASSERT(!m_cinematic.sequenceMusic);
    m_cinematic.sequenceMusic = SndInterfacePlayLoopedSound(m_cinematic.sequence->m_soundID, 0);
    if (m_cinematic.sequence) {
      m_cinematic.camera = g_cinematicCameraDB.GetRecord(m_cinematic.sequence->m_camera[0]);
    }
  }

  BeginCinematic();
}

void CGGameUI::BeginCinematic() {
  m_cinematic.zoneMusicWasEnabled = SndInterfaceIsZoneMusicPaused();
  if (m_cinematic.zoneMusicWasEnabled) {
    SndInterfacePauseZoneMusic(1);
  }
  HideCursor();
  EnableFadingScreen(CinematicFadeTime, BeginCinematicInternal, 0);
}

void CGGameUI::BeginCinematicInternal(LPVOID) {
  DisableLoadingScreen();
  FrameScript_SignalEvent(353);
  if (!StartCinematicCamera()) {
    StopCinematicInternal(0);
  }
}

static bool GetCinematicStartingCameraPosition(LPCSTR modelFile, NTempest::C3Vector &origin, float facing, NTempest::C3Vector &position) {
  CStatus status;
  HMODEL  m_model = ModelCreate(modelFile, 0, &status);
  if (!m_model) {
    return 0;
  }

  NTempest::C34Matrix m_modelMatrix;
  m_modelMatrix.Identity();
  m_modelMatrix.Translate(origin);
  m_modelMatrix.Rotate(facing, NTempest::C3Vector(0.0f, 0.0f, 1.0f), true);

  AsyncFileReadWaitAll();
  FATALASSERT(ModelIsLoaded(m_model));

  HCAMERA camera = ModelGetCamera(m_model, 0);
  ModelAnimateCameras(m_model, m_modelMatrix);
  DataMgrGetCoord(camera, 7, &position);
  HandleClose(camera);
  HandleClose(m_model);
  return 1;
}

BOOL CGGameUI::StartCinematicCamera() {
  static const float MAX_CAMERA_SHIFT = 50.0f;

  CGCamera *camera = CGWorldFrame::GetActiveCamera();
  FATALASSERT(camera);

  const CinematicCameraRec *cinematicCamera = m_cinematic.camera;
  if (!cinematicCamera) {
    return 0;
  }

  LPCSTR             cameraModel = cinematicCamera->m_model;
  NTempest::C3Vector cameraOrigin(cinematicCamera->m_originX, cinematicCamera->m_originY, cinematicCamera->m_originZ);
  float              cameraFacing = cinematicCamera->m_originFacing;
  NTempest::C3Vector initialPosition(0.0f);
  if (GetCinematicStartingCameraPosition(cameraModel, cameraOrigin, cameraFacing, initialPosition) &&
      (camera->Position() - initialPosition).SquaredMag() > MAX_CAMERA_SHIFT * MAX_CAMERA_SHIFT)
  {
    CWorld::Preload(initialPosition);
  }

  if (!camera->SetModelCamera(cameraModel, cameraOrigin, cameraFacing, NextCinematic, 0)) {
    return 0;
  }

  ScrnPaint();
  CGObject_C::UpdateAllWorldObjects();
  AsyncFileReadWaitAll();
  camera->ResetModelCamera();

  FATALASSERT(!m_cinematic.cameraMusic);
  m_cinematic.cameraMusic = SndInterfacePlayLoopedSound(cinematicCamera->m_soundID, 1);
  DisableFadingScreen(CinematicFadeTime, 0, 0);
  return 1;
}

BOOL CGGameUI::NextCinematic(LPVOID) {
  EnableFadingScreen(CinematicFadeTime, NextCinematicInternal, 0);
  return 1;
}

void CGGameUI::NextCinematicInternal(LPVOID) {
  m_cinematic.camera = 0;
  Sound::KillSound(m_cinematic.cameraMusic);

  if (m_cinematic.sequence && ++m_cinematic.sequenceIndex < 8u && m_cinematic.sequence->m_camera[m_cinematic.sequenceIndex]) {
    CDataStore msg;
    msg.Put(CMSG_NEXT_CINEMATIC_CAMERA);
    msg.Finalize();
    ClientServices_Send(&msg);
    m_cinematic.camera = g_cinematicCameraDB.GetRecord(m_cinematic.sequence->m_camera[m_cinematic.sequenceIndex]);
  }

  if (!StartCinematicCamera()) {
    StopCinematicInternal(0);
  }
}

BOOL CGGameUI::StopCinematic(LPVOID) {
  Sound::KillSound(m_cinematic.cameraMusic);
  EnableFadingScreen(CinematicFadeTime, StopCinematicInternal, 0);
  return 1;
}

void CGGameUI::StopCinematicInternal(LPVOID) {
  static const float MAX_CAMERA_SHIFT = 50.0f;

  CGCamera *camera = CGWorldFrame::GetActiveCamera();
  FATALASSERT(camera);

  FrameScript_SignalEvent(354);
  CGObject_C *target = ClntObjMgrObjectPtr(camera->GetTarget(), __FILE__, __LINE__);
  if (target && (camera->Position() - target->GetPosition()).SquaredMag() > MAX_CAMERA_SHIFT * MAX_CAMERA_SHIFT) {
    CWorld::Preload(target->GetPosition());
  }

  camera->ClearModelCamera();
  ScrnPaint();
  CGObject_C::UpdateAllWorldObjects();
  AsyncFileReadWaitAll();
  Sound::KillSound(m_cinematic.sequenceMusic);
  if (m_cinematic.zoneMusicWasEnabled) {
    SndInterfacePauseZoneMusic(0);
  }

  CDataStore msg;
  msg.Put(CMSG_COMPLETE_CINEMATIC);
  msg.Finalize();
  ClientServices_Send(&msg);
  ShowCursor();
  DisableFadingScreen(CinematicFadeTime, 0, 0);
}

void CGGameUI::CloseLoot(bool send, bool moving) {
  CGPlayer_C *playerPtr = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (playerPtr) {
    playerPtr->m_lootingUnitSent = 0;
  }

  DWORDLONG object = CGLootInfo::m_object;
  if (object) {
    CGObject_C *objectPtr = ClntObjMgrObjectPtr(object, __FILE__, __LINE__);
    if (!moving || !objectPtr || !objectPtr->IsA(ID_ITEM)) {
      if (send) {
        CDataStore lootRelease;
        lootRelease.Put(CMSG_LOOT_RELEASE);
        lootRelease.Put(object);
        lootRelease.Finalize();
        ClientServices_Send(&lootRelease);
        if (playerPtr) {
          playerPtr->m_flags |= 0x200;
        }
      }

      CGLootInfo::SetObject(0, 0, LOOT_ACQUIRE_FAILED);
      ClearTarget(object, 1);
      if (m_cursorItemType == UICURSOR_LOOT) {
        ClearCursor(1);
      }
    }
  }
}

void CGGameUI::ClearLootSlot(BYTE slot) {
  CGLootInfo::ClearSlot(slot);
}

void CGGameUI::OpenLoot(CGObject_C *object, int coins, LOOT_ACQUIRE lootType) {
  if (object->IsA(ID_UNIT)) {
    Target(object->GetGUID(), 0);
  }
  CGLootInfo::SetObject(object, coins, lootType);
}

void CGGameUI::OpenResurrectRequest(LPCSTR inviter) {
  FrameScript_SignalEvent(261, "%s", inviter);
}

void CGGameUI::OpenPartyInvite(LPCSTR inviter) {
  FrameScript_SignalEvent(262, "%s", inviter);
}

void CGGameUI::CancelPartyInvite() {
  FrameScript_SignalEvent(263);
}

void CGGameUI::OpenGuildInvite(LPCSTR inviter, LPCSTR guildName) {
  FrameScript_SignalEvent(264, "%s%s", inviter, guildName);
}

void CGGameUI::CancelGuildInvite() {
  FrameScript_SignalEvent(265);
}

void CGGameUI::SysMsgDisplay(LPCSTR msg, SYSMSG_TYPE severity) {
  char  string[512];
  float b;
  float r;
  float g;

  SysMsgGetSeverityColor(severity, r, g, b);

  char *write = string;
  if (*msg) {
    int count = 0;
    do {
      if (*msg == '|') {
        *write++ = '|';
        ++count;
        *write++ = '|';
      } else {
        *write++ = *msg;
      }
      if (++count == sizeof(string) - 1) {
        break;
      }
    } while (*++msg);
  }
  *write = 0;

  FrameScript_SignalEvent(214, "%s%f%f%f", string, r, g, b);
}

void CGGameUI::InitializeGame() {
  ConsoleCommandExecute("run worldexec.wtf", 0);
  PortraitInitialize();
  CGChat::InitializeGame();
  CGCharacterInfo::InitializeGame();
  CGPartyInfo::InitializeGame();
  CGSpellBook::InitializeGame();
  CGActionBar::InitializeGame();
  CGBuffBar::InitializeGame();
  CGLootInfo::InitializeGame();
  CGItemText::InitializeGame();
  CGTaxiMap::InitializeGame();
  CGQuestLog::InitializeGame();
  CGClassTrainer::InitializeGame();
  CGPetInfo::InitializeGame();
  CGWorldMap::InitializeGame();
  CGDuelInfo::InitializeGame();
  CGTutorial::InitializeGame();
  m_hasControl = 1;
  Initialize();
}

void CGGameUI::Initialize() {
  s_statusBarCVar = CVar::Register("statusBarText", 0, 0, "0", 0, DEFAULT, false, 0);
  s_assistAttackCVar = CVar::Register("assistAttack", 0, 0, "0", 0, DEFAULT, false, 0);
  s_minimapZoomCVar = CVar::Register("minimapZoom", 0, 0, "3", 0, DEFAULT, false, 0);
  s_minimapInsideZoomCVar = CVar::Register("minimapInsideZoom", 0, 0, "3", 0, DEFAULT, false, 0);
  s_combatLogCVar = CVar::Register("combatLogOn", 0, 0, "1", 0, DEFAULT, false, 0);

  NTempest::CRect screenRect;
  GxCapsScreenSize(screenRect);
  SysMsgSetCallback(SysMsgDisplay);

  m_simpleTop = NEW(CSimpleTop);
  m_simpleTop->m_mouseButtonCallback = FilterMouseDown;
  m_simpleTop->m_displaySizeCallback = HandleDisplaySizeChanged;
  CursorInitialize();
  m_simpleTop->SetCursor(g_cursor->GetModel());

  FrameScript_Flush();
  FrameScript_LoadTextTables("Interface\\FrameXML\\GlobalStrings.lua");
  LoadScriptFunctions();
  FrameScript_CreateEvents(g_scriptEvents, 0x177);

  CGUIBindings::Initialize("Interface\\Bindings.xml", 0);
  FATALASSERT(CGUIBindings::GetActive());
  CGUIBindings::LoadBindings(0);

  CWOWClientStatus status("FrameXML.log");
  FrameXML_CreateFrames("Interface\\FrameXML\\FrameXML.toc", &status);
  LoadPlacedFrames();

  m_UISimpleParent = SimpleFrameRegistryGetEntry("UIParent", 0);
  FATALASSERT(m_UISimpleParent);
  m_gameTooltip = (CGTooltip *)SimpleFrameRegistryGetEntry("GameTooltip", 0);
  FATALASSERT(m_gameTooltip);

  m_screenWidth = 0;
  CSizeEvent evt;
  evt.w = screenRect.r - screenRect.l;
  evt.h = screenRect.b - screenRect.t;
  HandleDisplaySizeChanged(evt);

  ConsoleCommandRegister("script", CCommand_Script, DEFAULT, 0);
  ConsoleCommandRegister("scaleui", CCommand_ScaleUI, DEFAULT, 0);

  m_reloadUI = false;
  m_areaID = 0;
  EventRegister(EVENT_ID_IDLE, Idle);
  if (ClntObjMgrGetActivePlayer()) {
    EnterWorld();
  }
}

inline void CGInputControl::Reset() {
  UnsetControlBit((INPUT_CONTROL)-1, 0);
}

void CGGameUI::EnterWorld() {
  ResetCamera();
  CGInputControl::GetActive()->Reset();
  FrameScript_MemoryCleanup(1);
  FrameScript_SignalEvent(253);
  CGMinimapFrame::Initialize(CGPlayer_C::GetNewContinentID());
  CGChat::EnterWorld();
  CGCharacterInfo::EnterWorld();
  CGActionBar::EnterWorld();
  CGBuffBar::EnterWorld();
  CGLootInfo::EnterWorld();
  CGItemText::EnterWorld();
  CGTaxiMap::EnterWorld();
  CGQuestInfo::EnterWorld();
  CGQuestLog::EnterWorld();
  CGContainerInfo::EnterWorld();
  CGClassTrainer::EnterWorld();
  CGMerchantInfo::EnterWorld();
  CGTradeInfo::EnterWorld();
  CGBankInfo::EnterWorld();
  CGTradeSkillInfo::EnterWorld();
  CGCraftInfo::EnterWorld();
  CGPetInfo::EnterWorld();
  CGWorldMap::EnterWorld();
  CGReputationInfo::EnterWorld();
  CGTabardCreationFrame::EnterWorld();
  CGGuildRegistrar::EnterWorld();
  CGPartyInfo::EnterWorld();
  CGPetitionInfo::EnterWorld();

  CGUnit_C *player = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    player->RegisterScript();
  }
  CGTutorial::TriggerTutorial(TUTORIAL_QUESTGIVERS);
}

void CGGameUI::LeaveWorld() {
  CGUnit_C *player = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    player->UnregisterScript();
  }

  CGMinimapFrame::Shutdown();
  CGChat::LeaveWorld();
  CGCharacterInfo::LeaveWorld();
  CGBuffBar::LeaveWorld();
  CGLootInfo::LeaveWorld();
  CGItemText::LeaveWorld();
  CGTaxiMap::LeaveWorld();
  CGQuestInfo::LeaveWorld();
  CGQuestLog::LeaveWorld();
  CGContainerInfo::LeaveWorld();
  CGClassTrainer::LeaveWorld();
  CGMerchantInfo::LeaveWorld();
  CGTradeInfo::LeaveWorld();
  CGBankInfo::LeaveWorld();
  CGTradeSkillInfo::LeaveWorld();
  CGPetInfo::LeaveWorld();
  CGWorldMap::LeaveWorld();
  CGReputationInfo::LeaveWorld();
  CGTabardCreationFrame::LeaveWorld();
  CGGuildRegistrar::LeaveWorld();
  CGPartyInfo::LeaveWorld();
  CGPetitionInfo::LeaveWorld();
  FrameScript_SignalEvent(254);

  CGWorldFrame *worldFrame = CGWorldFrame::GetActive();
  FATALASSERT(worldFrame);
  worldFrame->SetCameraTarget(0);
}

void CGGameUI::Shutdown() {
  if (ClntObjMgrGetActivePlayer()) {
    LeaveWorld();
  }

  CGUnit_C::NamePlateShow(0);
  CGUnit_C::RemoveAllNamePlates();

  Target(0, 0);
  Trade_C_CancelTrade();
  SavePlacedFrames(m_simpleTop);

  if (m_simpleTop) {
    DEL(m_simpleTop);
    m_simpleTop = 0;
  }

  CursorDestroy();
  UnloadScriptFunctions();
  ConsoleCommandUnregister("script");
  ConsoleCommandUnregister("scaleui");
  CGUIBindings::Shutdown();
  EventUnregister(EVENT_ID_IDLE, Idle);
  SysMsgSetCallback(0);
}

void CGGameUI::ShutdownGame() {
  Shutdown();
  PortraitShutdown();
  CGChat::ShutdownGame();
  CGCharacterInfo::ShutdownGame();
  CGPartyInfo::ShutdownGame();
  CGSpellBook::ShutdownGame();
  CGActionBar::ShutdownGame();
  CGBuffBar::ShutdownGame();
  CGLootInfo::ShutdownGame();
  CGItemText::ShutdownGame();
  CGTaxiMap::ShutdownGame();
  CGQuestLog::ShutdownGame();
  CGClassTrainer::ShutdownGame();
  CGTradeSkillInfo::ShutdownGame();
  CGPetInfo::ShutdownGame();
  CGCraftInfo::ShutdownGame();
  CGWorldMap::ShutdownGame();
  CGReputationInfo::ShutdownGame();
  CGDuelInfo::ShutdownGame();
  CGTutorial::ShutdownGame();
  FrameScript_Flush();

  FREEIFUSED(m_zoneText);
  m_zoneText = 0;
  FREEIFUSED(m_subZoneText);
  m_subZoneText = 0;
  FREEIFUSED(m_minimapZoneText);
  m_minimapZoneText = 0;
}

void CGGameUI::Reload() {
  m_reloadUI = true;
}

void CGGameUI::SetPartyLeader(DWORDLONG guid) {
  CGPartyInfo::SetLeader(guid);
}

void CGGameUI::AddPartyMember(DWORDLONG guid, int connected) {
  CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
  if (unit) {
    unit->RegisterScript();
  }
  CGPartyInfo::AddMember(guid, connected);
}

void CGGameUI::RemoveAllPartyMembers() {
  for (UINT i = 0; i < 4; ++i) {
    CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(CGPartyInfo::GetMember(i), __FILE__, __LINE__));
    if (unit) {
      unit->UnregisterScript();
    }
  }
  CGPartyInfo::RemoveAll();
}

void CGGameUI::EnablePartyMember(DWORDLONG guid, int enable) {
  CGPartyInfo::EnableMember(guid, enable);
}

BOOL CGGameUI::IsPartyMember(const DWORDLONG &guid) {
  return CGPartyInfo::IsMember(guid);
}

DWORDLONG CGGameUI::GetPartyMember(UINT index) {
  return CGPartyInfo::GetMember(index);
}

void CGGameUI::SetLootMethod(LOOT_METHOD method, DWORDLONG master) {
  CGPartyInfo::SetLootMethod(method, master);
}

void CGGameUI::UnitNameUpdate(const DWORDLONG &guid) {
  if (m_gameTooltip && m_gameTooltip->GetUnit() == guid) {
    m_gameTooltip->SetUnit(0);
    m_gameTooltip->SetUnit(guid);
  }

  CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
  if (unit) {
    unit->TriggerPlayerNameUpdate();
  }
  Script_SendUnitSignal(guid, 180);
}

void CGGameUI::UnitPortraitUpdate(const DWORDLONG &guid) {
  UpdatePortraitTexture(guid);
}

static void ItemPushItemStatsCallback(int id, const DWORDLONG &, LPVOID arg, bool granted) {
  if (granted) {
    ItemPushInfo *info = (ItemPushInfo *)arg;
    FATALASSERT(info);
    CGGameUI::OnItemPush(info->player, info->slot, id, info->pushed, info->display);
    SMemFree(info, "delete", -1, 0);
  }
}

void CGGameUI::OnItemPush(DWORDLONG player, int slot, int itemID, int pushed, int display) {
  const ItemStats_C *stats = g_itemDBCache.GetRecord(itemID, 0, 0, 0);
  if (!stats) {
    ItemPushInfo *info = NEW(ItemPushInfo);
    info->player = player;
    info->slot = slot;
    info->pushed = pushed;
    info->display = display;
    stats = g_itemDBCache.GetRecord(itemID, player, ItemPushItemStatsCallback, info);
    FATALASSERT(!stats);
    return;
  }

  LPCSTR colorString = stats->m_overallQualityID >= 3 ? CGTooltip::GetItemQualityColorString(stats->m_overallQualityID) : "";
  LPCSTR colorEnd = *colorString ? "|r" : "";
  LPCSTR temp;
  char   buffer[MAX_PATH];

  if (player == ClntObjMgrGetActivePlayer()) {
    temp = ClientDBStringLookup(SLOOKUP_INVENTORYICONPATH);
    SStrPrintf(buffer, sizeof(buffer), "%s%s", temp, *temp ? "\\" : "");
    SStrPack(buffer, CGItem_C::GetInventoryArt(stats->m_displayInfoID), sizeof(buffer));
    FrameScript_SignalEvent(249, "%d%s", slot == 255 ? 0 : slot + 1, buffer);

    if (display) {
      LPCSTR format = FrameScript_GetText(pushed ? "LOOT_ITEM_PUSHED_SELF" : "LOOT_ITEM_SELF", -1, GENDER_NOT_APPLICABLE);
      SStrPrintf(buffer, sizeof(buffer), format, colorString, itemID, stats->m_displayName[0], colorEnd);
      CGChat::AddChatMessage(buffer, SLASH_CMD_LOOT, 0, 0, 0, 0, 0);
    }
  } else if (!pushed && display) {
    CGObject_C *object = ClntObjMgrObjectPtr(player, __FILE__, __LINE__);
    if (object) {
      LPCSTR format = FrameScript_GetText("LOOT_ITEM", -1, GENDER_NOT_APPLICABLE);
      SStrPrintf(
          buffer, sizeof(buffer), format, ((CGUnit_C *)object)->GetUnitName(), colorString, itemID, stats->m_displayName[0], colorEnd
      );
      CGChat::AddChatMessage(buffer, SLASH_CMD_LOOT, 0, 0, 0, 0, 0);
    }
  }
}

int CGGameUI::OnTerrainClick(const CTerrainClickEvent &evt) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player) {
    return 0;
  }
  if (evt.button != MOUSE_BUTTON_LEFT) {
    return 0;
  }

  if (Spell_C_HandleTerrainClick(evt)) {
    return 1;
  }
  return player->OnTerrainClick(evt);
}

BOOL CGGameUI::OnSpriteLeftClick(DWORDLONG object, float x, float y) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player) {
    return 0;
  }

  CGObject_C *objectPtr = ClntObjMgrObjectPtr(object, __FILE__, __LINE__);
  if (!objectPtr) {
    return 0;
  }

  if (Spell_C_IsTargeting()) {
    Spell_C_HandleSpriteClick(objectPtr);
    return 1;
  }

  if (objectPtr->CanBeTargetted()) {
    Target(object, 0);
  }
  objectPtr->OnLeftClick();

  if (objectPtr->IsA(ID_PLAYER) && m_cursorItem && m_cursorItemContainer) {
    Trade_C_InitiateTrade(object, 1);
  }
  return 1;
}

BOOL CGGameUI::OnSpriteRightClick(DWORDLONG object, float x, float y) {
  if (ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__)) {
    CGObject_C *objectPtr = ClntObjMgrObjectPtr(object, __FILE__, __LINE__);
    if (!objectPtr) {
      return 0;
    }

    if (objectPtr->CanBeTargetted()) {
      Target(object, 0);
    }
    objectPtr->OnRightClick();
  }
  return 1;
}

void CGGameUI::OnTargetContextAction() {
  CGObject_C *object = ClntObjMgrObjectPtr(m_lockedTarget, __FILE__, __LINE__);
  if (!object) {
    return;
  }

  CGWorldFrame *worldFramePtr = CGWorldFrame::GetActive();
  FATALASSERT(worldFramePtr);

  float scale = object->GetScale();

  NTempest::C2Vector position =
      worldFramePtr->GetScreenCoordinates(object->GetPosition() + NTempest::C3Vector(0.0f, 0.0f, scale * object->GetObjectHeight() * 0.5f));
  OnSpriteRightClick(m_lockedTarget, position.x, position.y);
}

void CGGameUI::HandleObjectTrackChange(DWORDLONG object, DWORDLONG oldGUID, float x, float y) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player) {
    return;
  }

  if (!oldGUID) {
    oldGUID = m_currentObjectTrack;
  }
  if (oldGUID) {
    player->SetLocalTarget(0);
    m_currentObjectTrack = 0;
    CGObject_C *oldObject = ClntObjMgrObjectPtr(oldGUID, __FILE__, __LINE__);
    m_gameTooltip->FadeOut();
    if (oldObject) {
      oldObject->HideHighlightType(HT_MOUSEOVER);
    }
  }

  m_currentObjectTrack = object;
  CGObject_C *trackedObject = ClntObjMgrObjectPtr(object, __FILE__, __LINE__);
  if (!trackedObject) {
    return;
  }

  UpdateObjectHighlightColor(trackedObject->GetObjectModel(), trackedObject);
  switch (trackedObject->GetType()) {
    case HIER_TYPE_UNIT:
    case HIER_TYPE_PLAYER: {
      player->SetLocalTarget(object);
      FrameScript_SignalEvent(314);
      if (((CGUnit_C *)trackedObject)->UnitReaction(player) <= UNIT_REACTION_HOSTILE) {
        SndInterfacePlayInterfaceSound("GAMEHIGHLIGHTHOSTILEUNIT");
        return;
      }
      if (((CGUnit_C *)trackedObject)->UnitReaction(player) >= UNIT_REACTION_AMIABLE) {
        SndInterfacePlayInterfaceSound("GAMEHIGHLIGHTFRIENDLYUNIT");
        return;
      }
      break;
    }

    case HIER_TYPE_ITEM:
      m_gameTooltip->SetOwner(m_UISimpleParent, TOOLTIP_ANCHOR_FIXED, 0.0f);
      m_gameTooltip->SetItem(trackedObject->GetEntryID(), ClntObjMgrGetActivePlayer(), trackedObject->GetGUID(), 0, 0, 0);
      break;

    case HIER_TYPE_GAMEOBJECT:
      if (trackedObject->FloatingTooltip()) {
        m_gameTooltip->SetOwner(m_UISimpleParent, TOOLTIP_ANCHOR_CURSOR, 0.0f);
      } else {
        m_gameTooltip->SetOwner(m_UISimpleParent, TOOLTIP_ANCHOR_FIXED, 0.0f);
      }
      m_gameTooltip->SetObject(object);
      break;

    case HIER_TYPE_CORPSE:
      m_gameTooltip->SetOwner(m_UISimpleParent, TOOLTIP_ANCHOR_FIXED, 0.0f);
      m_gameTooltip->SetCorpse(object);
      break;

    default:
      return;
  }
  SndInterfacePlayInterfaceSound("GAMEHIGHLIGHTNEUTRALUNIT");
}

BOOL CGGameUI::FilterMouseDown(const CMouseEvent &evt) {
  if (evt.button == MOUSE_BUTTON_RIGHT &&
      (m_cursorItem || m_cursorVirtualID || m_cursorMoney || m_cursorSpell > 0 || m_cursorPetAction || CGPlayer_C::IsGiftWrapping()))
  {
    ClearCursor(1);
    return 1;
  }
  return 0;
}

BOOL CGGameUI::HandleMouseDown(const CMouseEvent &evt) {
  if (evt.button == MOUSE_BUTTON_RIGHT) {
    if (Spell_C_IsTargeting()) {
      if (Spell_C_WorldObjectHousing()) {
        Spell_C_WorldObjectRotate();
      } else {
        Spell_C_StopTargeting();
      }
    }
    if (CGPlayer_C::IsGiftWrapping()) {
      ClearCursor(1);
    }
  }
  return 0;
}

BOOL CGGameUI::HandleMouseUp(const CMouseEvent &evt) {
  return 0;
}

BOOL CGGameUI::HandleTerrainClick(const CTerrainClickEvent &evt) {
  if (evt.button == MOUSE_BUTTON_RIGHT || m_cursorVirtualID) {
    ClearCursor(1);
  }

  if (!OnTerrainClick(evt)) {
    Target(0, 0);
  }
  return 1;
}

BOOL CGGameUI::HandleSpriteClick(const CSpriteClickEvent &evt) {
  if (m_cursorVirtualID) {
    ClearCursor(1);
  }

  if (evt.button == MOUSE_BUTTON_LEFT) {
    OnSpriteLeftClick(evt.objectGUID, evt.pos.x, evt.pos.y);
    return 1;
  }
  OnSpriteRightClick(evt.objectGUID, evt.pos.x, evt.pos.y);
  return 1;
}

BOOL CGGameUI::HandleWorldClick(const CWorldClickEvent &evt) {
  int cursorWasEmpty = m_cursorItemType == UICURSOR_EMPTY;
  if (evt.button == MOUSE_BUTTON_RIGHT || !m_cursorItem || !m_cursorItemContainer) {
    ClearCursor(1);
  }

  if (evt.button == MOUSE_BUTTON_LEFT) {
    if (m_cursorItem && m_cursorItemContainer) {
      DeleteCursorItem();
      return 1;
    }
    if (cursorWasEmpty) {
      Target(0, 0);
    }
  }
  return 1;
}

void CGGameUI::HandleSpriteTrack(const CObjectTrackEvent &evt) {
  HandleObjectTrackChange(evt.object, evt.oldGUID, evt.x, evt.y);
}

BOOL CGGameUI::HandleDisplaySizeChanged(const CSizeEvent &evt) {
  if (m_screenWidth <= 0 || m_screenWidth != evt.w) {
    m_screenWidth = evt.w;
    if (evt.w < 1024 || evt.w / 4 != evt.h / 3) {
      CSimpleTexture::s_textureFilterMode = GxTex_Linear;
    } else {
      CSimpleTexture::s_textureFilterMode = GxTex_Nearest;
    }
    if (evt.w > 1024) {
      ScaleUI(1024.0f / evt.w, 1);
    } else {
      ScaleUI(1.0f, 1);
    }
  }
  return 1;
}

void CGGameUI::HandleScreenshot(int success) {
  if (success) {
    FrameScript_SignalEvent(199);
  } else {
    FrameScript_SignalEvent(200);
  }
}

void CGGameUI::SetInteractTarget(const DWORDLONG &target, float maxDist) {
  if (m_interactTarget != target) {
    if (m_interactTarget) {
      ClearInteractTarget(m_interactTarget);
    }
    m_interactTarget = target;
    m_interactMaxDist = maxDist;

    if (m_interactTarget) {
      CGObject_C *object = ClntObjMgrObjectPtr(m_interactTarget, __FILE__, __LINE__);
      if (object) {
        if (object->IsA(ID_UNIT)) {
          ((CGUnit_C *)object)->OnNPCHello();
        }
        if (object->IsA(ID_GAMEOBJECT)) {
          ((CGGameObject_C *)object)->StartInteraction();
        }
      }
    }
  }
}

void CGGameUI::ClearInteractTarget(const DWORDLONG &target) {
  if (m_interactTarget && m_interactTarget == target) {
    CloseInteraction();
  }
}

void CGGameUI::UpdateInteractTarget() {
  if (!m_interactTarget || m_interactMaxDist <= 0.0f) {
    return;
  }

  CGObject_C *target = ClntObjMgrObjectPtr(m_interactTarget, __FILE__, __LINE__);
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!target || target->IsDisabled() || !player) {
    CloseInteraction();
  } else if (!target->IsA(ID_ITEM)) {
    if (player->LinearDistanceSquared(target->GetPosition()) > m_interactMaxDist) {
      CloseInteraction();
    }
  }
}

struct ClosestObjectMatchData {
  OBJECT_TYPE type;
  LPCSTR      match;
  CGUnit_C   *source;
  CGObject_C *object;
  int         best_match;
  float       best_distance;
};

void CGGameUI::CloseInteraction() {
  DWORDLONG target = m_interactTarget;
  if (target) {
    m_interactTarget = 0;
    if (CGQuestInfo::GetQuestGiver() == target) {
      CGQuestInfo::QuestGiverFinished();
    } else if (CGItemText::GetItem() == target) {
      CGItemText::SetItem(0, 0);
    } else if (CGTaxiMap::GetTaxiVendor() == target) {
      CGTaxiMap::CloseMap();
    } else if (CGClassTrainer::GetTrainer() == target) {
      CGClassTrainer::SetTrainer(0, TRAINER_TYPE_GENERAL);
    } else if (CGMerchantInfo::GetMerchant() == target) {
      CGMerchantInfo::CloseMerchant();
    } else if (CGTradeInfo::GetTradePartner() == target) {
      CGTradeInfo::SetTradePartner(0);
    } else if (CGBankInfo::m_unit == target) {
      CGBankInfo::CloseBank();
    } else if (CGTabardCreationFrame::GetVendor() == target) {
      CGTabardCreationFrame::Close();
    } else if (CGGuildRegistrar::GetRegistrar() == target) {
      CGGuildRegistrar::CloseRegistrar();
    }

    CGObject_C *object = ClntObjMgrObjectPtr(target, __FILE__, __LINE__);
    if (object) {
      if (object->IsA(ID_UNIT)) {
        ((CGUnit_C *)object)->OnNPCGoodbye();
      }
      if (object->IsA(ID_GAMEOBJECT)) {
        ((CGGameObject_C *)object)->CloseInteraction();
      }
    }
  }
}

void CGGameUI::Target(const DWORDLONG &target, int usingNearest) {
  if (!usingNearest) {
    s_nearestListTime = 0;
  }

  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  CGObject_C *oldTarget = ClntObjMgrObjectPtr(m_lockedTarget, __FILE__, __LINE__);

  if (target == m_lockedTarget) {
    return;
  }

  bool enterCombatMode = false;
  if (player && player->IsInCombatMode() && oldTarget->IsA(ID_UNIT) && player->GetHealth() > 0 && !player->IsMounted() &&
      ((CGUnit_C *)oldTarget)->GetHealth() > 0 && player->CanAttack((CGUnit_C *)oldTarget))
  {
    enterCombatMode = true;
  }

  ClearTarget(m_lockedTarget, 0);

  CDataStore msg;
  msg.Put(CMSG_SET_SELECTION);
  msg.Put(target);
  msg.Finalize();
  ClientServices_Send(&msg);

  if (target) {
    m_lockedTarget = target;
    CGObject_C *object = ClntObjMgrObjectPtr(m_lockedTarget, __FILE__, __LINE__);
    if (object) {
      object->ShowHighlightType(HT_OBJSELECTION);
      object->UpdatePlayerName();

      if (object->IsA(ID_UNIT)) {
        CGUnit_C *unit = (CGUnit_C *)object;
        unit->RegisterScript();

        if (player && player->GetHealth() > 0 && !player->IsMounted() && unit->GetHealth() > 0 && player->CanAttack(unit)) {
          m_lastEnemyTarget = target;
        }

        CGTutorial::TriggerTutorial(TUTORIAL_TARGETING);
        if (player->CanCooperate(unit)) {
          CGTutorial::TriggerTutorial(TUTORIAL_GROUPING);
        }
        if (player->CanAttack(unit)) {
          CGTutorial::TriggerTutorial(TUTORIAL_TARGETING_ENEMY);
        }

        if (unit->IsNPC()) {
          SndInterfacePlayInterfaceSound("igCharacterNPCSelect");
        } else if (object->IsA(ID_PLAYER)) {
          SndInterfacePlayInterfaceSound("igCharacterSelect");
        } else if (unit->UnitReaction(player) <= UNIT_REACTION_HOSTILE) {
          SndInterfacePlayInterfaceSound("igCreatureAggroSelect");
        } else {
          SndInterfacePlayInterfaceSound("igCreatureNeutralSelect");
        }
      }
    } else {
      SndInterfacePlayInterfaceSound("igCharacterSelect");
    }

    FrameScript_SignalEvent(191);
    if (enterCombatMode && m_lockedTarget != player->GetGUID()) {
      player->SetCombatMode(1);
    }
  }
}

void CGGameUI::ClearTarget(DWORDLONG guid, int sendTarget) {
  if (!m_lockedTarget || (guid && guid != m_lockedTarget)) {
    return;
  }

  if (CGLootInfo::m_object == m_lockedTarget) {
    CloseLoot(1, 0);
  }

  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  CGObject_C *object = ClntObjMgrObjectPtr(m_lockedTarget, __FILE__, __LINE__);
  if (object) {
    object->HideHighlightType(HT_OBJSELECTION);
    object->UpdatePlayerName();
    if (object->IsA(ID_UNIT)) {
      CGUnit_C *unit = (CGUnit_C *)object;
      unit->UnregisterScript();
      if (unit->IsNPC()) {
        SndInterfacePlayInterfaceSound("igCharacterNPCDeselect");
      } else if (object->IsA(ID_PLAYER)) {
        SndInterfacePlayInterfaceSound("igCharacterDeselect");
      } else if (!player || unit->UnitReaction(player) > UNIT_REACTION_HOSTILE) {
        SndInterfacePlayInterfaceSound("igCreatureNeutralDeselect");
      } else {
        SndInterfacePlayInterfaceSound("igCreatureAggroDeselect");
      }
    }
  }

  m_lockedTarget = 0;
  if (player) {
    player->SetCombatMode(0);
  }

  if (sendTarget) {
    CDataStore msg;
    msg.Put(CMSG_SET_SELECTION);
    msg.Put(m_lockedTarget);
    msg.Finalize();
    ClientServices_Send(&msg);
  }

  FrameScript_SignalEvent(191);
}

static BOOL ClosestObjectMatchProc(DWORDLONG guid, LPVOID param) {
  ClosestObjectMatchData *data = (ClosestObjectMatchData *)param;
  CGObject_C             *object = ClntObjMgrObjectPtr(guid, __FILE__, __LINE__);
  if (!object || !(data->type & object->GetType())) {
    return 1;
  }

  LPCSTR name = object->GetObjectName();
  if (name) {
    LPCSTR match = data->match;
    while (*match && *name && toupper(*match) == toupper(*name)) {
      ++match;
      ++name;
    }
    int match_len = match - data->match;
    if (match_len >= data->best_match) {
      float distance = data->source->LinearDistanceSquared(object->GetPosition());
      if (match_len > data->best_match || distance < data->best_distance) {
        data->best_distance = distance;
        data->object = object;
        data->best_match = match_len;
      }
    }
  }
  return 1;
}

DWORDLONG CGGameUI::ClosestObjectMatch(LPCSTR match, OBJECT_TYPE type) {
  CGUnit_C *source = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!source) {
    return 0;
  }

  ClosestObjectMatchData data;
  data.type = type;
  data.match = match;
  data.source = source;
  data.object = 0;
  data.best_match = 1;
  data.best_distance = FLT_MAX;
  ClntObjMgrEnumVisibleObjects(ClosestObjectMatchProc, &data);
  return data.object ? data.object->GetGUID() : 0;
}

void CGGameUI::AssistByName(LPCSTR name) {
  CGUnit_C *unit =
      static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(name && *name ? ClosestObjectMatch(name, TYPE_UNIT) : m_lockedTarget, __FILE__, __LINE__));
  if (unit) {
    DWORDLONG newTarget;
    if (unit->IsA(ID_PLAYER)) {
      newTarget = ((CGPlayer_C *)unit)->GetSelection();
    } else {
      newTarget = unit->GetTarget();
    }
    if (newTarget) {
      Target(newTarget, 0);
      if (s_assistAttackCVar->GetInt()) {
        CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
        if (player) {
          player->SetCombatMode(1);
        }
      }
    }
  } else if (name && *name) {
    DisplayError(GERR_UNIT_NOT_FOUND);
  } else {
    DisplayError(GERR_GENERIC_NO_TARGET);
  }
}

void CGGameUI::FollowByName(LPCSTR name) {
  CGUnit_C *unit =
      static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(name && *name ? ClosestObjectMatch(name, TYPE_PLAYER) : m_lockedTarget, __FILE__, __LINE__));
  if (unit) {
    CGUnit_C *player = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
    if (player) {
      if (unit->IsA(ID_PLAYER) && unit->UnitReaction(player) >= UNIT_REACTION_AMIABLE) {
        player->SaveTrackingTarget(unit->GetGUID(), TRACKTYPE_FOLLOW, false);
      } else {
        DisplayError(GERR_INVALID_FOLLOW_TARGET);
      }
    } else {
      DisplayError(GERR_GENERIC_NO_TARGET);
    }
  } else if (name && *name) {
    DisplayError(GERR_UNIT_NOT_FOUND);
  } else {
    DisplayError(GERR_GENERIC_NO_TARGET);
  }
}

static BOOL TargetUpdateProc(DWORDLONG guid, LPVOID) {
  CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
  if (unit && unit->IsA(ID_UNIT)) {
    CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
    if (player && player->GetHealth() > 0 && !player->IsMounted() && unit->GetHealth() > 0 && player->CanAttack(unit) &&
        unit->GetHealth() > 0 && !unit->IsFeignDeath())
    {
      float distSq = (unit->GetPosition() - player->GetPosition()).SquaredMag();
      if (distSq > TARGET_NEAREST_MAX_DISTANCE_SQUARED) {
        return 1;
      }
      NearestEnemyData *entry = s_nearestList.New();
      entry->guid = guid;
      entry->distSq = distSq;
    }
  }
  return 1;
}

static int __cdecl QSortCompareNearestEnemy(LPCVOID a, LPCVOID b) {
  FATALASSERT(a);
  FATALASSERT(b);
  const NearestEnemyData *left = (const NearestEnemyData *)a;
  const NearestEnemyData *right = (const NearestEnemyData *)b;
  if (left->distSq == right->distSq) {
    return 0;
  }
  if (left->distSq > right->distSq) {
    return 1;
  }
  return -1;
}

void CGGameUI::TargetNearestEnemy(int reverse) {
  if (!s_nearestListTime || !s_sameTargetTime || s_sameTargetTime + 3000 <= OsGetAsyncTimeMs() || !s_nearestList.Count()) {
    s_nearestIndex = 0;
    s_nearestListTime = 0;
    s_sameTargetTime = 0;
    s_nearestList.SetCount(0);
    ClntObjMgrEnumVisibleObjects(TargetUpdateProc, 0);
    if (!s_nearestList.Count()) {
      return;
    }
    qsort(s_nearestList.Ptr(), s_nearestList.Count(), sizeof(NearestEnemyData), QSortCompareNearestEnemy);
    s_nearestListTime = OsGetAsyncTimeMs();
  } else if (reverse) {
    if (s_nearestIndex == 0) {
      s_nearestIndex = s_nearestList.Count() - 1;
    } else {
      --s_nearestIndex;
    }
  } else {
    ++s_nearestIndex;
    if (s_nearestIndex >= s_nearestList.Count()) {
      s_nearestIndex = 0;
      if (s_nearestListTime + 3000 <= OsGetAsyncTimeMs()) {
        s_nearestListTime = 0;
        TargetNearestEnemy(0);
        return;
      }
    }
  }

  if (!s_nearestList.Count()) {
    return;
  }

  s_sameTargetTime = OsGetAsyncTimeMs();
  CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(s_nearestList[s_nearestIndex].guid, __FILE__, __LINE__));
  UINT      index = s_nearestIndex;
  while (!unit || unit->GetHealth() <= 0) {
    ++index;
    if (index >= s_nearestList.Count()) {
      index = 0;
      if (s_nearestListTime + 3000 <= OsGetAsyncTimeMs()) {
        s_nearestListTime = 0;
        TargetNearestEnemy(0);
        return;
      }
    }
    if (index == s_nearestIndex) {
      s_nearestList.SetCount(0);
      return;
    }
    unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(s_nearestList[index].guid, __FILE__, __LINE__));
  }

  s_nearestIndex = index;
  Target(s_nearestList[index].guid, 1);
}

void CGGameUI::ScaleUI(float scale, int force) {
  if (m_UISimpleParent) {
    m_UISimpleParent->SetLayoutScale(scale, force != 0);
  }
}

void CGGameUI::ShowCursor() {
  m_simpleTop->m_cursorVisible = 1;
}

void CGGameUI::HideCursor() {
  m_simpleTop->m_cursorVisible = 0;
}

void CGGameUI::AddErrorMessage(LPCSTR string, int error) {
  if (string && *string) {
    FrameScript_SignalEvent(error ? 215 : 216, "%s", string);
  }
}

void CGGameUI::UpdateObjectHighlightColor(HMODEL__ *model, CGObject_C *object) {
  if (model && object) {
    object->ShowHighlightType(HT_MOUSEOVER);
  }
}

void CGGameUI::ShowCombatFeedback(const ATTACKROUNDINFO *info) {
  FATALASSERT(info);
  FATALASSERT(info->newVictimState < (sizeof(s_combatEvent) / sizeof(s_combatEvent[0])));

  LPCSTR flagText = "";
  if (info->flags & 0x10000) {
    flagText = "ABSORB";
  } else if (info->flags & 8) {
    flagText = "CRITICAL";
  }

  int    numnames;
  char **names = Script_GetNamesFromGUID(info->victim, numnames);
  for (int index = 0; index < numnames; ++index) {
    FrameScript_SignalEvent(
        178, "%s%s%s%d%d", names[index], s_combatEvent[info->newVictimState], flagText, info->dmg.totalDamage, info->dmg.damageType[0]
    );
  }
}

void CGGameUI::ShowCombatFeedback(const DWORDLONG &guid, int amount, int damageClass, UINT flags) {
  LPCSTR flagText = "";
  if (flags & 0x10000) {
    flagText = "ABSORB";
  } else if (flags & 0x8) {
    flagText = "CRITICAL";
  }

  int    numnames;
  char **names = Script_GetNamesFromGUID(guid, numnames);
  for (int index = 0; index < numnames; ++index) {
    FrameScript_SignalEvent(178, "%s%s%s%d%d", names[index], "WOUND", flagText, amount, damageClass);
  }
}

void CGGameUI::ShowCombatFeedback(const SPELLLOG &log) {
  if (log.flags & 0x200) {
    return;
  }

  if (log.flags & 0x100) {
    CVar *periodicSpells = CVar::Lookup("CombatLogPeriodicSpells");
    if (!periodicSpells || !periodicSpells->GetInt()) {
      return;
    }
  }

  CGUnit_C *player = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  DWORDLONG pet = player ? *(player->GetCharm() ? &player->GetCharm() : &player->GetSummon()) : 0;

  if (log.victim != pet && log.victim != ClntObjMgrGetActivePlayer()) {
    return;
  }

  if (log.flags & 2) {
    ShowHealingFeedback(log.victim, log.dmg.totalDamage);
  } else if (log.dmg.damageType[0] != -1) {
    UINT flags = 0;
    if (log.dmg.absorbed[0] == log.dmg.damage[0]) {
      flags = 0x10000;
    }
    if (log.flags & 0x40) {
      flags |= 8;
    }
    ShowCombatFeedback(log.victim, log.dmg.totalDamage, log.dmg.damageType[0], flags);
  }
}

void CGGameUI::ShowCombatFeedback(const MIRRORTIMERDAMAGE &log) {
  int    numnames;
  char **names = Script_GetNamesFromGUID(log.victim, numnames);
  for (int index = 0; index < numnames; ++index) {
    FrameScript_SignalEvent(178, "%s%s%s%d%d", names[index], s_combatEvent[1], "", log.amount, 0);
  }
}

void CGGameUI::ShowSpellMissFeedback(DWORDLONG victim, int reason) {
  FATALASSERT(reason < (sizeof(s_spellMissReasons) / sizeof(s_spellMissReasons[0])));
  int    numnames;
  char **names = Script_GetNamesFromGUID(victim, numnames);
  for (int index = 0; index < numnames; ++index) {
    FrameScript_SignalEvent(179, "%s%s", names[index], s_spellMissReasons[reason]);
  }
}

void CGGameUI::ShowHealingFeedback(const DWORDLONG &guid, int amount) {
  int    numnames;
  char **names = Script_GetNamesFromGUID(guid, numnames);
  for (int index = 0; index < numnames; ++index) {
    FrameScript_SignalEvent(178, "%s%s%s%d", names[index], "HEAL", "", amount);
  }
}

void CGGameUI::ShowAutoFollowChange(DWORDLONG newTarget, DWORDLONG oldTarget, int type) {
  if (type == 2) {
    int       event;
    DWORDLONG target;
    if (newTarget) {
      event = 350;
      target = newTarget;
    } else {
      event = 351;
      target = oldTarget;
    }

    CGObject_C *object = ClntObjMgrObjectPtr(target, __FILE__, __LINE__);
    FrameScript_SignalEvent(event, "%s", (object && object->IsA(ID_UNIT)) ? ((CGUnit_C *)object)->GetUnitName() : "");
  }
}

void CGGameUI::NewZoneFeedback(int areaID, LPCSTR zoneString, LPCSTR subZoneString) {
  int firstArea = !m_areaID;
  m_areaID = areaID;
  if (firstArea) {
    CGWorldMap::SetMapToCurrentZone();
  }

  int zoneChanged = 0;
  if (zoneString && *zoneString) {
    if (!m_zoneText || SStrCmpI(m_zoneText, zoneString, 0x7FFFFFFF)) {
      FREEIFUSED(m_zoneText);
      m_zoneText = SStrDupA(zoneString, __FILE__, __LINE__);
      zoneChanged = 1;
    }
  } else if (m_zoneText && *m_zoneText) {
    *m_zoneText = 0;
    zoneChanged = 1;
  }

  if (subZoneString && *subZoneString) {
    if (!m_subZoneText || SStrCmpI(m_subZoneText, subZoneString, 0x7FFFFFFF)) {
      FREEIFUSED(m_subZoneText);
      m_subZoneText = SStrDupA(subZoneString, __FILE__, __LINE__);
      FrameScript_SignalEvent(196);
      return;
    }
  } else if (m_subZoneText && *m_subZoneText) {
    *m_subZoneText = 0;
    FrameScript_SignalEvent(196);
    return;
  }

  if (zoneChanged) {
    FrameScript_SignalEvent(196);
  }
}

void CGGameUI::SetMinimapZoneText(LPCSTR areaName) {
  if (areaName && *areaName) {
    if (m_minimapZoneText && !SStrCmpI(m_minimapZoneText, areaName, 0x7FFFFFFF)) {
      return;
    }
    FREEIFUSED(m_minimapZoneText);
    m_minimapZoneText = SStrDupA(areaName, __FILE__, __LINE__);
    FrameScript_SignalEvent(197);
  } else if (m_minimapZoneText && *m_minimapZoneText) {
    *m_minimapZoneText = 0;
    FrameScript_SignalEvent(197);
  }
}

void CGGameUI::NamePlateClicked(DWORDLONG unit, MOUSEBUTTON button) {
  FATALASSERT(unit);

  if (ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__)) {
    if (button == MOUSE_BUTTON_LEFT) {
      OnSpriteLeftClick(unit, 0.0f, 0.0f);
    } else if (button == MOUSE_BUTTON_RIGHT) {
      OnSpriteRightClick(unit, 0.0f, 0.0f);
    }
  }
}

void CGGameUI::SetCursorItem(DWORDLONG itemGUID, DWORDLONG containerGUID, UINT slot, int unlock, UINT stackSplit) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player && (m_hasControl || player->IsOnTaxi())) {
    ClearCursor(unlock);
    if (itemGUID) {
      m_cursorItemSlot = slot;
      m_cursorItem = itemGUID;
      m_cursorItemContainer = containerGUID;
      m_stackSplit = stackSplit;
      m_cursorItemType = UICURSOR_ITEM;
      CursorSetHeldItem(itemGUID);
      CGItem_C *item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(itemGUID, __FILE__, __LINE__));
      if (item) {
        if (item->CanBeUsed()) {
          CGActionBar::ShowGrid();
          m_cursorHasAction = 1;
        }
        SndInterfacePlayItemSound(ITEMSOUND_PICKUP, item);
      }
      FrameScript_SignalEvent(272);
    }
  }
}

DWORDLONG CGGameUI::GetCursorItem() {
  if (m_cursorItemType == UICURSOR_ITEM) {
    return m_cursorItem;
  }

  return 0;
}

void CGGameUI::GetCursorItem(DWORDLONG &cursorItem, DWORDLONG &containerGUID, UINT &slot) {
  cursorItem = m_cursorItem;
  containerGUID = m_cursorItemContainer;
  slot = m_cursorItemSlot;
}

void CGGameUI::SetCursorMoney(UINT money) {
  if (m_hasControl && (money || m_cursorItemType == UICURSOR_MONEY)) {
    ClearCursor(1);
    if (money) {
      m_cursorMoney = money;
      m_cursorItemType = UICURSOR_MONEY;
      SndInterfacePlayInterfaceSound("LOOTWINDOWCOINSOUND");
      CursorGrabMoney(money);
      FrameScript_SignalEvent(49, "%s", "player");
    }
  }
}

void CGGameUI::SetCursorSpell(int spellId, int pet) {
  if ((m_hasControl || !pet) && spellId >= 0) {
    const SpellRec *spell = g_spellDB.GetRecord(spellId);
    LPCSTR          texture = 0;
    if (spell && spell->m_effect[0] == 78) {
      texture = CGActionBar::GetAttackTexture();
    } else if (spell) {
      const SpellIconRec *icon = g_spellIconDB.GetRecord(spell->m_spellIconID);
      texture = icon ? icon->m_textureFilename : 0;
    }
    if (texture && *texture) {
      ClearCursor(1);
      m_cursorSpell = spellId;
      m_cursorItemType = pet ? UICURSOR_PET_SPELL : UICURSOR_SPELL;
      SndInterfacePlayInterfaceSound("INTERFACESOUND_CURSORGRABOBJECT");
      CursorGrabSpell(texture);
      if (pet) {
        CGPetInfo::ShowGrid();
      } else {
        CGActionBar::ShowGrid();
      }
      m_cursorHasAction = 1;
    }
  }
}

void CGGameUI::DropCursorSpell() {
  if (m_cursorItemType == UICURSOR_SPELL || m_cursorItemType == UICURSOR_PET_SPELL) {
    ClearCursor(1);
  }
}

int CGGameUI::GetCursorSpell() {
  return m_cursorSpell;
}

void CGGameUI::SetCursorPetAction(const PetAction &action) {
  if (!m_hasControl) {
    return;
  }

  LPCSTR texture = 0;
  switch (action.GetActionType()) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5: {
      const SpellRec *spell = g_spellDB.GetRecord(action.GetActionID());
      if (spell) {
        const SpellIconRec *icon = g_spellIconDB.GetRecord(spell->m_spellIconID);
        if (icon) {
          texture = icon->m_textureFilename;
        }
      }
      break;
    }
    case 6: {
      char buf[64];
      SStrPrintf(buf, sizeof(buf), "PET_%s_TEXTURE", CGPetInfo::GetModeToken(action.GetActionID()));
      texture = FrameScript_GetText(buf, -1, GENDER_NOT_APPLICABLE);
      break;
    }
    case 7: {
      char buf[64];
      SStrPrintf(buf, sizeof(buf), "PET_%s_TEXTURE", CGPetInfo::GetOrdersToken(action.GetActionID()));
      texture = FrameScript_GetText(buf, -1, GENDER_NOT_APPLICABLE);
      break;
    }
  }

  if (texture) {
    ClearCursor(1);
    m_cursorPetAction = action.GetAction();
    m_cursorItemType = UICURSOR_PET_ACTION;
    SndInterfacePlayInterfaceSound("INTERFACESOUND_CURSORGRABOBJECT");
    CursorGrabSpell(texture);
    CGPetInfo::ShowGrid();
    m_cursorHasAction = 1;
  }
}

void CGGameUI::DropCursorPetAction() {
  if (m_cursorItemType == UICURSOR_PET_ACTION) {
    ClearCursor(1);
  }
}

void CGGameUI::SetCursorVirtualItem(UINT itemID, UINT displayID, UINT slot, UICURSORTYPE type) {
  if (m_hasControl || type == UICURSOR_ACTIONBAR) {
    ClearCursor(1);
    if (itemID) {
      m_cursorVirtualID = itemID;
      m_cursorVirtualDisplay = displayID;
      m_cursorVirtualSlot = slot;
      m_cursorItemType = type;
      if (type == UICURSOR_ACTIONBAR) {
        CGActionBar::ShowGrid();
        m_cursorHasAction = 1;
      }
      CursorSetHeldVirtualItem(displayID);
      SndInterfacePlayItemSound(ITEMSOUND_PICKUP, displayID);
    }
  }
}

UINT CGGameUI::GetCursorVirtualItem() {
  return m_cursorVirtualID;
}

void CGGameUI::GetCursorVirtualItem(UINT &cursorItem, UINT &slot) {
  cursorItem = m_cursorVirtualID;
  slot = m_cursorVirtualSlot;
}

void CGGameUI::ClearCursor(int unlock) {
  if (CGPlayer_C::IsGiftWrapping()) {
    CGPlayer_C::CancelGiftWrap();
  }

  switch (m_cursorItemType) {
    case UICURSOR_ITEM: {
      if (unlock && m_cursorItem) {
        UnlockItem(m_cursorItem);
      }

      CGItem_C *item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(m_cursorItem, __FILE__, __LINE__));
      if (item) {
        SndInterfacePlayItemSound(ITEMSOUND_DROP, item);
      }
      CursorSetHeldItem(0);
      m_cursorItem = 0;
      m_cursorItemContainer = 0;
      m_cursorItemSlot = 0;
      break;
    }

    case UICURSOR_MONEY:
      CursorDropMoney();
      m_cursorMoney = 0;
      SndInterfacePlayInterfaceSound("LOOTWINDOWCOINSOUND");
      FrameScript_SignalEvent(49, "%s", "player");
      break;

    case UICURSOR_SPELL:
    case UICURSOR_PET_SPELL:
      CursorDropSpell();
      m_cursorSpell = -1;
      SndInterfacePlayInterfaceSound("INTERFACESOUND_CURSORDROPOBJECT");
      break;

    case UICURSOR_PET_ACTION:
      CursorDropSpell();
      m_cursorPetAction = 0;
      SndInterfacePlayInterfaceSound("INTERFACESOUND_CURSORDROPOBJECT");
      break;

    case UICURSOR_MERCHANT:
    case UICURSOR_LOOT:
    case UICURSOR_ACTIONBAR:
      SndInterfacePlayItemSound(ITEMSOUND_DROP, m_cursorVirtualDisplay);
      CursorSetHeldItem(0);
      m_cursorVirtualID = 0;
      m_cursorVirtualDisplay = 0;
      m_cursorVirtualSlot = 0;
      SndInterfacePlayInterfaceSound("INTERFACESOUND_CURSORDROPOBJECT");
      break;

    default:
      break;
  }

  if (m_cursorHasAction) {
    if (m_cursorItemType == UICURSOR_PET_SPELL || m_cursorItemType == UICURSOR_PET_ACTION) {
      CGPetInfo::HideGrid();
    } else {
      CGActionBar::HideGrid();
    }
    m_cursorHasAction = 0;
  }

  m_cursorItemType = UICURSOR_EMPTY;
  FrameScript_SignalEvent(272);
}

void CGGameUI::DeleteCursorItem() {
  CGItem_C *item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(m_cursorItem, __FILE__, __LINE__));
  if (item && item->GetOwner() == ClntObjMgrGetActivePlayer()) {
    const ItemStats_C *stats = g_itemDBCache.GetRecord(item->GetEntryID(), 0, 0, 0);
    if (stats) {
      FrameScript_SignalEvent(271, "%s", stats->m_displayName[0]);
    }
  }
}

void CGGameUI::LockItem(DWORDLONG itemGUID) {
  if (itemGUID) {
    CGItem_C *item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(itemGUID, __FILE__, __LINE__));
    if (item) {
      item->Lock();
    }
    FrameScript_SignalEvent(184);
  }
}

void CGGameUI::UnlockItem(DWORDLONG itemGUID) {
  if (itemGUID) {
    CGItem_C *item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(itemGUID, __FILE__, __LINE__));
    if (item) {
      item->Unlock();
    }
    FrameScript_SignalEvent(184);
  }
}

void CGGameUI::UnlockAllItems() {
  CGObject_C *player = ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__);
  if (!player || !player->GetBag()) {
    return;
  }

  CGBag_C *bag = player->GetBag();
  UINT     numSlots = bag->NumSlots();
  for (UINT slot = 0; slot < numSlots; ++slot) {
    CGObject_C *item = ClntObjMgrObjectPtr(bag->GetItem(slot), __FILE__, __LINE__);
    if (!item) {
      continue;
    }

    ((CGItem_C *)item)->Unlock();
    if (item->IsA(TYPE_CONTAINER)) {
      if (item->GetBag()) {
        for (UINT bagSlot = 0; bagSlot < item->GetBag()->NumSlots(); ++bagSlot) {
          CGItem_C *bagItem = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(item->GetBag()->GetItem(slot), __FILE__, __LINE__));
          if (bagItem) {
            bagItem->Unlock();
          }
        }
      }
    }
  }

  FrameScript_SignalEvent(184);
}

BOOL CGGameUI::Idle(LPCVOID, LPVOID) {
  if (m_reloadUI) {
    Shutdown();
    Initialize();
  }
  return 1;
}

void CGGameUI::UpdateActivePlayer() {
  DWORDLONG guid = ClntObjMgrGetActivePlayer();
  CGUnit_C *player = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
  FATALASSERT(player);

  if (player->GetHealth() <= 0) {
    FrameScript_SignalEvent(256);
    ClearCursor(1);
  } else {
    FrameScript_SignalEvent(255);
  }
}

void CGGameUI::OnClientControlChanged(BOOL hasControl) {
  if (hasControl != m_hasControl) {
    m_hasControl = hasControl;
    if (hasControl) {
      FrameScript_SignalEvent(194);
    } else {
      ClearClientControls();
      FrameScript_SignalEvent(193);
    }
  }
}

void CGGameUI::ClearClientControls() {
  ClearCursor(1);
  Target(0, 0);
  Spell_C_CancelSpell(1, 1, SPELL_FAILED_ERROR);
}

void CGGameUI::RegisterFrameFactories() {
  FrameXML_RegisterFactory("WorldFrame", CGWorldFrame::Create);
  FrameXML_RegisterFactory("GameTooltip", CGTooltip::Create);
  FrameXML_RegisterFactory("Minimap", CGMinimapFrame::Create);
  FrameXML_RegisterFactory("PlayerModel", CGCharacterModelBase::Create);
  FrameXML_RegisterFactory("TabardModel", CGTabardModelFrame::Create);
}

void __cdecl CGGameUI::DisplayError(GAME_ERROR_TYPE errorType, ...) {
  FATALASSERT(errorType < GERR_NUM_TYPES);

  if (s_gameErrors[errorType].voiceID != VUI_NONE) {
    if (ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__)) {
      SndInterfacePlayVocalUISound(s_gameErrors[errorType].voiceID);
    }
  } else if (SStrCmpI(s_gameErrors[errorType].soundName, "NONE", 0x7FFFFFFF)) {
    SndInterfacePlayInterfaceSound(s_gameErrors[errorType].soundName);
  }

  if (!s_gameErrors[errorType].stringToken || !*s_gameErrors[errorType].stringToken) {
    return;
  }

  char   format[256];
  LPCSTR text = FrameScript_GetText(s_gameErrors[errorType].stringToken, -1, GENDER_NOT_APPLICABLE);
  SStrCopy(format, text, sizeof(format));
  if (!*text) {
    return;
  }

  char    buffer[256];
  va_list arguments;
  va_start(arguments, errorType);
  SStrVPrintf(buffer, sizeof(buffer), format, arguments);
  va_end(arguments);
  SStrCopy(s_lastErrorString, buffer, 256);

  switch (s_gameErrors[errorType].textPlacement) {
    case ERRORTEXT_CHAT:
      CGChat::AddChatMessage(buffer, s_gameErrors[errorType].slashCmd, 0, 0, 0, 0, 0);
      break;
    case ERRORTEXT_UIINFO:
      AddErrorMessage(buffer, 0);
      break;
    case ERRORTEXT_UIERROR:
      AddErrorMessage(buffer, 1);
      break;
    case ERRORTEXT_CONSOLE:
      ConsoleWriteA(buffer, ERROR_COLOR);
      break;
  }
}

LPCSTR CGGameUI::GetLastErrorString() {
  return s_lastErrorString;
}

void CGGameUI::PlayerCombatModeChanged(int newState) {
  if (newState) {
    FrameScript_SignalEvent(189);
  } else {
    FrameScript_SignalEvent(190);
  }
}
