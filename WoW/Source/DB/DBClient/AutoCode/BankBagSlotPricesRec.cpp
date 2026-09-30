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

  if (!SFileReadTyped(f, &m_ID) ||
      !SFileReadTyped(f, &m_Cost)) {
    ConsoleWrite("Error reading BankBagSlotPricesRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
