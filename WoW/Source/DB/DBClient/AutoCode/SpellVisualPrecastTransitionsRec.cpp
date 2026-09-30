#include "SpellVisualPrecastTransitionsRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR SpellVisualPrecastTransitionsRec::GetFilename() {
  return "DBFilesClient\\SpellVisualPrecastTransitions.dbc";
}

SpellVisualPrecastTransitionsRec::SpellVisualPrecastTransitionsRec() {
}

SpellVisualPrecastTransitionsRec::~SpellVisualPrecastTransitionsRec() {
}

bool SpellVisualPrecastTransitionsRec::Read(SFile *f, LPCSTR stringBuffer) {
  UINT tempPrecastLoadAnimNameIndices[1];
  UINT tempPrecastHoldAnimNameIndices[1];

  if (!SFileReadTyped(f, &m_ID) ||
      !SFileReadTyped(f, &tempPrecastLoadAnimNameIndices[0]) ||
      !SFile::Read(f, tempPrecastHoldAnimNameIndices, sizeof(tempPrecastHoldAnimNameIndices), 0, 0, 0)) {
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
