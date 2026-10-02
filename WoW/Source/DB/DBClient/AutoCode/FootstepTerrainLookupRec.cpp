#include "FootstepTerrainLookupRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR FootstepTerrainLookupRec::GetFilename() {
  return "DBFilesClient\\FootstepTerrainLookup.dbc";
}

FootstepTerrainLookupRec::FootstepTerrainLookupRec() {
}

FootstepTerrainLookupRec::~FootstepTerrainLookupRec() {
}

bool FootstepTerrainLookupRec::Read(SFile *f, LPCSTR stringBuffer) {
  int error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &m_CreatureFootstepID) == 0);
  error |= (SFileReadTyped(f, &m_TerrainSoundID) == 0);
  error |= (SFileReadTyped(f, &m_SoundID) == 0);
  error |= (SFileReadTyped(f, &m_SoundIDSplash) == 0);

  if (error) {
    ConsoleWrite("Error reading FootstepTerrainLookupRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
