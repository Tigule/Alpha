#include "DeathThudLookupsRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR DeathThudLookupsRec::GetFilename() {
  return "DBFilesClient\\DeathThudLookups.dbc";
}

DeathThudLookupsRec::DeathThudLookupsRec() {
}

DeathThudLookupsRec::~DeathThudLookupsRec() {
}

bool DeathThudLookupsRec::Read(SFile *f, LPCSTR stringBuffer) {
  int error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &m_SizeClass) == 0);
  error |= (SFileReadTyped(f, &m_TerrainTypeSoundID) == 0);
  error |= (SFileReadTyped(f, &m_SoundEntryID) == 0);
  error |= (SFileReadTyped(f, &m_SoundEntryIDWater) == 0);

  if (error) {
    ConsoleWrite("Error reading DeathThudLookupsRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
