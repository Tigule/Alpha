#include "FactionTemplateRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR FactionTemplateRec::GetFilename() {
  return "DBFilesClient\\FactionTemplate.dbc";
}

FactionTemplateRec::FactionTemplateRec() {
}

FactionTemplateRec::~FactionTemplateRec() {
}

bool FactionTemplateRec::Read(SFile *f, LPCSTR stringBuffer) {
  int error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &m_faction) == 0);
  error |= (SFileReadTyped(f, &m_factionGroup) == 0);
  error |= (SFileReadTyped(f, &m_friendGroup) == 0);
  error |= (SFileReadTyped(f, &m_enemyGroup) == 0);
  error |= (SFileReadTyped(f, &m_enemies) == 0);
  error |= (SFileReadTyped(f, &m_friend) == 0);

  if (error) {
    ConsoleWrite("Error reading FactionTemplateRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
