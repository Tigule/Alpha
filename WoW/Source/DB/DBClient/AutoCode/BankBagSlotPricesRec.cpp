#include "BankBagSlotPricesRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

const char *__fastcall BankBagSlotPricesRec::GetFilename() {
  return "DBFilesClient\\BankBagSlotPrices.dbc";
}

BankBagSlotPricesRec::BankBagSlotPricesRec() {
}

BankBagSlotPricesRec::~BankBagSlotPricesRec() {
}

bool BankBagSlotPricesRec::Read(SFile *f, const char *stringBuffer) {
  bool result = true;

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_Cost, sizeof(m_Cost), 0, 0, 0) && result;

  if (!result) {
    ConsoleWrite("Error reading BankBagSlotPricesRec", DEFAULT_COLOR);
  }

  return result;
}
