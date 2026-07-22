#include "UISoundLookupsRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

const char *__fastcall UISoundLookupsRec::GetFilename() {
  return "DBFilesClient\\UISoundLookups.dbc";
}

UISoundLookupsRec::UISoundLookupsRec() {
}

UISoundLookupsRec::~UISoundLookupsRec() {
}

bool UISoundLookupsRec::Read(SFile *f, const char *stringBuffer) {
  bool         result = true;
  unsigned int tempSoundNameIndices[1];

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_SoundID, sizeof(m_SoundID), 0, 0, 0) && result;
  result = SFile::Read(f, &tempSoundNameIndices[0], sizeof(tempSoundNameIndices[0]), 0, 0, 0) && result;

  if (!result) {
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
