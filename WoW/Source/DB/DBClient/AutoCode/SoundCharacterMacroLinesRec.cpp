#include "SoundCharacterMacroLinesRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR SoundCharacterMacroLinesRec::GetFilename() {
  return "DBFilesClient\\SoundCharacterMacroLines.dbc";
}

SoundCharacterMacroLinesRec::SoundCharacterMacroLinesRec() {
}

SoundCharacterMacroLinesRec::~SoundCharacterMacroLinesRec() {
}

bool SoundCharacterMacroLinesRec::Read(SFile *f, LPCSTR stringBuffer) {
  int error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &m_Category) == 0);
  error |= (SFileReadTyped(f, &m_Sex) == 0);
  error |= (SFileReadTyped(f, &m_Race) == 0);
  error |= (SFileReadTyped(f, &m_SoundID) == 0);

  if (error) {
    ConsoleWrite("Error reading SoundCharacterMacroLinesRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
