#include "WorldMapContinentRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

LPCSTR WorldMapContinentRec::GetFilename() {
  return "DBFilesClient\\WorldMapContinent.dbc";
}

WorldMapContinentRec::WorldMapContinentRec() {
}

WorldMapContinentRec::~WorldMapContinentRec() {
}

bool WorldMapContinentRec::Read(SFile *f, LPCSTR stringBuffer) {
  bool result = true;

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_mapID, sizeof(m_mapID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_leftBoundary, sizeof(m_leftBoundary), 0, 0, 0) && result;
  result = SFile::Read(f, &m_rightBoundary, sizeof(m_rightBoundary), 0, 0, 0) && result;
  result = SFile::Read(f, &m_topBoundary, sizeof(m_topBoundary), 0, 0, 0) && result;
  result = SFile::Read(f, &m_bottomBoundary, sizeof(m_bottomBoundary), 0, 0, 0) && result;
  result = SFile::Read(f, &m_continentOffsetX, sizeof(m_continentOffsetX), 0, 0, 0) && result;
  result = SFile::Read(f, &m_continentOffsetY, sizeof(m_continentOffsetY), 0, 0, 0) && result;

  if (!result) {
    ConsoleWrite("Error reading WorldMapContinentRec", DEFAULT_COLOR);
  }

  return result;
}
