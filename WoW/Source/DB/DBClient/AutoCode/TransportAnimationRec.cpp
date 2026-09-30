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

  if (!SFileReadTyped(f, &m_ID) ||
      !SFileReadTyped(f, &m_TransportID) ||
      !SFileReadTyped(f, &m_TimeIndex) ||
      !SFileReadTyped(f, &m_PosX) ||
      !SFileReadTyped(f, &m_PosY) ||
      !SFileReadTyped(f, &m_PosZ)) {
    ConsoleWrite("Error reading TransportAnimationRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
