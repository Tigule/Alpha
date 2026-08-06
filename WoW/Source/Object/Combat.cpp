#include <Base/Base.h>
#include <WowConst.h>
#include <MapDefs.h>

#include "Object/UnitCombat.h"

#include "Object/Object.h"

void CCombat::GetClientInitData(CClientObjCreate *init) const {
  init->victim = m_victim;
}

void CCombat::SetClientInitData(const CClientObjCreate &init) {
  m_victim = init.victim;
}

DWORDLONG CCombat::IsAttacking() const {
  return m_victim;
}

void CCombat::SetAttacking(DWORDLONG victim) {
  FATALASSERT(victim);
  m_victim = victim;
}

void CCombatClient::SetAttackSent(DWORDLONG victim) {
  m_victim = victim;
  m_attackSent = 1;
}
