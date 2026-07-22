#include "NPCSoundsRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

const char *__fastcall NPCSoundsRec::GetFilename() {
  return "DBFilesClient\\NPCSounds.dbc";
}

NPCSoundsRec::NPCSoundsRec() {
}

NPCSoundsRec::~NPCSoundsRec() {
}

bool NPCSoundsRec::Read(SFile *f, const char *stringBuffer) {
  bool result = true;

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_SoundID[0], sizeof(m_SoundID), 0, 0, 0) && result;

  if (!result) {
    ConsoleWrite("Error reading NPCSoundsRec", DEFAULT_COLOR);
  }

  return result;
}
