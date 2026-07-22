#include "FootstepTerrainLookupRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

const char *__fastcall FootstepTerrainLookupRec::GetFilename() {
  return "DBFilesClient\\FootstepTerrainLookup.dbc";
}

FootstepTerrainLookupRec::FootstepTerrainLookupRec() {
}

FootstepTerrainLookupRec::~FootstepTerrainLookupRec() {
}

bool FootstepTerrainLookupRec::Read(SFile *f, const char *stringBuffer) {
  bool result = true;

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_CreatureFootstepID, sizeof(m_CreatureFootstepID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_TerrainSoundID, sizeof(m_TerrainSoundID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_SoundID, sizeof(m_SoundID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_SoundIDSplash, sizeof(m_SoundIDSplash), 0, 0, 0) && result;

  if (!result) {
    ConsoleWrite("Error reading FootstepTerrainLookupRec", DEFAULT_COLOR);
  }

  return result;
}
