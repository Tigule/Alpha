#pragma once

#include <DB/WowClientDB.h>

class GroundEffectTextureRec {
 public:
  GroundEffectTextureRec();
  ~GroundEffectTextureRec();

  static const char *GetFilename();

  static unsigned int GetNumColumns() {
    return 12;
  }

  static unsigned int GetRowSize() {
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

  bool Read(SFile *f, const char *stringBuffer);

  int         m_ID;
  int         m_datestamp;
  int         m_continentId;
  int         m_zoneId;
  int         m_textureId;
  const char *m_textureName;
  int         m_doodadId[4];
  int         m_density;
  int         m_sound;
};

extern WowClientDB<GroundEffectTextureRec> g_groundEffectTextureDB;
