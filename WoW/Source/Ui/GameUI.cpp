#include "GameUI.h"
#include "ActionBarFrame.h"
#include "ChatFrame.h"
#include "CharacterModelBase.h"
#include "ClassTrainerFrame.h"
#include "ItemTextFrame.h"
#include "LootFrame.h"
#include "MinimapFrame.h"
#include "PaperDollInfoFrame.h"
#include "PartyFrame.h"
#include "PetInfo.h"
#include "QuestLog.h"
#include "ReputationInfo.h"
#include "SpellBookFrame.h"
#include "TabardModelFrame.h"
#include "TaxiMapFrame.h"
#include "Tutorial.h"
#include "WorldFrame.h"
#include "UIBindings.h"

#include "Client.h"
#include "Game/GameTime.h"
#include "Object/ObjectClient/Unit_C.h"
#include "Object/ObjectClient/GameObject_C.h"
#include "Object/ObjectClient/Item_C.h"
#include "Object/ObjectClient/Player_C.h"
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
#include <Frame/CSimpleTop.h>
#include <FrameXML/FrameXML.h>
#include <FrameXML/LoadXML.h>
#include <Gx/Gx.h>
#include <lauxlib.h>
#include <lua.h>
#include <Os/W32/OsFile.h>
#include <Os/W32/OsSound.h>
#include <Scrn/Scrn.h>
#include <Services/SysMessage.h>
#include <UIUtil/Tooltip.h>
#include <UIUtil/InputControl.h>
#include <storm.h>

extern FrameScript_Method s_GameUIScriptFunctions[126];
extern char **__fastcall Script_GetNamesFromGUID(const unsigned __int64 &guid, int &numnames);
#include <ctype.h>
#include <float.h>
#include <stdlib.h>

void __fastcall             PortraitInitialize();
void __fastcall             PortraitShutdown();
void __fastcall             Trade_C_CancelTrade();
void __fastcall             Trade_C_BeginTrade();
CGUnit_C *__fastcall        Script_GetUnitFromName(const char *name);
unsigned __int64 __fastcall Script_GetGUIDFromName(const char *name);

extern const char *g_scriptEvents[0x177];

void __fastcall InputControlRegisterScriptFunctions();
void __fastcall InputControlUnregisterScriptFunctions();
void __fastcall UIBindingsRegisterScriptFunctions();
void __fastcall UIBindingsUnegisterScriptFunctions();
void __fastcall CameraRegisterScriptFunctions();
void __fastcall CameraUnregisterScriptFunctions();
void __fastcall SpellRegisterScriptFunctions();
void __fastcall SpellUnregisterScriptFunctions();
void __fastcall ScriptEventsRegisterFunctions();
void __fastcall ScriptEventsUnregisterFunctions();
void __fastcall ActionBarRegisterScriptFunctions();
void __fastcall ActionBarUnregisterScriptFunctions();
void __fastcall BuffBarRegisterScriptFunctions();
void __fastcall BuffBarUnregisterScriptFunctions();
void __fastcall PartyInfoRegisterScriptFunctions();
void __fastcall PartyInfoUnregisterScriptFunctions();
void __fastcall ChatRegisterScriptFunctions();
void __fastcall ChatUnregisterScriptFunctions();
void __fastcall SpellBookRegisterScriptFunctions();
void __fastcall SpellBookUnregisterScriptFunctions();
void __fastcall CharacterInfoRegisterScriptFunctions();
void __fastcall CharacterInfoUnregisterScriptFunctions();
void __fastcall LootInfoRegisterScriptFunctions();
void __fastcall LootInfoUnregisterScriptFunctions();
void __fastcall ItemTextRegisterScriptFunctions();
void __fastcall ItemTextUnregisterScriptFunctions();
void __fastcall QuestInfoRegisterScriptFunctions();
void __fastcall QuestInfoUnregisterScriptFunctions();
void __fastcall QuestLogRegisterScriptFunctions();
void __fastcall QuestLogUnregisterScriptFunctions();
void __fastcall ClassTrainerRegisterScriptFunctions();
void __fastcall ClassTrainerUnregisterScriptFunctions();
void __fastcall CraftInfoRegisterScriptFunctions();
void __fastcall CraftInfoUnregisterScriptFunctions();
void __fastcall MerchantRegisterScriptFunctions();
void __fastcall MerchantUnregisterScriptFunctions();
void __fastcall TradeInfoRegisterScriptFunctions();
void __fastcall TradeInfoUnregisterScriptFunctions();
void __fastcall ContainerRegisterScriptFunctions();
void __fastcall ContainerUnregisterScriptFunctions();
void __fastcall BankRegisterScriptFunctions();
void __fastcall BankUnregisterScriptFunctions();
void __fastcall PetInfoRegisterScriptFunctions();
void __fastcall PetInfoUnregisterScriptFunctions();
void __fastcall TradeSkillRegisterScriptFunctions();
void __fastcall TradeSkillUnregisterScriptFunctions();
void __fastcall WorldMapRegisterScriptFunctions();
void __fastcall WorldMapUnregisterScriptFunctions();
void __fastcall ReputationInfoRegisterScriptFunctions();
void __fastcall ReputationInfoUnregisterScriptFunctions();
void __fastcall SndInterfaceRegisterVocalScriptFunctions();
void __fastcall SndInterfaceUnregisterVocalScriptFunctions();
void __fastcall TabardCreationRegisterScriptFunctions();
void __fastcall TabardCreationUnregisterScriptFunctions();
void __fastcall GuildRegistrarRegisterScriptFunctions();
void __fastcall GuildRegistrarUnregisterScriptFunctions();
void __fastcall DuelInfoRegisterScriptFunctions();
void __fastcall DuelInfoUnregisterScriptFunctions();
void __fastcall TutorialRegisterScriptFunctions();
void __fastcall TutorialUnregisterScriptFunctions();
void __fastcall PetitionInfoRegisterScriptFunctions();
void __fastcall PetitionInfoUnregisterScriptFunctions();

static void __fastcall LoadScriptFunctions();
static void __fastcall UnloadScriptFunctions();
static int __fastcall  CCommand_Script(const char *command, const char *arguments);
static int __fastcall  CCommand_ScaleUI(const char *__formal, const char *arguments);
static void __fastcall LoadPlacedFrames();
static int __fastcall  SavePlacedFrames(CSimpleTop *top);
static int __fastcall  PlacedFrameCallback(CSimpleFrame *frame, void *param);
static void __fastcall PlaceFrame(CSimpleFrame *frame, int framelevel, int x, int y, int w, int h);

CVar *s_minimapZoomCVar;
CVar *s_minimapInsideZoomCVar;
CVar *s_statusBarCVar;
CVar *s_assistAttackCVar;
CVar *s_combatLogCVar;

class CGWorldMap {
 public:
  static void __fastcall InitializeGame();
  static void __fastcall ShutdownGame();
  static void __fastcall EnterWorld();
  static void __fastcall LeaveWorld();
  static void __fastcall SetMapToCurrentZone();
};

class CWorld {
 public:
  static float __fastcall       GetFramerate();
  static void __fastcall        GetCounts(int *const counts);
  static const char *__fastcall QueryChunkName();
};

class CGBuffBar {
 public:
  static void __fastcall InitializeGame();
  static void __fastcall ShutdownGame();
  static void __fastcall EnterWorld();
  static void __fastcall LeaveWorld();
};

class CGQuestInfo {
 public:
  static void __fastcall                    EnterWorld();
  static void __fastcall                    LeaveWorld();
  static void __fastcall                    QuestGiverFinished();
  static const unsigned __int64 &__fastcall GetQuestGiver();
};

class CGContainerInfo {
 public:
  static void __fastcall EnterWorld();
  static void __fastcall LeaveWorld();
};

class CGMerchantInfo {
 public:
  static void __fastcall             EnterWorld();
  static void __fastcall             LeaveWorld();
  static void __fastcall             CloseMerchant();
  static unsigned __int64 __fastcall GetMerchant();
};

class CGTradeInfo {
 public:
  static void __fastcall             EnterWorld();
  static void __fastcall             LeaveWorld();
  static void __fastcall             SetTradePartner(unsigned __int64 partner);
  static unsigned __int64 __fastcall GetTradePartner();
};

class CGBankInfo {
 public:
  static void __fastcall  EnterWorld();
  static void __fastcall  LeaveWorld();
  static void __fastcall  CloseBank();
  static unsigned __int64 m_unit;
};

class CGTradeSkillInfo {
 public:
  static void __fastcall EnterWorld();
  static void __fastcall LeaveWorld();
  static void __fastcall ShutdownGame();
};

class CGCraftInfo {
 public:
  static void __fastcall EnterWorld();
  static void __fastcall ShutdownGame();
};

class CGDuelInfo {
 public:
  static void __fastcall InitializeGame();
  static void __fastcall ShutdownGame();
};

class CGTabardCreationFrame {
 public:
  static void __fastcall             EnterWorld();
  static void __fastcall             LeaveWorld();
  static void __fastcall             Close();
  static unsigned __int64 __fastcall GetVendor();
};

class CGGuildRegistrar {
 public:
  static void __fastcall             EnterWorld();
  static void __fastcall             LeaveWorld();
  static void __fastcall             CloseRegistrar();
  static unsigned __int64 __fastcall GetRegistrar();
};

class CGPetitionInfo {
 public:
  static void __fastcall EnterWorld();
  static void __fastcall LeaveWorld();
};

enum SPELL_FAILED_REASON {
  SPELL_FAILED_ERROR = 14
};

void __fastcall         Spell_C_CancelSpell(unsigned int failed, unsigned int notifyServer, SPELL_FAILED_REASON reason);
bool __fastcall         Spell_C_CastSpell(int spellID, const CGItem_C *item);
unsigned int __fastcall CurrencyTotal(int coins[3]);
int __fastcall          CursorGrabMoney(unsigned int amount);
int __fastcall          CursorGrabSpell(const char *filename);
unsigned int __fastcall CursorGetCursorMode();
void __fastcall         CursorSetHeldItem(unsigned __int64 itemGuid);
void __fastcall         CursorSetHeldVirtualItem(unsigned int displayID);
void __fastcall         CursorSetCursorMode(CURSORANIMATIONS mode);

enum ERROR_TEXT_PLACEMENT {
  ERRORTEXT_CHAT = 0,
  ERRORTEXT_UIINFO = 1,
  ERRORTEXT_UIERROR = 2,
  ERRORTEXT_CONSOLE = 3
};

