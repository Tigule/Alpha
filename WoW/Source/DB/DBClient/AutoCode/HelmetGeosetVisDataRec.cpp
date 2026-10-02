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
  int error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &m_DefaultFlags) == 0);
  error |= (SFileReadTyped(f, &m_PreferredFlags) == 0);
  error |= (SFileReadTyped(f, &m_HideFlags) == 0);

  if (error) {
    ConsoleWrite("Error reading HelmetGeosetVisDataRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
