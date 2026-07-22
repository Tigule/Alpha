#include "WorldSafeLocsRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

const char *__fastcall WorldSafeLocsRec::GetFilename() {
  return "DBFilesClient\\WorldSafeLocs.dbc";
}

WorldSafeLocsRec::WorldSafeLocsRec() {
}

WorldSafeLocsRec::~WorldSafeLocsRec() {
}

bool WorldSafeLocsRec::Read(SFile *f, const char *stringBuffer) {
  bool         result = true;
  unsigned int tempAreaName_langIndices[8];

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_continent, sizeof(m_continent), 0, 0, 0) && result;
  result = SFile::Read(f, &m_locX, sizeof(m_locX), 0, 0, 0) && result;
  result = SFile::Read(f, &m_locY, sizeof(m_locY), 0, 0, 0) && result;
  result = SFile::Read(f, &m_locZ, sizeof(m_locZ), 0, 0, 0) && result;
  result = SFile::Read(f, &tempAreaName_langIndices[0], sizeof(tempAreaName_langIndices[0]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempAreaName_langIndices[1], sizeof(tempAreaName_langIndices[1]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempAreaName_langIndices[2], sizeof(tempAreaName_langIndices[2]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempAreaName_langIndices[3], sizeof(tempAreaName_langIndices[3]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempAreaName_langIndices[4], sizeof(tempAreaName_langIndices[4]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempAreaName_langIndices[5], sizeof(tempAreaName_langIndices[5]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempAreaName_langIndices[6], sizeof(tempAreaName_langIndices[6]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempAreaName_langIndices[7], sizeof(tempAreaName_langIndices[7]), 0, 0, 0) && result;
  result = SFile::Read(f, &m_AreaName_flag, sizeof(m_AreaName_flag), 0, 0, 0) && result;

  if (!result) {
    ConsoleWrite("Error reading WorldSafeLocsRec", DEFAULT_COLOR);
    return false;
  }

  if (stringBuffer) {
    m_AreaName_lang[0] = stringBuffer + tempAreaName_langIndices[0];
    m_AreaName_lang[1] = stringBuffer + tempAreaName_langIndices[1];
    m_AreaName_lang[2] = stringBuffer + tempAreaName_langIndices[2];
    m_AreaName_lang[3] = stringBuffer + tempAreaName_langIndices[3];
    m_AreaName_lang[4] = stringBuffer + tempAreaName_langIndices[4];
    m_AreaName_lang[5] = stringBuffer + tempAreaName_langIndices[5];
    m_AreaName_lang[6] = stringBuffer + tempAreaName_langIndices[6];
    m_AreaName_lang[7] = stringBuffer + tempAreaName_langIndices[7];
  } else {
    m_AreaName_lang[0] = "";
    m_AreaName_lang[1] = "";
    m_AreaName_lang[2] = "";
    m_AreaName_lang[3] = "";
    m_AreaName_lang[4] = "";
    m_AreaName_lang[5] = "";
    m_AreaName_lang[6] = "";
    m_AreaName_lang[7] = "";
  }

  return true;
}