struct GAMEERRORDESC {
  GAMEERRORDESC(
      const char          *_stringToken,
      ERROR_TEXT_PLACEMENT _textPlacement,
      const char          *_soundName,
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

  const char          *stringToken;
  ERROR_TEXT_PLACEMENT textPlacement;
  const char          *soundName;
  VOCALUISOUNDS        voiceID;
  int                  supressText;
  SLASH_COMMAND_ID     slashCmd;
};

static GAMEERRORDESC s_gameErrors[GERR_NUM_TYPES] = {
    GAMEERRORDESC(
        "ERR_INV_FULL",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "GAMEERRORUNABLETOEQUIP",
        static_cast<VOCALUISOUNDS>(0),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_CANT_EQUIP_LEVEL_I",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "GAMEERRORUNABLETOEQUIP",
        static_cast<VOCALUISOUNDS>(2),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_CANT_EQUIP_SKILL",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "GAMEERRORUNABLETOEQUIP",
        static_cast<VOCALUISOUNDS>(2),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_CANT_EQUIP_EVER",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "GAMEERRORUNABLETOEQUIP",
        static_cast<VOCALUISOUNDS>(3),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_PROFICIENCY_NEEDED",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "GAMEERRORUNABLETOEQUIP",
        static_cast<VOCALUISOUNDS>(48),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_WRONG_SLOT",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "GAMEERRORUNABLETOEQUIP",
        static_cast<VOCALUISOUNDS>(27),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_BAG_FULL",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "GAMEERRORUNABLETOEQUIP",
        static_cast<VOCALUISOUNDS>(29),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_DESTROY_NONEMPTY_BAG",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "GAMEERRORUNABLETOEQUIP",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_BAG_IN_BAG",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "GAMEERRORUNABLETOEQUIP",
        static_cast<VOCALUISOUNDS>(26),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_AMMO_ONLY",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "GAMEERRORUNABLETOEQUIP",
        static_cast<VOCALUISOUNDS>(28),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_NO_SLOT_AVAILABLE",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "GAMEERRORUNABLETOEQUIP",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_WRONG_BAG_TYPE",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "GAMEERRORUNABLETOEQUIP",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_ITEM_MAX_COUNT",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "GAMEERRORUNABLETOEQUIP",
        static_cast<VOCALUISOUNDS>(30),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_NOT_EQUIPPABLE",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "GAMEERRORUNABLETOEQUIP",
        static_cast<VOCALUISOUNDS>(44),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_CANT_STACK",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "GAMEERRORUNABLETOEQUIP",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_CANT_SWAP",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "GAMEERRORUNABLETOEQUIP",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_SLOT_EMPTY",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "GAMEERRORUNABLETOEQUIP",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_ITEM_NOT_FOUND",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "GAMEERRORUNABLETOEQUIP",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_TOO_FEW_TO_SPLIT",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "GAMEERRORUNABLETOEQUIP",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_SPLIT_FAILED",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "GAMEERRORUNABLETOEQUIP",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_NOT_ENOUGH_GOLD",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "GAMEERRORUNABLETOEQUIP",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_NOT_A_BAG",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "GAMEERRORUNABLETOEQUIP",
        static_cast<VOCALUISOUNDS>(25),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_NOT_OWNER",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "GAMEERRORUNABLETOEQUIP",
        static_cast<VOCALUISOUNDS>(60),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_ONLY_ONE_QUIVER",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "GAMEERRORUNABLETOEQUIP",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_NO_BANK_SLOT",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "GAMEERRORUNABLETOEQUIP",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_NO_BANK_HERE",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "GAMEERRORUNABLETOEQUIP",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_ITEM_LOCKED",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "GAMEERRORUNABLETOEQUIP",
        static_cast<VOCALUISOUNDS>(61),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_2HANDED_EQUIPPED",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "GAMEERRORUNABLETOEQUIP",
        static_cast<VOCALUISOUNDS>(42),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_VENDOR_NOT_INTERESTED",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "GAMEERRORUNABLETOEQUIP",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_VENDOR_HATES_YOU",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "GAMEERRORUNABLETOEQUIP",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_VENDOR_SOLD_OUT",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "GAMEERRORUNABLETOEQUIP",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_VENDOR_TOO_FAR",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "GAMEERRORUNABLETOEQUIP",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_NOT_ENOUGH_MONEY",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "GAMEERRORUNABLETOEQUIP",
        static_cast<VOCALUISOUNDS>(40),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_RECEIVE_ITEM_S",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "ITEMGENERICSOUND",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_DROP_BOUND_ITEM",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(4),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_TRADE_BOUND_ITEM",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(59),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_TRADE_QUEST_ITEM",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(59),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_TRADE_GROUND_ITEM",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC("ERR_TRADE_BAG", static_cast<ERROR_TEXT_PLACEMENT>(2), "NONE", static_cast<VOCALUISOUNDS>(59), 1, static_cast<SLASH_COMMAND_ID>(9)),
    GAMEERRORDESC(
        "ERR_SPELL_FAILED_S",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_ITEM_COOLDOWN",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(5),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_POTION_COOLDOWN",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(6),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_FOOD_COOLDOWN",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(7),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_SPELL_COOLDOWN",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(12),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_ABILITY_COOLDOWN",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(50),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_SPELL_ALREADY_KNOWN_S",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(13),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_SKILL_GAINED_S",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(23)
    ),
    GAMEERRORDESC(
        "ERR_SKILL_UP_SI",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(23)
    ),
    GAMEERRORDESC(
        "ERR_LEARN_SPELL_S",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_LEARN_ABILITY_S",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_LEARN_RECIPE_S",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_INVITE_PLAYER_S",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_INVITED_TO_GROUP_S",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_ALREADY_IN_GROUP_S",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(8),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_PLAYER_BUSY_S",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_NEW_LEADER_S",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_NEW_LEADER_YOU",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_LEFT_GROUP_S",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_LEFT_GROUP_YOU",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_GROUP_DISBANDED",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_DECLINE_GROUP_S",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "igPlayerInviteDecline",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_JOINED_GROUP_S",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "igPlayerInviteAccept",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_UNINVITE_YOU",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_BAD_PLAYER_NAME_S",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_NOT_IN_GROUP",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_TARGET_NOT_IN_GROUP_S",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_GROUP_FULL",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_NOT_LEADER",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_PLAYER_DIED_S",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_GUILD_CREATE_S",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "LEVELUP",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_GUILD_INVITE_S",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_INVITED_TO_GUILD_SS",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "LEVELUP",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_ALREADY_IN_GUILD_S",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_ALREADY_INVITED_TO_GUILD_S",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_INVITED_TO_GUILD",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_ALREADY_IN_GUILD",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_GUILD_ACCEPT",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_GUILD_DECLINE_S",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_GUILD_PERMISSIONS",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(62),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_GUILD_JOIN_S",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_GUILD_FOUNDER_S",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_GUILD_PROMOTE_SS",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_GUILD_DEMOTE_SS",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_GUILD_QUIT_S",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_GUILD_LEAVE_S",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_GUILD_REMOVE_SS",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_GUILD_REMOVE_SELF",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_GUILD_DISBAND_S",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_GUILD_DISBAND_SELF",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_GUILD_LEADER_S",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_GUILD_LEADER_SELF",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_GUILD_MOTD_S",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_GUILD_PLAYER_NOT_FOUND_S",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_GUILD_PLAYER_NOT_IN_GUILD_S",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_GUILD_PLAYER_NOT_IN_GUILD",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_GUILD_CANT_PROMOTE_S",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_GUILD_CANT_DEMOTE_S",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_GUILD_NOT_IN_A_GUILD",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_GUILD_INTERNAL",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_GUILD_LEADER_IS_S",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_GUILD_LEADER_CHANGED_SS",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_GUILD_DISBANDED",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_GUILD_NOT_ALLIED",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_GUILD_LEADER_LEAVE",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_GUILD_NAME_INVALID",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_GUILD_NAME_EXISTS_S",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_GUILD_ENTER_NAME",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_GUILD_NAME_TOO_SHORT",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_GUILD_NAME_MIXED_LANGUAGES",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_GUILD_NAME_PROFANE",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_GUILD_NAME_RESERVED",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_NO_GUILD_CHARTER",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_OUT_OF_RANGE",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(10),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_PLAYER_DEAD",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_CLIENT_LOCKED_OUT",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_KILLED_BY_S",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_LOOT_LOCKED",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(33),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_LOOT_TOO_FAR",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "GAMEERRORINVALIDTARGET",
        static_cast<VOCALUISOUNDS>(35),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_LOOT_DIDNT_KILL",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "GAMEERRORINVALIDTARGET",
        static_cast<VOCALUISOUNDS>(31),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_LOOT_BAD_FACING",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "GAMEERRORINVALIDTARGET",
        static_cast<VOCALUISOUNDS>(32),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_LOOT_NOTSTANDING",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(34),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_LOOT_STUNNED",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_LOOT_NO_UI",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_QUEST_ACCEPTED_S",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "QUESTADDED",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_QUEST_COMPLETE_S",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "igQuestListComplete",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_QUEST_FAILED_S",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "igQuestFailed",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_QUEST_FAILED_BAG_FULL_S",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "igQuestFailed",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_QUEST_FAILED_MAX_COUNT_S",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "igQuestFailed",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_QUEST_FAILED_LOW_LEVEL",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "igQuestFailed",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_QUEST_FAILED_MISSING_ITEMS",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "igQuestFailed",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_QUEST_REWARD_EXP_I",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_QUEST_REWARD_ITEM_S",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_QUEST_REWARD_MONEY_S",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_QUEST_MUST_CHOOSE",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "igQuestFailed",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_QUEST_LOG_FULL",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "igQuestFailed",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_COMBAT_DAMAGE_SSI",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC("ERR_INSPECT_S", static_cast<ERROR_TEXT_PLACEMENT>(0), "NONE", static_cast<VOCALUISOUNDS>(66), 1, static_cast<SLASH_COMMAND_ID>(9)),
    GAMEERRORDESC(
        "ERR_CANT_USE_ITEM",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(51),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_MUST_EQUIP_ITEM",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(49),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_PASSIVE_ABILITY",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_2HSKILLNOTFOUND",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(41),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_NO_ATTACK_TARGET",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "GAMEERROROUTOFRANGE",
        static_cast<VOCALUISOUNDS>(38),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_INVALID_ATTACK_TARGET",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(11),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_ATTACK_PACIFIED",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_ATTACK_DEAD",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_HUNGER_VERY_LOW",
        static_cast<ERROR_TEXT_PLACEMENT>(1),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_HUNGER_LOW",
        static_cast<ERROR_TEXT_PLACEMENT>(1),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_HUNGER_MED",
        static_cast<ERROR_TEXT_PLACEMENT>(1),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_HUNGER_HIGH",
        static_cast<ERROR_TEXT_PLACEMENT>(1),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_HUNGER_SATIATED",
        static_cast<ERROR_TEXT_PLACEMENT>(1),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_THIRST_VERY_LOW",
        static_cast<ERROR_TEXT_PLACEMENT>(1),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_THIRST_LOW",
        static_cast<ERROR_TEXT_PLACEMENT>(1),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_THIRST_MED",
        static_cast<ERROR_TEXT_PLACEMENT>(1),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_THIRST_HIGH",
        static_cast<ERROR_TEXT_PLACEMENT>(1),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_THIRST_SATIATED",
        static_cast<ERROR_TEXT_PLACEMENT>(1),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_TAXISAMENODE",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_TAXINOSUCHPATH",
        static_cast<ERROR_TEXT_PLACEMENT>(1),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_TAXIUNSPECIFIEDSERVERERROR",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_TAXINOTENOUGHMONEY",
        static_cast<ERROR_TEXT_PLACEMENT>(1),
        "NONE",
        static_cast<VOCALUISOUNDS>(54),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_TAXITOOFARAWAY",
        static_cast<ERROR_TEXT_PLACEMENT>(1),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_TAXINOVENDORNEARBY",
        static_cast<ERROR_TEXT_PLACEMENT>(1),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_TAXINOTVISITED",
        static_cast<ERROR_TEXT_PLACEMENT>(1),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_TAXIPLAYERBUSY",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_TAXIPLAYERALREADYMOUNTED",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_TAXIPLAYERSHAPESHIFTED",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_TAXIPLAYERMOVING",
        static_cast<ERROR_TEXT_PLACEMENT>(1),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_TAXINOPATHS",
        static_cast<ERROR_TEXT_PLACEMENT>(1),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_NO_REPLY_TARGET",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_GENERIC_NO_TARGET",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "GAMEERRORINVALIDTARGET",
        static_cast<VOCALUISOUNDS>(45),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_INITIATE_TRADE_S",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_TRADE_REQUEST_S",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "LEVELUP",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_TRADE_TOO_FAR",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_TRADE_CANCELLED",
        static_cast<ERROR_TEXT_PLACEMENT>(1),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_TRADE_COMPLETE",
        static_cast<ERROR_TEXT_PLACEMENT>(1),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_TRADE_BAG_FULL",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_TRADE_TARGET_BAG_FULL",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_TRADE_MAX_COUNT_EXCEEDED",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_TRADE_TARGET_MAX_COUNT_EXCEEDED",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_MOUNT_INVALIDMOUNTEE",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_MOUNT_TOOFARAWAY",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_MOUNT_ALREADYMOUNTED",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_MOUNT_NOTMOUNTABLE",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_MOUNT_NOTYOURPET",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_MOUNT_OTHER",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_MOUNT_LOOTING",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_MOUNT_RACECANTMOUNT",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_MOUNT_SHAPESHIFTED",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_DISMOUNT_NOPET",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_DISMOUNT_NOTMOUNTED",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_DISMOUNT_NOTYOURPET",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_SPELL_FAILED_TOTEMS",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_SPELL_FAILED_REAGENTS",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_SPELL_FAILED_EQUIPPED_ITEM",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_SPELL_FAILED_EQUIPPED_ITEM_CLASS_S",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_SPELL_FAILED_SHAPESHIFT_FORM_S",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_BADATTACKFACING",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_BADATTACKPOS",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_CHEST_IN_USE",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(52),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_USE_CANT_OPEN",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_USE_LOCKED",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(61),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_USE_LOCKED_WITH_ITEM_S",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_USE_LOCKED_WITH_SPELL_S",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_USE_LOCKED_WITH_SPELL_KNOWN_SI",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_USE_TOO_FAR",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(57),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_USE_BAD_ANGLE",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_USE_OBJECT_MOVING",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_USE_SPELL_FOCUS",
        static_cast<ERROR_TEXT_PLACEMENT>(1),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_USE_DESTROYED",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_CANTATTACK_NOTSTANDING",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(37),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_SET_LOOT_FREEFORALL",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_SET_LOOT_ROUNDROBIN",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_SET_LOOT_MASTER",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_NEW_LOOT_MASTER_S",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_SPECIFY_MASTER_LOOTER",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_TAME_FAILED",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_CHAT_WHILE_DEAD",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_NEWTAXIPATH",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "TaxiNodeDiscovered",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC("ERR_NO_PET", static_cast<ERROR_TEXT_PLACEMENT>(2), "NONE", static_cast<VOCALUISOUNDS>(66), 0, static_cast<SLASH_COMMAND_ID>(9)),
    GAMEERRORDESC(
        "ERR_NOTYOURPET",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_PET_NOT_RENAMEABLE",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_NULL_PETNAME",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_INVALID_PETNAME",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_QUEST_OBJECTIVE_COMPLETE_S",
        static_cast<ERROR_TEXT_PLACEMENT>(1),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_QUEST_UNKNOWN_COMPLETE",
        static_cast<ERROR_TEXT_PLACEMENT>(1),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_QUEST_ADD_KILL_SII",
        static_cast<ERROR_TEXT_PLACEMENT>(1),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_QUEST_ADD_FOUND_SII",
        static_cast<ERROR_TEXT_PLACEMENT>(1),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_QUEST_ADD_ITEM_SII",
        static_cast<ERROR_TEXT_PLACEMENT>(1),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_CANNOTCREATEDIRECTORY",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_CANNOTCREATEFILE",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_PLAYER_WRONG_FACTION",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "GAMEERRORINVALIDTARGET",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_BANKSLOT_FAILED_TOO_MANY",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_BANKSLOT_INSUFFICIENT_FUNDS",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(22),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_BANKSLOT_NOTBANKER",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_FRIEND_DB_ERROR",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_FRIEND_LIST_FULL",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_FRIEND_ADDED_S",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_FRIEND_ONLINE_S",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "FRIENDJOINGAME",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_FRIEND_OFFLINE_S",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_FRIEND_NOT_FOUND",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_FRIEND_WRONG_FACTION",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_FRIEND_REMOVED_S",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_FRIEND_ERROR",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_FRIEND_ALREADY_S",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_FRIEND_SELF",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_IGNORE_FULL",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_IGNORE_SELF",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_IGNORE_NOT_FOUND",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_IGNORE_ALREADY_S",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_IGNORE_ADDED_S",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_IGNORE_REMOVED_S",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_ONLY_ONE_BOLT",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "GAMEERRORUNABLETOEQUIP",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_ONLY_ONE_AMMO",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "GAMEERRORUNABLETOEQUIP",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_SPELL_FAILED_EQUIPPED_SPECIFIC_ITEM",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_WRONG_BAG_TYPE_SUBCLASS",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_CANT_WRAP_STACKABLE",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_CANT_WRAP_EQUIPPED",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_CANT_WRAP_WRAPPED",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_CANT_WRAP_BOUND",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_CANT_WRAP_UNIQUE",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_CANT_WRAP_BAGS",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_OUT_OF_MANA",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(15),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_OUT_OF_RAGE",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_OUT_OF_FOCUS",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_OUT_OF_ENERGY",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_OUT_OF_HEALTH",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC("ERR_LOOT_GONE", static_cast<ERROR_TEXT_PLACEMENT>(2), "NONE", static_cast<VOCALUISOUNDS>(66), 1, static_cast<SLASH_COMMAND_ID>(9)),
    GAMEERRORDESC(
        "ERR_MOUNT_FORCEDDISMOUNT",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_AUTOFOLLOW_TOO_FAR",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_UNIT_NOT_FOUND",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_INVALID_FOLLOW_TARGET",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_GUILDEMBLEM_SUCCESS",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_GUILDEMBLEM_INVALID_TABARD_COLORS",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_GUILDEMBLEM_NOGUILD",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_GUILDEMBLEM_COLORSPRESENT",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_GUILDEMBLEM_NOTGUILDMASTER",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_GUILDEMBLEM_NOTENOUGHMONEY",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_GUILDEMBLEM_INVALIDVENDOR",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_SPELL_OUT_OF_RANGE",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "GAMEERROROUTOFRANGE",
        static_cast<VOCALUISOUNDS>(46),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_COMMAND_NEEDS_TARGET",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC("ERR_NOAMMO_S", static_cast<ERROR_TEXT_PLACEMENT>(2), "NONE", static_cast<VOCALUISOUNDS>(1), 1, static_cast<SLASH_COMMAND_ID>(9)),
    GAMEERRORDESC(
        "ERR_TOOBUSYTOFOLLOW",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        1,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_DUEL_REQUESTED",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "LEVELUP",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_DUEL_CANCELLED",
        static_cast<ERROR_TEXT_PLACEMENT>(1),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_DEATHBINDALREADYBOUND",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_NOEMOTEWHILERUNNING",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_ZONE_EXPLORED",
        static_cast<ERROR_TEXT_PLACEMENT>(1),
        "TaxiNodeDiscovered",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_ZONE_EXPLORED_XP",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_INVALID_ITEM_TARGET",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "GAMEERRORINVALIDTARGET",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_IGNORING_YOU_S",
        static_cast<ERROR_TEXT_PLACEMENT>(0),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_FISH_NOT_HOOKED",
        static_cast<ERROR_TEXT_PLACEMENT>(1),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_FISH_ESCAPED",
        static_cast<ERROR_TEXT_PLACEMENT>(1),
        "GAMEERRORINVALIDTARGET",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_SPELL_FAILED_NOTUNSHEATHED",
        static_cast<ERROR_TEXT_PLACEMENT>(1),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_PETITION_SIGNED",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_PETITION_ALREADY_SIGNED",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_PETITION_IN_GUILD",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_PETITION_CREATOR",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    ),
    GAMEERRORDESC(
        "ERR_PETITION_NOT_ENOUGH_SIGNATURES",
        static_cast<ERROR_TEXT_PLACEMENT>(2),
        "NONE",
        static_cast<VOCALUISOUNDS>(66),
        0,
        static_cast<SLASH_COMMAND_ID>(9)
    )
};

bool             CGGameUI::m_reloadUI;
unsigned int     CGGameUI::m_stackSplit;
unsigned __int64 CGGameUI::m_cursorItem;
unsigned __int64 CGGameUI::m_cursorItemContainer;
unsigned int     CGGameUI::m_cursorItemSlot;
unsigned int     CGGameUI::m_cursorMoney;
int              CGGameUI::m_cursorSpell;
unsigned int     CGGameUI::m_cursorPetAction;
unsigned int     CGGameUI::m_cursorVirtualID;
unsigned int     CGGameUI::m_cursorVirtualDisplay;
unsigned int     CGGameUI::m_cursorVirtualSlot;
int              CGGameUI::m_cursorHasAction;
UICURSORTYPE     CGGameUI::m_cursorItemType;
unsigned __int64 CGGameUI::m_currentObjectTrack;
float            CGGameUI::m_interactMaxDist;
unsigned __int64 CGGameUI::m_interactTarget;
unsigned __int64 CGGameUI::m_lockedTarget;
unsigned __int64 CGGameUI::m_lastEnemyTarget;
char            *CGGameUI::m_zoneText;
char            *CGGameUI::m_subZoneText;
char            *CGGameUI::m_minimapZoneText;
int              CGGameUI::m_areaID;
CSimpleFrame    *CGGameUI::m_UISimpleParent;
CSimpleTop      *CGGameUI::m_simpleTop;
CGTooltip       *CGGameUI::m_gameTooltip;
int              CGGameUI::m_screenWidth;
int              CGGameUI::m_hasControl;
CinematicData    CGGameUI::m_cinematic;
char             CGGameUI::s_lastErrorString[512];

static unsigned int s_nearestIndex;
struct NearestEnemyData {
  unsigned __int64 guid;
  float            distSq;
};
static TSGrowableArray<NearestEnemyData> s_nearestList;
static unsigned int                      s_nearestListTime;
static unsigned int                      s_sameTargetTime;
static const char                       *compasDirStr[8] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
static const float                       s_distCullValues[3] = {350.0f, 550.0f, 750.0f};
static const float                       s_smallCullValues[3] = {0.07f, 0.04f, 0.01f};
static const float                       s_interactDistances[3] = {5.5555553f, 11.111111f, 10.0f};
static const char                       *s_screenResolutions[4] = {"800x600", "1024x768", "1280x1024", "1600x1200"};

struct ItemPushInfo {
  unsigned __int64 player;
  int              slot;
  int              pushed;
  int              display;
};

bool __fastcall         Spell_C_IsTargeting();
bool __fastcall         Spell_C_HandleSpriteClick(CGObject_C *object);
unsigned int __fastcall Spell_C_HandleTerrainClick(CTerrainClickEvent &evt);
void __fastcall         Trade_C_InitiateTrade(unsigned __int64 target, int useCursorItem);

static int __fastcall CCommand_Script(const char *__formal, const char *arguments) {
  FrameScript_Execute(arguments, arguments);
  return 1;
}

void __fastcall EnableFadingScreen(float fadeTime, void(__fastcall *fadedCallback)(void *), void *param);

static int __fastcall CCommand_ScaleUI(const char *__formal, const char *arguments) {
  float scale = SStrToFloat(arguments);
  if (scale > 0.0f) {
    CGGameUI::ScaleUI(scale, 0);
  }
  return 1;
}

static int __fastcall Script_FrameXML_Debug(lua_State *L) {
  int level = FrameXML_GetDebugLevel();
  if (lua_isnumber(L, 1)) {
    level = static_cast<int>(lua_tonumber(L, 1));
    FrameXML_SetDebugLevel(level);
  }
  return 1;
}

static int __fastcall Script_ReloadUI(lua_State *L) {
  CGGameUI::Reload();
  return 0;
}

static int __fastcall Script_SetLayoutMode(lua_State *L) {
  int mode = 1;
  if (lua_isnumber(L, 1)) {
    mode = static_cast<int>(lua_tonumber(L, 1));
  }
  CSimpleTop::GetInstance()->SetLayoutMode(mode);
  return 0;
}

static int __fastcall Script_IsShiftKeyDown(lua_State *L) {
  if (EventIsKeyDown(KEY_SHIFT)) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int __fastcall Script_IsControlKeyDown(lua_State *L) {
  if (EventIsKeyDown(KEY_CONTROL)) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int __fastcall Script_IsAltKeyDown(lua_State *L) {
  if (EventIsKeyDown(KEY_ALT)) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int __fastcall Script_GetDebugStats(lua_State *L) {
  char               buffer[1024];
  int                counts[32];
  char               tempBuffer[128];
  NTempest::C3Vector pos;
  float              dayProgression;

  SStrPrintf(buffer, sizeof(buffer), "%02d:%02d", g_clientGameTime.m_minute, g_clientGameTime.m_hour);

  CGObject_C *player = ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__);
  if (player) {
    SStrPack(buffer, "\nPlayer position: ", sizeof(buffer));
    player->GetPosition(pos);
    SStrPrintf(tempBuffer, sizeof(tempBuffer), "%d, %d, %d\n", static_cast<int>(pos.x), static_cast<int>(pos.y), static_cast<int>(pos.z));
    SStrPack(buffer, tempBuffer, sizeof(buffer));
    SStrPack(buffer, "Player facing: ", sizeof(buffer));

    double facing = fmod(360.0 - player->GetFacing() * 57.29578 + 22.5, 360.0);
    if (facing < 0.0) {
      facing += 360.0;
    }
    int direction = static_cast<int>(facing * 0.022222223);
    if (static_cast<unsigned int>(direction) > 7) {
      direction = direction < 0 ? 0 : 7;
    }
    SStrPrintf(tempBuffer, sizeof(tempBuffer), "%-2s\n", compasDirStr[direction]);
    SStrPack(buffer, tempBuffer, sizeof(buffer));
  }

  const char *chunkName = CWorld::QueryChunkName();
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

static int __fastcall Script_GetCVar(lua_State *L) {
  char message[512];
  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: GetCVar(\"cvar\")");
  }

  CVar *cvar = CVar::Lookup(lua_tostring(L, 1));
  if (!cvar) {
    SStrPrintf(message, sizeof(message), "Couldn't find CVar named '%s'", lua_tostring(L, 1));
    luaL_error(L, message);
  }

  lua_pushstring(L, cvar->GetString());
  return 1;
}

static int __fastcall Script_SetCVar(lua_State *L) {
  char message[512];
  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: SetCVar(\"cvar\", value [, \"scriptCvar\")");
  }

  CVar *cvar = CVar::Lookup(lua_tostring(L, 1));
  if (!cvar) {
    SStrPrintf(message, sizeof(message), "Couldn't find CVar named '%s'", lua_tostring(L, 1));
    luaL_error(L, message);
  }

  const char *value = lua_tostring(L, 2);
  if (!value) {
    value = "0";
  }
  cvar->Set(value, 1, 0, 0);

  if (lua_isstring(L, 3)) {
    FrameScript_SignalEvent(289, "%s%s", lua_tostring(L, 3), value);
  }
  return 0;
}

static int __fastcall Script_GetWorldDetail(lua_State *L) {
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

static int __fastcall Script_SetWorldDetail(lua_State *L) {
  char buf[32];
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: SetWorldDetail(value)");
  }

  int terrainDetail = static_cast<int>(lua_tonumber(L, 1));
  if (static_cast<unsigned int>(terrainDetail) > 2) {
    luaL_error(L, "value must be in the range 0, 2");
  }

  SStrPrintf(buf, sizeof(buf), "%f", s_distCullValues[terrainDetail]);
  CVar::Lookup("DistCull")->Set(buf, 1, 0, 0);
  SStrPrintf(buf, sizeof(buf), "%f", s_smallCullValues[terrainDetail]);
  CVar::Lookup("smallCull")->Set(buf, 1, 0, 0);
  return 0;
}

static int __fastcall Script_GetWaterDetail(lua_State *L) {
  lua_pushnumber(L, 0.0);
  return 1;
}

static int __fastcall Script_SetWaterDetail(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: SetWaterDetail(value)");
  }
  return 0;
}

static int __fastcall Script_GetFarclip(lua_State *L) {
  lua_pushnumber(L, CVar::Lookup("farclip")->GetFloat());
  return 1;
}

static int __fastcall Script_SetFarclip(lua_State *L) {
  char strVal[16];
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: SetFarclip(value)");
  }

  CVar  *cvar = CVar::Lookup("farclip");
  double value = lua_tonumber(L, 1);
  SStrPrintf(strVal, sizeof(strVal), "%f", value);
  cvar->Set(strVal, 1, 0, 0);
  return 0;
}

static int __fastcall Script_GetTerrainMip(lua_State *L) {
  lua_pushnumber(L, 1.0 - CVar::Lookup("alphaLevel")->GetInt());
  return 1;
}

static int __fastcall Script_SetTerrainMip(lua_State *L) {
  char buf[16];
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: SetTerrainMip(value)");
  }

  int value = 1 - static_cast<int>(lua_tonumber(L, 1));
  SStrPrintf(buf, sizeof(buf), "%d", value);
  CVar::Lookup("alphaLevel")->Set(buf, 1, 0, 0);
  SStrPrintf(buf, sizeof(buf), "%d", value);
  CVar::Lookup("shadowLevel")->Set(buf, 1, 0, 0);
  return 0;
}

static int __fastcall Script_GetDoodadAnim(lua_State *L) {
  lua_pushnumber(L, CVar::Lookup("doodadAnim")->GetFloat());
  return 1;
}

static int __fastcall Script_SetDoodadAnim(lua_State *L) {
  char strVal[16];
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: SetDoodadAnim(value)");
  }

  CVar  *cvar = CVar::Lookup("doodadAnim");
  double value = lua_tonumber(L, 1);
  SStrPrintf(strVal, sizeof(strVal), "%f", value);
  cvar->Set(strVal, 1, 0, 0);
  return 0;
}

static int __fastcall Script_GetTexLodBias(lua_State *L) {
  lua_pushnumber(L, CVar::Lookup("texLodBias")->GetFloat());
  return 1;
}

static int __fastcall Script_SetTexLodBias(lua_State *L) {
  char strVal[16];
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: SetTexLodBias(value)");
  }

  CVar  *cvar = CVar::Lookup("texLodBias");
  double value = lua_tonumber(L, 1);
  SStrPrintf(strVal, sizeof(strVal), "%f", value);
  cvar->Set(strVal, 1, 0, 0);
  return 0;
}

static int __fastcall Script_GetGamma(lua_State *L) {
  lua_pushnumber(L, 1.0 - CVar::Lookup("gamma")->GetFloat());
  return 1;
}

static int __fastcall Script_SetGamma(lua_State *L) {
  char strVal[16];
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: SetGamma(value)");
  }

  CVar  *cvar = CVar::Lookup("gamma");
  double value = 1.0 - lua_tonumber(L, 1);
  SStrPrintf(strVal, sizeof(strVal), "%f", value);
  cvar->Set(strVal, 1, 0, 0);
  return 0;
}

static int __fastcall Script_ToggleTris(lua_State *__formal) {
  return 0;
}

static int __fastcall Script_TogglePortals(lua_State *__formal) {
  return 0;
}

static int __fastcall Script_ToggleCollision(lua_State *__formal) {
  return 0;
}

static int __fastcall Script_ToggleCollisionDisplay(lua_State *__formal) {
  return 0;
}

static int __fastcall Script_Logout(lua_State *__formal) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  FATALASSERT(player);

