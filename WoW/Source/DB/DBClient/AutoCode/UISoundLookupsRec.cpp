#include "UISoundLookupsRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR UISoundLookupsRec::GetFilename() {
  return "DBFilesClient\\UISoundLookups.dbc";
}

UISoundLookupsRec::UISoundLookupsRec() {
}

UISoundLookupsRec::~UISoundLookupsRec() {
}

bool UISoundLookupsRec::Read(SFile *f, LPCSTR stringBuffer) {
  UINT tempSoundNameIndices[1];

  if (!SFileReadTyped(f, &m_ID) ||
      !SFileReadTyped(f, &m_SoundID) ||
      !SFileReadTyped(f, &tempSoundNameIndices[0])) {
    ConsoleWrite("Error reading UISoundLookupsRec", DEFAULT_COLOR);
    return false;
  }

  if (stringBuffer) {
    m_SoundName = stringBuffer + tempSoundNameIndices[0];
  } else {
    m_SoundName = "";
  }

  return true;
}
