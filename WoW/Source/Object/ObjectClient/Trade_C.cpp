#include "Base/CDataStore.h"
#include "Console/ConsoleClient.h"
#include "Console/ConsoleCommand.h"
#include "DB/DBClient/DBCacheInstances.h"
#include "Object/ObjectClient/Item_C.h"
#include "Object/ObjectClient/Bag_C.h"
#include "Object/ObjectClient/Player_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "Ui/GameUI.h"
#include "WowSvcs/WowSvcsClient/ClientServices.h"

#include <FrameScript/FrameScript.h>

static int              s_initiator;
static int              s_useCursorItem;
static unsigned __int64 s_tradePartner;
static int              s_tradeProposedEnchantment[2];
static int              s_tradeProposedEnchantmentSlot[2];
static unsigned int     s_tradeGold[2];
static unsigned int     s_tradeFlags[2];

struct TradeItemData {
  unsigned int     entryID;
  unsigned int     displayID;
  unsigned int     count;
  unsigned int     enchantmentID;
  unsigned __int64 creator;
};

static TradeItemData s_tradeItems[2][8];

enum TRADE_STATUS {
  TRADE_STATUS_PLAYER_BUSY = 0,
  TRADE_STATUS_PROPOSED = 1,
  TRADE_STATUS_INITIATED = 2,
  TRADE_STATUS_CANCELLED = 3,
  TRADE_STATUS_ACCEPTED = 4,
  TRADE_STATUS_ALREADY_TRADING = 5,
  TRADE_STATUS_PLAYER_NOT_FOUND = 6,
  TRADE_STATUS_STATE_CHANGED = 7,
  TRADE_STATUS_COMPLETE = 8,
  TRADE_STATUS_UNACCEPTED = 9,
  TRADE_STATUS_TOO_FAR_AWAY = 10,
  TRADE_STATUS_WRONG_FACTION = 11,
  TRADE_STATUS_FAILED = 12,
  TRADE_STATUS_DEAD = 13,
  TRADE_STATUS_PETITION = 14,
  TRADE_STATUS_PLAYER_IGNORED = 15
};

class CGTradeInfo {
 public:
  static void Update(TradeItemData *items);
  static void HandleTradeMessage(TRADE_STATUS status, BAG_RESULT bagResult, int myFailure, int itemID);
};

void Trade_C_InitiateTrade(unsigned __int64 target, int useCursorItem);
void Trade_C_AcceptTrade();
void Trade_C_UnacceptTrade();
void Trade_C_CancelTrade();
bool Trade_C_AddItem(unsigned __int64 item, unsigned __int64 itemContainer, unsigned int itemSlot, unsigned int tradeSlot);
void Trade_C_RemoveItem(unsigned int slot);

static int CCommand_Trade(const char* command, const char* arguments) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    Trade_C_InitiateTrade(player->GetLocalTarget(), 0);
  }
  return 1;
}

static int CCommand_AddTradeItem(const char* command, const char* arguments) {
  unsigned __int64 item;
  unsigned __int64 container;
  unsigned int     slot;
  CGGameUI::GetCursorItem(item, container, slot);
  unsigned int tradeSlot = arguments && *arguments ? static_cast<unsigned char>(SStrToInt(arguments)) : 0;
  Trade_C_AddItem(item, container, slot, tradeSlot);
  return 1;
}

static int CCommand_ClearTradeItem(const char* command, const char* arguments) {
  if (ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__)) {
    Trade_C_RemoveItem(arguments && *arguments ? SStrToInt(arguments) : 0);
  }
  return 1;
}

static int CCommand_ClearTrade(const char* command, const char* arguments) {
  if (ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__)) {
    Trade_C_RemoveItem(0xFF);
  }
  return 1;
}

static int CCommand_AcceptTrade(const char* command, const char* arguments) {
  if (ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__)) {
    Trade_C_AcceptTrade();
  }
  return 1;
}

static int CCommand_CancelTrade(const char* command, const char* arguments) {
  if (ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__)) {
    Trade_C_CancelTrade();
  }
  return 1;
}

static int CCommand_ShowTrade(const char*, const char*) {
  for (unsigned int player = 0; player < 2; ++player) {
    ConsoleWrite(player ? "He is offering:" : "You are offering:", DEFAULT_COLOR);
    for (unsigned int slot = 0; slot < 8; ++slot) {
      ConsolePrintf("%d: item=%d, display=%d", slot, s_tradeItems[player][slot].entryID, s_tradeItems[player][slot].displayID);
    }
  }
  return 1;
}