  const CGUnitData *unitData = player->GetUnitData();
  unsigned int      flags = unitData->flags;
  if ((flags & 0x01000000) || ((player->GetType() & TYPE_PLAYER) && !unitData->charmedBy && ((flags & 2) || !(flags & 0x00C00004)) && !(flags & 1))) {
    ClientServices_CharacterLogout(false);
  }
  return 0;
}

static int __fastcall Script_Quit(lua_State *__formal) {
  ClientServices_Exit();
  return 0;
}

static int __fastcall Script_Screenshot(lua_State *L) {
  ClientServices_ReportScreenshot();
  ScrnScreenshot(CGGameUI::HandleScreenshot);
  return 0;
}

static int __fastcall Script_GetFramerate(lua_State *L) {
  lua_pushnumber(L, CWorld::GetFramerate());
  return 1;
}

static int __fastcall Script_TogglePerformanceDisplay(lua_State *__formal) {
  ScrnPerfEnable(!ScrnPerfIsEnabled());
  return 0;
}

static int __fastcall Script_TogglePerformanceValues(lua_State *__formal) {
  ScrnPerfToggleDisplayedValues();
  return 0;
}

static int __fastcall Script_ResetPerformanceValues(lua_State *__formal) {
  ScrnPerfResetTimePeaks();
  return 0;
}

static int __fastcall Script_TogglePlayerBounds(lua_State *__formal) {
  CGPlayer_C::TogglePlayerBounds();
  return 0;
}

static int __fastcall Script_ShowNameplates(lua_State *__formal) {
  CGUnit_C::NamePlateShow(1);
  PlayerNameShow(0);
  return 0;
}

static int __fastcall Script_HideNameplates(lua_State *__formal) {
  CGUnit_C::NamePlateShow(0);
  PlayerNameShow(1);
  return 0;
}

static int __fastcall Script_SetCursor(lua_State *L) {
  struct {
    CURSORANIMATIONS mode;
    const char      *name;
  } cursorModes[8] = {
      {       POINT_CURSOR,        "POINT_CURSOR"},
      {        CAST_CURSOR,         "CAST_CURSOR"},
      {         BUY_CURSOR,          "BUY_CURSOR"},
      {      ATTACK_CURSOR,       "ATTACK_CURSOR"},
      { POINT_ERROR_CURSOR,  "POINT_ERROR_CURSOR"},
      {  CAST_ERROR_CURSOR,   "CAST_ERROR_CURSOR"},
      {   BUY_ERROR_CURSOR,    "BUY_ERROR_CURSOR"},
      {ATTACK_ERROR_CURSOR, "ATTACK_ERROR_CURSOR"}
  };

  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: SetCursor(\"cursor\")");
  }

  const char *name = lua_tostring(L, 1);
  int         i;
  for (i = 0; i < 8; ++i) {
    if (!SStrCmp(cursorModes[i].name, name, INT_MAX)) {
      break;
    }
  }
  if (i == 8) {
    luaL_error(L, "Usage: SetCursor(\"cursor\")");
  }
  CursorSetCursorMode(cursorModes[i].mode);
  return 0;
}

