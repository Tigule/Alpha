#include "Object/UnitCombat.h"

#include "Object/Object.h"

void CCombat::SetClientInitData(CClientObjCreate &init) {
  m_victim = init.victim;
}

unsigned __int64 CCombat::IsAttacking() {
  return m_victim;
}

void CCombatClient::SetAttackSent(unsigned __int64 victim) {
  m_victim = victim;
  m_attackSent = 1;
}
