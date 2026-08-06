#include "TerrainTypeRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

LPCSTR TerrainTypeRec::GetFilename() {
  return "DBFilesClient\\TerrainType.dbc";
}

TerrainTypeRec::TerrainTypeRec() {
}

TerrainTypeRec::~TerrainTypeRec() {
}

bool TerrainTypeRec::Read(SFile *f, LPCSTR stringBuffer) {
  bool result = true;
  UINT tempTerrainDescIndices[1];

  result = SFile::Read(f, &m_TerrainID, sizeof(m_TerrainID), 0, 0, 0) && result;
  result = SFile::Read(f, &tempTerrainDescIndices[0], sizeof(tempTerrainDescIndices[0]), 0, 0, 0) && result;
  result = SFile::Read(f, &m_FootstepSprayRun, sizeof(m_FootstepSprayRun), 0, 0, 0) && result;
  result = SFile::Read(f, &m_FootstepSprayWalk, sizeof(m_FootstepSprayWalk), 0, 0, 0) && result;
  result = SFile::Read(f, &m_SoundID, sizeof(m_SoundID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_Flags, sizeof(m_Flags), 0, 0, 0) && result;

  if (!result) {
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
