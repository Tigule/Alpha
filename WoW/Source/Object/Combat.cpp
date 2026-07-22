#include "Object/UnitCombat.h"

#include "Object/Object.h"

unsigned __int64 CCombat::IsAttacking() {
  return m_victim;
}

void CCombat::SetClientInitData(CClientObjCreate &init) {
  m_victim = init.victim;
}

void CCombatClient::SetAttackSent(unsigned __int64 victim) {
  m_victim = victim;
  m_attackSent = 1;
}
