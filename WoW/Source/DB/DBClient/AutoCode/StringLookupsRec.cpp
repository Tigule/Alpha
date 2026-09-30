#include "StringLookupsRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR StringLookupsRec::GetFilename() {
  return "DBFilesClient\\StringLookups.dbc";
}

StringLookupsRec::StringLookupsRec() {
}

StringLookupsRec::~StringLookupsRec() {
}

bool StringLookupsRec::Read(SFile *f, LPCSTR stringBuffer) {
  UINT tempStringIndices[1];

  if (!SFileReadTyped(f, &m_ID) ||
      !SFileReadTyped(f, &tempStringIndices[0])) {
    ConsoleWrite("Error reading StringLookupsRec", DEFAULT_COLOR);
    return false;
  }

  if (stringBuffer) {
    m_String = stringBuffer + tempStringIndices[0];
  } else {
    m_String = "";
  }

  return true;
}