int __fastcall Script_CursorHasItem(lua_State *L) {
  if (CGGameUI::m_cursorItemType == UICURSOR_ITEM) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

int __fastcall Script_CursorHasSpell(lua_State *L) {
  if (CGGameUI::m_cursorItemType == UICURSOR_SPELL) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

int __fastcall Script_CursorHasMoney(lua_State *L) {
  if (CGGameUI::m_cursorItemType == UICURSOR_MONEY) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int __fastcall Script_EquipCursorItem(lua_State *L) {
  unsigned __int64 cursorItemPack;
  unsigned __int64 cursorItem;
  unsigned int     cursorItemSlot;
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: EquipCursorItem(slot)");
  }

  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  FATALASSERT(player);
  CGGameUI::GetCursorItem(cursorItem, cursorItemPack, cursorItemSlot);
  if (cursorItem) {
    int slot = static_cast<int>(lua_tonumber(L, 1));
    if (slot != 0xFF && slot != -1) {
      player->SwapItems(cursorItem, cursorItemPack, static_cast<int>(cursorItemSlot), player->GetGUID(), slot, 1);
    } else {
      player->AutoEquipCursorItem(1);
    }
  }
  return 0;
}

int __fastcall Script_DeleteCursorItem(lua_State *__formal) {
  CDataStore       msg;
  unsigned __int64 cursorItemPack;
  unsigned __int64 cursorItem;
  unsigned int     cursorItemSlot;
  unsigned int     cursorItemPackIndex;

  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    CGGameUI::GetCursorItem(cursorItem, cursorItemPack, cursorItemSlot);
    if (cursorItem) {
      cursorItemPackIndex = player->FindSlotIndex(cursorItemPack);
      msg.Put(static_cast<unsigned int>(CMSG_DESTROYITEM));
      msg.Put(static_cast<unsigned char>(cursorItemPackIndex));
      msg.Put(static_cast<unsigned char>(cursorItemSlot));
      msg.Put(CGGameUI::m_stackSplit);
      msg.Finalize();
      ClientServices_Send(&msg);
      CGGameUI::ClearCursor(1);
      CGGameUI::UnlockItem(cursorItem);
    }
  }
  return 0;
}

static int __fastcall Script_EquipPendingItem(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: EquipPendingItem(index)");
  }
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  FATALASSERT(player);
  player->ClearPendingEquip(static_cast<unsigned int>(lua_tonumber(L, 1)), 1);
  return 0;
}

static int __fastcall Script_CancelPendingEquip(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: CancelPendingEquip(index)");
  }
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  FATALASSERT(player);
  player->ClearPendingEquip(static_cast<unsigned int>(lua_tonumber(L, 1)), 0);
  return 0;
}

static int __fastcall Script_TargetUnit(lua_State *L) {
  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: TargetUnit(\"unit\")");
  }

  unsigned __int64 guid = Script_GetGUIDFromName(lua_tostring(L, 1));
  if (!ClntObjMgrObjectPtr(guid, __FILE__, __LINE__) && !CGPartyInfo::IsMember(guid)) {
    return 0;
  }
  CGGameUI::Target(guid, 0);
  return 0;
}

static int __fastcall Script_TargetUnitsPet(lua_State *L) {
  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: TargetUnitsPet(\"unit\")");
  }

  CGUnit_C *unit = Script_GetUnitFromName(lua_tostring(L, 1));
  if (!unit) {
    return 0;
  }

  unsigned __int64 petGUID = unit->GetUnitData()->charm;
  if (!petGUID) {
    petGUID = unit->GetUnitData()->summon;
  }
  if (petGUID) {
    CGGameUI::Target(petGUID, 0);
  }
  return 0;
}

static int __fastcall Script_TargetNearestEnemy(lua_State *L) {
  int reverse = 0;
  if (lua_isnumber(L, 1)) {
    reverse = static_cast<int>(lua_tonumber(L, 1));
  } else if (lua_isstring(L, 1)) {
    reverse = StringToBOOL(lua_tostring(L, 1));
  }
  CGGameUI::TargetNearestEnemy(reverse);
  return 0;
}

int __fastcall Script_TargetLastEnemy(lua_State *__formal) {
  unsigned __int64 target = CGGameUI::m_lastEnemyTarget;
  if (target) {
    CGGameUI::Target(target, 0);
  }
  return 0;
}

static int __fastcall Script_AttackTarget(lua_State *__formal) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  FATALASSERT(player);
  player->OnAttackIconPressed();
  return 0;
}

static int __fastcall Script_AssistUnit(lua_State *L) {
  unsigned __int64 newTarget = 0;
  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: AssistUnit(\"unit\")");
  }
  CGUnit_C *unit = Script_GetUnitFromName(lua_tostring(L, 1));
  if (!unit) {
    CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(168));
    return 0;
  }
  if (unit->GetType() & TYPE_PLAYER) {
    newTarget = static_cast<CGPlayer_C *>(unit)->GetLocalTarget();
  } else {
    newTarget = unit->GetUnitData()->target;
  }
  if (newTarget) {
    CGGameUI::Target(newTarget, 0);
    CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
    FATALASSERT(player);
    player->SetCombatMode(1);
  }
  return 0;
}

static int __fastcall Script_AssistByName(lua_State *L) {
  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: AssistByName(\"name\")");
  }
  CGGameUI::AssistByName(lua_tostring(L, 1));
  return 0;
}

static int __fastcall Script_FollowUnit(lua_State *L) {
  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: FollowUnit(\"unit\")");
  }
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  FATALASSERT(player);
  CGUnit_C *target = Script_GetUnitFromName(lua_tostring(L, 1));
  if (!target) {
    CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(168));
  } else if ((target->GetType() & TYPE_PLAYER) && target->UnitReaction(player) >= UNIT_REACTION_AMIABLE) {
    player->SaveTrackingTarget(target->GetGUID(), TRACKTYPE_FOLLOW, 0);
  } else {
    CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(269));
  }
  return 0;
}

static int __fastcall Script_FollowByName(lua_State *L) {
  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: FollowByName(\"name\")");
  }
  CGGameUI::FollowByName(lua_tostring(L, 1));
  return 0;
}

static int __fastcall Script_ClearTarget(lua_State *L) {
  int                    hadTarget = CGGameUI::GetLockedTarget() != 0;
  const unsigned __int64 noTarget = 0;
  CGGameUI::Target(noTarget, 0);
  if (hadTarget) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int __fastcall Script_AutoEquipCursorItem(lua_State *__formal) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  FATALASSERT(player);
  player->AutoEquipCursorItem(0);
  return 0;
}

static int __fastcall Script_ToggleSheath(lua_State *__formal) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  FATALASSERT(player);
  player->ToggleSheathe(0);
  return 0;
}

static int __fastcall Script_ToggleRun(lua_State *L) {
  unsigned long eventTime = lua_isnumber(L, 1) ? static_cast<unsigned long>(lua_tonumber(L, 1)) : GetTickCount();
  CGUnit_C     *mover = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(CGUnit_C::m_activeMover, __FILE__, __LINE__));
  FATALASSERT(mover);
  const CGUnitData *unitData = mover->GetUnitData();
  unsigned int      flags = unitData->flags;
  if (unitData->health > 0 &&
      ((flags & 0x01000000) ||
       ((mover->GetType() & TYPE_PLAYER) && !unitData->charmedBy && ((flags & 2) || !(flags & 0x00C00004)) && !(flags & 1))) &&
      !mover->IsInStandSitTransition())
  {
    mover->ToggleRunModeLocal(eventTime);
  }
  return 0;
}

static int __fastcall Script_Jump(lua_State *L) {
  unsigned long eventTime = lua_isnumber(L, 1) ? static_cast<unsigned long>(lua_tonumber(L, 1)) : GetTickCount();
  CGUnit_C     *mover = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(CGUnit_C::m_activeMover, __FILE__, __LINE__));
  FATALASSERT(mover);
  const CGUnitData *unitData = mover->GetUnitData();
  unsigned int      flags = unitData->flags;
  if (unitData->health > 0 &&
      ((flags & 0x01000000) ||
       ((mover->GetType() & TYPE_PLAYER) && !unitData->charmedBy && ((flags & 2) || !(flags & 0x00C00004)) && !(flags & 1))) &&
      !mover->IsInStandSitTransition())
  {
    unsigned int standState = unitData->standState;
    if (standState != 1 && standState != 3 && (standState < 4 || standState > 6)) {
      mover->OnJumpLocal(eventTime);
    } else {
      mover->ChangeStandState(0);
    }
  }
  return 0;
}

static int __fastcall Script_GetZoneText(lua_State *L) {
  const char *text = CGGameUI::GetZoneText();
  lua_pushstring(L, text ? text : "");
  return 1;
}

static int __fastcall Script_GetSubZoneText(lua_State *L) {
  const char *text = CGGameUI::GetSubZoneText();
  lua_pushstring(L, text ? text : "");
  return 1;
}

static int __fastcall Script_GetMinimapZoneText(lua_State *L) {
  const char *text = CGGameUI::GetMinimapZoneText();
  lua_pushstring(L, text ? text : "");
  return 1;
}

static int __fastcall Script_InitiateTrade(lua_State *L) {
  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: InitiateTrade(\"unit\")");
  }
  unsigned __int64 guid = Script_GetGUIDFromName(lua_tostring(L, 1));
  if (guid) {
    Trade_C_InitiateTrade(guid, 0);
  }
  return 0;
}

static int __fastcall Script_NotifyInspect(lua_State *L) {
  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: NotifyInspect(unit)");
  }
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  FATALASSERT(player);
  unsigned __int64 guid = Script_GetGUIDFromName(lua_tostring(L, 1));
  player->InspectPlayer(guid);
  return 0;
}

static int __fastcall Script_InviteToParty(lua_State *L) {
  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: InviteToParty(\"unit\")");
  }
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  FATALASSERT(player);
  unsigned __int64 guid = Script_GetGUIDFromName(lua_tostring(L, 1));
  if (guid) {
    player->InviteToGroup(guid);
  }
  return 0;
}

static int __fastcall Script_InviteByName(lua_State *L) {
  CDataStore msg;
  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: InviteByName(\"name\")");
  }
  msg.Put(static_cast<unsigned int>(CMSG_GROUP_INVITE));
  msg.PutString(lua_tostring(L, 1));
  msg.Finalize();
  ClientServices_Send(&msg);
  return 0;
}

static int __fastcall Script_UninviteFromParty(lua_State *L) {
  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: UninviteFromParty(\"unit\")");
  }
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  FATALASSERT(player);
  unsigned __int64 guid = Script_GetGUIDFromName(lua_tostring(L, 1));
  if (guid) {
    player->Uninvite(guid);
  }
  return 0;
}

static int __fastcall Script_UninviteByName(lua_State *L) {
  CDataStore msg;
  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: UninviteByName(\"name\")");
  }
  msg.Put(static_cast<unsigned int>(CMSG_GROUP_UNINVITE));
  msg.PutString(lua_tostring(L, 1));
  msg.Finalize();
  ClientServices_Send(&msg);
  return 0;
}

static int __fastcall Script_PromoteToPartyLeader(lua_State *L) {
  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: PromoteToPartyLeader(\"unit\")");
  }
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  FATALASSERT(player);
  unsigned __int64 guid = Script_GetGUIDFromName(lua_tostring(L, 1));
  if (guid) {
    player->SetNewLeader(guid);
  }
  return 0;
}

static int __fastcall Script_PromoteByName(lua_State *L) {
  CDataStore msg;
  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: PromoteByName(\"name\")");
  }
  msg.Put(static_cast<unsigned int>(CMSG_GROUP_SET_LEADER));
  msg.PutString(lua_tostring(L, 1));
  msg.Finalize();
  ClientServices_Send(&msg);
  return 0;
}

static int __fastcall Script_RequestTimePlayed(lua_State *__formal) {
  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_PLAYED_TIME));
  msg.Finalize();
  ClientServices_Send(&msg);
  return 0;
}

static int __fastcall Script_RepopMe(lua_State *__formal) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  FATALASSERT(player);
  player->HandleRepopRequest();
  return 0;
}

static int __fastcall Script_AcceptResurrect(lua_State *__formal) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  FATALASSERT(player);
  player->AcceptResurrectRequest(1);
  return 0;
}

static int __fastcall Script_DeclineResurrect(lua_State *__formal) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  FATALASSERT(player);
  player->AcceptResurrectRequest(0);
  return 0;
}

static int __fastcall Script_BeginTrade(lua_State *__formal) {
  Trade_C_BeginTrade();
  return 0;
}

static int __fastcall Script_CancelTrade(lua_State *__formal) {
  Trade_C_CancelTrade();
  return 0;
}

static int __fastcall Script_AcceptGroup(lua_State *__formal) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  FATALASSERT(player);
  player->AcceptGroup();
  return 0;
}

static int __fastcall Script_DeclineGroup(lua_State *__formal) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  FATALASSERT(player);
  player->DeclineGroup();
  return 0;
}

static int __fastcall Script_AcceptGuild(lua_State *__formal) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  FATALASSERT(player);
  player->AcceptGuild();
  return 0;
}

static int __fastcall Script_DeclineGuild(lua_State *__formal) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  FATALASSERT(player);
  player->DeclineGuild();
  return 0;
}

static int __fastcall Script_CancelLogout(lua_State *__formal) {
  ClientServices_CharacterAbortLogout();
  return 0;
}

static int __fastcall Script_ForceLogout(lua_State *__formal) {
  ClientServices_CharacterForceLogout();
  return 0;
}

static int __fastcall Script_ForceQuit(lua_State *__formal) {
  ClientPostClose();
  return 0;
}

static int __fastcall Script_ReportBug(lua_State *L) {
  bool success = ClientServices_Report(0, lua_tostring(L, 1), lua_tostring(L, 2));
  CGChat::AddChatMessage(success ? "Bug submitted" : "Bug submission failed", static_cast<SLASH_COMMAND_ID>(9), 0, 0, 0, 0, 0);
  return 0;
}

static int __fastcall Script_ReportSuggestion(lua_State *L) {
  bool success = ClientServices_Report(1, lua_tostring(L, 1), lua_tostring(L, 2));
  CGChat::AddChatMessage(success ? "Suggestion submitted" : "Suggestion submission failed", static_cast<SLASH_COMMAND_ID>(9), 0, 0, 0, 0, 0);
  return 0;
}

static int __fastcall Script_ReportNote(lua_State *L) {
  bool success = ClientServices_Report(2, lua_tostring(L, 1), lua_tostring(L, 2));
  CGChat::AddChatMessage(success ? "Note submitted" : "Note submission failed", static_cast<SLASH_COMMAND_ID>(9), 0, 0, 0, 0, 0);
  return 0;
}

