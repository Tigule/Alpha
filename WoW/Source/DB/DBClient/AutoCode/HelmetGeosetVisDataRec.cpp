#include "HelmetGeosetVisDataRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR HelmetGeosetVisDataRec::GetFilename() {
  return "DBFilesClient\\HelmetGeosetVisData.dbc";
}

HelmetGeosetVisDataRec::HelmetGeosetVisDataRec() {
}

HelmetGeosetVisDataRec::~HelmetGeosetVisDataRec() {
}

bool HelmetGeosetVisDataRec::Read(SFile *f, LPCSTR stringBuffer) {

  if (!SFileReadTyped(f, &m_ID) ||
      !SFile::Read(f, &m_DefaultFlags[0], sizeof(m_DefaultFlags), 0, 0, 0) ||
      !SFile::Read(f, &m_PreferredFlags[0], sizeof(m_PreferredFlags), 0, 0, 0) ||
      !SFile::Read(f, &m_HideFlags[0], sizeof(m_HideFlags), 0, 0, 0)) {
    ConsoleWrite("Error reading HelmetGeosetVisDataRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
