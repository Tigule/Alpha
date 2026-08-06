#pragma once

#include <DB/WowClientDB.h>

class SpellChainEffectsRec {
 public:
  SpellChainEffectsRec();
  ~SpellChainEffectsRec();

  static LPCSTR GetFilename();

  static UINT GetNumColumns() {
    return 8;
  }

  static UINT GetRowSize() {
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

  bool Read(SFile *f, LPCSTR stringBuffer);

  int    m_ID;
  float  m_AvgSegLen;
  float  m_Width;
  float  m_NoiseScale;
  float  m_TexCoordScale;
  int    m_SegDuration;
  int    m_SegDelay;
  LPCSTR m_Texture;
};

extern WowClientDB<SpellChainEffectsRec> g_spellChainEffectsDB;
