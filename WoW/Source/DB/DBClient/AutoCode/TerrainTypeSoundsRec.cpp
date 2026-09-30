#include "TerrainTypeSoundsRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR TerrainTypeSoundsRec::GetFilename() {
  return "DBFilesClient\\TerrainTypeSounds.dbc";
}

TerrainTypeSoundsRec::TerrainTypeSoundsRec() {
}

TerrainTypeSoundsRec::~TerrainTypeSoundsRec() {
}

bool TerrainTypeSoundsRec::Read(SFile *f, LPCSTR stringBuffer) {

  if (!SFileReadTyped(f, &m_ID)) {
    ConsoleWrite("Error reading TerrainTypeSoundsRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
