#include "AreaPOIRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR AreaPOIRec::GetFilename() {
  return "DBFilesClient\\AreaPOI.dbc";
}

AreaPOIRec::AreaPOIRec() {
}

AreaPOIRec::~AreaPOIRec() {
}

bool AreaPOIRec::Read(SFile *f, LPCSTR stringBuffer) {
  UINT tempname_langIndices[8];

  if (!SFileReadTyped(f, &m_ID) ||
      !SFileReadTyped(f, &m_importance) ||
      !SFileReadTyped(f, &m_icon) ||
      !SFileReadTyped(f, &m_factionID) ||
      !SFileReadTyped(f, &m_x) ||
      !SFileReadTyped(f, &m_y) ||
      !SFileReadTyped(f, &m_z) ||
      !SFileReadTyped(f, &m_continentID) ||
      !SFileReadTyped(f, &m_flags) ||
      !SFileReadTyped(f, &tempname_langIndices[0]) ||
      !SFileReadTyped(f, &tempname_langIndices[1]) ||
      !SFileReadTyped(f, &tempname_langIndices[2]) ||
      !SFileReadTyped(f, &tempname_langIndices[3]) ||
      !SFileReadTyped(f, &tempname_langIndices[4]) ||
      !SFileReadTyped(f, &tempname_langIndices[5]) ||
      !SFileReadTyped(f, &tempname_langIndices[6]) ||
      !SFileReadTyped(f, &tempname_langIndices[7]) ||
      !SFileReadTyped(f, &m_name_flag)) {
    ConsoleWrite("Error reading AreaPOIRec", DEFAULT_COLOR);
    return false;
  }

  if (stringBuffer) {
    m_name_lang[0] = stringBuffer + tempname_langIndices[0];
    m_name_lang[1] = stringBuffer + tempname_langIndices[1];
    m_name_lang[2] = stringBuffer + tempname_langIndices[2];
    m_name_lang[3] = stringBuffer + tempname_langIndices[3];
    m_name_lang[4] = stringBuffer + tempname_langIndices[4];
    m_name_lang[5] = stringBuffer + tempname_langIndices[5];
    m_name_lang[6] = stringBuffer + tempname_langIndices[6];
    m_name_lang[7] = stringBuffer + tempname_langIndices[7];
  } else {
    m_name_lang[0] = "";
    m_name_lang[1] = "";
    m_name_lang[2] = "";
    m_name_lang[3] = "";
    m_name_lang[4] = "";
    m_name_lang[5] = "";
    m_name_lang[6] = "";
    m_name_lang[7] = "";
  }

  return true;
}
