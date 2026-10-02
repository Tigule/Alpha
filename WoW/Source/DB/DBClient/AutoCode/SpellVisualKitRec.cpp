#include "SpellVisualKitRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR SpellVisualKitRec::GetFilename() {
  return "DBFilesClient\\SpellVisualKit.dbc";
}

SpellVisualKitRec::SpellVisualKitRec() {
}

SpellVisualKitRec::~SpellVisualKitRec() {
}

bool SpellVisualKitRec::Read(SFile *f, LPCSTR stringBuffer) {
  int error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &m_kitType) == 0);
  error |= (SFileReadTyped(f, &m_anim) == 0);
  error |= (SFileReadTyped(f, &m_headEffect) == 0);
  error |= (SFileReadTyped(f, &m_chestEffect) == 0);
  error |= (SFileReadTyped(f, &m_baseEffect) == 0);
  error |= (SFileReadTyped(f, &m_leftHandEffect) == 0);
  error |= (SFileReadTyped(f, &m_rightHandEffect) == 0);
  error |= (SFileReadTyped(f, &m_breathEffect) == 0);
  error |= (SFileReadTyped(f, &m_specialEffect) == 0);
  error |= (SFileReadTyped(f, &m_characterProcedure) == 0);
  error |= (SFileReadTyped(f, &m_characterParam) == 0);
  error |= (SFileReadTyped(f, &m_soundID) == 0);
  error |= (SFileReadTyped(f, &m_shakeID) == 0);

  if (error) {
    ConsoleWrite("Error reading SpellVisualKitRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
