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

  if (!SFileReadTyped(f, &m_ID) ||
      !SFileReadTyped(f, &m_PathID) ||
      !SFileReadTyped(f, &m_NodeIndex) ||
      !SFileReadTyped(f, &m_ContinentID) ||
      !SFileReadTyped(f, &m_LocX) ||
      !SFileReadTyped(f, &m_LocY) ||
      !SFileReadTyped(f, &m_LocZ) ||
      !SFileReadTyped(f, &m_flags)) {
    ConsoleWrite("Error reading TaxiPathNodeRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
