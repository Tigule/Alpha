#pragma once

#include <DB/WowClientDB.h>

class SpellChainEffectsRec {
 public:
  SpellChainEffectsRec();
  ~SpellChainEffectsRec();

  static const char *GetFilename();

  static unsigned int GetNumColumns() {
    return 8;
  }

  static unsigned int GetRowSize() {
    return 32;
  }

  int GetID() const {
    return m_ID;
  }

  bool NeedIDAssigned() {
    return false;
  }

  void SetID(int id) {
  }

  bool Read(SFile *f, const char *stringBuffer);

  int         m_ID;
  float       m_AvgSegLen;
  float       m_Width;
  float       m_NoiseScale;
  float       m_TexCoordScale;
  int         m_SegDuration;
  int         m_SegDelay;
  const char *m_Texture;
};

extern WowClientDB<SpellChainEffectsRec> g_spellChainEffectsDB;
