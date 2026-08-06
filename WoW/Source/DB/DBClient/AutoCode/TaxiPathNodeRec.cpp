#include "TaxiPathNodeRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

LPCSTR TaxiPathNodeRec::GetFilename() {
  return "DBFilesClient\\TaxiPathNode.dbc";
}

TaxiPathNodeRec::TaxiPathNodeRec() {
}

TaxiPathNodeRec::~TaxiPathNodeRec() {
}

bool TaxiPathNodeRec::Read(SFile *f, LPCSTR stringBuffer) {
  bool result = true;

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_PathID, sizeof(m_PathID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_NodeIndex, sizeof(m_NodeIndex), 0, 0, 0) && result;
  result = SFile::Read(f, &m_ContinentID, sizeof(m_ContinentID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_LocX, sizeof(m_LocX), 0, 0, 0) && result;
  result = SFile::Read(f, &m_LocY, sizeof(m_LocY), 0, 0, 0) && result;
  result = SFile::Read(f, &m_LocZ, sizeof(m_LocZ), 0, 0, 0) && result;
  result = SFile::Read(f, &m_flags, sizeof(m_flags), 0, 0, 0) && result;

  if (!result) {
    ConsoleWrite("Error reading TaxiPathNodeRec", DEFAULT_COLOR);
  }

  return result;
}
