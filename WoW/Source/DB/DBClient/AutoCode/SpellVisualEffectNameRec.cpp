#include "SpellVisualEffectNameRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR SpellVisualEffectNameRec::GetFilename() {
  return "DBFilesClient\\SpellVisualEffectName.dbc";
}

SpellVisualEffectNameRec::SpellVisualEffectNameRec() {
}

SpellVisualEffectNameRec::~SpellVisualEffectNameRec() {
}

bool SpellVisualEffectNameRec::Read(SFile *f, LPCSTR stringBuffer) {
  UINT tempfileNameIndices[1];

  if (!SFileReadTyped(f, &m_ID) ||
      !SFileReadTyped(f, &tempfileNameIndices[0]) ||
      !SFileReadTyped(f, &m_specialID) ||
      !SFileReadTyped(f, &m_specialAttachPoint) ||
      !SFileReadTyped(f, &m_areaEffectSize) ||
      !SFileReadTyped(f, &m_VisualEffectNameFlags)) {
    ConsoleWrite("Error reading SpellVisualEffectNameRec", DEFAULT_COLOR);
    return false;
  }

  if (stringBuffer) {
    m_fileName = stringBuffer + tempfileNameIndices[0];
  } else {
    m_fileName = "";
  }

  return true;
}
