#include "WorldMapAreaRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR WorldMapAreaRec::GetFilename() {
  return "DBFilesClient\\WorldMapArea.dbc";
}

WorldMapAreaRec::WorldMapAreaRec() {
}

WorldMapAreaRec::~WorldMapAreaRec() {
}

bool WorldMapAreaRec::Read(SFile *f, LPCSTR stringBuffer) {
  UINT tempareaNameIndices[1];

  if (!SFileReadTyped(f, &m_ID) ||
      !SFileReadTyped(f, &m_mapID) ||
      !SFileReadTyped(f, &m_areaID) ||
      !SFileReadTyped(f, &m_leftBoundary) ||
      !SFileReadTyped(f, &m_rightBoundary) ||
      !SFileReadTyped(f, &m_topBoundary) ||
      !SFileReadTyped(f, &m_bottomBoundary) ||
      !SFileReadTyped(f, &tempareaNameIndices[0])) {
    ConsoleWrite("Error reading WorldMapAreaRec", DEFAULT_COLOR);
    return false;
  }

  if (stringBuffer) {
    m_areaName = &stringBuffer[tempareaNameIndices[0]];
  } else {
    m_areaName = "";
  }

  return true;
}
