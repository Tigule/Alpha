#include "MapRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

const char *__fastcall MapRec::GetFilename() {
  return "DBFilesClient\\Map.dbc";
}

MapRec::MapRec() {
}

MapRec::~MapRec() {
}

bool MapRec::Read(SFile *f, const char *stringBuffer) {
  bool         result = true;
  unsigned int tempDirectoryIndices[1];
  unsigned int tempMapName_langIndices[8];

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, &tempDirectoryIndices[0], sizeof(tempDirectoryIndices[0]), 0, 0, 0) && result;
  result = SFile::Read(f, &m_PVP, sizeof(m_PVP), 0, 0, 0) && result;
  result = SFile::Read(f, &m_IsInMap, sizeof(m_IsInMap), 0, 0, 0) && result;
  result = SFile::Read(f, &tempMapName_langIndices[0], sizeof(tempMapName_langIndices[0]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempMapName_langIndices[1], sizeof(tempMapName_langIndices[1]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempMapName_langIndices[2], sizeof(tempMapName_langIndices[2]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempMapName_langIndices[3], sizeof(tempMapName_langIndices[3]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempMapName_langIndices[4], sizeof(tempMapName_langIndices[4]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempMapName_langIndices[5], sizeof(tempMapName_langIndices[5]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempMapName_langIndices[6], sizeof(tempMapName_langIndices[6]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempMapName_langIndices[7], sizeof(tempMapName_langIndices[7]), 0, 0, 0) && result;
  result = SFile::Read(f, &m_MapName_flag, sizeof(m_MapName_flag), 0, 0, 0) && result;

  if (!result) {
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
