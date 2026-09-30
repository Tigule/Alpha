#include "SpellVisualAnimNameRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR SpellVisualAnimNameRec::GetFilename() {
  return "DBFilesClient\\SpellVisualAnimName.dbc";
}

SpellVisualAnimNameRec::SpellVisualAnimNameRec() {
}

SpellVisualAnimNameRec::~SpellVisualAnimNameRec() {
}

bool SpellVisualAnimNameRec::Read(SFile *f, LPCSTR stringBuffer) {
  UINT tempnameIndices[1];

  if (!SFileReadTyped(f, &m_AnimID) ||
      !SFileReadTyped(f, &tempnameIndices[0])) {
    ConsoleWrite("Error reading SpellVisualAnimNameRec", DEFAULT_COLOR);
    return false;
  }

  if (stringBuffer) {
    m_name = stringBuffer + tempnameIndices[0];
  } else {
    m_name = "";
  }

  return true;
}
