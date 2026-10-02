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
  int  error = 0;

  error |= (SFileReadTyped(f, &m_TerrainID) == 0);
  error |= (SFileReadTyped(f, &tempTerrainDescIndices[0]) == 0);
  error |= (SFileReadTyped(f, &m_FootstepSprayRun) == 0);
  error |= (SFileReadTyped(f, &m_FootstepSprayWalk) == 0);
  error |= (SFileReadTyped(f, &m_SoundID) == 0);
  error |= (SFileReadTyped(f, &m_Flags) == 0);

  if (error) {
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
