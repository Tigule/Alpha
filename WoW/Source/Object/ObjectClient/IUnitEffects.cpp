#include "IUnitEffects.h"

#include "DB/DBClient/AutoCode/SpellVisualEffectNameRec.h"

void __fastcall LoadUnitDefs() {
  for (int i = 0; i < g_spellVisualEffectNameDB.GetNumRecords(); ++i) {
    const SpellVisualEffectNameRec *effect = g_spellVisualEffectNameDB.GetRecordByIndex(i);
    if (effect && effect->m_specialID >= 0 && effect->m_specialID < 43) {
      g_specialSpellIDs[effect->m_specialID] = effect->m_ID;
    }
  }
}
