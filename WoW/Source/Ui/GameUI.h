#ifndef WOW_SOURCE_UI_GAMEUI_H
#define WOW_SOURCE_UI_GAMEUI_H

#include "Object/Object.h"
#include "Object/Unit.h"
#include "WowServices/WDataStore.h"

#include <Event/EvtApi.h>

class CSimpleFrame;
class CSimpleTop;
class CGTooltip;
class CGCursor;
class CGSpellBook;
class CGBankInfo;
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

static BOOL DebugAIStateHandler(LPVOID, NETMESSAGE, DWORD, CDataStore *);
static int  Script_PickupPetAction(lua_State *L);
static int  Script_TogglePetAutocast(lua_State *L);
static int  Script_CastPetAction(lua_State *L);
class CMouseEvent;
class CSizeEvent;
class PetAction;
enum SYSMSG_TYPE;

struct CinematicData {
  const CinematicSequencesRec *sequence;
  Sound                       *sequenceMusic;
  int                          sequenceIndex;
  const CinematicCameraRec    *camera;
  Sound                       *cameraMusic;
  int                          zoneMusicWasEnabled;
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
  friend BOOL DebugAIStateHandler(LPVOID, NETMESSAGE, DWORD, CDataStore *);
  friend class CGTooltip;
  friend class CGCursor;
  friend class CGSpellBook;
  friend class CGBankInfo;
  friend class CGActionBar;
  friend class CGPlayer_C;
  friend class CGMinimapFrame;
  friend class CGWorldMap;
  friend int Script_PickupPetAction(lua_State *L);
  friend int Script_TogglePetAutocast(lua_State *L);
  friend int Script_CastPetAction(lua_State *L);
  friend int Script_CursorHasItem(lua_State *L);
  friend int Script_CursorHasSpell(lua_State *L);
  friend int Script_CursorHasMoney(lua_State *L);
  friend int Script_HasFullControl(lua_State *L);
  friend int Script_DeleteCursorItem(lua_State *);
  friend int Script_TargetLastEnemy(lua_State *);

 public:
  static void          InitializeGame();
  static void          Initialize();
  static void          EnterWorld();
  static void          LeaveWorld();
  static void          Shutdown();
  static void          ShutdownGame();
  static void          Reload();
  static void          SysMsgDisplay(LPCSTR msg, SYSMSG_TYPE severity);
  static BOOL          FilterMouseDown(const CMouseEvent &evt);
  static BOOL          HandleMouseDown(const CMouseEvent &evt);
  static BOOL          HandleMouseUp(const CMouseEvent &evt);
  static BOOL          HandleTerrainClick(const CTerrainClickEvent &evt);
  static BOOL          HandleSpriteClick(const CSpriteClickEvent &evt);
  static BOOL          HandleWorldClick(const CWorldClickEvent &evt);
  static void          HandleSpriteTrack(const CObjectTrackEvent &evt);
  static BOOL          HandleDisplaySizeChanged(const CSizeEvent &evt);
  static void          HandleScreenshot(int success);
  static void          CloseLoot(bool send, bool moving);
  static void          OpenLoot(CGObject_C *object, int coins, LOOT_ACQUIRE lootType);
  static void          ClearLootSlot(BYTE slot);
  static void          OpenResurrectRequest(LPCSTR inviter);
  static void          OpenPartyInvite(LPCSTR inviter);
  static void          CancelPartyInvite();
  static void          OpenGuildInvite(LPCSTR inviter, LPCSTR guildName);
  static void          CancelGuildInvite();
  static void          ShowCursor();
  static void          HideCursor();
  static void          AddErrorMessage(LPCSTR string, int error);
  static void          SetInteractTarget(const DWORDLONG &target, float maxDist);
  static void          ClearInteractTarget(const DWORDLONG &target);
  static void          ClearInteractTarget();
  static void          UpdateInteractTarget();
  static void          CloseInteraction();
  static void          Target(const DWORDLONG &target, int usingNearest);
  static void          TargetIfNone(const DWORDLONG &target);
  static DWORDLONG     ClosestObjectMatch(LPCSTR match, OBJECT_TYPE type);
  static void          AssistByName(LPCSTR name);
  static void          FollowByName(LPCSTR name);
  static void          TargetNearestEnemy(int reverse);
  static void          ClearTarget(DWORDLONG guid, int sendTarget);
  static void          ScaleUI(float scale, int force);
  static CSimpleFrame *GetUISimpleParent();
  static CGTooltip *GetGameTooltip() {
    return m_gameTooltip;
  }

  static LPCSTR GetZoneText() {
    return m_zoneText;
  }

  static LPCSTR GetSubZoneText() {
    return m_subZoneText;
  }

  static LPCSTR GetMinimapZoneText() {
    return m_minimapZoneText;
  }
  static void ShowCombatFeedback(const ATTACKROUNDINFO *info);
  static void ShowCombatFeedback(const DWORDLONG &guid, int amount, int damageClass, UINT flags);
  static void ShowCombatFeedback(const SPELLLOG &log);
  static void ShowCombatFeedback(const MIRRORTIMERDAMAGE &log);
  static void ShowSpellMissFeedback(DWORDLONG victim, int reason);
  static void ShowHealingFeedback(const DWORDLONG &guid, int amount);
  static void ShowAutoFollowChange(DWORDLONG newTarget, DWORDLONG oldTarget, int type);

  static const DWORDLONG &GetCurrentObjectTrack() {
    return m_currentObjectTrack;
  }

  static const DWORDLONG &GetInteractTarget() {
    return m_interactTarget;
  }

