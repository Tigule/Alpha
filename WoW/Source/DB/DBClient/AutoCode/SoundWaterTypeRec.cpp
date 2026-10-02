#include "SoundWaterTypeRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR SoundWaterTypeRec::GetFilename() {
  return "DBFilesClient\\SoundWaterType.dbc";
}

SoundWaterTypeRec::SoundWaterTypeRec() {
}

SoundWaterTypeRec::~SoundWaterTypeRec() {
}

bool SoundWaterTypeRec::Read(SFile *f, LPCSTR stringBuffer) {
  int error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &m_soundType) == 0);
  error |= (SFileReadTyped(f, &m_soundSubtype) == 0);
  error |= (SFileReadTyped(f, &m_SoundID) == 0);

  if (error) {
    ConsoleWrite("Error reading SoundWaterTypeRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