static int __fastcall Script_GetCursorMoney(lua_State *L) {
  lua_pushnumber(L, CGGameUI::GetCursorMoney());
  return 1;
}

static int __fastcall Script_DropCursorMoney(lua_State *__formal) {
  if (CGGameUI::GetCursorMoney()) {
    CGGameUI::SetCursorMoney(0);
  }
  return 0;
}

static int __fastcall Script_PickupPlayerMoney(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: PickupPlayerMoney(amount)");
  }
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  FATALASSERT(player);
  unsigned int amount = static_cast<unsigned int>(lua_tonumber(L, 1));
  if (amount && amount <= player->GetUnitData()->coinage) {
    CGGameUI::SetCursorMoney(amount);
  }
  return 0;
}

static int __fastcall Script_HideSellCursor(lua_State *__formal) {
  CURSORANIMATIONS mode = static_cast<CURSORANIMATIONS>(CursorGetCursorMode());
  if (mode == BUY_CURSOR || mode == BUY_ERROR_CURSOR) {
    CursorSetCursorMode(POINT_CURSOR);
  }
  return 0;
}

static int __fastcall Script_HasSoulstone(lua_State *L) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  FATALASSERT(player);
  if (player->GetSoulstone()) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int __fastcall Script_UseSoulstone(lua_State *L) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  FATALASSERT(player);
  player->UseSoulstone();
  return 0;
}

static int __fastcall Script_JoinChannelByName(lua_State *L) {
  CDataStore msg;
  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: JoinChannelByName(\"name\")");
  }
  msg.Put(static_cast<unsigned int>(CMSG_JOIN_CHANNEL));
  msg.PutString(lua_tostring(L, 1));
  msg.Finalize();
  ClientServices_Send(&msg);
  return 0;
}

static int __fastcall Script_LeaveChannelByName(lua_State *L) {
  CDataStore msg;
  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: LeaveChannelByName(\"name\")");
  }
  msg.Put(static_cast<unsigned int>(CMSG_LEAVE_CHANNEL));
  msg.PutString(lua_tostring(L, 1));
  msg.Finalize();
  ClientServices_Send(&msg);
  return 0;
}

static int __fastcall Script_GuildInviteByName(lua_State *L) {
  CDataStore  msg;
  const char *name = lua_isstring(L, 1) ? lua_tostring(L, 1) : 0;
  if (!name || !*name) {
    CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(278));
    return 0;
  }
  msg.Put(static_cast<unsigned int>(CMSG_GUILD_INVITE));
  msg.PutString(name);
  msg.Finalize();
  ClientServices_Send(&msg);
  return 0;
}

static int __fastcall Script_GuildUninviteByName(lua_State *L) {
  CDataStore msg;
  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: GuildUninviteByName(\"name\")");
  }
  msg.Put(static_cast<unsigned int>(CMSG_GUILD_REMOVE));
  msg.PutString(lua_tostring(L, 1));
  msg.Finalize();
  ClientServices_Send(&msg);
  return 0;
}

static int __fastcall Script_GuildPromoteByName(lua_State *L) {
  CDataStore msg;
  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: GuildPromoteByName(\"name\")");
  }
  msg.Put(static_cast<unsigned int>(CMSG_GUILD_PROMOTE));
  msg.PutString(lua_tostring(L, 1));
  msg.Finalize();
  ClientServices_Send(&msg);
  return 0;
}

static int __fastcall Script_GuildDemoteByName(lua_State *L) {
  CDataStore msg;
  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: GuildDemoteByName(\"name\")");
  }
  msg.Put(static_cast<unsigned int>(CMSG_GUILD_DEMOTE));
  msg.PutString(lua_tostring(L, 1));
  msg.Finalize();
  ClientServices_Send(&msg);
  return 0;
}

static int __fastcall Script_GuildSetLeaderByName(lua_State *L) {
  CDataStore msg;
  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: GuildSetLeaderByName(\"name\")");
  }
  const char *name = lua_tostring(L, 1);
  msg.Put(static_cast<unsigned int>(CMSG_GUILD_LEADER));
  if (*name && SStrCmp(name, "target", INT_MAX)) {
    msg.PutString(name);
  }
  msg.Finalize();
  ClientServices_Send(&msg);
  return 0;
}

static int __fastcall Script_GuildSetMOTD(lua_State *L) {
  CDataStore msg;
  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: GuildSetMOTD(\"message\")");
  }
  const char *message = lua_tostring(L, 1);
  msg.Put(static_cast<unsigned int>(CMSG_GUILD_MOTD));
  if (*message) {
    msg.PutString(message);
  }
  msg.Finalize();
  ClientServices_Send(&msg);
  return 0;
}

static int __fastcall Script_GuildLeave(lua_State *L) {
  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_GUILD_LEAVE));
  msg.Finalize();
  ClientServices_Send(&msg);
  return 0;
}

static int __fastcall Script_GuildDisband(lua_State *L) {
  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_GUILD_DISBAND));
  msg.Finalize();
  ClientServices_Send(&msg);
  return 0;
}

static int __fastcall Script_GuildInfo(lua_State *L) {
  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_GUILD_INFO));
  msg.Finalize();
  ClientServices_Send(&msg);
  return 0;
}

static int __fastcall Script_GuildRoster(lua_State *L) {
  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_GUILD_ROSTER));
  msg.Finalize();
  ClientServices_Send(&msg);
  return 0;
}

static int __fastcall Script_GetScreenWidth(lua_State *L) {
  NTempest::CRect screenRect;
  GxCapsWindowSize(screenRect);
  float width = screenRect.r - screenRect.l;
  lua_pushnumber(L, width > 1024.0f ? width : 1024.0f);
  return 1;
}

static int __fastcall Script_GetScreenHeight(lua_State *L) {
  NTempest::CRect screenRect;
  GxCapsWindowSize(screenRect);
  float height = screenRect.b - screenRect.t;
  lua_pushnumber(L, height > 768.0f ? height : 768.0f);
  return 1;
}

static int __fastcall Script_PVPPort(lua_State *L) {
  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_PVP_PORT));
  msg.Finalize();
  ClientServices_Send(&msg);
  return 0;
}

static int __fastcall Script_GetDamageBonusStat(lua_State *L) {
  CGPlayer_C          *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  const ChrClassesRec *rec = player ? g_chrClassesDB.GetRecord(player->GetUnitData()->classId) : 0;
  lua_pushnumber(L, rec ? rec->m_DamageBonusStat + 1.0 : 0.0);
  return 1;
}

static int __fastcall Script_GetReleaseTimeRemaining(lua_State *L) {
  lua_pushnumber(L, 300.0);
  return 1;
}

static int __fastcall Script_GetBindZone(lua_State *L) {
  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_GETDEATHBINDZONE));
  msg.Finalize();
  ClientServices_Send(&msg);
  return 0;
}

static int __fastcall Script_SplitMoney(lua_State *L) {
  CDataStore msg;
  int        coins[3] = {0, 0, 0};
  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: SplitMoney(\"gold silver copper\")");
  }
  const char *text = lua_tostring(L, 1);
  int         parsed[3];
  int         count = 0;
  while (*text && count < 3) {
    while (*text == ' ') {
      ++text;
    }
    if (!*text) {
      break;
    }
    parsed[count++] = SStrToInt(text);
    while (*text && *text != ' ') {
      ++text;
    }
  }
  for (int i = 0; i < count; ++i) {
    coins[3 - count + i] = parsed[i];
  }
  unsigned int money = CurrencyTotal(coins);
  if (!money) {
    luaL_error(L, "Usage: SplitMoney(\"gold silver copper\")");
  }
  msg.Put(static_cast<unsigned int>(MSG_SPLIT_MONEY));
  msg.Put(money);
  msg.Finalize();
  ClientServices_Send(&msg);
  return 0;
}

static int __fastcall Script_GetDate(lua_State *L) {
  lua_pushstring(L, "Dec 11 2003");
  return 1;
}

static int __fastcall Script_GetBuildVersion(lua_State *L) {
  char buf[256];
  SStrPrintf(buf, sizeof(buf), FrameScript_GetText("ALPHA_BUILD", -1, GENDER_NOT_APPLICABLE));
  SStrPack(buf, " ", sizeof(buf));
  SStrPack(buf, "5.3", sizeof(buf));
  lua_pushstring(L, buf);
  return 1;
}

static int __fastcall Script_GetCurrentPosition(lua_State *L) {
  char               buf[256];
  NTempest::C3Vector pos;
  CGPlayer_C        *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  FATALASSERT(player);
  buf[0] = 0;
  player->GetPosition(pos);
  SStrPrintf(buf, sizeof(buf), "%.2f, %.2f, %.2f", pos.x, pos.y, pos.z);
  lua_pushstring(L, buf);
  return 1;
}

static int __fastcall Script_GetCursorPosition(lua_State *L) {
  NTempest::C2Vector pos;
  CSimpleTop        *top = CSimpleTop::GetInstance();
  DDCToNDC(top->m_mousePosition.x, top->m_mousePosition.y, &pos.x, &pos.y);
  lua_pushnumber(L, pos.x * 1024.0f * 1.25f);
  lua_pushnumber(L, pos.y * 1024.0f * 1.25f);
  return 2;
}

static int __fastcall Script_GetNetStats(lua_State *L) {
  unsigned long latency;
  float         bandwidthOut;
  float         bandwidthIn;
  ClientServices_GetNetStats(bandwidthIn, bandwidthOut, latency);
  lua_pushnumber(L, bandwidthIn);
  lua_pushnumber(L, bandwidthOut);
  lua_pushnumber(L, latency);
  return 3;
}

static int __fastcall Script_SitOrStand(lua_State *__formal) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  FATALASSERT(player);
  player->ChangeStandState(player->GetUnitData()->standState ? 0 : 1);
  return 0;
}

static int __fastcall Script_StopCinematic(lua_State *__formal) {
  CGGameUI::StopCinematic(0);
  return 0;
}

static int __fastcall Script_RunScript(lua_State *L) {
  if (lua_isstring(L, 1)) {
    const char *script = lua_tostring(L, 1);
    if (*script) {
      FrameScript_Execute(script, script);
    }
  }
  return 0;
}

static int __fastcall Script_CheckInteractDistance(lua_State *L) {
  CGUnit_C *unit;
  if (!lua_isstring(L, 1) || !lua_isnumber(L, 2)) {
    luaL_error(L, "Usage: CheckInteractDistance(\"unit\", distIndex)");
  }
  unit = Script_GetUnitFromName(lua_tostring(L, 1));
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  FATALASSERT(player);
  unsigned int index = static_cast<int>(lua_tonumber(L, 2)) - 1;
  int          inRange = 0;
  if (unit && index < 3) {
    NTempest::C3Vector unitPos;
    NTempest::C3Vector playerPos;
    unit->GetPosition(unitPos);
    player->GetPosition(playerPos);
    NTempest::C3Vector delta = unitPos - playerPos;
    float              distance = s_interactDistances[index];
    inRange = delta.SquaredMag() <= distance * distance;
  }
  if (inRange) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int __fastcall Script_GetScreenResolutions(lua_State *L) {
  for (unsigned int i = 0; i < 4; ++i) {
    lua_pushstring(L, s_screenResolutions[i]);
  }
  return 4;
}

static int __fastcall Script_GetCurrentResolution(lua_State *L) {
  CVar *cvar = CVar::Lookup("gxResolution");
  int   index = 0;
  if (cvar) {
    while (index < 4 && SStrCmp(cvar->GetString(), s_screenResolutions[index], INT_MAX)) {
      ++index;
    }
    if (index == 4) {
      index = 0;
    }
  }
  lua_pushnumber(L, index + 1);
  return 1;
}

static int __fastcall Script_SetScreenResolution(lua_State *L) {
  int index = 0;
  if (lua_isnumber(L, 1)) {
    int candidate = static_cast<int>(lua_tonumber(L, 1)) - 1;
    index = static_cast<unsigned int>(candidate) <= 4 ? candidate : 4;
  }
  CVar *cvar = CVar::Lookup("gxResolution");
  if (cvar && SStrCmp(cvar->GetString(), s_screenResolutions[index], INT_MAX)) {
    cvar->Set(s_screenResolutions[index], 1, 0, 0);
    ConsoleCommandExecute("gxRestart", 1);
  }
  return 0;
}

int __fastcall Script_Stuck(lua_State *L) {
  Spell_C_CastSpell(CGSpellBook::GetStuckSpell(), 0);
  return 0;
}

static int __fastcall Script_RandomRoll(lua_State *L) {
  CDataStore msg;
  if (!lua_isstring(L, 1) || !lua_isstring(L, 2)) {
    luaL_error(L, "Usage: RandomRoll(\"max\") or RandomRoll(\"min\", \"max\")");
  }
  int min = SStrToInt(lua_tostring(L, 1));
  int max = SStrToInt(lua_tostring(L, 2));
  if ((min || max) && min >= 0 && max >= min) {
    msg.Put(static_cast<unsigned int>(MSG_RANDOM_ROLL));
    msg.Put(min);
    msg.Put(max);
    msg.Finalize();
    ClientServices_Send(&msg);
  }
  return 0;
}

static int __fastcall Script_OpeningCinematic(lua_State *L) {
  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_OPENING_CINEMATIC));
  msg.Finalize();
  ClientServices_Send(&msg);
  return 0;
}

