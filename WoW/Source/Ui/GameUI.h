#ifndef WOW_SOURCE_UI_GAMEUI_H
#define WOW_SOURCE_UI_GAMEUI_H

#include "Object/Object.h"
#include "Net/NetClient/NetClient.h"
#include "LootFrame.h"
#include "PartyFrame.h"

#include <Event/EvtApi.h>

class CSimpleFrame;
class CSimpleTop;
class CGTooltip;
class CGCursor;
class CGSpellBook;
class CGObject_C;
class CinematicSequencesRec;
class CinematicCameraRec;
class CDataStore;
struct Sound;
struct CSpriteClickEvent;
struct CTerrainClickEvent;
struct CWorldClickEvent;
struct CObjectTrackEvent;
struct ATTACKROUNDINFO;
struct MIRRORTIMERDAMAGE;
struct SPELLLOG;
struct HMODEL__;
struct lua_State;

static int DebugAIStateHandler(void *, NETMESSAGE, unsigned long, CDataStore *);
static int Script_PickupPetAction(lua_State *L);
static int Script_TogglePetAutocast(lua_State *L);
static int Script_CastPetAction(lua_State *L);
class CMouseEvent;
class CSizeEvent;
class PetAction;
enum SYSMSG_TYPE;

struct CinematicData {
  CinematicSequencesRec *sequence;
  Sound                 *sequenceMusic;
  int                    currentCamera;
  CinematicCameraRec    *camera;
  Sound                 *cameraMusic;
  int                    zoneMusicPaused;
};

enum GAME_ERROR_TYPE {
  GAME_ERROR_NONE = 0,
  GERR_SPELL_FAILED_S = 39,
  GERR_SPELL_ALREADY_KNOWN_S = 45,
  GERR_PLAYER_DIED_S = 68,
  GERR_PLAYER_DEAD = 113,
  GERR_QUEST_ACCEPTED_S = 123,
  GERR_SPELL_FAILED_TOTEMS = 190,
  GERR_SPELL_FAILED_REAGENTS = 191,
  GERR_CHEST_IN_USE = 197,
  GERR_USE_CANT_OPEN = 198,
  GERR_USE_LOCKED = 199,
  GERR_USE_LOCKED_WITH_ITEM_S = 200,
  GERR_USE_LOCKED_WITH_SPELL_S = 201,
  GERR_USE_LOCKED_WITH_SPELL_KNOWN_SI = 202,
  GERR_USE_TOO_FAR = 203,
  GERR_USE_OBJECT_MOVING = 205,
  GERR_USE_DESTROYED = 207,
  GERR_TAME_FAILED = 214,
  GERR_FRIEND_ERROR = 241,
  GERR_OUT_OF_MANA = 260,
  GERR_OUT_OF_RAGE = 261,
  GERR_OUT_OF_FOCUS = 262,
  GERR_OUT_OF_ENERGY = 263,
  GERR_OUT_OF_HEALTH = 264,
  GERR_SPELL_OUT_OF_RANGE = 277,
  GERR_NUM_TYPES = 297,
  GERR_NONE = 297
};

enum UICURSORTYPE {
  UICURSOR_EMPTY = 0,
  UICURSOR_ITEM = 1,
  UICURSOR_MONEY = 2,
  UICURSOR_SPELL = 3,
  UICURSOR_PET_SPELL = 4,
  UICURSOR_PET_ACTION = 5,
  UICURSOR_MERCHANT = 6,
  UICURSOR_LOOT = 7,
  UICURSOR_ACTIONBAR = 8
};

