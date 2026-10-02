#include <Base/Base.h>
#include <Gx/Gx.h>
#include <MapDefs.h>
#include <WorldClient/World.h>
#include "WowServices/WowConnection.h"
#include <WowConst.h>
#include <Frame/CSimpleTop.h>
#include "Object/ObjectClient/Unit_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "SoundInterface/SoundInterface.h"
#include "Ui/WorldFrame.h"
#include "Ui/GameUI.h"

#include "IUnitEffects.h"

#include "DB/DBClient/AutoCode/SpellVisualEffectNameRec.h"

void LoadUnitDefs() {
  for (int i = 0; i < g_spellVisualEffectNameDB.GetNumRecords(); ++i) {
    const SpellVisualEffectNameRec *effect = g_spellVisualEffectNameDB.GetRecordByIndex(i);
    if (effect && effect->m_specialID >= 0 && effect->m_specialID < 43) {
      g_specialSpellIDs[effect->m_specialID] = effect->m_ID;
    }
  }
}
