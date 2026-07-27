#include "GameObjectDisplayInfoRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

const char *GameObjectDisplayInfoRec::GetFilename() {
  return "DBFilesClient\\GameObjectDisplayInfo.dbc";
}

GameObjectDisplayInfoRec::GameObjectDisplayInfoRec() {
}

GameObjectDisplayInfoRec::~GameObjectDisplayInfoRec() {
}

bool GameObjectDisplayInfoRec::Read(SFile *f, const char *stringBuffer) {
  bool         result = true;
  unsigned int tempmodelNameIndices[1];

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, &tempmodelNameIndices[0], sizeof(tempmodelNameIndices[0]), 0, 0, 0) && result;
  result = SFile::Read(f, &m_Sound[0], sizeof(m_Sound), 0, 0, 0) && result;

  if (!result) {
    ConsoleWrite("Error reading GameObjectDisplayInfoRec", DEFAULT_COLOR);
    return false;
  }

  if (stringBuffer) {
    m_modelName = stringBuffer + tempmodelNameIndices[0];
  } else {
    m_modelName = "";
  }

  return true;
}
