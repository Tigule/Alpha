#ifndef WOW_SOURCE_UI_SPELLBOOKFRAME_H
#define WOW_SOURCE_UI_SPELLBOOKFRAME_H

#include <WowServices/BitField.h>

#define MAXIMUM_LEARNED_SPELLS 1024

enum UI_SPELL_TYPE {
  PLAYER_SPELL = 0,
  PLAYER_ABILITY = 1,
  PET_SPELL = 2,
  NUM_SPELL_TYPES = 3
};

class CGSpellBook {
  friend class CGGameObject_C;

 public:
  static void InitializeGame();
  static void ShutdownGame();
  static void ClearSpells();
  static BYTE IsSpellKnown(int spellID) {
    return m_knownSpellBits.IsBitSet(spellID);
  }
  static BYTE IsPetSpellKnown(int spellID) {
    for (UINT i = 0; i < MAXIMUM_LEARNED_SPELLS; ++i) {
      if (m_petSpells[i] == spellID) {
        return 1;
      }
    }
    return 0;
  }
  static void ClearPetSpells();
  static void AddPetSpell(int spellID);
  static void SetKnowsPetSpells() {
    m_knowsPetSpells = 1;
  }
  static void UpdateSpells();
  static void ReplaceSpell(int oldSpell, int newSpell);
  static void UpdateSelection();
  static void UpdateCooldowns();
  static int  GetLanguageSpell(UINT language) {
    return m_languageSpells[language];
  }
  static int GetStuckSpell() {
    return m_stuckSpell;
  }
  static int GetDuelSpell() {
    return m_duelSpell;
  }

  static void AddKnownSpell(int spellID, int slot, int learned);
  static void DelKnownSpell(int spellID);
  static void PickupSpell(int slot, UI_SPELL_TYPE type);
  static void CastSpell(int slot, UI_SPELL_TYPE type);
  static int  GetSpell(UINT slot, UI_SPELL_TYPE type) {
    if (slot >= MAXIMUM_LEARNED_SPELLS) {
      return 0;
    }
    if (type == PLAYER_SPELL) {
      return m_knownSpells[slot];
    }
    if (type == PLAYER_ABILITY) {
      return m_knownAbilities[slot];
    }
    return type == PET_SPELL ? m_petSpells[slot] : 0;
  }
  static BOOL                        IsSelectedSlot(int slot, UI_SPELL_TYPE type);
  static BOOL                        IsToggledSpell(int slot, UI_SPELL_TYPE type);
  static const TSGrowableArray<int> &GetUnlockSpells();
  static const TSGrowableArray<int> &GetShapeshiftForms() {
    return m_shapeshiftForms;
  }
  static int KnowsSpells() {
    return m_knowsSpells;
  }
  static int KnowsPetSpells() {
    return m_knowsPetSpells;
  }

 private:
  static FBitField            m_knownSpellBits;
  static int                  m_knownSpells[MAXIMUM_LEARNED_SPELLS];
  static int                  m_knownAbilities[MAXIMUM_LEARNED_SPELLS];
  static int                  m_petSpells[MAXIMUM_LEARNED_SPELLS];
  static int                  m_duelSpell;
  static int                  m_stuckSpell;
  static TSFixedArray<int>    m_languageSpells;
  static TSGrowableArray<int> m_unlockSpells;
  static TSGrowableArray<int> m_shapeshiftForms;
  static int                  m_selectedSlot;
  static UI_SPELL_TYPE        m_selectedType;
  static int                  m_knowsSpells;
  static int                  m_knowsPetSpells;

 protected:
  static void SetSpell(int slot, int spellID, UI_SPELL_TYPE type);
  static void SendSpellSlot(int slot, UI_SPELL_TYPE type);
};

#endif
