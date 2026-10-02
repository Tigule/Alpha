#include "TaxiPathNodeRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR TaxiPathNodeRec::GetFilename() {
  return "DBFilesClient\\TaxiPathNode.dbc";
}

TaxiPathNodeRec::TaxiPathNodeRec() {
}

TaxiPathNodeRec::~TaxiPathNodeRec() {
}

bool TaxiPathNodeRec::Read(SFile *f, LPCSTR stringBuffer) {
  int error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &m_PathID) == 0);
  error |= (SFileReadTyped(f, &m_NodeIndex) == 0);
  error |= (SFileReadTyped(f, &m_ContinentID) == 0);
  error |= (SFileReadTyped(f, &m_LocX) == 0);
  error |= (SFileReadTyped(f, &m_LocY) == 0);
  error |= (SFileReadTyped(f, &m_LocZ) == 0);
  error |= (SFileReadTyped(f, &m_flags) == 0);

  if (error) {
    ConsoleWrite("Error reading TaxiPathNodeRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
