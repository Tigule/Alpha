#include "GameObjectDisplayInfoRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR GameObjectDisplayInfoRec::GetFilename() {
  return "DBFilesClient\\GameObjectDisplayInfo.dbc";
}

GameObjectDisplayInfoRec::GameObjectDisplayInfoRec() {
}

GameObjectDisplayInfoRec::~GameObjectDisplayInfoRec() {
}

bool GameObjectDisplayInfoRec::Read(SFile *f, LPCSTR stringBuffer) {
  UINT tempmodelNameIndices[1];

  if (!SFileReadTyped(f, &m_ID) ||
      !SFileReadTyped(f, &tempmodelNameIndices[0]) ||
      !SFile::Read(f, &m_Sound[0], sizeof(m_Sound), 0, 0, 0)) {
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
