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
  int  error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &m_mapID) == 0);
  error |= (SFileReadTyped(f, &m_areaID) == 0);
  error |= (SFileReadTyped(f, &m_leftBoundary) == 0);
  error |= (SFileReadTyped(f, &m_rightBoundary) == 0);
  error |= (SFileReadTyped(f, &m_topBoundary) == 0);
  error |= (SFileReadTyped(f, &m_bottomBoundary) == 0);
  error |= (SFileReadTyped(f, &tempareaNameIndices[0]) == 0);

  if (error) {
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
