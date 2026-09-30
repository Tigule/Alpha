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

  if (!SFileReadTyped(f, &m_ID) ||
      !SFileReadTyped(f, &temptextureFilenameIndices[0])) {
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
