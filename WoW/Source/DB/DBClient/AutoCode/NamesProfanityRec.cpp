#include "NamesProfanityRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR NamesProfanityRec::GetFilename() {
  return "DBFilesClient\\NamesProfanity.dbc";
}

NamesProfanityRec::NamesProfanityRec() {
}

NamesProfanityRec::~NamesProfanityRec() {
}

bool NamesProfanityRec::Read(SFile *f, LPCSTR stringBuffer) {
  UINT tempNameIndices[1];
  int  error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &tempNameIndices[0]) == 0);

  if (error) {
    ConsoleWrite("Error reading NamesProfanityRec", DEFAULT_COLOR);
    return false;
  }

  if (stringBuffer) {
    m_Name = &stringBuffer[tempNameIndices[0]];
  } else {
    m_Name = "";
  }

  return true;
}
