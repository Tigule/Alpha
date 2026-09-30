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

  if (!SFileReadTyped(f, &m_ID) ||
      !SFileReadTyped(f, &tempNameIndices[0])) {
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