static void __fastcall LoadScriptFunctions() {
  RegisterSimpleFrameScriptMethods();
  CGTooltip::RegisterScriptMethods();
  CGMinimapFrame::RegisterScriptMethods();
  CGCharacterModelBase::RegisterScriptMethods();
  CGTabardModelFrame::RegisterScriptMethods();

  for (unsigned int i = 0; i < 126; ++i) {
    FrameScript_RegisterFunction(s_GameUIScriptFunctions[i].name, s_GameUIScriptFunctions[i].method);
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
  CraftInfoRegisterScriptFunctions();
  MerchantRegisterScriptFunctions();
  TradeInfoRegisterScriptFunctions();
  ContainerRegisterScriptFunctions();
  BankRegisterScriptFunctions();
  FriendList::RegisterScriptFunctions();
  PetInfoRegisterScriptFunctions();
  TradeSkillRegisterScriptFunctions();
  WorldMapRegisterScriptFunctions();
  ReputationInfoRegisterScriptFunctions();
  SndInterfaceRegisterVocalScriptFunctions();
  TabardCreationRegisterScriptFunctions();
  GuildRegistrarRegisterScriptFunctions();
  DuelInfoRegisterScriptFunctions();
  TutorialRegisterScriptFunctions();
  PetitionInfoRegisterScriptFunctions();
}

static void __fastcall UnloadScriptFunctions() {
  TabardCreationUnregisterScriptFunctions();
  SndInterfaceUnregisterVocalScriptFunctions();
  UnregisterSimpleFrameScriptMethods();
  CGTooltip::UnregisterScriptMethods();
  CGMinimapFrame::UnregisterScriptMethods();
  CGCharacterModelBase::UnregisterScriptMethods();
  CGTabardModelFrame::UnregisterScriptMethods();

  for (unsigned int i = 0; i < 126; ++i) {
    FrameScript_UnregisterFunction(s_GameUIScriptFunctions[i].name);
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
  CraftInfoUnregisterScriptFunctions();
  MerchantUnregisterScriptFunctions();
  TradeInfoUnregisterScriptFunctions();
  ContainerUnregisterScriptFunctions();
  BankUnregisterScriptFunctions();
  FriendList::UnregisterScriptFunctions();
  PetInfoUnregisterScriptFunctions();
  TradeSkillUnregisterScriptFunctions();
  WorldMapUnregisterScriptFunctions();
  ReputationInfoUnregisterScriptFunctions();
  GuildRegistrarUnregisterScriptFunctions();
  DuelInfoUnregisterScriptFunctions();
  TutorialUnregisterScriptFunctions();
  PetitionInfoUnregisterScriptFunctions();
}

static int __fastcall PlacedFrameCallback(CSimpleFrame *frame, void *param) {
  char            line[128];
  NTempest::CRect toprect;
  NTempest::CRect rect;
  unsigned long   count;

  const char *name = frame->GetName();
  if (!name || !name[0] || !frame->IsUserPlaced() || (!frame->IsMovable() && !frame->IsResizable())) {
    return 1;
  }

  if (!frame->GetRect(&rect)) {
    return 1;
  }

  if (!frame->GetLayoutParent() || !frame->GetLayoutParent()->GetRect(&toprect)) {
    return 1;
  }

  SStrPrintf(line, sizeof(line), "Frame: %s\n", name);
  OsWriteFile(static_cast<HOSFILE>(param), line, SStrLen(line), &count);
  SStrPrintf(line, sizeof(line), "FrameLevel: %d\n", frame->GetFrameLevel());
  OsWriteFile(static_cast<HOSFILE>(param), line, SStrLen(line), &count);

  if (frame->IsMovable()) {
    SStrPrintf(line, sizeof(line), "X: %d\n", static_cast<int>(-(toprect.l - rect.l) * 1024.0f * 1.25f));
    OsWriteFile(static_cast<HOSFILE>(param), line, SStrLen(line), &count);
    SStrPrintf(line, sizeof(line), "Y: %d\n", static_cast<int>(-(toprect.b - rect.b) * 1024.0f * 1.25f));
    OsWriteFile(static_cast<HOSFILE>(param), line, SStrLen(line), &count);
  }

  if (frame->IsResizable()) {
    SStrPrintf(line, sizeof(line), "W: %d\n", static_cast<int>((rect.r - rect.l) * 1024.0f * 1.25f));
    OsWriteFile(static_cast<HOSFILE>(param), line, SStrLen(line), &count);
    SStrPrintf(line, sizeof(line), "H: %d\n", static_cast<int>((rect.b - rect.t) * 1024.0f * 1.25f));
    OsWriteFile(static_cast<HOSFILE>(param), line, SStrLen(line), &count);
  }

  return 1;
}

static int __fastcall SavePlacedFrames(CSimpleTop *top) {
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

static void __fastcall PlaceFrame(CSimpleFrame *frame, int framelevel, int x, int y, int w, int h) {
  if (!frame) {
    return;
  }

  if (framelevel >= 0) {
    frame->SetFrameLevel(framelevel, 1);
  }

  if (frame->IsMovable() && x && y) {
    frame->ClearAllPoints(1);
    frame->SetPoint(FRAMEPOINT_TOPLEFT, frame->GetLayoutParent(), FRAMEPOINT_TOPLEFT, x / 1024.0f * 0.8f, y / 1024.0f * 0.8f, 1);
    frame->SetUserPlaced(1);
  }

  if (frame->IsResizable()) {
    if (w) {
      frame->SetWidth(w / 1024.0f * 0.8f);
      if (!h) {
        frame->SetUserPlaced(1);
      }
    }
    if (h) {
      frame->SetHeight(h / 1024.0f * 0.8f);
      frame->SetUserPlaced(1);
    }
  }
}

static void __fastcall LoadPlacedFrames() {
  char          line[128];
  void         *buffer;
  const char   *readCursor;
  int           h = 0;
  CSimpleFrame *frame = 0;
  int           framelevel = -1;
  int           x = 0;
  int           y = 0;
  int           w = 0;

  if (!SFileLoadFile("PlacedFrames.txt", &buffer, 0, 1, 0)) {
    return;
  }

  readCursor = static_cast<const char *>(buffer);
  do {
    SStrTokenize(&readCursor, line, sizeof(line), "\r\n", 0);
    if (!SStrCmp(line, "Frame: ", SStrLen("Frame: "))) {
      PlaceFrame(frame, framelevel, x, y, w, h);
      frame = SimpleFrameRegistryGetEntry(line + SStrLen("Frame: "), 0);
      framelevel = -1;
      x = 0;
      y = 0;
      w = 0;
    } else if (!SStrCmp(line, "FrameLevel: ", SStrLen("FrameLevel: "))) {
      framelevel = SStrToInt(line + SStrLen("FrameLevel: "));
    } else if (!SStrCmp(line, "X: ", SStrLen("X: "))) {
      x = SStrToInt(line + SStrLen("X: "));
    } else if (!SStrCmp(line, "Y: ", SStrLen("Y: "))) {
      y = SStrToInt(line + SStrLen("Y: "));
    } else if (!SStrCmp(line, "W: ", SStrLen("W: "))) {
      w = SStrToInt(line + SStrLen("W: "));
    } else if (!SStrCmp(line, "H: ", SStrLen("H: "))) {
      h = SStrToInt(line + SStrLen("H: "));
    }
  } while (line[0] && *readCursor);

  PlaceFrame(frame, framelevel, x, y, w, h);
  SFileUnloadFile(buffer);
}

void __fastcall CGGameUI::ResetCamera() {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  FATALASSERT(player);

  CGWorldFrame *worldFramePtr = CGWorldFrame::GetActive();
  FATALASSERT(worldFramePtr);
  worldFramePtr->SetCameraTarget(player);
}

FrameScript_Method s_GameUIScriptFunctions[126] = {
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

void __fastcall CGGameUI::StartCinematic(int cinematicID) {
  memset(&m_cinematic, 0, sizeof(m_cinematic));
  m_cinematic.sequence = g_cinematicSequencesDB.GetRecord(cinematicID);
  if (m_cinematic.sequence) {
    FATALASSERT(!m_cinematic.sequenceMusic);
    m_cinematic.sequenceMusic = SndInterfacePlayLoopedSound(m_cinematic.sequence->m_soundID, 0);
    if (m_cinematic.sequence) {
      m_cinematic.camera = g_cinematicCameraDB.GetRecord(m_cinematic.sequence->m_camera[0]);
    }
  }

  m_cinematic.zoneMusicPaused = SndInterfaceIsZoneMusicPaused();
  if (m_cinematic.zoneMusicPaused) {
    SndInterfacePauseZoneMusic(1);
  }
  HideCursor();
  EnableFadingScreen(0.25f, 0, 0);
}

static unsigned char GetCinematicStartingCameraPosition(const char* modelFile, NTempest::C3Vector& origin, float facing, NTempest::C3Vector& position) {
    // TODO: implement
    return 0;
}

int __fastcall CGGameUI::StopCinematic(void *__formal) {
  Sound::KillSound(m_cinematic.sequenceMusic);
  EnableFadingScreen(0.25f, 0, 0);
  return 1;
}

void __fastcall CGGameUI::CloseLoot(unsigned int send, unsigned int moving) {
  CGPlayer_C *playerPtr = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (playerPtr) {
    playerPtr->m_lootingUnit = 0;
  }

  unsigned __int64 object = CGLootInfo::m_object;
  if (object) {
    CGObject_C *objectPtr = ClntObjMgrObjectPtr(object, __FILE__, __LINE__);
    if (!moving || !objectPtr || !(objectPtr->GetType() & TYPE_ITEM)) {
      if (send) {
        CDataStore lootRelease;
        lootRelease.Put(static_cast<unsigned int>(CMSG_LOOT_RELEASE));
        lootRelease.Put(object);
        lootRelease.Finalize();
        ClientServices_Send(&lootRelease);
        if (playerPtr) {
          playerPtr->m_flags |= 0x200;
        }
      }

      CGLootInfo::SetObject(0, 0, LOOT_ACQUIRE_FAILED);
      ClearTarget(object, 0);
      if (m_cursorItemType == UICURSOR_LOOT) {
        ClearCursor(1);
      }
    }
  }
}

void __fastcall CGGameUI::OpenResurrectRequest(const char *inviter) {
  FrameScript_SignalEvent(261, "%s", inviter);
}

void __fastcall CGGameUI::OpenPartyInvite(const char *inviter) {
  FrameScript_SignalEvent(262, "%s", inviter);
}

void __fastcall CGGameUI::CancelPartyInvite() {
  FrameScript_SignalEvent(263);
}

void __fastcall CGGameUI::OpenGuildInvite(const char *inviter, const char *guildName) {
  FrameScript_SignalEvent(264, "%s%s", inviter, guildName);
}

void __fastcall CGGameUI::CancelGuildInvite() {
  FrameScript_SignalEvent(265);
}

void __fastcall CGGameUI::SysMsgDisplay(const char *msg, SYSMSG_TYPE severity) {
  char  string[512];
  float b;
  float r;
  float g;

  SysMsgGetSeverityColor(severity, r, g, b);

  char *write = string;
  int   count = 0;
  if (*msg) {
    do {
      if (*msg == '|') {
        *write++ = '|';
        ++count;
        *write = '|';
      } else {
        *write = *msg;
      }
      ++write;
      if (++count == sizeof(string) - 1) {
        break;
      }
    } while (*++msg);
  }
  *write = 0;

  FrameScript_SignalEvent(214, "%s%f%f%f", string, r, g, b);
}

void __fastcall CGGameUI::InitializeGame() {
  ConsoleCommandExecute("run worldexec.wtf", 0);
  PortraitInitialize();
  CGCharacterInfo::InitializeGame();
  CGPartyInfo::InitializeGame();
  CGSpellBook::InitializeGame();
  CGActionBar::InitializeGame();
  CGLootInfo::InitializeGame();
  CGItemText::InitializeGame();
  CGTaxiMap::InitializeGame();
  CGQuestLog::InitializeGame();
  CGClassTrainer::InitializeGame();
  CGPetInfo::InitializeGame();
  CGBuffBar::InitializeGame();
  CGChat::InitializeGame();
  CGDuelInfo::InitializeGame();
  CGWorldMap::InitializeGame();
  CGTutorial::InitializeGame();
  m_hasControl = 1;
  Initialize();
}

void __fastcall CGGameUI::Initialize() {
  CSizeEvent evt;

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
  m_gameTooltip = static_cast<CGTooltip *>(SimpleFrameRegistryGetEntry("GameTooltip", 0));
  FATALASSERT(m_gameTooltip);

  m_screenWidth = 0;
  evt.SetId(8);
  evt.w = static_cast<int>(screenRect.r - screenRect.l);
  evt.h = static_cast<int>(screenRect.b - screenRect.t);
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

void __fastcall CGGameUI::EnterWorld() {
  ResetCamera();
  CGInputControl::GetActive()->UnsetControlBit(static_cast<INPUT_CONTROL>(-1), 0);
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

void __fastcall CGGameUI::LeaveWorld() {
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

void __fastcall CGGameUI::Shutdown() {
  if (ClntObjMgrGetActivePlayer()) {
    LeaveWorld();
  }

  CGUnit_C::UpdateUnitNameplates(0);
  CGUnit_C::RemoveAllNamePlates();

  const unsigned __int64 noTarget = 0;
  Target(noTarget, 0);
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

void __fastcall CGGameUI::ShutdownGame() {
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

void __fastcall CGGameUI::Reload() {
  m_reloadUI = true;
}

int __fastcall CGGameUI::IsPartyMember(const unsigned __int64 &guid) {
  return CGPartyInfo::IsMember(guid);
}

unsigned __int64 __fastcall CGGameUI::GetPartyMember(unsigned int index) {
  return CGPartyInfo::GetMember(index);
}

void __fastcall CGGameUI::UnitNameUpdate(const unsigned __int64 &guid) {
}

void __fastcall ItemPushItemStatsCallback(int id, const unsigned __int64 &, void *arg, bool granted) {
  if (granted) {
    ItemPushInfo *info = static_cast<ItemPushInfo *>(arg);
    FATALASSERT(info);
    CGGameUI::OnItemPush(info->player, info->slot, id, info->pushed, info->display);
    DEL(info);
  }
}

void __fastcall CGGameUI::OnItemPush(unsigned __int64 player, int slot, int itemID, int pushed, int display) {
  const unsigned __int64 noGuid = 0;
  const ItemStats_C     *stats = g_itemDBCache.GetRecord(itemID, noGuid, 0, 0);
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

  const char *colorString = stats->m_overallQualityID < 3 ? "" : CGTooltip::GetItemQualityColorString(stats->m_overallQualityID);
  const char *colorEnd = *colorString ? "|r" : "";
  char        buffer[260];

  if (player == ClntObjMgrGetActivePlayer()) {
    const char *temp = ClientDBStringLookup(SLOOKUP_INVENTORYICONPATH);
    SStrPrintf(buffer, sizeof(buffer), "%s%s", temp, *temp ? "\\" : "");
    SStrPack(buffer, CGItem_C::GetInventoryArt(stats->m_displayInfoID), sizeof(buffer));
    FrameScript_SignalEvent(249, "%d%s", slot == 255 ? 0 : slot + 1, buffer);

    if (display) {
      const char *format = FrameScript_GetText(pushed ? "LOOT_ITEM_PUSHED_SELF" : "LOOT_ITEM_SELF", -1, GENDER_NOT_APPLICABLE);
      SStrPrintf(buffer, sizeof(buffer), format, colorString, itemID, stats->m_displayName[0], colorEnd);
      CGChat::AddChatMessage(buffer, static_cast<SLASH_COMMAND_ID>(24), 0, 0, 0, 0, 0);
    }
  } else if (!pushed && display) {
    CGObject_C *object = ClntObjMgrObjectPtr(player, __FILE__, __LINE__);
    if (object) {
      const char *format = FrameScript_GetText("LOOT_ITEM", -1, GENDER_NOT_APPLICABLE);
      SStrPrintf(
          buffer, sizeof(buffer), format, static_cast<CGUnit_C *>(object)->GetUnitName(), colorString, itemID, stats->m_displayName[0], colorEnd
      );
      CGChat::AddChatMessage(buffer, static_cast<SLASH_COMMAND_ID>(24), 0, 0, 0, 0, 0);
    }
  }
}

int __fastcall CGGameUI::OnTerrainClick(CTerrainClickEvent &evt) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player || evt.button != MOUSE_BUTTON_LEFT) {
    return 0;
  }

  if (Spell_C_HandleTerrainClick(evt)) {
    return 1;
  }
  return player->OnTerrainClick(evt);
}

unsigned __int64 __fastcall CGGameUI::GetCursorItem() {
  if (m_cursorItemType == UICURSOR_ITEM) {
    return m_cursorItem;
  }

  return 0;
}

void __fastcall CGGameUI::GetCursorItem(unsigned __int64 &cursorItem, unsigned __int64 &containerGUID, unsigned int &slot) {
  cursorItem = m_cursorItem;
  containerGUID = m_cursorItemContainer;
  slot = m_cursorItemSlot;
}

int __fastcall CGGameUI::OnSpriteLeftClick(unsigned __int64 object, float x, float y) {
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

  if ((objectPtr->GetType() & TYPE_PLAYER) && m_lockedTarget && m_currentObjectTrack) {
    Trade_C_InitiateTrade(object, 1);
  }
  return 1;
}

int __fastcall CGGameUI::OnSpriteRightClick(unsigned __int64 object, float x, float y) {
  if (!ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__)) {
    return 1;
  }

  CGObject_C *objectPtr = ClntObjMgrObjectPtr(object, __FILE__, __LINE__);
  if (!objectPtr) {
    return 1;
  }

  if (objectPtr->CanBeTargetted()) {
    Target(object, 0);
  }
  objectPtr->OnRightClick();
  return 1;
}

void __fastcall CGGameUI::OnTargetContextAction() {
  CGObject_C *object = ClntObjMgrObjectPtr(m_lockedTarget, __FILE__, __LINE__);
  if (!object || !(object->GetType() & TYPE_UNIT)) {
    return;
  }

  CGUnit_C *unit = static_cast<CGUnit_C *>(object);
  if (unit->GetUnitData()->charm != ClntObjMgrGetActivePlayer()) {
    return;
  }

  NTempest::C2Vector      position(0.0f);
  const unsigned __int64 &guid = *reinterpret_cast<const unsigned __int64 *>(&position);
  const CreatureStats_C  *stats = g_creatureDBCache.GetRecord(unit->GetEntryID(), guid, 0, 0);
  if (stats) {
    FrameScript_SignalEvent(271, "%s", stats->m_name[2]);
  }
}

void __fastcall CGGameUI::HandleObjectTrackChange(unsigned __int64 object, unsigned __int64 oldGUID, float x, float y) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player) {
    return;
  }

  unsigned __int64 untrack = oldGUID ? oldGUID : m_currentObjectTrack;
  if (untrack) {
    player->SetLocalTarget(0);
    m_currentObjectTrack = 0;
    CGObject_C *oldObject = ClntObjMgrObjectPtr(untrack, __FILE__, __LINE__);
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
      UNIT_REACTION reaction = static_cast<CGUnit_C *>(trackedObject)->UnitReaction(player);
      if (reaction <= UNIT_REACTION_HOSTILE) {
        SndInterfacePlayInterfaceSound("GAMEHIGHLIGHTHOSTILEUNIT");
      } else if (reaction >= UNIT_REACTION_AMIABLE) {
        SndInterfacePlayInterfaceSound("GAMEHIGHLIGHTFRIENDLYUNIT");
      } else {
        SndInterfacePlayInterfaceSound("GAMEHIGHLIGHTNEUTRALUNIT");
      }
      break;
    }

    case HIER_TYPE_ITEM:
      m_gameTooltip->SetOwner(m_UISimpleParent, TOOLTIP_ANCHOR_FIXED, 0.0f);
      m_gameTooltip->SetItem(trackedObject->GetEntryID(), ClntObjMgrGetActivePlayer(), trackedObject->GetGUID(), 0, 0, 0);
      SndInterfacePlayInterfaceSound("GAMEHIGHLIGHTNEUTRALUNIT");
      break;

    case HIER_TYPE_GAMEOBJECT:
      m_gameTooltip->SetOwner(
          m_UISimpleParent, static_cast<TOOLTIP_ANCHORPOINT>(trackedObject->CanHighlight() ? TOOLTIP_ANCHOR_CURSOR : TOOLTIP_ANCHOR_FIXED), 0.0f
      );
      m_gameTooltip->SetObject(object);
      SndInterfacePlayInterfaceSound("GAMEHIGHLIGHTNEUTRALUNIT");
      break;

    case HIER_TYPE_CORPSE:
      m_gameTooltip->SetOwner(m_UISimpleParent, TOOLTIP_ANCHOR_FIXED, 0.0f);
      m_gameTooltip->SetCorpse(object);
      SndInterfacePlayInterfaceSound("GAMEHIGHLIGHTNEUTRALUNIT");
      break;

    default:
      break;
  }
}

int __fastcall CGGameUI::FilterMouseDown(const CMouseEvent &evt) {
  if (evt.button == MOUSE_BUTTON_RIGHT &&
      (m_cursorItem || m_cursorMoney || m_cursorSpell || m_cursorPetAction > 0 || m_cursorVirtualID || Spell_C_IsTargeting()))
  {
    ClearCursor(1);
    return 1;
  }
  return 0;
}

int __fastcall CGGameUI::HandleTerrainClick(CTerrainClickEvent &evt) {
  if (evt.button == MOUSE_BUTTON_RIGHT || m_cursorItemType != UICURSOR_EMPTY) {
    ClearCursor(1);
  }

  if (!OnTerrainClick(evt)) {
    const unsigned __int64 target = 0;
    Target(target, 0);
  }
  return 1;
}

int __fastcall CGGameUI::HandleSpriteClick(CSpriteClickEvent &evt) {
  if (m_cursorItemType != UICURSOR_EMPTY) {
    ClearCursor(1);
  }

  if (evt.button == MOUSE_BUTTON_LEFT) {
    return OnSpriteLeftClick(evt.objectGUID, evt.pos.x, evt.pos.y);
  }
  return OnSpriteRightClick(evt.objectGUID, evt.pos.x, evt.pos.y);
}

int __fastcall CGGameUI::HandleWorldClick(CWorldClickEvent &evt) {
  int cursorWasEmpty = m_cursorItemType == UICURSOR_EMPTY;
  if (evt.button == MOUSE_BUTTON_RIGHT || !m_lockedTarget || !m_currentObjectTrack) {
    ClearCursor(1);
  }

  if (evt.button == MOUSE_BUTTON_LEFT) {
    if (m_lockedTarget && m_currentObjectTrack) {
      OnTargetContextAction();
      return 1;
    }
    if (cursorWasEmpty) {
      const unsigned __int64 target = 0;
      Target(target, 0);
    }
  }
  return 1;
}

void __fastcall CGGameUI::HandleSpriteTrack(CObjectTrackEvent &evt) {
  HandleObjectTrackChange(evt.object, evt.oldGUID, evt.x, evt.y);
}

int __fastcall CGGameUI::HandleDisplaySizeChanged(const CSizeEvent &evt) {
  if (m_screenWidth <= 0 || m_screenWidth != evt.w) {
    m_screenWidth = evt.w;
    if (evt.w < 1024) {
      CSimpleTexture::s_textureFilterMode = GxTex_Linear;
    } else {
      CSimpleTexture::s_textureFilterMode = GxTex_Nearest;
      if (evt.w / 4 != evt.h / 3) {
        CSimpleTexture::s_textureFilterMode = GxTex_Linear;
      }
    }
    if (evt.w > 1024) {
      ScaleUI(1024.0f / evt.w, 1);
    } else {
      ScaleUI(1.0f, 1);
    }
  }
  return 1;
}

void __fastcall CGGameUI::HandleScreenshot(int success) {
  FrameScript_SignalEvent(success ? 199 : 200);
}

void __fastcall CGGameUI::SetInteractTarget(const unsigned __int64 &target, float maxDist) {
  if (m_interactTarget != target) {
    if (m_interactTarget) {
      ClearInteractTarget(m_interactTarget);
    }
    m_interactTarget = target;
    m_interactMaxDist = maxDist;

    if (target) {
      CGObject_C *object = ClntObjMgrObjectPtr(target, __FILE__, __LINE__);
      if (object) {
        if (object->GetType() & TYPE_UNIT) {
          static_cast<CGUnit_C *>(object)->OnNPCHello();
        }
        if (object->GetType() & TYPE_GAMEOBJECT) {
          static_cast<CGGameObject_C *>(object)->StartInteraction();
        }
      }
    }
  }
}

void __fastcall CGGameUI::ClearInteractTarget(const unsigned __int64 &target) {
  if (m_interactTarget && m_interactTarget == target) {
    CloseInteraction();
  }
}

void __fastcall CGGameUI::UpdateInteractTarget() {
  if (m_interactTarget && m_interactMaxDist > 0.0f) {
    CGObject_C *target = ClntObjMgrObjectPtr(m_interactTarget, __FILE__, __LINE__);
    CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
    if (!target || target->IsDisabled() || !player) {
      CloseInteraction();
    } else if (!(target->GetType() & TYPE_ITEM)) {
      NTempest::C3Vector targetPosition = target->GetPosition();
      NTempest::C3Vector playerPosition = player->GetPosition();
      if ((targetPosition - playerPosition).SquaredMag() > m_interactMaxDist * m_interactMaxDist) {
        CloseInteraction();
      }
    }
  }
}

struct ClosestObjectMatchData {
  OBJECT_TYPE type;
  const char *match;
  CGObject_C *origin;
  CGObject_C *object;
  int         matchLength;
  float       distanceSq;
};

void __fastcall CGGameUI::CloseInteraction() {
  unsigned __int64 target = m_interactTarget;
  if (target) {
    m_interactTarget = 0;
    if (target == CGQuestInfo::GetQuestGiver()) {
      CGQuestInfo::QuestGiverFinished();
    } else if (target == CGItemText::GetItem()) {
      CGItemText::SetItem(0, 0);
    } else if (target == CGTaxiMap::GetTaxiVendor()) {
      CGTaxiMap::CloseMap();
    } else if (target == CGClassTrainer::GetTrainer()) {
      CGClassTrainer::SetTrainer(0, TRAINER_TYPE_CLASS);
    } else if (target == CGMerchantInfo::GetMerchant()) {
      CGMerchantInfo::CloseMerchant();
    } else if (target == CGTradeInfo::GetTradePartner()) {
      CGTradeInfo::SetTradePartner(0);
    } else if (target == CGBankInfo::m_unit) {
      CGBankInfo::CloseBank();
    } else if (target == CGTabardCreationFrame::GetVendor()) {
      CGTabardCreationFrame::Close();
    } else if (target == CGGuildRegistrar::GetRegistrar()) {
      CGGuildRegistrar::CloseRegistrar();
    }

    CGObject_C *object = ClntObjMgrObjectPtr(target, __FILE__, __LINE__);
    if (object) {
      if (object->GetType() & TYPE_UNIT) {
        static_cast<CGUnit_C *>(object)->OnNPCGoodbye();
      }
      if (object->GetType() & TYPE_GAMEOBJECT) {
        static_cast<CGGameObject_C *>(object)->CloseInteraction();
      }
    }
  }
}

void __fastcall CGGameUI::Target(const unsigned __int64 &target, int usingNearest) {
  if (!usingNearest) {
    s_nearestIndex = 0;
  }

  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  CGObject_C *oldTarget = ClntObjMgrObjectPtr(m_lockedTarget, __FILE__, __LINE__);

  if (target == m_lockedTarget) {
    return;
  }

  bool enterCombatMode = false;
  if (player && player->IsInCombatMode() && (oldTarget->GetType() & TYPE_UNIT)) {
    CGUnit_C *oldUnit = static_cast<CGUnit_C *>(oldTarget);
    if (player->GetUnitData()->health > 0 && !(player->GetUnitData()->flags & 0x2000) && oldUnit->GetUnitData()->health > 0) {
      enterCombatMode = player->CanAttack(oldUnit) != 0;
    }
  }

  ClearTarget(m_lockedTarget, 0);

  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_SET_SELECTION));
  msg.Put(target);
  msg.Finalize();
  ClientServices_Send(&msg);

  if (!target) {
    return;
  }

  m_lockedTarget = target;
  CGObject_C *object = ClntObjMgrObjectPtr(target, __FILE__, __LINE__);
  if (!object) {
    SndInterfacePlayInterfaceSound("igCharacterSelect");
    FrameScript_SignalEvent(191);
    return;
  }

  object->ShowHighlightType(HT_OBJSELECTION);
  object->UpdatePlayerName();

  if (object->GetType() & TYPE_UNIT) {
    CGUnit_C *unit = static_cast<CGUnit_C *>(object);
    unit->RegisterScript();

    if (player && player->GetUnitData()->health > 0 && !(player->GetUnitData()->flags & 0x2000) && unit->GetUnitData()->health > 0 &&
        player->CanAttack(unit))
    {
      m_lastEnemyTarget = target;
    }

    CGTutorial::TriggerTutorial(TUTORIAL_TARGETING);
    if (player && player->CanCooperate(unit)) {
      CGTutorial::TriggerTutorial(TUTORIAL_GROUPING);
    }
    if (player && player->CanAttack(unit)) {
      CGTutorial::TriggerTutorial(TUTORIAL_TARGETING_ENEMY);
    }

    if (unit->GetUnitData()->npcFlags & 0xFF00) {
      SndInterfacePlayInterfaceSound("igCharacterNPCSelect");
    } else if (object->GetType() & TYPE_PLAYER) {
      SndInterfacePlayInterfaceSound("igCharacterSelect");
    } else if (unit->UnitReaction(player) <= UNIT_REACTION_HOSTILE) {
      SndInterfacePlayInterfaceSound("igCreatureAggroSelect");
    } else {
      SndInterfacePlayInterfaceSound("igCreatureNeutralSelect");
    }
  }

  FrameScript_SignalEvent(191);
  if (enterCombatMode && player && m_lockedTarget != player->GetGUID()) {
    player->SetCombatMode(1);
  }
}

void __fastcall CGGameUI::ClearTarget(unsigned __int64 guid, int sendTarget) {
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
    if (object->GetType() & TYPE_UNIT) {
      CGUnit_C *unit = static_cast<CGUnit_C *>(object);
      unit->UnregisterScript();
      if (unit->GetUnitData()->npcFlags & 0xFF00) {
        SndInterfacePlayInterfaceSound("igCharacterNPCDeselect");
      } else if (object->GetType() & TYPE_PLAYER) {
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
    msg.Put(static_cast<unsigned int>(CMSG_SET_SELECTION));
    msg.Put(m_lockedTarget);
    msg.Finalize();
    ClientServices_Send(&msg);
  }

  FrameScript_SignalEvent(191);
}

static int __fastcall ClosestObjectMatchProc(unsigned __int64 guid, void *param) {
  ClosestObjectMatchData *data = static_cast<ClosestObjectMatchData *>(param);
  CGObject_C             *object = ClntObjMgrObjectPtr(guid, __FILE__, __LINE__);
  if (!object || !(object->GetType() & data->type)) {
    return 1;
  }

  const char *name = 0;
  if (object->GetType() & TYPE_UNIT) {
    name = static_cast<CGUnit_C *>(object)->GetUnitName();
  }
  if (!name) {
    return 1;
  }

  const char *match = data->match;
  const char *candidate = name;
  while (*match && *candidate && toupper(*match) == toupper(*candidate)) {
    ++match;
    ++candidate;
  }
  int matchLength = static_cast<int>(match - data->match);
  if (matchLength < data->matchLength) {
    return 1;
  }

  NTempest::C3Vector origin = data->origin->GetPosition();
  NTempest::C3Vector position = object->GetPosition();
  float              distanceSq = (origin - position).SquaredMag();
  if (matchLength <= data->matchLength && distanceSq >= data->distanceSq) {
    return 1;
  }

  data->object = object;
  data->matchLength = matchLength;
  data->distanceSq = distanceSq;
  return 1;
}

unsigned __int64 __fastcall CGGameUI::ClosestObjectMatch(const char *match, OBJECT_TYPE type) {
  CGObject_C *player = ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__);
  if (!player) {
    return 0;
  }

  ClosestObjectMatchData data;
  data.type = type;
  data.match = match;
  data.origin = player;
  data.object = 0;
  data.matchLength = 1;
  data.distanceSq = FLT_MAX;
  ClntObjMgrEnumVisibleObjects(ClosestObjectMatchProc, &data);
  return data.object ? data.object->GetGUID() : 0;
}

void __fastcall CGGameUI::AssistByName(const char *name) {
  unsigned __int64 target = name && *name ? ClosestObjectMatch(name, TYPE_UNIT) : m_lockedTarget;
  CGUnit_C        *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(target, __FILE__, __LINE__));
  if (unit) {
    unsigned __int64 newTarget = unit->GetUnitData()->target;
    if (newTarget) {
      Target(newTarget, 0);
      CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
      if (player && m_currentObjectTrack) {
        player->SetCombatMode(1);
      }
    }
  } else if (name && *name) {
    DisplayError(static_cast<GAME_ERROR_TYPE>(268));
  } else {
    DisplayError(static_cast<GAME_ERROR_TYPE>(168));
  }
}

