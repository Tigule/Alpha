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

  if (!SFileReadTyped(f, &m_vendorID) ||
      !SFileReadTyped(f, &m_deviceID) ||
      !SFileReadTyped(f, &m_farclipIdx) ||
      !SFileReadTyped(f, &m_terrainLODDistIdx) ||
      !SFileReadTyped(f, &m_terrainShadowLOD) ||
      !SFileReadTyped(f, &m_detailDoodadDensityIdx) ||
      !SFileReadTyped(f, &m_detailDoodadAlpha) ||
      !SFileReadTyped(f, &m_animatingDoodadIdx) ||
      !SFileReadTyped(f, &m_trilinear) ||
      !SFileReadTyped(f, &m_numLights) ||
      !SFileReadTyped(f, &m_specularity) ||
      !SFileReadTyped(f, &m_waterLODIdx) ||
      !SFileReadTyped(f, &m_particleDensityIdx) ||
      !SFileReadTyped(f, &m_unitDrawDistIdx) ||
      !SFileReadTyped(f, &m_smallCullDistIdx) ||
      !SFileReadTyped(f, &m_resolutionIdx) ||
      !SFileReadTyped(f, &m_baseMipLevel) ||
      !SFileReadTyped(f, &m_oglPixelShader) ||
      !SFileReadTyped(f, &m_d3dPixelShader)) {
    ConsoleWrite("Error reading VideoHardwareRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
