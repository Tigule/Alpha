#include "BankBagSlotPricesRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR BankBagSlotPricesRec::GetFilename() {
  return "DBFilesClient\\BankBagSlotPrices.dbc";
}

BankBagSlotPricesRec::BankBagSlotPricesRec() {
}

BankBagSlotPricesRec::~BankBagSlotPricesRec() {
}

bool BankBagSlotPricesRec::Read(SFile *f, LPCSTR stringBuffer) {
  int error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &m_Cost) == 0);

  if (error) {
    ConsoleWrite("Error reading BankBagSlotPricesRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
