#include "MapRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR MapRec::GetFilename() {
  return "DBFilesClient\\Map.dbc";
}

MapRec::MapRec() {
}

MapRec::~MapRec() {
}

bool MapRec::Read(SFile *f, LPCSTR stringBuffer) {
  UINT tempDirectoryIndices[1];
  UINT tempMapName_langIndices[8];
  int  error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &tempDirectoryIndices[0]) == 0);
  error |= (SFileReadTyped(f, &m_PVP) == 0);
  error |= (SFileReadTyped(f, &m_IsInMap) == 0);
  error |= (SFileReadTyped(f, &tempMapName_langIndices[0]) == 0);
  error |= (SFileReadTyped(f, &tempMapName_langIndices[1]) == 0);
  error |= (SFileReadTyped(f, &tempMapName_langIndices[2]) == 0);
  error |= (SFileReadTyped(f, &tempMapName_langIndices[3]) == 0);
  error |= (SFileReadTyped(f, &tempMapName_langIndices[4]) == 0);
  error |= (SFileReadTyped(f, &tempMapName_langIndices[5]) == 0);
  error |= (SFileReadTyped(f, &tempMapName_langIndices[6]) == 0);
  error |= (SFileReadTyped(f, &tempMapName_langIndices[7]) == 0);
  error |= (SFileReadTyped(f, &m_MapName_flag) == 0);

  if (error) {
    ConsoleWrite("Error reading MapRec", DEFAULT_COLOR);
    return false;
  }

  if (stringBuffer) {
    m_Directory = stringBuffer + tempDirectoryIndices[0];
    m_MapName_lang[0] = stringBuffer + tempMapName_langIndices[0];
    m_MapName_lang[1] = stringBuffer + tempMapName_langIndices[1];
    m_MapName_lang[2] = stringBuffer + tempMapName_langIndices[2];
    m_MapName_lang[3] = stringBuffer + tempMapName_langIndices[3];
    m_MapName_lang[4] = stringBuffer + tempMapName_langIndices[4];
    m_MapName_lang[5] = stringBuffer + tempMapName_langIndices[5];
    m_MapName_lang[6] = stringBuffer + tempMapName_langIndices[6];
    m_MapName_lang[7] = stringBuffer + tempMapName_langIndices[7];
  } else {
    m_Directory = "";
    m_MapName_lang[0] = "";
    m_MapName_lang[1] = "";
    m_MapName_lang[2] = "";
    m_MapName_lang[3] = "";
    m_MapName_lang[4] = "";
    m_MapName_lang[5] = "";
    m_MapName_lang[6] = "";
    m_MapName_lang[7] = "";
  }

  return true;
}
