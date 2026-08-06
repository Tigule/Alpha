#include <Base/Base.h>
#include <WowConst.h>
#include <MapDefs.h>

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

static int       s_initiator;
static int       s_useCursorItem;
static DWORDLONG s_tradePartner;
static int       s_tradeProposedEnchantment[2];
static int       s_tradeProposedEnchantmentSlot[2];
static UINT      s_tradeGold[2];
static UINT      s_tradeFlags[2];

struct TradeItemData {
  UINT      entryID;
  UINT      displayID;
  UINT      count;
  UINT      enchantmentID;
  DWORDLONG creator;
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

void Trade_C_InitiateTrade(DWORDLONG target, int useCursorItem);
void Trade_C_AcceptTrade();
void Trade_C_UnacceptTrade();
void Trade_C_CancelTrade();
bool Trade_C_AddItem(DWORDLONG item, DWORDLONG itemContainer, UINT itemSlot, UINT tradeSlot);
void Trade_C_RemoveItem(UINT slot);

static int CCommand_Trade(LPCSTR command, LPCSTR arguments) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    Trade_C_InitiateTrade(player->GetLocalTarget(), 0);
  }
  return 1;
}

static int CCommand_AddTradeItem(LPCSTR command, LPCSTR arguments) {
  DWORDLONG cursorItem;
  DWORDLONG cursorItemContainer;
  UINT      cursorItemSlot;
  CGGameUI::GetCursorItem(cursorItem, cursorItemContainer, cursorItemSlot);
  UINT tradeSlot = arguments && *arguments ? static_cast<BYTE>(SStrToInt(arguments)) : 0;
  Trade_C_AddItem(cursorItem, cursorItemContainer, cursorItemSlot, tradeSlot);
  return 1;
}

static int CCommand_ClearTradeItem(LPCSTR command, LPCSTR arguments) {
  if (ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__)) {
    Trade_C_RemoveItem(arguments && *arguments ? SStrToInt(arguments) : 0);
  }
  return 1;
}

static int CCommand_ClearTrade(LPCSTR command, LPCSTR arguments) {
  if (ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__)) {
    CDataStore msg;
    msg.Put(static_cast<UINT>(CMSG_CLEAR_TRADE_ITEM));
    msg.Put(0xFFu);
    msg.Finalize();
    ClientServices_Send(&msg);
  }
  return 1;
}

static int CCommand_AcceptTrade(LPCSTR command, LPCSTR arguments) {
  if (ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__)) {
    Trade_C_AcceptTrade();
  }
  return 1;
}

static int CCommand_CancelTrade(LPCSTR command, LPCSTR arguments) {
  if (ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__)) {
    Trade_C_CancelTrade();
  }
  return 1;
}

static int CCommand_ShowTrade(LPCSTR, LPCSTR) {
  for (BYTE player = 0; player < 2; ++player) {
    ConsoleWrite(player ? "He is offering:" : "You are offering:", DEFAULT_COLOR);
    for (UINT slot = 0; slot < 8; ++slot) {
      ConsolePrintf("%d: item=%d, display=%d", slot, s_tradeItems[player][slot].entryID, s_tradeItems[player][slot].displayID);
    }
  }
  return 1;
}

static int CCommand_TradeGold(LPCSTR, LPCSTR arguments) {
  CDataStore msg;
  msg.Put(CMSG_SET_TRADE_GOLD);
  msg.Put(arguments ? SStrToInt(arguments) : 0);
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
}

static int CCommand_UnacceptTrade(LPCSTR, LPCSTR) {
  Trade_C_UnacceptTrade();
  return 1;
}