class CGGameUI {
 public:
  static void ClearClientControls();
  static void InitializeGame();
  static void Initialize();
  static void Shutdown();
  static void RegisterFrameFactories();
  static void Reload();
  static void ShutdownGame();
  static void UpdateActivePlayer();
  static CGTooltip *GetGameTooltip() {
    return m_gameTooltip;
  }
  static void UnitNameUpdate(const unsigned __int64 &guid);
  static void UnitPortraitUpdate(const unsigned __int64 &guid);
  static void SetPartyLeader(unsigned __int64 guid);
  static void AddPartyMember(unsigned __int64 guid, int connected);
  static void RemoveAllPartyMembers();
  static void SetLootMethod(LOOT_METHOD method, unsigned __int64 master);
  static void ClearLootSlot(unsigned char slot);
  static void OpenLoot(CGObject_C *object, int coins, LOOT_ACQUIRE lootType);
  static void Target(const unsigned __int64 &target, int usingNearest);
  static unsigned __int64 ClosestObjectMatch(const char *match, OBJECT_TYPE type);
  static void TargetNearestEnemy(int reverse);
  static void AssistByName(const char *name);
  static void FollowByName(const char *name);
  static int IsPartyMember(const unsigned __int64 &guid);
  static void EnablePartyMember(unsigned __int64 guid, int enable);
  static unsigned __int64 GetPartyMember(unsigned int index);
  static void ClearTarget(unsigned __int64 guid, int sendTarget);
  static void ClearInteractTarget(const unsigned __int64 &target);
  static void SetInteractTarget(const unsigned __int64 &target, float maxDist);
  static void CloseInteraction();
  static void ResetCamera();
  static void SysMsgDisplay(const char *msg, SYSMSG_TYPE severity);
  static int FilterMouseDown(const CMouseEvent &evt);
  static int HandleMouseDown(const CMouseEvent &evt);
  static int HandleMouseUp(const CMouseEvent &evt);
  static int HandleDisplaySizeChanged(const CSizeEvent &evt);
  static void ScaleUI(float scale, int force);
  static void NamePlateClicked(unsigned __int64 unit, MOUSEBUTTON button);
  static void EnterWorld();
  static void LeaveWorld();
  static void UpdateInteractTarget();
  static unsigned __int64 GetCursorItem();
  static void
  SetCursorItem(unsigned __int64 itemGUID, unsigned __int64 containerGUID, unsigned int slot, int unlock, unsigned int stackSplit);
  static unsigned int GetCursorMoney() {
    return m_cursorMoney;
  }
  static unsigned int GetCursorStackSplit() {
    return m_stackSplit;
  }
  static void SetCursorMoney(unsigned int money);
  static void SetCursorSpell(int spellId, int pet);
  static void DropCursorSpell();
  static void SetCursorPetAction(const PetAction &action);
  static void DropCursorPetAction();
  static int GetCursorSpell();
  static void GetCursorItem(unsigned __int64 &cursorItem, unsigned __int64 &containerGUID, unsigned int &slot);
  static unsigned int GetCursorVirtualItem();
  static void SetCursorVirtualItem(unsigned int itemID, unsigned int displayID, unsigned int slot, UICURSORTYPE type);
  static void GetCursorVirtualItem(unsigned int &cursorItem, unsigned int &slot);
  static void UnlockItem(unsigned __int64 itemGUID);
  static void UnlockAllItems();
  static void LockItem(unsigned __int64 itemGUID);
  static void CloseLoot(bool send, bool moving);
  static void NewZoneFeedback(int areaID, const char *zoneString, const char *subZoneString);
  static void SetMinimapZoneText(const char *areaName);
  static void ClearCursor(int unlock);
  static void DeleteCursorItem();
  static void PlayerCombatModeChanged(int newState);
  static void StartCinematic(int cinematicID);
  static void BeginCinematic();
  static void BeginCinematicInternal(void *);
  static int StartCinematicCamera();
  static int NextCinematic(void *);
  static void NextCinematicInternal(void *);
  static int StopCinematic(void *__formal);
  static void StopCinematicInternal(void *);
  static void HideCursor();
  static void ShowCursor();
  static void ShowHealingFeedback(const unsigned __int64 &guid, int amount);
  static void ShowSpellMissFeedback(unsigned __int64 victim, int reason);
  static void OnClientControlChanged(int hasControl);
  static void ShowCombatFeedback(const ATTACKROUNDINFO *info);
  static void ShowCombatFeedback(const SPELLLOG &log);
  static void ShowCombatFeedback(const unsigned __int64 &guid, int amount, int damageClass, unsigned int flags);
  static void ShowCombatFeedback(const MIRRORTIMERDAMAGE &log);
  static void OnItemPush(unsigned __int64 player, int slot, int itemID, int pushed, int display);
  static void OpenPartyInvite(const char *inviter);
  static void OpenResurrectRequest(const char *inviter);
  static void CancelPartyInvite();
  static void OpenGuildInvite(const char *inviter, const char *guildName);
  static void CancelGuildInvite();
  static void AddErrorMessage(const char *string, int error);
  static void __cdecl            DisplayError(GAME_ERROR_TYPE errorType, ...);
  static const char *GetLastErrorString();
  static void ShowAutoFollowChange(unsigned __int64 newTarget, unsigned __int64 oldTarget, int type);
  static int HandleSpriteClick(const CSpriteClickEvent &evt);
  static int HandleTerrainClick(const CTerrainClickEvent &evt);
  static int HandleWorldClick(const CWorldClickEvent &evt);
  static void HandleSpriteTrack(const CObjectTrackEvent &evt);
  static void HandleScreenshot(int success);

