#include "CharacterCreateCamerasRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

const char *CharacterCreateCamerasRec::GetFilename() {
  return "DBFilesClient\\CharacterCreateCameras.dbc";
}

CharacterCreateCamerasRec::CharacterCreateCamerasRec() {
}

CharacterCreateCamerasRec::~CharacterCreateCamerasRec() {
}

bool CharacterCreateCamerasRec::Read(SFile *f, const char *stringBuffer) {
  bool result = true;

  result = SFile::Read(f, &m_Race, sizeof(m_Race), 0, 0, 0) && result;
  result = SFile::Read(f, &m_Sex, sizeof(m_Sex), 0, 0, 0) && result;
  result = SFile::Read(f, &m_Camera, sizeof(m_Camera), 0, 0, 0) && result;
  result = SFile::Read(f, &m_Height, sizeof(m_Height), 0, 0, 0) && result;
  result = SFile::Read(f, &m_Radius, sizeof(m_Radius), 0, 0, 0) && result;
  result = SFile::Read(f, &m_Target, sizeof(m_Target), 0, 0, 0) && result;

  if (!result) {
    ConsoleWrite("Error reading CharacterCreateCamerasRec", DEFAULT_COLOR);
  }

  return result;
}
