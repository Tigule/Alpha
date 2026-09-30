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

  if (!SFileReadTyped(f, &m_ID) ||
      !SFileReadTyped(f, &m_faction) ||
      !SFileReadTyped(f, &m_factionGroup) ||
      !SFileReadTyped(f, &m_friendGroup) ||
      !SFileReadTyped(f, &m_enemyGroup) ||
      !SFile::Read(f, &m_enemies[0], sizeof(m_enemies), 0, 0, 0) ||
      !SFile::Read(f, &m_friend[0], sizeof(m_friend), 0, 0, 0)) {
    ConsoleWrite("Error reading FactionTemplateRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
