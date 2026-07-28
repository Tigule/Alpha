#ifndef WOW_SOURCE_UI_TRADEFRAME_H
#define WOW_SOURCE_UI_TRADEFRAME_H

#include "Ui/GameUI.h"

#include "Object/ObjectClient/Bag_C.h"

struct TradeItemData {
  unsigned int     entryID;
  unsigned int     displayID;
  unsigned int     count;
  unsigned int     enchantmentID;
  unsigned __int64 creator;
};

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
  static void EnterWorld();
  static void LeaveWorld();
  static unsigned __int64 GetTradePartner() {
    return m_tradingPlayer;
  }
  static void SetTradePartner(unsigned __int64 partner);
  static void Update(TradeItemData *items);
  static void PlayerAccept(int accept);
  static void TargetAccept(int accept);
  static void ClearAccept();
  static void HandleTradeMessage(TRADE_STATUS status, BAG_RESULT bagResult, int myFailure, int itemID);
  static int SetPlayerItem(int index, unsigned __int64 guid, unsigned __int64 bag, unsigned char slot);
  static unsigned __int64 GetPlayerTradeSlot(int index);
  static void RemovePlayerItem(unsigned __int64 guid);
  static void UpdatePlayerItem(unsigned __int64 guid);
  static void UnlockTradeItems();
  static GAME_ERROR_TYPE GetGameError(BAG_RESULT bagResult, int myFailure);
  static void GetPlayerItemInfo(int index, unsigned __int64 &guid, unsigned __int64 &bag, unsigned char &slot) {
    if (index >= 0 && index < 8) {
      guid = m_playerItems[index];
      bag = m_playerItemBag[index];
      slot = m_playerItemSlot[index];
    } else {
      guid = 0;
      bag = 0;
      slot = 0;
    }
  }
  static int GetTargetTradeItem(int index) {
    return index >= 0 && index < 8 ? m_targetItems[index] : 0;
  }
  static int GetTargetTradeItemCount(int index) {
    return index >= 0 && index < 8 ? m_targetItemCount[index] : 0;
  }
  static int GetTargetTradeItemEnachantment(int index) {
    return index >= 0 && index < 8 ? m_targetItemEnchantment[index] : 0;
  }
  static unsigned __int64 GetTargetTradeItemCreator(int index) {
    return index >= 0 && index < 8 ? m_targetItemCreator[index] : 0;
  }
  static int GetPlayerEnchantSlot() {
    return m_playerEnchantSlot;
  }
  static int GetTargetEnchantSlot() {
    return m_targetEnchantSlot;
  }

 protected:
  static unsigned __int64 m_tradingPlayer;
  static int              m_playerAccepted;
  static int              m_targetAccepted;
  static unsigned __int64 m_playerItems[8];
  static unsigned __int64 m_playerItemBag[8];
  static unsigned char    m_playerItemSlot[8];
  static int              m_targetItems[8];
  static int              m_targetItemCount[8];
  static int              m_targetItemEnchantment[8];
  static unsigned __int64 m_targetItemCreator[8];
  static int              m_playerEnchantSlot;
  static int              m_targetEnchantSlot;
  static unsigned int     m_playerMoney;
  static unsigned int     m_targetMoney;
};

#endif
