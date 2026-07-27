#pragma once

#include <storm.h>

class VideoHardwareRec {
 public:
  VideoHardwareRec();
  ~VideoHardwareRec();

  static const char *GetFilename();

  static unsigned int GetNumColumns() {
    return 19;
  }

  static unsigned int GetRowSize() {
    return 76;
  }

  int GetID() const {
    return m_generatedID;
  }

  bool NeedIDAssigned() {
    return true;
  }

  void SetID(int id) {
    m_generatedID = id;
  }

  bool Read(SFile *f, const char *stringBuffer);

  int m_vendorID;
  int m_deviceID;
  int m_farclipIdx;
  int m_terrainLODDistIdx;
  int m_terrainShadowLOD;
  int m_detailDoodadDensityIdx;
  int m_detailDoodadAlpha;
  int m_animatingDoodadIdx;
  int m_trilinear;
  int m_numLights;
  int m_specularity;
  int m_waterLODIdx;
  int m_particleDensityIdx;
  int m_unitDrawDistIdx;
  int m_smallCullDistIdx;
  int m_resolutionIdx;
  int m_baseMipLevel;
  int m_oglPixelShader;
  int m_d3dPixelShader;
  int m_generatedID;
};
