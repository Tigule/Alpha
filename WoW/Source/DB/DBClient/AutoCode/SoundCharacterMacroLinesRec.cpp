#include "SoundCharacterMacroLinesRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

const char *SoundCharacterMacroLinesRec::GetFilename() {
  return "DBFilesClient\\SoundCharacterMacroLines.dbc";
}

SoundCharacterMacroLinesRec::SoundCharacterMacroLinesRec() {
}

SoundCharacterMacroLinesRec::~SoundCharacterMacroLinesRec() {
}

bool SoundCharacterMacroLinesRec::Read(SFile *f, const char *stringBuffer) {
  bool result = true;

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_Category, sizeof(m_Category), 0, 0, 0) && result;
  result = SFile::Read(f, &m_Sex, sizeof(m_Sex), 0, 0, 0) && result;
  result = SFile::Read(f, &m_Race, sizeof(m_Race), 0, 0, 0) && result;
  result = SFile::Read(f, &m_SoundID, sizeof(m_SoundID), 0, 0, 0) && result;

  if (!result) {
    ConsoleWrite("Error reading SoundCharacterMacroLinesRec", DEFAULT_COLOR);
  }

  return result;
}
