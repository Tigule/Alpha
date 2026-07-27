#include "VideoHardwareRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

const char *VideoHardwareRec::GetFilename() {
  return "DBFilesClient\\VideoHardware.dbc";
}

VideoHardwareRec::VideoHardwareRec() {
}

VideoHardwareRec::~VideoHardwareRec() {
}

bool VideoHardwareRec::Read(SFile *f, const char *stringBuffer) {
  bool result = true;

  result = SFile::Read(f, &m_vendorID, sizeof(m_vendorID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_deviceID, sizeof(m_deviceID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_farclipIdx, sizeof(m_farclipIdx), 0, 0, 0) && result;
  result = SFile::Read(f, &m_terrainLODDistIdx, sizeof(m_terrainLODDistIdx), 0, 0, 0) && result;
  result = SFile::Read(f, &m_terrainShadowLOD, sizeof(m_terrainShadowLOD), 0, 0, 0) && result;
  result = SFile::Read(f, &m_detailDoodadDensityIdx, sizeof(m_detailDoodadDensityIdx), 0, 0, 0) && result;
  result = SFile::Read(f, &m_detailDoodadAlpha, sizeof(m_detailDoodadAlpha), 0, 0, 0) && result;
  result = SFile::Read(f, &m_animatingDoodadIdx, sizeof(m_animatingDoodadIdx), 0, 0, 0) && result;
  result = SFile::Read(f, &m_trilinear, sizeof(m_trilinear), 0, 0, 0) && result;
  result = SFile::Read(f, &m_numLights, sizeof(m_numLights), 0, 0, 0) && result;
  result = SFile::Read(f, &m_specularity, sizeof(m_specularity), 0, 0, 0) && result;
  result = SFile::Read(f, &m_waterLODIdx, sizeof(m_waterLODIdx), 0, 0, 0) && result;
  result = SFile::Read(f, &m_particleDensityIdx, sizeof(m_particleDensityIdx), 0, 0, 0) && result;
  result = SFile::Read(f, &m_unitDrawDistIdx, sizeof(m_unitDrawDistIdx), 0, 0, 0) && result;
  result = SFile::Read(f, &m_smallCullDistIdx, sizeof(m_smallCullDistIdx), 0, 0, 0) && result;
  result = SFile::Read(f, &m_resolutionIdx, sizeof(m_resolutionIdx), 0, 0, 0) && result;
  result = SFile::Read(f, &m_baseMipLevel, sizeof(m_baseMipLevel), 0, 0, 0) && result;
  result = SFile::Read(f, &m_oglPixelShader, sizeof(m_oglPixelShader), 0, 0, 0) && result;
  result = SFile::Read(f, &m_d3dPixelShader, sizeof(m_d3dPixelShader), 0, 0, 0) && result;

  if (!result) {
    ConsoleWrite("Error reading VideoHardwareRec", DEFAULT_COLOR);
  }

  return result;
}
