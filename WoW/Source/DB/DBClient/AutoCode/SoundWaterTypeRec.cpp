#include "SoundWaterTypeRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

const char *__fastcall SoundWaterTypeRec::GetFilename() {
  return "DBFilesClient\\SoundWaterType.dbc";
}

SoundWaterTypeRec::SoundWaterTypeRec() {
}

SoundWaterTypeRec::~SoundWaterTypeRec() {
}

bool SoundWaterTypeRec::Read(SFile *f, const char *stringBuffer) {
  bool result = true;

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_soundType, sizeof(m_soundType), 0, 0, 0) && result;
  result = SFile::Read(f, &m_soundSubtype, sizeof(m_soundSubtype), 0, 0, 0) && result;
  result = SFile::Read(f, &m_SoundID, sizeof(m_SoundID), 0, 0, 0) && result;

  if (!result) {
    ConsoleWrite("Error reading SoundWaterTypeRec", DEFAULT_COLOR);
  }

  return result;
}
