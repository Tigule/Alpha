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

  if (!SFileReadTyped(f, &m_materialID) ||
      !SFileReadTyped(f, &m_flags) ||
      !SFileReadTyped(f, &m_foleySoundID)) {
    ConsoleWrite("Error reading MaterialRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
