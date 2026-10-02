#include "FactionGroupRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR FactionGroupRec::GetFilename() {
  return "DBFilesClient\\FactionGroup.dbc";
}

FactionGroupRec::FactionGroupRec() {
}

FactionGroupRec::~FactionGroupRec() {
}

bool FactionGroupRec::Read(SFile *f, LPCSTR stringBuffer) {
  UINT tempname_langIndices[8];
  UINT tempinternalNameIndices[1];
  int  error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &m_maskID) == 0);
  error |= (SFileReadTyped(f, &tempinternalNameIndices[0]) == 0);
  error |= (SFileReadTyped(f, &tempname_langIndices[0]) == 0);
  error |= (SFileReadTyped(f, &tempname_langIndices[1]) == 0);
  error |= (SFileReadTyped(f, &tempname_langIndices[2]) == 0);
  error |= (SFileReadTyped(f, &tempname_langIndices[3]) == 0);
  error |= (SFileReadTyped(f, &tempname_langIndices[4]) == 0);
  error |= (SFileReadTyped(f, &tempname_langIndices[5]) == 0);
  error |= (SFileReadTyped(f, &tempname_langIndices[6]) == 0);
  error |= (SFileReadTyped(f, &tempname_langIndices[7]) == 0);
  error |= (SFileReadTyped(f, &m_name_flag) == 0);

  if (error) {
    ConsoleWrite("Error reading FactionGroupRec", DEFAULT_COLOR);
    return false;
  }

  if (stringBuffer) {
    m_internalName = stringBuffer + tempinternalNameIndices[0];
    m_name_lang[0] = stringBuffer + tempname_langIndices[0];
    m_name_lang[1] = stringBuffer + tempname_langIndices[1];
    m_name_lang[2] = stringBuffer + tempname_langIndices[2];
    m_name_lang[3] = stringBuffer + tempname_langIndices[3];
    m_name_lang[4] = stringBuffer + tempname_langIndices[4];
    m_name_lang[5] = stringBuffer + tempname_langIndices[5];
    m_name_lang[6] = stringBuffer + tempname_langIndices[6];
    m_name_lang[7] = stringBuffer + tempname_langIndices[7];
  } else {
    m_internalName = "";
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
