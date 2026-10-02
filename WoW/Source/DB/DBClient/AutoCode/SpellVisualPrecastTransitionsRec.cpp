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
  int  error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &tempPrecastLoadAnimNameIndices[0]) == 0);
  error |= (SFileReadTyped(f, &tempPrecastHoldAnimNameIndices) == 0);

  if (error) {
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
