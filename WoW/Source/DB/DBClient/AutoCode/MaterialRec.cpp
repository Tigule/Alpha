#include "MaterialRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR MaterialRec::GetFilename() {
  return "DBFilesClient\\Material.dbc";
}

MaterialRec::MaterialRec() {
}

MaterialRec::~MaterialRec() {
}

bool MaterialRec::Read(SFile *f, LPCSTR stringBuffer) {
  int error = 0;

  error |= (SFileReadTyped(f, &m_materialID) == 0);
  error |= (SFileReadTyped(f, &m_flags) == 0);
  error |= (SFileReadTyped(f, &m_foleySoundID) == 0);

  if (error) {
    ConsoleWrite("Error reading MaterialRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
