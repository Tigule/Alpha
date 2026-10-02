#include "TransportAnimationRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR TransportAnimationRec::GetFilename() {
  return "DBFilesClient\\TransportAnimation.dbc";
}

TransportAnimationRec::TransportAnimationRec() {
}

TransportAnimationRec::~TransportAnimationRec() {
}

bool TransportAnimationRec::Read(SFile *f, LPCSTR stringBuffer) {
  int error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &m_TransportID) == 0);
  error |= (SFileReadTyped(f, &m_TimeIndex) == 0);
  error |= (SFileReadTyped(f, &m_PosX) == 0);
  error |= (SFileReadTyped(f, &m_PosY) == 0);
  error |= (SFileReadTyped(f, &m_PosZ) == 0);

  if (error) {
    ConsoleWrite("Error reading TransportAnimationRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
