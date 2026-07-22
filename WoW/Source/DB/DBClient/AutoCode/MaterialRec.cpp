#include "MaterialRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

const char *__fastcall MaterialRec::GetFilename() {
  return "DBFilesClient\\Material.dbc";
}

MaterialRec::MaterialRec() {
}

MaterialRec::~MaterialRec() {
}

bool MaterialRec::Read(SFile *f, const char *stringBuffer) {
  bool result = true;

  result = SFile::Read(f, &m_materialID, sizeof(m_materialID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_flags, sizeof(m_flags), 0, 0, 0) && result;
  result = SFile::Read(f, &m_foleySoundID, sizeof(m_foleySoundID), 0, 0, 0) && result;

  if (!result) {
    ConsoleWrite("Error reading MaterialRec", DEFAULT_COLOR);
  }

  return result;
}
