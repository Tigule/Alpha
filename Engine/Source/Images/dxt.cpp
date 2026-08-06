#include <Base/Base.h>

#include "dxt.h"

DxtColorBlock::Tables DxtColorBlock::tables;

DxtColorBlock::Tables::Tables() {
  UINT i;
  for (i = 0; i < 32; ++i) {
    dt135[i] = static_cast<WORD>((i << 8) / 3 + 1);
    dt235[i] = static_cast<WORD>((i << 9) / 3 + 1);
  }

  for (i = 0; i < 64; ++i) {
    dt136[i] = static_cast<WORD>((i << 8) / 3 + 1);
    dt236[i] = static_cast<WORD>((i << 9) / 3 + 1);
  }
}
