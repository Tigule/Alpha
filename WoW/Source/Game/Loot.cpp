#include <storm.h>

static int          s_currencyMultiplier[3];
static unsigned int s_lootInitialized;

int CurrencyMultiplier(int denomination) {
  FATALASSERT(s_lootInitialized);
  FATALASSERT(denomination >= 0 && denomination < 3);
  return s_currencyMultiplier[denomination];
}

const char *CurrencyAbbreviation(int coinType) {
  switch (coinType) {
    case 0:
      return "COPPER";
    case 1:
      return "SILVER";
    case 2:
      return "GOLD";
    default:
      return "ERROR_CAPS";
  }
}

void CurrencyBreakdown(int money, int *coins) {
  FATALASSERT(s_lootInitialized);
  FATALASSERT(coins);

  for (int coinType = 2; coinType > 0; --coinType) {
    int multiplier = CurrencyMultiplier(coinType);
    if (money < multiplier) {
      coins[coinType] = 0;
    } else {
      coins[coinType] = money / multiplier;
      money %= multiplier;
    }
  }
  coins[0] = money;
}

unsigned int CurrencyTotal(int *const coins) {
  FATALASSERT(s_lootInitialized);
  FATALASSERT(coins);

  unsigned int total = 0;
  for (int coinType = 2; coinType >= 0; --coinType) {
    total += coins[coinType] * CurrencyMultiplier(coinType);
  }
  return total;
}

void LootInitialize() {
  s_currencyMultiplier[0] = 1;
  for (int denomination = 1; denomination < 3; ++denomination) {
    s_currencyMultiplier[denomination] = 100 * s_currencyMultiplier[denomination - 1];
  }

  s_lootInitialized = true;
}

void LootDestroy() {
  s_lootInitialized = false;
}
