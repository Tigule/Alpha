#pragma once

#include <DB/WowClientDB.h>

class GroundEffectTextureRec {
 public:
  GroundEffectTextureRec();
  ~GroundEffectTextureRec();

  static LPCSTR GetFilename();

  static UINT GetNumColumns() {
    return 12;
  }

  static UINT GetRowSize() {
    return 48;
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
  int    m_datestamp;
  int    m_continentId;
  int    m_zoneId;
  int    m_textureId;
  LPCSTR m_textureName;
  int    m_doodadId[4];
  int    m_density;
  int    m_sound;
};

extern WowClientDB<GroundEffectTextureRec> g_groundEffectTextureDB;
