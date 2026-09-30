#include "SheatheSoundLookupsRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR SheatheSoundLookupsRec::GetFilename() {
  return "DBFilesClient\\SheatheSoundLookups.dbc";
}

SheatheSoundLookupsRec::SheatheSoundLookupsRec() {
}

SheatheSoundLookupsRec::~SheatheSoundLookupsRec() {
}

bool SheatheSoundLookupsRec::Read(SFile *f, LPCSTR stringBuffer) {

  if (!SFileReadTyped(f, &m_ID) ||
      !SFileReadTyped(f, &m_classID) ||
      !SFileReadTyped(f, &m_subclassID) ||
      !SFileReadTyped(f, &m_material) ||
      !SFileReadTyped(f, &m_checkMaterial) ||
      !SFileReadTyped(f, &m_sheatheSound) ||
      !SFileReadTyped(f, &m_unsheatheSound)) {
    ConsoleWrite("Error reading SheatheSoundLookupsRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
