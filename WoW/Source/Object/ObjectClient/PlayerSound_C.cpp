#include "Object/ObjectClient/Unit_C.h"
#include "Object/ObjectClient/Player_C.h"
#include "Object/ObjectClient/Item_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "DB/DBClient/AutoCode/MaterialRec.h"
#include "DB/DBClient/AutoCode/SpellRec.h"
#include "DB/DBClient/AutoCode/SpellVisualRec.h"
#include "DB/DBClient/DBCacheInstances.h"
#include "Client.h"
#include "Services/SysMessage.h"
#include "SoundInterface/SoundInterface.h"

int GetSoundID(const CreatureSoundDataRec *soundData, UNITSOUNDTYPE soundType);
int CheckUnitSoundTimer(UNITSOUNDTYPE soundType);

static unsigned int s_playerSoundChances[16] = {35, 100, 30, 100, 100, 100, 40, 100, 100, 100, 100, 100, 100, 100, 100, 100};

static int CheckPlayerPlaySound(UNITSOUNDTYPE soundType) {
  FATALASSERT(soundType < NUM_UNITSOUNDTYPES);
  unsigned int random = NTempest::CRandom::uint32_(g_rndSeed);
  unsigned int value = NTempest::CMath::mulhwu_(random, 101);
  return s_playerSoundChances[soundType] >= value;
}

void CGPlayer_C::PlayUnitSound(UNITSOUNDTYPE soundType, int alwaysPlay) const {
  if (soundType != UNITSOUNDTYPE_FOOTFALL &&
      (alwaysPlay || CheckPlayerPlaySound(soundType)) &&
      CheckUnitSoundTimer(soundType)) {
    int soundID = GetSoundID(GetSoundData(), soundType);
    if (soundID) {
      NTempest::C3Vector position = GetPosition();
      position.z += 2.0f;
      SndInterfacePlaySound(soundID, position, -1, 1.0f);
    }
  }
}

void CGPlayer_C::PlayFoleySound() const {
  const CGBag_C *inventory = GetBag();
  FATALASSERT(inventory);

  CGItem_C *item =
      static_cast<CGItem_C *>(ClntObjMgrObjectPtr(inventory->GetItem(4), __FILE__, __LINE__));
  if (item) {
    SndInterfacePlayFoleySound(item->GetMaterial(), GetPosition());
  }
}

void CGPlayer_C::HandleSpellEventSound() {
  if (m_currentTorsoAnimState == ANIM_STATE_EMOTE || !m_castingSpell) {
    return;
  }

  const SpellRec *spell = g_spellDB.GetRecord(m_castingSpell);
  if (!spell) {
    SysMsgPrintf(SYSMSG_WARNING, 2, "NOSPELLIDFOUND|%d", m_castingSpell);
    return;
  }

  SpellVisualRec visRecData;
  const SpellVisualRec *visual = GetAppropriateSpellVisual(spell, visRecData);
  if (!visual) {
    SysMsgPrintf(SYSMSG_WARNING, 2, "SPELLVISUALIDNOTFOUND|%d", spell->m_spellVisualID);
    return;
  }

  if (visual->m_animEventSoundID) {
    NTempest::C3Vector position = GetPosition();
    position.z += 1.0f;
    SndInterfacePlaySound(visual->m_animEventSoundID, position, -1, 1.0f);
  }
}

unsigned int CGPlayer_C::GetImpactType() const {
  const CGBag_C *inventory = GetBag();
  CGItem_C *item =
      static_cast<CGItem_C *>(ClntObjMgrObjectPtr(inventory->GetItem(4), __FILE__, __LINE__));
  if (!item) {
    return 0;
  }

  const MaterialRec *material = g_materialDB.GetRecord(item->GetMaterial());
  if (!material) {
    return 0;
  }

  if (material->m_flags & 2) {
    return 2;
  }
  return (material->m_flags & 4) >> 2;
}

void PlayerInitializeSounds() {
}

void PlayerShutdownSounds() {
}
