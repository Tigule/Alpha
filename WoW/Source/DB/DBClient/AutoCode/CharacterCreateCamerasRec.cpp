#include "CharacterCreateCamerasRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR CharacterCreateCamerasRec::GetFilename() {
  return "DBFilesClient\\CharacterCreateCameras.dbc";
}

CharacterCreateCamerasRec::CharacterCreateCamerasRec() {
}

CharacterCreateCamerasRec::~CharacterCreateCamerasRec() {
}

bool CharacterCreateCamerasRec::Read(SFile *f, LPCSTR stringBuffer) {
  int error = 0;

  error |= (SFileReadTyped(f, &m_Race) == 0);
  error |= (SFileReadTyped(f, &m_Sex) == 0);
  error |= (SFileReadTyped(f, &m_Camera) == 0);
  error |= (SFileReadTyped(f, &m_Height) == 0);
  error |= (SFileReadTyped(f, &m_Radius) == 0);
  error |= (SFileReadTyped(f, &m_Target) == 0);

  if (error) {
    ConsoleWrite("Error reading CharacterCreateCamerasRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
