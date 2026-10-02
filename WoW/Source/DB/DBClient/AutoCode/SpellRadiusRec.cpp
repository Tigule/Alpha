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
  int error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &m_radius) == 0);
  error |= (SFileReadTyped(f, &m_radiusPerLevel) == 0);
  error |= (SFileReadTyped(f, &m_radiusMax) == 0);

  if (error) {
    ConsoleWrite("Error reading SpellRadiusRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