  static const DWORDLONG &GetLockedTarget() {
    return m_lockedTarget;
  }

  static const DWORDLONG &GetLastEnemyTarget() {
    return m_lastEnemyTarget;
  }
  static void      NewZoneFeedback(int areaID, LPCSTR zoneString, LPCSTR subZoneString);
  static void      SetMinimapZoneText(LPCSTR areaName);
  static int       GetCurrentAreaID();
  static void      NamePlateClicked(DWORDLONG unit, MOUSEBUTTON button);
  static void      SetPartyLeader(DWORDLONG guid);
  static void      AddPartyMember(DWORDLONG guid, int connected);
  static void      RemoveAllPartyMembers();
  static void      EnablePartyMember(DWORDLONG guid, int enable);
  static BOOL      IsPartyMember(const DWORDLONG &guid);
  static DWORDLONG GetPartyMember(UINT index);
  static void      SetLootMethod(LOOT_METHOD method, DWORDLONG master);
  static void      UnitNameUpdate(const DWORDLONG &guid);
  static void      UnitPortraitUpdate(const DWORDLONG &guid);
  static void      UpdateActivePlayer();
  static BOOL          HasPlayerControl() {
    return m_hasControl;
  }
  static void OnClientControlChanged(BOOL hasControl);
  static void ClearClientControls();
  static void RegisterFrameFactories();

 private:
  static bool        m_reloadUI;
  static CSimpleTop *m_simpleTop;
  static char       *m_zoneText;
  static char       *m_subZoneText;
  static char       *m_minimapZoneText;

  static CSimpleFrame *m_UISimpleParent;
  static CGTooltip    *m_gameTooltip;
  static float         m_interactMaxDist;
  static DWORDLONG     m_interactTarget;
  static DWORDLONG     m_lockedTarget;
  static DWORDLONG     m_lastEnemyTarget;
  static DWORDLONG     m_currentObjectTrack;
  static UINT          m_stackSplit;
  static DWORDLONG     m_cursorItem;
  static DWORDLONG     m_cursorItemContainer;
  static UINT          m_cursorItemSlot;
  static UINT          m_cursorMoney;
  static int           m_cursorSpell;
  static UINT          m_cursorPetAction;
  static UINT          m_cursorVirtualID;
  static UINT          m_cursorVirtualDisplay;
  static UINT          m_cursorVirtualSlot;
  static int           m_cursorHasAction;
  static UICURSORTYPE  m_cursorItemType;
  static int           m_hasControl;
  static char          s_lastErrorString[512];
  static int           m_screenWidth;
  static CinematicData m_cinematic;
  static int           m_areaID;

 public:
  static void      StartCinematic(int cinematicID);
  static void      BeginCinematic();
  static void      BeginCinematicInternal(LPVOID);
  static BOOL      StartCinematicCamera();
  static BOOL      NextCinematic(LPVOID);
  static void      NextCinematicInternal(LPVOID);
  static BOOL      StopCinematic(LPVOID);
  static void      StopCinematicInternal(LPVOID);
  static void      ResetCamera();
  static void      PlayerCombatModeChanged(int newState);
  static void      HandleObjectTrackChange(DWORDLONG object, DWORDLONG oldGUID, float x, float y);
  static void      SetCursorItem(DWORDLONG itemGUID, DWORDLONG containerGUID, UINT slot, int unlock, UINT stackSplit);
  static DWORDLONG GetCursorItem();
  static void      GetCursorItem(DWORDLONG &cursorItem, DWORDLONG &containerGUID, UINT &slot);
  static void      SetCursorMoney(UINT money);
  static UINT      GetCursorMoney() {
    return m_cursorMoney;
  }
  static void SetCursorSpell(int spellId, int pet);
  static void DropCursorSpell();
  static int  GetCursorSpell();
  static BOOL IsCursorPetSpell();
  static void SetCursorPetAction(const PetAction &action);
  static UINT GetCursorPetAction();
  static void DropCursorPetAction();
  static void SetCursorVirtualItem(UINT itemID, UINT displayID, UINT slot, UICURSORTYPE type);
  static UINT GetCursorVirtualItem();
  static void GetCursorVirtualItem(UINT &cursorItem, UINT &slot);
  static UINT GetCursorVirtualItem(UICURSORTYPE type);
  static UINT GetCursorStackSplit() {
    return m_stackSplit;
  }
  static BOOL IsCursorEmpty();
  static void ClearCursor(int unlock);
  static void DeleteCursorItem();
  static UICURSORTYPE  GetCursorType() {
    return m_cursorItemType;
  }
  static void         LockItem(DWORDLONG itemGUID);
  static void         UnlockItem(DWORDLONG itemGUID);
  static void         UnlockAllItems();
  static void         OnTargetContextAction();
  static void         OnItemPush(DWORDLONG player, int slot, int itemID, int pushed, int display);
  static void __cdecl DisplayError(GAME_ERROR_TYPE errorType, ...);
  static LPCSTR       GetLastErrorString();

 private:
  static int  OnTerrainClick(const CTerrainClickEvent &evt);
  static BOOL OnSpriteLeftClick(DWORDLONG object, float x, float y);
  static BOOL OnSpriteRightClick(DWORDLONG object, float x, float y);
  static void UpdatePlayerAlpha(float alpha);

  static void UpdateObjectHighlightColor(HMODEL__ *model, CGObject_C *object);
  static void ResetStaticVars();

  static BOOL Idle(LPCVOID data, LPVOID param);
};

#endif
