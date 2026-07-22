#ifndef WOW_SOURCE_UI_GAMEUI_H
#define WOW_SOURCE_UI_GAMEUI_H

#include "Object/Object.h"

class CSimpleFrame;
class CSimpleTop;
class CGTooltip;
class CGCursor;
class CGObject_C;
class CinematicSequencesRec;
class CinematicCameraRec;
struct Sound;
struct CSpriteClickEvent;
struct CTerrainClickEvent;
struct CWorldClickEvent;
struct CObjectTrackEvent;
struct HMODEL__;
struct lua_State;
class CMouseEvent;
class CSizeEvent;
enum SYSMSG_TYPE;

struct CinematicData {
  CinematicSequencesRec *sequence;
  Sound                 *sequenceMusic;
  int                    currentCamera;
  CinematicCameraRec    *camera;
  void                  *cameraModel;
  int                    zoneMusicPaused;
};

enum GAME_ERROR_TYPE {
  GAME_ERROR_NONE = 0,
  GERR_NUM_TYPES = 297
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
  static void __fastcall             ClearClientControls();
  static void __fastcall             InitializeGame();
  static void __fastcall             Initialize();
  static void __fastcall             Shutdown();
  static void __fastcall             RegisterFrameFactories();
  static void __fastcall             Reload();
  static void __fastcall             ShutdownGame();
  static void __fastcall             UpdateActivePlayer();
  static void __fastcall             UnitNameUpdate(const unsigned __int64 &guid);
  static void __fastcall             Target(const unsigned __int64 &target, int usingNearest);
  static unsigned __int64 __fastcall ClosestObjectMatch(const char *match, OBJECT_TYPE type);
  static void __fastcall             TargetNearestEnemy(int reverse);
  static void __fastcall             AssistByName(const char *name);
  static void __fastcall             FollowByName(const char *name);
  static int __fastcall              IsPartyMember(const unsigned __int64 &guid);
  static unsigned __int64 __fastcall GetPartyMember(unsigned int index);
  static void __fastcall             ClearTarget(unsigned __int64 guid, int sendTarget);
  static void __fastcall             ClearInteractTarget(const unsigned __int64 &target);
  static void __fastcall             SetInteractTarget(const unsigned __int64 &target, float maxDist);
  static void __fastcall             CloseInteraction();
  static void __fastcall             ResetCamera();
  static void __fastcall             SysMsgDisplay(const char *msg, SYSMSG_TYPE severity);
  static int __fastcall              FilterMouseDown(const CMouseEvent &evt);
  static int __fastcall              HandleDisplaySizeChanged(const CSizeEvent &evt);
  static void __fastcall             ScaleUI(float scale, int force);
  static void __fastcall             EnterWorld();
  static void __fastcall             LeaveWorld();
  static void __fastcall             UpdateInteractTarget();
  static unsigned __int64 __fastcall GetCursorItem();
  static void __fastcall
  SetCursorItem(unsigned __int64 itemGUID, unsigned __int64 containerGUID, unsigned int slot, int unlock, unsigned int stackSplit);
  static unsigned int GetCursorMoney() {
    return m_cursorMoney;
  }
  static void __fastcall         SetCursorMoney(unsigned int money);
  static void __fastcall         SetCursorSpell(int spellId, int pet);
  static void __fastcall         DropCursorSpell();
  static int __fastcall          GetCursorSpell();
  static void __fastcall         GetCursorItem(unsigned __int64 &cursorItem, unsigned __int64 &containerGUID, unsigned int &slot);
  static unsigned int __fastcall GetCursorVirtualItem();
  static void __fastcall         SetCursorVirtualItem(unsigned int itemID, unsigned int displayID, unsigned int slot, UICURSORTYPE type);
  static unsigned int __fastcall GetCursorVirtualItem(UICURSORTYPE type);
  static void __fastcall         GetCursorVirtualItem(unsigned int &cursorItem, unsigned int &slot);
  static void __fastcall         UnlockItem(unsigned __int64 itemGUID);
  static void __fastcall         LockItem(unsigned __int64 itemGUID);
  static void __fastcall         CloseLoot(unsigned int send, unsigned int moving);
  static void __fastcall         NewZoneFeedback(int areaID, const char *zoneString, const char *subZoneString);
  static void __fastcall         SetMinimapZoneText(const char *areaName);
  static void __fastcall         ClearCursor(int unlock);
  static void __fastcall         PlayerCombatModeChanged(int newState);
  static void __fastcall         StartCinematic(int cinematicID);
  static int __fastcall          StopCinematic(void *__formal);
  static void __fastcall         HideCursor();
  static void __fastcall         ShowHealingFeedback(const unsigned __int64 &guid, int amount);
  static void __fastcall         ShowCombatFeedback(const unsigned __int64 &guid, int amount, int damageClass, unsigned int flags);
  static void __fastcall         OnItemPush(unsigned __int64 player, int slot, int itemID, int pushed, int display);
  static void __fastcall         OpenPartyInvite(const char *inviter);
  static void __fastcall         OpenResurrectRequest(const char *inviter);
  static void __fastcall         CancelPartyInvite();
  static void __fastcall         OpenGuildInvite(const char *inviter, const char *guildName);
  static void __fastcall         CancelGuildInvite();
  static void __fastcall         AddErrorMessage(const char *string, int error);
  static void __cdecl            DisplayError(GAME_ERROR_TYPE errorType, ...);
  static const char *__fastcall  GetLastErrorString();
  static void __fastcall         ShowAutoFollowChange(unsigned __int64 newTarget, unsigned __int64 oldTarget, int type);
  static int __fastcall          HandleSpriteClick(CSpriteClickEvent &evt);
  static int __fastcall          HandleTerrainClick(CTerrainClickEvent &evt);
  static int __fastcall          HandleWorldClick(CWorldClickEvent &evt);
  static void __fastcall         HandleSpriteTrack(CObjectTrackEvent &evt);
  static void __fastcall         HandleScreenshot(int success);

  static int __fastcall  OnTerrainClick(CTerrainClickEvent &evt);
  static int __fastcall  OnSpriteLeftClick(unsigned __int64 object, float x, float y);
  static int __fastcall  OnSpriteRightClick(unsigned __int64 object, float x, float y);
  static void __fastcall OnTargetContextAction();

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
  static void __fastcall HandleObjectTrackChange(unsigned __int64 object, unsigned __int64 oldGUID, float x, float y);
  static void __fastcall UpdateObjectHighlightColor(HMODEL__ *model, CGObject_C *object);
  friend class CGTooltip;
  friend class CGCursor;
  friend class CGMinimapFrame;
  friend int __fastcall Script_CursorHasItem(lua_State *L);
  friend int __fastcall Script_CursorHasSpell(lua_State *L);
  friend int __fastcall Script_CursorHasMoney(lua_State *L);
  friend int __fastcall Script_DeleteCursorItem(lua_State *__formal);
  friend int __fastcall Script_TargetLastEnemy(lua_State *__formal);

  static int __fastcall Idle(const void *data, void *param);

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
