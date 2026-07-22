#include "TransportAnimationRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

const char *__fastcall TransportAnimationRec::GetFilename() {
  return "DBFilesClient\\TransportAnimation.dbc";
}

TransportAnimationRec::TransportAnimationRec() {
}

TransportAnimationRec::~TransportAnimationRec() {
}

bool TransportAnimationRec::Read(SFile *f, const char *stringBuffer) {
  bool result = true;

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_TransportID, sizeof(m_TransportID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_TimeIndex, sizeof(m_TimeIndex), 0, 0, 0) && result;
  result = SFile::Read(f, &m_PosX, sizeof(m_PosX), 0, 0, 0) && result;
  result = SFile::Read(f, &m_PosY, sizeof(m_PosY), 0, 0, 0) && result;
  result = SFile::Read(f, &m_PosZ, sizeof(m_PosZ), 0, 0, 0) && result;

  if (!result) {
    ConsoleWrite("Error reading TransportAnimationRec", DEFAULT_COLOR);
  }

  return result;
}
