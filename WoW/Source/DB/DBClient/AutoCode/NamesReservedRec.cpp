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
  int  error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &tempNameIndices[0]) == 0);

  if (error) {
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
