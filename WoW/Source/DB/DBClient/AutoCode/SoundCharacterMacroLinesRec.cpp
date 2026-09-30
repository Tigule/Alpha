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

  if (!SFileReadTyped(f, &m_ID) ||
      !SFileReadTyped(f, &m_Category) ||
      !SFileReadTyped(f, &m_Sex) ||
      !SFileReadTyped(f, &m_Race) ||
      !SFileReadTyped(f, &m_SoundID)) {
    ConsoleWrite("Error reading SoundCharacterMacroLinesRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
