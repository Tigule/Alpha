#include "WorldMapContinentRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR WorldMapContinentRec::GetFilename() {
  return "DBFilesClient\\WorldMapContinent.dbc";
}

WorldMapContinentRec::WorldMapContinentRec() {
}

WorldMapContinentRec::~WorldMapContinentRec() {
}

bool WorldMapContinentRec::Read(SFile *f, LPCSTR stringBuffer) {
  int error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &m_mapID) == 0);
  error |= (SFileReadTyped(f, &m_leftBoundary) == 0);
  error |= (SFileReadTyped(f, &m_rightBoundary) == 0);
  error |= (SFileReadTyped(f, &m_topBoundary) == 0);
  error |= (SFileReadTyped(f, &m_bottomBoundary) == 0);
  error |= (SFileReadTyped(f, &m_continentOffsetX) == 0);
  error |= (SFileReadTyped(f, &m_continentOffsetY) == 0);

  if (error) {
    ConsoleWrite("Error reading WorldMapContinentRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
