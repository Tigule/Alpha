#include "SpellIconRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR SpellIconRec::GetFilename() {
  return "DBFilesClient\\SpellIcon.dbc";
}

SpellIconRec::SpellIconRec() {
}

SpellIconRec::~SpellIconRec() {
}

bool SpellIconRec::Read(SFile *f, LPCSTR stringBuffer) {
  UINT temptextureFilenameIndices[1];
  int  error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &temptextureFilenameIndices[0]) == 0);

  if (error) {
    ConsoleWrite("Error reading SpellIconRec", DEFAULT_COLOR);
    return false;
  }

  if (stringBuffer) {
    m_textureFilename = stringBuffer + temptextureFilenameIndices[0];
  } else {
    m_textureFilename = "";
  }

  return true;
}
