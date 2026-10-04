#ifndef WOW_SOURCE_UI_SPELLBOOKFRAME_H
#define WOW_SOURCE_UI_SPELLBOOKFRAME_H

#include <WowServices/BitField.h>

#define MAXIMUM_LEARNED_SPELLS 1024u

enum UI_SPELL_TYPE {
  PLAYER_SPELL = 0,
  PLAYER_ABILITY = 1,
  PET_SPELL = 2,
  NUM_SPELL_TYPES = 3
};

class CGSpellBook {
  friend class CGGameObject_C;
  friend class CGTooltip;

 public:
  static void InitializeGame();
  static void ShutdownGame();
  static void ClearSpells();
  static bool IsSpellKnown(int spellID) {
    return m_knownSpellBits.IsBitSet(spellID);
  }
  static bool IsPetSpellKnown(int spellID) {
    for (UINT i = 0; i < MAXIMUM_LEARNED_SPELLS; ++i) {
      if (m_petSpells[i] == spellID) {
        return 1;
      }
    }
    return 0;
  }

  static void AddKnownSpell(int spellID, int slot, int learned);
  static void DelKnownSpell(int spellID);
  static void ReplaceSpell(int oldSpell, int newSpell);
  static void ClearPetSpells();
  static void AddPetSpell(int spellID);
  static void SetKnowsPetSpells() {
    m_knowsPetSpells = 1;
  }
  static void UpdateSpells();
  static void UpdateSelection();
  static void UpdateCooldowns();
  static void PickupSpell(int slot, UI_SPELL_TYPE type);
  static void CastSpell(int slot, UI_SPELL_TYPE type);
  static int  GetSpell(UINT slot, UI_SPELL_TYPE type) {
    switch (type) {
      case PLAYER_SPELL:
        return m_knownSpells[slot];
      case PLAYER_ABILITY:
        return m_knownAbilities[slot];
      case PET_SPELL:
        return m_petSpells[slot];
      default:
        return 0;
    }
  }
  static int GetDuelSpell() {
    return m_duelSpell;
  }
  static int GetStuckSpell() {
    return m_stuckSpell;
  }
  static int  GetLanguageSpell(UINT language) {
    return m_languageSpells[language];
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

 protected:
  static void SetSpell(int slot, int spellID, UI_SPELL_TYPE type);
  static void SendSpellSlot(int slot, UI_SPELL_TYPE type);

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
};

#endif
