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

  if (!SFileReadTyped(f, &m_ID) ||
      !SFileReadTyped(f, &m_SizeClass) ||
      !SFileReadTyped(f, &m_TerrainTypeSoundID) ||
      !SFileReadTyped(f, &m_SoundEntryID) ||
      !SFileReadTyped(f, &m_SoundEntryIDWater)) {
    ConsoleWrite("Error reading DeathThudLookupsRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
