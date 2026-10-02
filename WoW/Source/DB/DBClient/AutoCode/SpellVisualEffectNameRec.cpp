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
  int  error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &tempfileNameIndices[0]) == 0);
  error |= (SFileReadTyped(f, &m_specialID) == 0);
  error |= (SFileReadTyped(f, &m_specialAttachPoint) == 0);
  error |= (SFileReadTyped(f, &m_areaEffectSize) == 0);
  error |= (SFileReadTyped(f, &m_VisualEffectNameFlags) == 0);

  if (error) {
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