  static int OnTerrainClick(const CTerrainClickEvent &evt);
  static int OnSpriteLeftClick(unsigned __int64 object, float x, float y);
  static int OnSpriteRightClick(unsigned __int64 object, float x, float y);
  static void OnTargetContextAction();

  static const unsigned __int64 &GetCurrentObjectTrack() {
    return m_currentObjectTrack;
  }

  static const unsigned __int64 &GetInteractTarget() {
    return m_interactTarget;
  }

  static const unsigned __int64 &GetLockedTarget() {
    return m_lockedTarget;
  }

  static const unsigned __int64 &GetLastEnemyTarget() {
    return m_lastEnemyTarget;
  }

  static const char *GetZoneText() {
    return m_zoneText;
  }

  static const char *GetSubZoneText() {
    return m_subZoneText;
  }

  static const char *GetMinimapZoneText() {
    return m_minimapZoneText;
  }

 private:
  friend int DebugAIStateHandler(void *, NETMESSAGE, unsigned long, CDataStore *);

  static void HandleObjectTrackChange(unsigned __int64 object, unsigned __int64 oldGUID, float x, float y);
  static void UpdateObjectHighlightColor(HMODEL__ *model, CGObject_C *object);
  friend class CGTooltip;
  friend class CGCursor;
  friend class CGSpellBook;
  friend class CGActionBar;
  friend class CGMinimapFrame;
  friend int Script_PickupPetAction(lua_State *L);
  friend int Script_TogglePetAutocast(lua_State *L);
  friend int Script_CastPetAction(lua_State *L);
  friend int Script_CursorHasItem(lua_State *L);
  friend int Script_CursorHasSpell(lua_State *L);
  friend int Script_CursorHasMoney(lua_State *L);
  friend int Script_HasFullControl(lua_State *L);
  friend int Script_DeleteCursorItem(lua_State *__formal);
  friend int Script_TargetLastEnemy(lua_State *__formal);

  static int Idle(const void *data, void *param);

  static CSimpleFrame    *m_UISimpleParent;
  static CSimpleTop      *m_simpleTop;
  static bool             m_reloadUI;
  static unsigned int     m_stackSplit;
  static unsigned __int64 m_cursorItem;
  static unsigned __int64 m_cursorItemContainer;
  static unsigned int     m_cursorItemSlot;
  static unsigned int     m_cursorMoney;
  static int              m_cursorSpell;
  static unsigned int     m_cursorPetAction;
  static unsigned int     m_cursorVirtualID;
  static unsigned int     m_cursorVirtualDisplay;
  static unsigned int     m_cursorVirtualSlot;
  static int              m_cursorHasAction;
  static UICURSORTYPE     m_cursorItemType;
  static unsigned __int64 m_currentObjectTrack;
  static float            m_interactMaxDist;
  static unsigned __int64 m_interactTarget;
  static unsigned __int64 m_lockedTarget;
  static unsigned __int64 m_lastEnemyTarget;
  static char            *m_zoneText;
  static char            *m_subZoneText;
  static char            *m_minimapZoneText;
  static int              m_areaID;
  static int              m_hasControl;
  static int              m_screenWidth;
  static CGTooltip       *m_gameTooltip;
  static CinematicData    m_cinematic;
  static char             s_lastErrorString[512];
};

#endif
