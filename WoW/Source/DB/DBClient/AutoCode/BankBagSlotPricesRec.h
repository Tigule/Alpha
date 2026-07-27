#pragma once

#include <DB/WowClientDB.h>

class BankBagSlotPricesRec {
 public:
  BankBagSlotPricesRec();
  ~BankBagSlotPricesRec();

  static const char *GetFilename();

  static unsigned int GetNumColumns() {
    return 2;
  }

  static unsigned int GetRowSize() {
    return 8;
  }

  int GetID() const {
    return m_ID;
  }

  bool NeedIDAssigned() {
    return false;
  }

  void SetID(int id) {
  }

  bool Read(SFile *f, const char *stringBuffer);

  int m_ID;
  int m_Cost;
};

extern WowClientDB<BankBagSlotPricesRec> g_bankBagSlotPricesDB;
