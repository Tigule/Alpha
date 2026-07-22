#include "Base/CDataStore.h"
#include "Console/ConsoleCommand.h"
#include "DB/DBClient/DBCacheInstances.h"
#include "Object/ObjectClient/Item_C.h"
#include "Object/ObjectClient/Player_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "Ui/GameUI.h"
#include "WowSvcs/WowSvcsClient/ClientServices.h"

static int              s_initiator;
static int              s_useCursorItem;
static unsigned __int64 s_tradePartner;
static int              s_tradeProposedEnchantment[2];
static int              s_tradeProposedEnchantmentSlot[2];
static unsigned int     s_tradeGold[2];
static unsigned int     s_tradeFlags[2];

struct TradeItemData {
  int              itemID;
  int              displayID;
  int              enchantment;
  int              count;
  unsigned __int64 creator;
};

static TradeItemData s_tradeItems[2][8];

class CGTradeInfo {
 public:
  static void __fastcall Update(TradeItemData *items);
};

int __fastcall Trade_C_GetProposedEnchantment(unsigned int player, int &spellID, int &slot) {
  int enchantment = s_tradeProposedEnchantment[player];
  if (enchantment <= 0) {
    return 0;
  }
  spellID = enchantment;
  slot = s_tradeProposedEnchantmentSlot[player];
  return 1;
}

unsigned int __fastcall Trade_C_GetPlayerTradeGold() {
  return s_tradeGold[0];
}

unsigned int __fastcall Trade_C_GetTargetTradeGold() {
  return s_tradeGold[1];
}

static int __fastcall TradeExtendedStatusHandler(void *, NETMESSAGE, unsigned long, CDataStore *msg) {
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
    msg->Get(item.itemID);
    msg->Get(item.displayID);
    msg->Get(item.enchantment);
    msg->Get(item.count);
    msg->Get(item.creator);
  }
  FATALASSERT(msg->IsRead() && msg->IsValid());
  CGTradeInfo::Update(s_tradeItems[1]);
  return 1;
}

void __fastcall Trade_C_Initialize() {
  memset(s_tradeItems, 0, sizeof(s_tradeItems));
  memset(s_tradeProposedEnchantment, 0, sizeof(s_tradeProposedEnchantment));
  memset(s_tradeProposedEnchantmentSlot, 0, sizeof(s_tradeProposedEnchantmentSlot));
  memset(s_tradeGold, 0, sizeof(s_tradeGold));
  memset(s_tradeFlags, 0, sizeof(s_tradeFlags));
  ClientServices_SetMessageHandler(SMSG_TRADE_STATUS_EXTENDED, TradeExtendedStatusHandler, 0);
}

void __fastcall TradeNameCallback(int, const unsigned __int64 &guid, void *, bool granted) {
  if (granted) {
    const NameCache *name = g_nameDBCache.GetRecord(guid, guid, 0, 0);
    if (name) {
      CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(169), name->m_name);
    }
  }
}

int __fastcall Trade_C_UseCursorItem() {
  return s_initiator && s_useCursorItem;
}

void __fastcall Trade_C_InitiateTrade(unsigned __int64 target, int useCursorItem) {
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

void __fastcall Trade_C_BeginTrade() {
  CDataStore msg;
  msg.Put(CMSG_BEGIN_TRADE);
  msg.Finalize();
  ClientServices_Send(&msg);
}

void __fastcall Trade_C_CancelTrade() {
  CDataStore msg;
  msg.Put(CMSG_CANCEL_TRADE);
  msg.Finalize();
  ClientServices_Send(&msg);
}

void __fastcall Trade_C_AcceptTrade() {
  CDataStore msg;
  msg.Put(CMSG_ACCEPT_TRADE);
  msg.Put(s_tradeFlags[1]);
  msg.Finalize();
  ClientServices_Send(&msg);
}

void __fastcall Trade_C_UnacceptTrade() {
  CDataStore msg;
  msg.Put(CMSG_UNACCEPT_TRADE);
  msg.Finalize();
  ClientServices_Send(&msg);
}

bool __fastcall Trade_C_AddItem(unsigned __int64 item, unsigned __int64 itemContainer, unsigned int itemSlot, unsigned int tradeSlot) {
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

void __fastcall Trade_C_RemoveItem(unsigned int slot) {
  CDataStore msg;
  msg.Put(CMSG_CLEAR_TRADE_ITEM);
  msg.Put(static_cast<unsigned char>(slot));
  msg.Finalize();
  ClientServices_Send(&msg);
}

void __fastcall Trade_C_AddMoney(unsigned int money) {
  if (money) {
    CDataStore msg;
    msg.Put(CMSG_SET_TRADE_GOLD);
    msg.Put(s_tradeGold[0] + money);
    msg.Finalize();
    ClientServices_Send(&msg);
  }
}

void __fastcall Trade_C_RemoveMoney(unsigned int money) {
  if (money && money <= s_tradeGold[0]) {
    CDataStore msg;
    msg.Put(CMSG_SET_TRADE_GOLD);
    msg.Put(s_tradeGold[0] - money);
    msg.Finalize();
    ClientServices_Send(&msg);
  }
}

void __fastcall Trade_C_Destroy() {
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
