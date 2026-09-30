#include "WorldSafeLocsRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR WorldSafeLocsRec::GetFilename() {
  return "DBFilesClient\\WorldSafeLocs.dbc";
}

WorldSafeLocsRec::WorldSafeLocsRec() {
}

WorldSafeLocsRec::~WorldSafeLocsRec() {
}

bool WorldSafeLocsRec::Read(SFile *f, LPCSTR stringBuffer) {
  UINT tempAreaName_langIndices[8];

  if (!SFileReadTyped(f, &m_ID) ||
      !SFileReadTyped(f, &m_continent) ||
      !SFileReadTyped(f, &m_locX) ||
      !SFileReadTyped(f, &m_locY) ||
      !SFileReadTyped(f, &m_locZ) ||
      !SFileReadTyped(f, &tempAreaName_langIndices[0]) ||
      !SFileReadTyped(f, &tempAreaName_langIndices[1]) ||
      !SFileReadTyped(f, &tempAreaName_langIndices[2]) ||
      !SFileReadTyped(f, &tempAreaName_langIndices[3]) ||
      !SFileReadTyped(f, &tempAreaName_langIndices[4]) ||
      !SFileReadTyped(f, &tempAreaName_langIndices[5]) ||
      !SFileReadTyped(f, &tempAreaName_langIndices[6]) ||
      !SFileReadTyped(f, &tempAreaName_langIndices[7]) ||
      !SFileReadTyped(f, &m_AreaName_flag)) {
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