static int CCommand_TradeGold(const char*, const char* arguments) {
  CDataStore msg;
  msg.Put(CMSG_SET_TRADE_GOLD);
  msg.Put(arguments ? SStrToInt(arguments) : 0);
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
}

static int CCommand_UnacceptTrade(const char*, const char*) {
  Trade_C_UnacceptTrade();
  return 1;
}

static int TradeStatusHandler(void*, NETMESSAGE, unsigned long, CDataStore* netmsg) {
  unsigned int status;
  netmsg->Get(status);
  BAG_RESULT bagResult = BAG_OK;
  unsigned char myFailure = 0;
  int itemID = 0;
  const char *message = 0;
  char buffer[256];
  char messageBuffer[100];
  int clearTrade = 0;

  switch (status) {
    case 0:
      SStrCopy(buffer, FrameScript_GetText("PLAYER_BUSY", -1, GENDER_NOT_APPLICABLE), sizeof(buffer));
      if (s_initiator) message = buffer;
      clearTrade = 1;
      break;
    case 1:
      netmsg->Get(s_tradePartner);
      break;
    case 2: {
      CGUnit_C *partner = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(s_tradePartner, __FILE__, __LINE__));
      SStrCopy(buffer, FrameScript_GetText("TRADE_INITIATED", -1, GENDER_NOT_APPLICABLE), sizeof(buffer));
      SStrPrintf(messageBuffer, sizeof(messageBuffer), buffer, partner ? partner->GetUnitName() : "???");
      message = messageBuffer;
      s_tradeFlags[0] = s_tradeFlags[1] = 1;
      memset(s_tradeItems, 0, sizeof(s_tradeItems));
      s_tradeGold[0] = s_tradeGold[1] = 0;
      s_tradeProposedEnchantment[0] = s_tradeProposedEnchantment[1] = 0;
      s_tradeProposedEnchantmentSlot[0] = s_tradeProposedEnchantmentSlot[1] = 0;
      break;
    }
    case 3:
    case 14:
      SStrCopy(buffer, FrameScript_GetText("TRADE_CANCELLED", -1, GENDER_NOT_APPLICABLE), sizeof(buffer));
      message = buffer;
      clearTrade = 1;
      memset(s_tradeItems, 0, sizeof(s_tradeItems));
      s_tradeGold[0] = s_tradeGold[1] = 0;
      break;
    case 4: SStrCopy(buffer, FrameScript_GetText("TRADE_ACCEPTED", -1, GENDER_NOT_APPLICABLE), sizeof(buffer)); message = buffer; break;
    case 5: SStrCopy(buffer, FrameScript_GetText("ALREADY_TRADING", -1, GENDER_NOT_APPLICABLE), sizeof(buffer)); message = buffer; clearTrade = 1; break;
    case 6: SStrCopy(buffer, FrameScript_GetText("PLAYER_NOT_FOUND", -1, GENDER_NOT_APPLICABLE), sizeof(buffer)); message = buffer; clearTrade = 1; break;
    case 7: SStrCopy(buffer, FrameScript_GetText("TRADE_STATE_CHANGED", -1, GENDER_NOT_APPLICABLE), sizeof(buffer)); message = buffer; break;
    case 8:
      SStrCopy(buffer, FrameScript_GetText("TRADE_COMPLETE", -1, GENDER_NOT_APPLICABLE), sizeof(buffer));
      message = buffer;
      clearTrade = 1;
      memset(s_tradeItems, 0, sizeof(s_tradeItems));
      s_tradeGold[0] = s_tradeGold[1] = 0;
      break;
    case 9: SStrCopy(buffer, FrameScript_GetText("TRADE_UNACCEPTED", -1, GENDER_NOT_APPLICABLE), sizeof(buffer)); message = buffer; break;
    case 10: SStrCopy(buffer, FrameScript_GetText("TOO_FAR_TO_TRADE", -1, GENDER_NOT_APPLICABLE), sizeof(buffer)); message = buffer; clearTrade = 1; break;
    case 12:
      netmsg->Get(reinterpret_cast<int &>(bagResult));
      netmsg->Get(myFailure);
      netmsg->Get(itemID);
      SStrCopy(buffer, FrameScript_GetText("TRADE_FAILED", -1, GENDER_NOT_APPLICABLE), sizeof(buffer));
      message = buffer;
      clearTrade = 1;
      memset(s_tradeItems, 0, sizeof(s_tradeItems));
      s_tradeGold[0] = s_tradeGold[1] = 0;
      break;
    case 13: SStrCopy(buffer, FrameScript_GetText("TRADE_TARGET_DEAD", -1, GENDER_NOT_APPLICABLE), sizeof(buffer)); message = buffer; clearTrade = 1; break;
    case 15: {
      CGUnit_C *partner = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(s_tradePartner, __FILE__, __LINE__));
      if (partner) {
        SStrPrintf(messageBuffer, sizeof(messageBuffer),
                   FrameScript_GetText("ERR_IGNORING_YOU_S", -1, GENDER_NOT_APPLICABLE), partner->GetUnitName());
        message = messageBuffer;
      }
      clearTrade = 1;
      break;
    }
    default:
      SStrCopy(buffer, FrameScript_GetText("TRADE_STATUS_UNKNOWN", -1, GENDER_NOT_APPLICABLE), sizeof(buffer));
      message = buffer;
      break;
  }

  CGTradeInfo::HandleTradeMessage(static_cast<TRADE_STATUS>(status), bagResult, myFailure != 0, itemID);
  if (clearTrade) {
    s_initiator = 0;
    s_tradePartner = 0;
  }
  if (message) {
    ConsoleWriteA(message, DEFAULT_COLOR);
  }
  return 1;
}

