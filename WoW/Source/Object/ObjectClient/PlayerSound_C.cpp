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

int __fastcall GetSoundID(CreatureSoundDataRec *soundData, UNITSOUNDTYPE soundType);
int __fastcall CheckUnitSoundTimer(UNITSOUNDTYPE soundType);

static unsigned int s_playerSoundChances[16] = {35, 100, 30, 100, 100, 100, 40, 100, 100, 100, 100, 100, 100, 100, 100, 100};

int __fastcall CheckPlayerPlaySound(UNITSOUNDTYPE soundType) {
  FATALASSERT(static_cast<unsigned int>(soundType) < 16);
  unsigned int random = NTempest::CRandom::uint32_(g_rndSeed);
  unsigned int value = static_cast<unsigned int>((static_cast<unsigned __int64>(101) * random) >> 32);
  return s_playerSoundChances[soundType] >= value;
}

void CGPlayer_C::PlayUnitSound(UNITSOUNDTYPE soundType, int alwaysPlay) const {
  if (soundType == 8 || (!alwaysPlay && !CheckPlayerPlaySound(soundType))) {
    return;
  }
  if (!CheckUnitSoundTimer(soundType)) {
    return;
  }
  int soundID = GetSoundID(m_soundData, soundType);
  if (!soundID) {
    return;
  }
  NTempest::C3Vector position;
  GetPosition(position);
  position.z += 2.0f;
  SndInterfacePlaySound(soundID, position, -1, 1.0f);
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
  if (m_currentTorsoAnimState == 46 || !m_castingSpell) {
    return;
  }

  SpellRec *spell = g_spellDB.GetRecord(m_castingSpell);
  if (!spell) {
    SysMsgPrintf(SYSMSG_WARNING, 2, "NOSPELLIDFOUND|%d", m_castingSpell);
    return;
  }

  SpellVisualRec visualData;
  const SpellVisualRec *visual = GetAppropriateSpellVisual(spell, visualData);
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

  MaterialRec *material = g_materialDB.GetRecord(item->GetMaterial());
  if (!material) {
    return 0;
  }

  if (material->m_flags & 2) {
    return 2;
  }
  return (material->m_flags & 4) >> 2;
}

const VirtualItemInfo *CGPlayer_C::GetDefendingItem() const {
  const CGBag_C *inventory = GetBag();
  CGItem_C *item =
      static_cast<CGItem_C *>(ClntObjMgrObjectPtr(inventory->GetItem(4), __FILE__, __LINE__));
  return item ? &item->m_itemInfo : CGUnit_C::GetDefendingItem();
}

void __fastcall PlayerInitializeSounds() {
}

void __fastcall PlayerShutdownSounds() {
}
