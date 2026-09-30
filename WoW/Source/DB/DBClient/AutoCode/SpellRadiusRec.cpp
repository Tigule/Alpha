#include "SpellRadiusRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR SpellRadiusRec::GetFilename() {
  return "DBFilesClient\\SpellRadius.dbc";
}

SpellRadiusRec::SpellRadiusRec() {
}

SpellRadiusRec::~SpellRadiusRec() {
}

bool SpellRadiusRec::Read(SFile *f, LPCSTR stringBuffer) {

  if (!SFileReadTyped(f, &m_ID) ||
      !SFileReadTyped(f, &m_radius) ||
      !SFileReadTyped(f, &m_radiusPerLevel) ||
      !SFileReadTyped(f, &m_radiusMax)) {
    ConsoleWrite("Error reading SpellRadiusRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