static int TradeExtendedStatusHandler(void *, NETMESSAGE, unsigned long, CDataStore *msg) {
  unsigned char whichPlayer;
  msg->Get(whichPlayer);
  FATALASSERT(whichPlayer < 2);

  msg->Get(s_tradeFlags[whichPlayer]);
  msg->Get(s_tradeGold[whichPlayer]);
  memset(s_tradeItems[whichPlayer], 0, sizeof(s_tradeItems[whichPlayer]));
  msg->Get(s_tradeProposedEnchantment[whichPlayer]);
  msg->Get(s_tradeProposedEnchantmentSlot[whichPlayer]);

  while (!msg->IsRead()) {
    unsigned char slot;
    msg->Get(slot);
    TradeItemData &item = s_tradeItems[whichPlayer][slot];
    msg->Get(item.entryID);
    msg->Get(item.displayID);
    msg->Get(item.count);
    msg->Get(item.enchantmentID);
    msg->Get(item.creator);
  }
  FATALASSERT(msg->IsRead() && msg->IsValid());
  CGTradeInfo::Update(s_tradeItems[1]);
  return 1;
}

unsigned __int64 Trade_C_GetTradeTarget() {
  return s_tradePartner;
}

int Trade_C_IsInitiator() {
  return s_initiator;
}

int Trade_C_UseCursorItem() {
  return s_initiator && s_useCursorItem;
}

int Trade_C_GetProposedEnchantment(unsigned int player, int &spellID, int &slot) {
  int enchantment = s_tradeProposedEnchantment[player];
  if (enchantment <= 0) {
    return 0;
  }
  spellID = enchantment;
  slot = s_tradeProposedEnchantmentSlot[player];
  return 1;
}

void TradeNameCallback(int, const unsigned __int64 &guid, void *, bool granted) {
  if (granted) {
    const NameCache *name = g_nameDBCache.GetRecord(guid, guid, 0, 0);
    if (name) {
      CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(169), name->m_name);
    }
  }
}

void Trade_C_InitiateTrade(unsigned __int64 target, int useCursorItem) {
  CDataStore msg;
  msg.Put(CMSG_INITIATE_TRADE);
  msg.Put(target);
  msg.Finalize();
  ClientServices_Send(&msg);

  s_initiator = 1;
  s_useCursorItem = useCursorItem;
  const NameCache *name = g_nameDBCache.GetRecord(target, target, TradeNameCallback, 0);
  if (name) {
    CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(169), name->m_name);
  }
}

void Trade_C_BeginTrade() {
  CDataStore msg;
  msg.Put(CMSG_BEGIN_TRADE);
  msg.Finalize();
  ClientServices_Send(&msg);
}

void Trade_C_PlayerBusy() {
  CDataStore msg;
  msg.Put(CMSG_BUSY_TRADE);
  msg.Finalize();
  ClientServices_Send(&msg);
}

void Trade_C_PlayerIgnored() {
  CDataStore msg;
  msg.Put(CMSG_IGNORE_TRADE);
  msg.Finalize();
  ClientServices_Send(&msg);
}

void Trade_C_AcceptTrade() {
  CDataStore msg;
  msg.Put(CMSG_ACCEPT_TRADE);
  msg.Put(s_tradeFlags[1]);
  msg.Finalize();
  ClientServices_Send(&msg);
}

