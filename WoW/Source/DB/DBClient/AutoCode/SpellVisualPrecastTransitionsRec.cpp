#include "SpellVisualPrecastTransitionsRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

const char *__fastcall SpellVisualPrecastTransitionsRec::GetFilename() {
  return "DBFilesClient\\SpellVisualPrecastTransitions.dbc";
}

SpellVisualPrecastTransitionsRec::SpellVisualPrecastTransitionsRec() {
}

SpellVisualPrecastTransitionsRec::~SpellVisualPrecastTransitionsRec() {
}

bool SpellVisualPrecastTransitionsRec::Read(SFile *f, const char *stringBuffer) {
  bool         result = true;
  unsigned int tempPrecastLoadAnimNameIndices[1];
  unsigned int tempPrecastHoldAnimNameIndices[1];

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, tempPrecastLoadAnimNameIndices, sizeof(tempPrecastLoadAnimNameIndices), 0, 0, 0) && result;
  result = SFile::Read(f, tempPrecastHoldAnimNameIndices, sizeof(tempPrecastHoldAnimNameIndices), 0, 0, 0) && result;

  if (!result) {
    ConsoleWrite("Error reading SpellVisualPrecastTransitionsRec", DEFAULT_COLOR);
    return false;
  }

  if (stringBuffer) {
    m_PrecastLoadAnimName = &stringBuffer[tempPrecastLoadAnimNameIndices[0]];
    m_PrecastHoldAnimName = &stringBuffer[tempPrecastHoldAnimNameIndices[0]];
  } else {
    m_PrecastLoadAnimName = "";
    m_PrecastHoldAnimName = "";
  }

  return true;
}
