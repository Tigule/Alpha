#include "SpellVisualKitRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

const char *SpellVisualKitRec::GetFilename() {
  return "DBFilesClient\\SpellVisualKit.dbc";
}

SpellVisualKitRec::SpellVisualKitRec() {
}

SpellVisualKitRec::~SpellVisualKitRec() {
}

bool SpellVisualKitRec::Read(SFile *f, const char *stringBuffer) {
  bool result = true;

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_kitType, sizeof(m_kitType), 0, 0, 0) && result;
  result = SFile::Read(f, &m_anim, sizeof(m_anim), 0, 0, 0) && result;
  result = SFile::Read(f, &m_headEffect, sizeof(m_headEffect), 0, 0, 0) && result;
  result = SFile::Read(f, &m_chestEffect, sizeof(m_chestEffect), 0, 0, 0) && result;
  result = SFile::Read(f, &m_baseEffect, sizeof(m_baseEffect), 0, 0, 0) && result;
  result = SFile::Read(f, &m_leftHandEffect, sizeof(m_leftHandEffect), 0, 0, 0) && result;
  result = SFile::Read(f, &m_rightHandEffect, sizeof(m_rightHandEffect), 0, 0, 0) && result;
  result = SFile::Read(f, &m_breathEffect, sizeof(m_breathEffect), 0, 0, 0) && result;
  result = SFile::Read(f, &m_specialEffect[0], sizeof(m_specialEffect), 0, 0, 0) && result;
  result = SFile::Read(f, &m_characterProcedure, sizeof(m_characterProcedure), 0, 0, 0) && result;
  result = SFile::Read(f, &m_characterParam[0], sizeof(m_characterParam), 0, 0, 0) && result;
  result = SFile::Read(f, &m_soundID, sizeof(m_soundID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_shakeID, sizeof(m_shakeID), 0, 0, 0) && result;

  if (!result) {
    ConsoleWrite("Error reading SpellVisualKitRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
