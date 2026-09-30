#include "TerrainTypeRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR TerrainTypeRec::GetFilename() {
  return "DBFilesClient\\TerrainType.dbc";
}

TerrainTypeRec::TerrainTypeRec() {
}

TerrainTypeRec::~TerrainTypeRec() {
}

bool TerrainTypeRec::Read(SFile *f, LPCSTR stringBuffer) {
  UINT tempTerrainDescIndices[1];

  if (!SFileReadTyped(f, &m_TerrainID) ||
      !SFileReadTyped(f, &tempTerrainDescIndices[0]) ||
      !SFileReadTyped(f, &m_FootstepSprayRun) ||
      !SFileReadTyped(f, &m_FootstepSprayWalk) ||
      !SFileReadTyped(f, &m_SoundID) ||
      !SFileReadTyped(f, &m_Flags)) {
    ConsoleWrite("Error reading TerrainTypeRec", DEFAULT_COLOR);
    return false;
  }

  if (stringBuffer) {
    m_TerrainDesc = stringBuffer + tempTerrainDescIndices[0];
  } else {
    m_TerrainDesc = "";
  }

  return true;
}
