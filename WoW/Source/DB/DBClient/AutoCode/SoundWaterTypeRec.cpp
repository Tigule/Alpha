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

  if (!SFileReadTyped(f, &m_ID) ||
      !SFileReadTyped(f, &m_soundType) ||
      !SFileReadTyped(f, &m_soundSubtype) ||
      !SFileReadTyped(f, &m_SoundID)) {
    ConsoleWrite("Error reading SoundWaterTypeRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
