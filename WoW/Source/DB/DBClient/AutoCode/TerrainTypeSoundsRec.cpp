#include "TerrainTypeSoundsRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

const char *__fastcall TerrainTypeSoundsRec::GetFilename() {
  return "DBFilesClient\\TerrainTypeSounds.dbc";
}

TerrainTypeSoundsRec::TerrainTypeSoundsRec() {
}

TerrainTypeSoundsRec::~TerrainTypeSoundsRec() {
}

bool TerrainTypeSoundsRec::Read(SFile *f, const char *stringBuffer) {
  bool result = true;

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;

  if (!result) {
    ConsoleWrite("Error reading TerrainTypeSoundsRec", DEFAULT_COLOR);
  }

  return result;
}
