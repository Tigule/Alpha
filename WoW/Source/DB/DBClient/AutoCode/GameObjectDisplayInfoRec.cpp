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
  int  error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &tempmodelNameIndices[0]) == 0);
  error |= (SFileReadTyped(f, &m_Sound) == 0);

  if (error) {
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
