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
  static void __fastcall         InitializeGame();
  static void __fastcall         ShutdownGame();
  static void __fastcall         ClearSpells();
  static unsigned int __fastcall IsSpellKnown(int spellID);
  static unsigned int __fastcall IsPetSpellKnown(int spellID);
  static void __fastcall         ClearPetSpells();
  static void __fastcall         AddPetSpell(int spellID);
  static void                    SetKnowsPetSpells() {
    m_knowsPetSpells = 1;
  }
  static void __fastcall UpdateSpells();
  static void __fastcall ReplaceSpell(int oldSpell, int newSpell);
  static void __fastcall UpdateSelection();
  static void __fastcall UpdateCooldowns();
  static int             GetLanguageSpell(unsigned int language) {
    return m_languageSpells[language];
  }
  static int GetStuckSpell() {
    return m_stuckSpell;
  }
  static int GetDuelSpell() {
    return m_duelSpell;
  }

  static void __fastcall                  AddKnownSpell(int spellID, int slot, int learned);
  static void __fastcall                  DelKnownSpell(int spellID);
  static void __fastcall                  SetSpell(int slot, int spellID, UI_SPELL_TYPE type);
  static void __fastcall                  SendSpellSlot(int slot, UI_SPELL_TYPE type);
  static void __fastcall                  PickupSpell(int slot, UI_SPELL_TYPE type);
  static void __fastcall                  CastSpell(int slot, UI_SPELL_TYPE type);
  static int __fastcall                   GetSpell(unsigned int slot, UI_SPELL_TYPE type);
  static int __fastcall                   IsSelectedSlot(int slot, UI_SPELL_TYPE type);
  static int __fastcall                   IsToggledSpell(int slot, UI_SPELL_TYPE type);
  static TSGrowableArray<int> &__fastcall GetShapeshiftForms() {
    return m_shapeshiftForms;
  }
  static int __fastcall KnowsSpells() {
    return m_knowsSpells;
  }
  static int __fastcall KnowsPetSpells() {
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
