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

  if (!SFileReadTyped(f, &m_ID) ||
      !SFileReadTyped(f, &m_CreatureFootstepID) ||
      !SFileReadTyped(f, &m_TerrainSoundID) ||
      !SFileReadTyped(f, &m_SoundID) ||
      !SFileReadTyped(f, &m_SoundIDSplash)) {
    ConsoleWrite("Error reading FootstepTerrainLookupRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