void __fastcall CGGameUI::FollowByName(const char *name) {
  unsigned __int64 target = name && *name ? ClosestObjectMatch(name, TYPE_UNIT) : m_lockedTarget;
  CGUnit_C        *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(target, __FILE__, __LINE__));
  CGUnit_C        *player = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!unit) {
    DisplayError(static_cast<GAME_ERROR_TYPE>(name && *name ? 268 : 168));
  } else if (player && (unit->GetType() & TYPE_UNIT) && unit->UnitReaction(player) >= UNIT_REACTION_AMIABLE) {
    player->SaveTrackingTarget(target, TRACKTYPE_FOLLOW, false);
  } else {
    DisplayError(static_cast<GAME_ERROR_TYPE>(269));
  }
}

static int __fastcall TargetUpdateProc(unsigned __int64 guid, void *__formal) {
  CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
  if (!unit || !(unit->GetType() & TYPE_UNIT)) {
    return 1;
  }
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player || player->GetUnitData()->health <= 0 || (player->GetUnitData()->flags & 0x2000) || unit->GetUnitData()->health <= 0 ||
      (unit->GetUnitData()->flags & 0x4000) || !player->CanAttack(unit))
  {
    return 1;
  }

  float distSq = (unit->GetPosition() - player->GetPosition()).SquaredMag();
  if (distSq <= 400.0f) {
    NearestEnemyData *entry = s_nearestList.New();
    entry->guid = guid;
    entry->distSq = distSq;
  }
  return 1;
}

