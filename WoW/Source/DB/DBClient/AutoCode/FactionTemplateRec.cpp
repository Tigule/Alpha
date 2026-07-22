#include "FactionTemplateRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

const char *__fastcall FactionTemplateRec::GetFilename() {
  return "DBFilesClient\\FactionTemplate.dbc";
}

FactionTemplateRec::FactionTemplateRec() {
}

FactionTemplateRec::~FactionTemplateRec() {
}

bool FactionTemplateRec::Read(SFile *f, const char *stringBuffer) {
  bool result = true;

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_faction, sizeof(m_faction), 0, 0, 0) && result;
  result = SFile::Read(f, &m_factionGroup, sizeof(m_factionGroup), 0, 0, 0) && result;
  result = SFile::Read(f, &m_friendGroup, sizeof(m_friendGroup), 0, 0, 0) && result;
  result = SFile::Read(f, &m_enemyGroup, sizeof(m_enemyGroup), 0, 0, 0) && result;
  result = SFile::Read(f, &m_enemies[0], sizeof(m_enemies), 0, 0, 0) && result;
  result = SFile::Read(f, &m_friend[0], sizeof(m_friend), 0, 0, 0) && result;

  if (!result) {
    ConsoleWrite("Error reading FactionTemplateRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
