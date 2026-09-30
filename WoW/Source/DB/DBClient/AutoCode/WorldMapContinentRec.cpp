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

  if (!SFileReadTyped(f, &m_ID) ||
      !SFileReadTyped(f, &m_mapID) ||
      !SFileReadTyped(f, &m_leftBoundary) ||
      !SFileReadTyped(f, &m_rightBoundary) ||
      !SFileReadTyped(f, &m_topBoundary) ||
      !SFileReadTyped(f, &m_bottomBoundary) ||
      !SFileReadTyped(f, &m_continentOffsetX) ||
      !SFileReadTyped(f, &m_continentOffsetY)) {
    ConsoleWrite("Error reading WorldMapContinentRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
