#include "NamesReservedRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR NamesReservedRec::GetFilename() {
  return "DBFilesClient\\NamesReserved.dbc";
}

NamesReservedRec::NamesReservedRec() {
}

NamesReservedRec::~NamesReservedRec() {
}

bool NamesReservedRec::Read(SFile *f, LPCSTR stringBuffer) {
  UINT tempNameIndices[1];

  if (!SFileReadTyped(f, &m_ID) ||
      !SFileReadTyped(f, &tempNameIndices[0])) {
    ConsoleWrite("Error reading NamesReservedRec", DEFAULT_COLOR);
    return false;
  }

  if (stringBuffer) {
    m_Name = &stringBuffer[tempNameIndices[0]];
  } else {
    m_Name = "";
  }

  return true;
}
