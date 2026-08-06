#include "ItemGroupSoundsRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

LPCSTR ItemGroupSoundsRec::GetFilename() {
  return "DBFilesClient\\ItemGroupSounds.dbc";
}

ItemGroupSoundsRec::ItemGroupSoundsRec() {
}

ItemGroupSoundsRec::~ItemGroupSoundsRec() {
}

bool ItemGroupSoundsRec::Read(SFile *f, LPCSTR stringBuffer) {
  bool result = true;

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_sound[0], sizeof(m_sound), 0, 0, 0) && result;

  if (!result) {
    ConsoleWrite("Error reading ItemGroupSoundsRec", DEFAULT_COLOR);
  }

  return result;
}
