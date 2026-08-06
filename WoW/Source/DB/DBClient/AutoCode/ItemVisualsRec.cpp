#include "ItemVisualsRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

LPCSTR ItemVisualsRec::GetFilename() {
  return "DBFilesClient\\ItemVisuals.dbc";
}

ItemVisualsRec::ItemVisualsRec() {
}

ItemVisualsRec::~ItemVisualsRec() {
}

bool ItemVisualsRec::Read(SFile *f, LPCSTR stringBuffer) {
  bool result = true;

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, m_Slot, sizeof(m_Slot), 0, 0, 0) && result;

  if (!result) {
    ConsoleWrite("Error reading ItemVisualsRec", DEFAULT_COLOR);
  }

  return result;
}
