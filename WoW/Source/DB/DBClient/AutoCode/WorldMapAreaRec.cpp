#include "WorldMapAreaRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

const char *__fastcall WorldMapAreaRec::GetFilename() {
  return "DBFilesClient\\WorldMapArea.dbc";
}

WorldMapAreaRec::WorldMapAreaRec() {
}

WorldMapAreaRec::~WorldMapAreaRec() {
}

bool WorldMapAreaRec::Read(SFile *f, const char *stringBuffer) {
  bool         result = true;
  unsigned int tempareaNameIndices[1];

  result = SFileReadTyped(f, &m_ID) && result;
  result = SFileReadTyped(f, &m_mapID) && result;
  result = SFileReadTyped(f, &m_areaID) && result;
  result = SFileReadTyped(f, &m_leftBoundary) && result;
  result = SFileReadTyped(f, &m_rightBoundary) && result;
  result = SFileReadTyped(f, &m_topBoundary) && result;
  result = SFileReadTyped(f, &m_bottomBoundary) && result;
  result = SFileReadTyped(f, &tempareaNameIndices[0]) && result;

  if (!result) {
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
