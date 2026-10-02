#include "VideoHardwareRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR VideoHardwareRec::GetFilename() {
  return "DBFilesClient\\VideoHardware.dbc";
}

VideoHardwareRec::VideoHardwareRec() {
}

VideoHardwareRec::~VideoHardwareRec() {
}

bool VideoHardwareRec::Read(SFile *f, LPCSTR stringBuffer) {
  int error = 0;

  error |= (SFileReadTyped(f, &m_vendorID) == 0);
  error |= (SFileReadTyped(f, &m_deviceID) == 0);
  error |= (SFileReadTyped(f, &m_farclipIdx) == 0);
  error |= (SFileReadTyped(f, &m_terrainLODDistIdx) == 0);
  error |= (SFileReadTyped(f, &m_terrainShadowLOD) == 0);
  error |= (SFileReadTyped(f, &m_detailDoodadDensityIdx) == 0);
  error |= (SFileReadTyped(f, &m_detailDoodadAlpha) == 0);
  error |= (SFileReadTyped(f, &m_animatingDoodadIdx) == 0);
  error |= (SFileReadTyped(f, &m_trilinear) == 0);
  error |= (SFileReadTyped(f, &m_numLights) == 0);
  error |= (SFileReadTyped(f, &m_specularity) == 0);
  error |= (SFileReadTyped(f, &m_waterLODIdx) == 0);
  error |= (SFileReadTyped(f, &m_particleDensityIdx) == 0);
  error |= (SFileReadTyped(f, &m_unitDrawDistIdx) == 0);
  error |= (SFileReadTyped(f, &m_smallCullDistIdx) == 0);
  error |= (SFileReadTyped(f, &m_resolutionIdx) == 0);
  error |= (SFileReadTyped(f, &m_baseMipLevel) == 0);
  error |= (SFileReadTyped(f, &m_oglPixelShader) == 0);
  error |= (SFileReadTyped(f, &m_d3dPixelShader) == 0);

  if (error) {
    ConsoleWrite("Error reading VideoHardwareRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