static int TradeStatusHandler(LPVOID, NETMESSAGE, DWORD, CDataStore *netmsg) {
  UINT statusint;
  netmsg->Get(statusint);
  TRADE_STATUS status = static_cast<TRADE_STATUS>(statusint);
  BAG_RESULT   bagResult = BAG_OK;
  BYTE         myFailure = 0;
  int          itemID = 0;
  LPCSTR       message = 0;
  char         buf[256];
  char         msgbuf[100];
  DWORDLONG    guid;
  int          clearTrade = 0;

  switch (statusint) {
    case 0:
      SStrCopy(buf, FrameScript_GetText("PLAYER_BUSY", -1, GENDER_NOT_APPLICABLE), sizeof(buf));
      if (s_initiator)
        message = buf;
      clearTrade = 1;
      break;
    case 1:
      netmsg->Get(guid);
      s_tradePartner = guid;
      break;
    case 2: {
      CGUnit_C *partner = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(s_tradePartner, __FILE__, __LINE__));
      SStrCopy(buf, FrameScript_GetText("TRADE_INITIATED", -1, GENDER_NOT_APPLICABLE), sizeof(buf));
      SStrPrintf(msgbuf, sizeof(msgbuf), buf, partner ? partner->GetUnitName() : "???");
      message = msgbuf;
      s_tradeFlags[0] = s_tradeFlags[1] = 1;
      memset(s_tradeItems, 0, sizeof(s_tradeItems));
      s_tradeGold[0] = s_tradeGold[1] = 0;
      s_tradeProposedEnchantment[0] = s_tradeProposedEnchantment[1] = 0;
      s_tradeProposedEnchantmentSlot[0] = s_tradeProposedEnchantmentSlot[1] = 0;
      break;
    }
    case 3:
    case 14:
      SStrCopy(buf, FrameScript_GetText("TRADE_CANCELLED", -1, GENDER_NOT_APPLICABLE), sizeof(buf));
      message = buf;
      clearTrade = 1;
      memset(s_tradeItems, 0, sizeof(s_tradeItems));
      s_tradeGold[0] = s_tradeGold[1] = 0;
      break;
    case 4:
      SStrCopy(buf, FrameScript_GetText("TRADE_ACCEPTED", -1, GENDER_NOT_APPLICABLE), sizeof(buf));
      message = buf;
      break;
    case 5:
      SStrCopy(buf, FrameScript_GetText("ALREADY_TRADING", -1, GENDER_NOT_APPLICABLE), sizeof(buf));
      message = buf;
      clearTrade = 1;
      break;
    case 6:
      SStrCopy(buf, FrameScript_GetText("PLAYER_NOT_FOUND", -1, GENDER_NOT_APPLICABLE), sizeof(buf));
      message = buf;
      clearTrade = 1;
      break;
    case 7:
      SStrCopy(buf, FrameScript_GetText("TRADE_STATE_CHANGED", -1, GENDER_NOT_APPLICABLE), sizeof(buf));
      message = buf;
      break;
    case 8:
      SStrCopy(buf, FrameScript_GetText("TRADE_COMPLETE", -1, GENDER_NOT_APPLICABLE), sizeof(buf));
      message = buf;
      clearTrade = 1;
      memset(s_tradeItems, 0, sizeof(s_tradeItems));
      s_tradeGold[0] = s_tradeGold[1] = 0;
      break;
    case 9:
      SStrCopy(buf, FrameScript_GetText("TRADE_UNACCEPTED", -1, GENDER_NOT_APPLICABLE), sizeof(buf));
      message = buf;
      break;
    case 10:
      SStrCopy(buf, FrameScript_GetText("TOO_FAR_TO_TRADE", -1, GENDER_NOT_APPLICABLE), sizeof(buf));
      message = buf;
      clearTrade = 1;
      break;
    case 12:
      netmsg->Get(reinterpret_cast<int &>(bagResult));
      netmsg->Get(myFailure);
      netmsg->Get(itemID);
      SStrCopy(buf, FrameScript_GetText("TRADE_FAILED", -1, GENDER_NOT_APPLICABLE), sizeof(buf));
      message = buf;
      clearTrade = 1;
      memset(s_tradeItems, 0, sizeof(s_tradeItems));
      s_tradeGold[0] = s_tradeGold[1] = 0;
      break;
    case 13:
      SStrCopy(buf, FrameScript_GetText("TRADE_TARGET_DEAD", -1, GENDER_NOT_APPLICABLE), sizeof(buf));
      message = buf;
      clearTrade = 1;
      break;
    case 15: {
      CGUnit_C *partner = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(s_tradePartner, __FILE__, __LINE__));
      if (partner) {
        SStrPrintf(msgbuf, sizeof(msgbuf), FrameScript_GetText("ERR_IGNORING_YOU_S", -1, GENDER_NOT_APPLICABLE), partner->GetUnitName());
        message = msgbuf;
      }
      clearTrade = 1;
      break;
    }
    default:
      SStrCopy(buf, FrameScript_GetText("TRADE_STATUS_UNKNOWN", -1, GENDER_NOT_APPLICABLE), sizeof(buf));
      message = buf;
      break;
  }

  CGTradeInfo::HandleTradeMessage(status, bagResult, myFailure != 0, itemID);
  if (clearTrade) {
    s_initiator = 0;
    s_tradePartner = 0;
  }
  if (message) {
    ConsoleWriteA(message, DEFAULT_COLOR);
  }
  return 1;
}

