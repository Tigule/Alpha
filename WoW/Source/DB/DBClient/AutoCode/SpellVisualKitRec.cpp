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

  if (!SFileReadTyped(f, &m_ID) ||
      !SFileReadTyped(f, &m_kitType) ||
      !SFileReadTyped(f, &m_anim) ||
      !SFileReadTyped(f, &m_headEffect) ||
      !SFileReadTyped(f, &m_chestEffect) ||
      !SFileReadTyped(f, &m_baseEffect) ||
      !SFileReadTyped(f, &m_leftHandEffect) ||
      !SFileReadTyped(f, &m_rightHandEffect) ||
      !SFileReadTyped(f, &m_breathEffect) ||
      !SFile::Read(f, &m_specialEffect[0], sizeof(m_specialEffect), 0, 0, 0) ||
      !SFileReadTyped(f, &m_characterProcedure) ||
      !SFile::Read(f, &m_characterParam[0], sizeof(m_characterParam), 0, 0, 0) ||
      !SFileReadTyped(f, &m_soundID) ||
      !SFileReadTyped(f, &m_shakeID)) {
    ConsoleWrite("Error reading SpellVisualKitRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
