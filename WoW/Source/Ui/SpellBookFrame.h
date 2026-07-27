#ifndef WOW_SOURCE_UI_SPELLBOOKFRAME_H
#define WOW_SOURCE_UI_SPELLBOOKFRAME_H

#include <WowServices/BitField.h>

enum UI_SPELL_TYPE {
  PLAYER_SPELL = 0,
  PLAYER_ABILITY = 1,
  PET_SPELL = 2
};

class CGSpellBook {
  friend class CGGameObject_C;

 public:
  static void InitializeGame();
  static void ShutdownGame();
  static void ClearSpells();
  static unsigned int IsSpellKnown(int spellID);
  static unsigned int IsPetSpellKnown(int spellID);
  static void ClearPetSpells();
  static void AddPetSpell(int spellID);
  static void                    SetKnowsPetSpells() {
    m_knowsPetSpells = 1;
  }
  static void UpdateSpells();
  static void ReplaceSpell(int oldSpell, int newSpell);
  static void UpdateSelection();
  static void UpdateCooldowns();
  static int             GetLanguageSpell(unsigned int language) {
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
  static void SetSpell(int slot, int spellID, UI_SPELL_TYPE type);
  static void SendSpellSlot(int slot, UI_SPELL_TYPE type);
  static void PickupSpell(int slot, UI_SPELL_TYPE type);
  static void CastSpell(int slot, UI_SPELL_TYPE type);
  static int GetSpell(unsigned int slot, UI_SPELL_TYPE type);
  static int IsSelectedSlot(int slot, UI_SPELL_TYPE type);
  static int IsToggledSpell(int slot, UI_SPELL_TYPE type);
  static TSGrowableArray<int> &GetShapeshiftForms() {
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
  static int                  m_knownSpells[1024];
  static int                  m_knownAbilities[1024];
  static int                  m_petSpells[1024];
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