static int TradeExtendedStatusHandler(LPVOID, NETMESSAGE, DWORD, CDataStore *msg) {
  BYTE whichPlayer;
  msg->Get(whichPlayer);
  FATALASSERT(whichPlayer < 2);

  msg->Get(s_tradeFlags[whichPlayer]);
  msg->Get(s_tradeGold[whichPlayer]);
  memset(s_tradeItems[whichPlayer], 0, sizeof(s_tradeItems[whichPlayer]));
  msg->Get(s_tradeProposedEnchantment[whichPlayer]);
  msg->Get(s_tradeProposedEnchantmentSlot[whichPlayer]);

  while (!msg->IsRead()) {
    BYTE index;
    msg->Get(index);
    TradeItemData &item = s_tradeItems[whichPlayer][index];
    msg->Get(item.entryID);
    msg->Get(item.displayID);
    msg->Get(item.count);
    msg->Get(item.enchantmentID);
    msg->Get(item.creator);
  }
  FATALASSERT(msg->IsRead() && msg->IsValid());
  CGTradeInfo::Update(s_tradeItems[1]);
  char buf[128];
  SStrCopy(buf, FrameScript_GetText("TRADE_ITEMS_MODIFIED", -1, GENDER_NOT_APPLICABLE), sizeof(buf));
  ConsoleWriteA(buf, DEFAULT_COLOR);
  return 1;
}

DWORDLONG Trade_C_GetTradeTarget() {
  return s_tradePartner;
}

int Trade_C_IsInitiator() {
  return s_initiator;
}

int Trade_C_UseCursorItem() {
  return s_initiator && s_useCursorItem;
}

int Trade_C_GetProposedEnchantment(UINT player, int &spellID, int &slot) {
  int enchantment = s_tradeProposedEnchantment[player];
  if (enchantment <= 0) {
    return 0;
  }
  spellID = enchantment;
  slot = s_tradeProposedEnchantmentSlot[player];
  return 1;
}

void TradeNameCallback(int id, const DWORDLONG &guid, LPVOID, bool granted) {
  if (granted) {
    const NameCache *name = g_nameDBCache.GetRecord(guid, guid, 0, 0);
    if (name) {
      CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(169), name->m_name);
    }
  }
}

void Trade_C_InitiateTrade(DWORDLONG target, int useCursorItem) {
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

bool Trade_C_AddItem(DWORDLONG item, DWORDLONG itemContainer, UINT itemSlot, UINT tradeSlot) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player) {
    return false;
  }

  CGItem_C *itemPtr = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(item, __FILE__, __LINE__));
  if (!itemPtr || !itemPtr->IsUnlocked()) {
    return false;
  }

  UINT       itemContainerIndex = player->FindSlotIndex(itemContainer);
  CDataStore msg;
  msg.Put(CMSG_SET_TRADE_ITEM);
  msg.Put(static_cast<BYTE>(tradeSlot));
  msg.Put(static_cast<BYTE>(itemContainerIndex));
  msg.Put(static_cast<BYTE>(itemSlot));
  msg.Finalize();
  ClientServices_Send(&msg);
  return true;
}

void Trade_C_RemoveItem(UINT slot) {
  CDataStore msg;
  msg.Put(CMSG_CLEAR_TRADE_ITEM);
  msg.Put(static_cast<BYTE>(slot));
  msg.Finalize();
  ClientServices_Send(&msg);
}

void Trade_C_AddMoney(UINT money) {
  if (money) {
    CDataStore msg;
    msg.Put(CMSG_SET_TRADE_GOLD);
    msg.Put(s_tradeGold[0] + money);
    msg.Finalize();
    ClientServices_Send(&msg);
  }
}

void Trade_C_RemoveMoney(UINT money) {
  if (money && money <= s_tradeGold[0]) {
    CDataStore msg;
    msg.Put(CMSG_SET_TRADE_GOLD);
    msg.Put(s_tradeGold[0] - money);
    msg.Finalize();
    ClientServices_Send(&msg);
  }
}

UINT Trade_C_GetPlayerTradeGold() {
  return s_tradeGold[0];
}

UINT Trade_C_GetTargetTradeGold() {
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
