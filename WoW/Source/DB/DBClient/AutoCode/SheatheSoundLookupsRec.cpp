#include "SheatheSoundLookupsRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

LPCSTR SheatheSoundLookupsRec::GetFilename() {
  return "DBFilesClient\\SheatheSoundLookups.dbc";
}

SheatheSoundLookupsRec::SheatheSoundLookupsRec() {
}

SheatheSoundLookupsRec::~SheatheSoundLookupsRec() {
}

bool SheatheSoundLookupsRec::Read(SFile *f, LPCSTR stringBuffer) {
  bool result = true;

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_classID, sizeof(m_classID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_subclassID, sizeof(m_subclassID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_material, sizeof(m_material), 0, 0, 0) && result;
  result = SFile::Read(f, &m_checkMaterial, sizeof(m_checkMaterial), 0, 0, 0) && result;
  result = SFile::Read(f, &m_sheatheSound, sizeof(m_sheatheSound), 0, 0, 0) && result;
  result = SFile::Read(f, &m_unsheatheSound, sizeof(m_unsheatheSound), 0, 0, 0) && result;

  if (!result) {
    ConsoleWrite("Error reading SheatheSoundLookupsRec", DEFAULT_COLOR);
  }

  return result;
}
