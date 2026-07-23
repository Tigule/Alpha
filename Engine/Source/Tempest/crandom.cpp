#include "crandom.h"

#include "c3vector.h"

namespace NTempest {

  extern const unsigned long gnoise32_[64] = {
      0x9927148EUL, 0x08C7AAFDUL, 0x1F3EE6D5UL, 0xDA55BBF6UL, 0x6A4AA075UL, 0xFF97BDE8UL, 0x9FBC9BDEUL, 0x46A18A81UL, 0x63E30B6EUL, 0x5D6C7A76UL,
      0xCA69D388UL, 0x25B947C3UL, 0x3FA2AB83UL, 0xBA7C41A6UL, 0x0195ACE5UL, 0xC109CF7EUL, 0x717062D9UL, 0x0205DB8DUL, 0x54EF8724UL, 0x3037D4C6UL,
      0x7BCB1BD0UL, 0xECD8E4B8UL, 0xDCADCE49UL, 0xC494A913UL, 0x0DAE398FUL, 0x0EDD5218UL, 0x85F5FA78UL, 0x6DAFD258UL, 0x3B53B2A4UL, 0xBE50A551UL,
      0x11F42DFCUL, 0xF1169848UL, 0x663DDF86UL, 0x2F2E445EUL, 0x176B0736UL, 0xB64C298BUL, 0xE75F89E2UL, 0xE121A7CDUL, 0xED65C94DUL, 0x239CEEFEUL,
      0x04B77D33UL, 0x402A9A9EUL, 0xF35B10B3UL, 0x921C7782UL, 0x571E4E20UL, 0x8C067222UL, 0xFB732C67UL, 0xBF0AC259UL, 0x0CF95C79UL, 0x68121A28UL,
      0x42193474UL, 0xF884C0B1UL, 0x9D15F038UL, 0x6F3AF260UL, 0x91EB90B4UL, 0x61357F1DUL, 0x5603325AUL, 0x932BC5A3UL, 0x434B0F80UL, 0x3CE0A8F7UL,
      0x2664D196UL, 0x4FCC45D7UL, 0xB5E9B0C8UL, 0xEA31D600UL,
  };

  void CRndSeed::SetSeed(unsigned long seed) {
    rndacc = seed;
    rndvls = 4 * ((seed % 61) | (((seed % 59) | (((seed % 53) | ((seed % 47) << 8)) << 8)) << 8));
  }



  C3Vector __fastcall CRandom::C3Vector_(CRndSeed &seed) {
    const float z = reals_(seed);
    const float angle = real_(seed) * 6.28318530717958647692f;

    ASSERT(z >= -1.0f && z <= 1.0f);

    const float radius = CMath::sqrt_(1.0f - z * z);
    const float x = CMath::cos_(angle) * radius;
    const float y = CMath::sin_(angle) * radius;

    return C3Vector(x, y, z);
  }

}  // namespace NTempest

static unsigned long lattice_(long x) {
    // TODO: implement
    return 0;
}
