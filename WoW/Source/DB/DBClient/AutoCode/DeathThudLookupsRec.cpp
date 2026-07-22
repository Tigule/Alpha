#include "DeathThudLookupsRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

const char *__fastcall DeathThudLookupsRec::GetFilename() {
  return "DBFilesClient\\DeathThudLookups.dbc";
}

DeathThudLookupsRec::DeathThudLookupsRec() {
}

DeathThudLookupsRec::~DeathThudLookupsRec() {
}

bool DeathThudLookupsRec::Read(SFile *f, const char *stringBuffer) {
  bool result = true;

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_SizeClass, sizeof(m_SizeClass), 0, 0, 0) && result;
  result = SFile::Read(f, &m_TerrainTypeSoundID, sizeof(m_TerrainTypeSoundID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_SoundEntryID, sizeof(m_SoundEntryID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_SoundEntryIDWater, sizeof(m_SoundEntryIDWater), 0, 0, 0) && result;

  if (!result) {
    ConsoleWrite("Error reading DeathThudLookupsRec", DEFAULT_COLOR);
  }

  return result;
}