void Trade_C_UnacceptTrade() {
  CDataStore msg;
  msg.Put(CMSG_UNACCEPT_TRADE);
  msg.Finalize();
  ClientServices_Send(&msg);
}

void Trade_C_CancelTrade() {
  CDataStore msg;
  msg.Put(CMSG_CANCEL_TRADE);
  msg.Finalize();
  ClientServices_Send(&msg);
}

bool Trade_C_AddItem(unsigned __int64 item, unsigned __int64 itemContainer, unsigned int itemSlot, unsigned int tradeSlot) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player) {
    return false;
  }

  CGItem_C *itemPtr = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(item, __FILE__, __LINE__));
  if (!itemPtr || !itemPtr->IsUnlocked()) {
    return false;
  }

  unsigned int itemContainerIndex = player->FindSlotIndex(itemContainer);
  CDataStore   msg;
  msg.Put(CMSG_SET_TRADE_ITEM);
  msg.Put(static_cast<unsigned char>(tradeSlot));
  msg.Put(static_cast<unsigned char>(itemContainerIndex));
  msg.Put(static_cast<unsigned char>(itemSlot));
  msg.Finalize();
  ClientServices_Send(&msg);
  return true;
}

void Trade_C_RemoveItem(unsigned int slot) {
  CDataStore msg;
  msg.Put(CMSG_CLEAR_TRADE_ITEM);
  msg.Put(static_cast<unsigned char>(slot));
  msg.Finalize();
  ClientServices_Send(&msg);
}

void Trade_C_AddMoney(unsigned int money) {
  if (money) {
    CDataStore msg;
    msg.Put(CMSG_SET_TRADE_GOLD);
    msg.Put(s_tradeGold[0] + money);
    msg.Finalize();
    ClientServices_Send(&msg);
  }
}

void Trade_C_RemoveMoney(unsigned int money) {
  if (money && money <= s_tradeGold[0]) {
    CDataStore msg;
    msg.Put(CMSG_SET_TRADE_GOLD);
    msg.Put(s_tradeGold[0] - money);
    msg.Finalize();
    ClientServices_Send(&msg);
  }
}

unsigned int Trade_C_GetPlayerTradeGold() {
  return s_tradeGold[0];
}

unsigned int Trade_C_GetTargetTradeGold() {
  return s_tradeGold[1];
}

void Trade_C_Initialize() {
  memset(s_tradeItems, 0, sizeof(s_tradeItems));
  memset(s_tradeProposedEnchantment, 0, sizeof(s_tradeProposedEnchantment));
  memset(s_tradeProposedEnchantmentSlot, 0, sizeof(s_tradeProposedEnchantmentSlot));
  memset(s_tradeGold, 0, sizeof(s_tradeGold));
  memset(s_tradeFlags, 0, sizeof(s_tradeFlags));
  ConsoleCommandRegister("trade", CCommand_Trade, GAME, 0);
  ConsoleCommandRegister("addtradeitem", CCommand_AddTradeItem, GAME, 0);
  ConsoleCommandRegister("cleartradeitem", CCommand_ClearTradeItem, GAME, 0);
  ConsoleCommandRegister("cleartrade", CCommand_ClearTrade, GAME, 0);
  ConsoleCommandRegister("accepttrade", CCommand_AcceptTrade, GAME, 0);
  ConsoleCommandRegister("canceltrade", CCommand_CancelTrade, GAME, 0);
  ConsoleCommandRegister("showtrade", CCommand_ShowTrade, GAME, 0);
  ConsoleCommandRegister("tradegold", CCommand_TradeGold, GAME, 0);
  ConsoleCommandRegister("unaccepttrade", CCommand_UnacceptTrade, GAME, 0);
  ClientServices_SetMessageHandler(SMSG_TRADE_STATUS, TradeStatusHandler, 0);
  ClientServices_SetMessageHandler(SMSG_TRADE_STATUS_EXTENDED, TradeExtendedStatusHandler, 0);
}

void Trade_C_Destroy() {
  ConsoleCommandUnregister("trade");
  ConsoleCommandUnregister("addtradeitem");
  ConsoleCommandUnregister("cleartradeitem");
  ConsoleCommandUnregister("cleartrade");
  ConsoleCommandUnregister("accepttrade");
  ConsoleCommandUnregister("canceltrade");
  ConsoleCommandUnregister("showtrade");
  ConsoleCommandUnregister("tradegold");
  ConsoleCommandUnregister("unaccepttrade");

  ClientServices_ClearMessageHandler(SMSG_TRADE_STATUS);
  ClientServices_ClearMessageHandler(SMSG_TRADE_STATUS_EXTENDED);
}
