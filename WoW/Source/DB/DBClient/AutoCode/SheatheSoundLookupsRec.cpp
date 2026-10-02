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
  int error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &m_classID) == 0);
  error |= (SFileReadTyped(f, &m_subclassID) == 0);
  error |= (SFileReadTyped(f, &m_material) == 0);
  error |= (SFileReadTyped(f, &m_checkMaterial) == 0);
  error |= (SFileReadTyped(f, &m_sheatheSound) == 0);
  error |= (SFileReadTyped(f, &m_unsheatheSound) == 0);

  if (error) {
    ConsoleWrite("Error reading SheatheSoundLookupsRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
