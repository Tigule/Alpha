#include "CharVariationsRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

const char *__fastcall CharVariationsRec::GetFilename() {
  return "DBFilesClient\\CharVariations.dbc";
}

CharVariationsRec::CharVariationsRec() {
}

CharVariationsRec::~CharVariationsRec() {
}

bool CharVariationsRec::Read(SFile *f, const char *stringBuffer) {
  bool result = true;

  result = SFile::Read(f, &m_RaceID, sizeof(m_RaceID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_SexID, sizeof(m_SexID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_TextureHoldLayer[0], sizeof(m_TextureHoldLayer), 0, 0, 0) && result;

  if (!result) {
    ConsoleWrite("Error reading CharVariationsRec", DEFAULT_COLOR);
  }

  return result;
}