static int __cdecl QSortCompareNearestEnemy(const void *a, const void *b) {
  FATALASSERT(a);
  FATALASSERT(b);
  const NearestEnemyData *left = static_cast<const NearestEnemyData *>(a);
  const NearestEnemyData *right = static_cast<const NearestEnemyData *>(b);
  if (left->distSq == right->distSq) {
    return 0;
  }
  return left->distSq < right->distSq ? -1 : 1;
}

void __fastcall CGGameUI::TargetNearestEnemy(int reverse) {
  unsigned int now = GetTickCount();
  if (!s_nearestListTime || !s_sameTargetTime || s_sameTargetTime + 3000 <= now || !s_nearestList.Count()) {
    s_nearestList.SetCount(0);
    s_nearestIndex = 0;
    s_nearestListTime = 0;
    s_sameTargetTime = 0;
    ClntObjMgrEnumVisibleObjects(TargetUpdateProc, 0);
    if (!s_nearestList.Count()) {
      return;
    }
    qsort(s_nearestList.Ptr(), s_nearestList.Count(), sizeof(NearestEnemyData), QSortCompareNearestEnemy);
    s_nearestListTime = GetTickCount();
  } else if (reverse) {
    if (s_nearestIndex) {
      --s_nearestIndex;
    } else {
      s_nearestIndex = s_nearestList.Count() - 1;
    }
  } else if (++s_nearestIndex >= s_nearestList.Count()) {
    s_nearestIndex = 0;
    if (s_nearestListTime + 3000 <= GetTickCount()) {
      s_nearestListTime = 0;
      TargetNearestEnemy(0);
      return;
    }
  }

  unsigned int start = s_nearestIndex;
  do {
    CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(s_nearestList[s_nearestIndex].guid, __FILE__, __LINE__));
    if (unit && unit->GetUnitData()->health > 0) {
      s_sameTargetTime = GetTickCount();
      Target(s_nearestList[s_nearestIndex].guid, 1);
      return;
    }
    if (++s_nearestIndex >= s_nearestList.Count()) {
      s_nearestIndex = 0;
    }
  } while (s_nearestIndex != start);

  s_nearestList.SetCount(0);
}

void __fastcall CGGameUI::ScaleUI(float scale, int force) {
  if (m_UISimpleParent) {
    m_UISimpleParent->SetLayoutScale(scale, force != 0);
  }
}

void __fastcall CGGameUI::HideCursor() {
  m_simpleTop->m_cursorVisible = 0;
}

void __fastcall CGGameUI::AddErrorMessage(const char *string, int error) {
  if (string && *string) {
    FrameScript_SignalEvent(error ? 215 : 216, "%s", string);
  }
}

unsigned int __fastcall CGGameUI::GetCursorVirtualItem() {
  return m_cursorVirtualID;
}

void __fastcall CGGameUI::UpdateObjectHighlightColor(HMODEL__ *model, CGObject_C *object) {
  if (model && object) {
    object->ShowHighlightType(HT_MOUSEOVER);
  }
}

unsigned int __fastcall CGGameUI::GetCursorVirtualItem(UICURSORTYPE type) {
  return m_cursorItemType == type ? m_cursorVirtualID : 0;
}

void __fastcall CGGameUI::GetCursorVirtualItem(unsigned int &cursorItem, unsigned int &slot) {
  cursorItem = m_cursorVirtualID;
  slot = m_cursorVirtualSlot;
}

void __fastcall CGGameUI::ShowHealingFeedback(const unsigned __int64 &guid, int amount) {
  int    numnames;
  char **names = Script_GetNamesFromGUID(guid, numnames);
  for (int index = 0; index < numnames; ++index) {
    FrameScript_SignalEvent(178, "%s%s%s%d", names[index], "HEAL", "", amount);
  }
}

void __fastcall CGGameUI::ShowAutoFollowChange(unsigned __int64 newTarget, unsigned __int64 oldTarget, int type) {
  if (type != 2) {
    return;
  }

  int              event;
  unsigned __int64 target;
  if (newTarget) {
    event = 350;
    target = newTarget;
  } else {
    event = 351;
    target = oldTarget;
  }

  CGObject_C *object = ClntObjMgrObjectPtr(target, __FILE__, __LINE__);
  if (object && (object->GetType() & TYPE_UNIT)) {
    FrameScript_SignalEvent(event, "%s", static_cast<CGUnit_C *>(object)->GetUnitName());
  } else {
    FrameScript_SignalEvent(event, "%s", "");
  }
}

void __fastcall CGGameUI::NewZoneFeedback(int areaID, const char *zoneString, const char *subZoneString) {
  int zoneChanged = 0;

  int firstArea = !m_areaID;
  m_areaID = areaID;
  if (firstArea) {
    CGWorldMap::SetMapToCurrentZone();
  }

  if (zoneString && *zoneString) {
    if (!m_zoneText || SStrCmp(m_zoneText, zoneString, 0x7FFFFFFF)) {
      FREEIFUSED(m_zoneText);
      m_zoneText = SStrDupA(zoneString, __FILE__, __LINE__);
      zoneChanged = 1;
    }
  } else if (m_zoneText && *m_zoneText) {
    *m_zoneText = 0;
    zoneChanged = 1;
  }

  if (subZoneString && *subZoneString) {
    if (!m_subZoneText || SStrCmp(m_subZoneText, subZoneString, 0x7FFFFFFF)) {
      FREEIFUSED(m_subZoneText);
      m_subZoneText = SStrDupA(subZoneString, __FILE__, __LINE__);
      FrameScript_SignalEvent(196);
    }
  } else if (m_subZoneText && *m_subZoneText) {
    *m_subZoneText = 0;
    FrameScript_SignalEvent(196);
  }

  if (zoneChanged) {
    FrameScript_SignalEvent(196);
  }
}

void __fastcall CGGameUI::SetMinimapZoneText(const char *areaName) {
  if (areaName && *areaName) {
    if (m_minimapZoneText && !SStrCmp(m_minimapZoneText, areaName, 0x7FFFFFFF)) {
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

void __fastcall CGGameUI::SetCursorItem(
    unsigned __int64 itemGUID,
    unsigned __int64 containerGUID,
    unsigned int     slot,
    int              unlock,
    unsigned int     stackSplit
) {
  if (m_simpleTop) {
    ClearCursor(unlock);
    if (itemGUID) {
      m_cursorItemContainer = containerGUID;
      m_cursorItem = itemGUID;
      m_cursorItemSlot = slot;
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

void __fastcall CGGameUI::SetCursorMoney(unsigned int money) {
  if (m_simpleTop && (money || m_cursorItemType == UICURSOR_MONEY)) {
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

void __fastcall CGGameUI::SetCursorSpell(int spellId, int pet) {
  if ((m_simpleTop || !pet) && spellId >= 0) {
    SpellRec     *spell = g_spellDB.GetRecord(spellId);
    SpellIconRec *icon = spell ? g_spellIconDB.GetRecord(spell->m_spellIconID) : 0;
    if (icon && icon->m_textureFilename && *icon->m_textureFilename) {
      ClearCursor(1);
      m_cursorSpell = spellId;
      m_cursorItemType = pet ? UICURSOR_PET_SPELL : UICURSOR_SPELL;
      SndInterfacePlayInterfaceSound("INTERFACESOUND_CURSORGRABOBJECT");
      CursorGrabSpell(icon->m_textureFilename);
      if (pet) {
        CGPetInfo::ShowGrid();
      } else {
        CGActionBar::ShowGrid();
      }
      m_cursorHasAction = 1;
    }
  }
}

void __fastcall CGGameUI::DropCursorSpell() {
  if (m_cursorItemType == UICURSOR_SPELL || m_cursorItemType == UICURSOR_PET_SPELL) {
    ClearCursor(1);
  }
}

void __fastcall CGGameUI::ShowCombatFeedback(const unsigned __int64 &guid, int amount, int damageClass, unsigned int flags) {
  const char *flagText = "";
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

int __fastcall CGGameUI::GetCursorSpell() {
  return m_cursorSpell;
}

void __fastcall CGGameUI::SetCursorVirtualItem(unsigned int itemID, unsigned int displayID, unsigned int slot, UICURSORTYPE type) {
  if (m_simpleTop || type == UICURSOR_ACTIONBAR) {
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
      SndInterfacePlayItemSound(ITEMSOUND_PICKUP, static_cast<int>(displayID));
    }
  }
}

void __fastcall CGGameUI::ClearCursor(int unlock) {
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
      SndInterfacePlayItemSound(ITEMSOUND_DROP, static_cast<int>(m_cursorVirtualDisplay));
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

void __fastcall CGGameUI::LockItem(unsigned __int64 itemGUID) {
  if (itemGUID) {
    CGItem_C *item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(itemGUID, __FILE__, __LINE__));
    if (item) {
      item->Lock();
    }
    FrameScript_SignalEvent(184);
  }
}

void __fastcall CGGameUI::UnlockItem(unsigned __int64 itemGUID) {
  if (itemGUID) {
    CGItem_C *item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(itemGUID, __FILE__, __LINE__));
    if (item) {
      item->Unlock();
    }
    FrameScript_SignalEvent(184);
  }
}

int __fastcall CGGameUI::Idle(const void *, void *) {
  if (m_reloadUI) {
    Shutdown();
    Initialize();
  }
  return 1;
}

void __fastcall CGGameUI::UpdateActivePlayer() {
  unsigned __int64 guid = ClntObjMgrGetActivePlayer();
  CGUnit_C        *player = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
  FATALASSERT(player);

  if (player->GetUnitData()->health > 0) {
    FrameScript_SignalEvent(255);
  } else {
    FrameScript_SignalEvent(256);
  }
}

void __fastcall CGGameUI::ClearClientControls() {
  ClearCursor(1);
  const unsigned __int64 noTarget = 0;
  Target(noTarget, 0);
  Spell_C_CancelSpell(1, 1, SPELL_FAILED_ERROR);
}

void __fastcall CGGameUI::RegisterFrameFactories() {
  FrameXML_RegisterFactory("WorldFrame", CGWorldFrame::Create);
  FrameXML_RegisterFactory("GameTooltip", CGTooltip::Create);
  FrameXML_RegisterFactory("Minimap", CGMinimapFrame::Create);
  FrameXML_RegisterFactory("PlayerModel", CGCharacterModelBase::Create);
  FrameXML_RegisterFactory("TabardModel", CGTabardModelFrame::Create);
}

void __cdecl CGGameUI::DisplayError(GAME_ERROR_TYPE errorType, ...) {
  FATALASSERT(errorType < GERR_NUM_TYPES);

  GAMEERRORDESC &desc = s_gameErrors[errorType];
  if (desc.voiceID != static_cast<VOCALUISOUNDS>(66)) {
    if (ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__)) {
      SndInterfacePlayVocalUISound(desc.voiceID);
    }
  } else if (SStrCmp(desc.soundName, "NONE", 0x7FFFFFFF)) {
    SndInterfacePlayInterfaceSound(desc.soundName);
  }

  if (!desc.stringToken || !*desc.stringToken) {
    return;
  }

  char        format[256];
  const char *text = FrameScript_GetText(desc.stringToken, -1, GENDER_NOT_APPLICABLE);
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

  switch (desc.textPlacement) {
    case ERRORTEXT_CHAT:
      CGChat::AddChatMessage(buffer, desc.slashCmd, 0, 0, 0, 0, 0);
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

const char *__fastcall CGGameUI::GetLastErrorString() {
  return s_lastErrorString;
}

void __fastcall CGGameUI::PlayerCombatModeChanged(int newState) {
  FrameScript_SignalEvent(newState ? 189 : 190);
}
