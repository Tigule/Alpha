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

  if (!SFileReadTyped(f, &m_Race) ||
      !SFileReadTyped(f, &m_Sex) ||
      !SFileReadTyped(f, &m_Camera) ||
      !SFileReadTyped(f, &m_Height) ||
      !SFileReadTyped(f, &m_Radius) ||
      !SFileReadTyped(f, &m_Target)) {
    ConsoleWrite("Error reading CharacterCreateCamerasRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
