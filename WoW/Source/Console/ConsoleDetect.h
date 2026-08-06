#pragma once

#include <DB/DBClient/AutoCode/VideoHardwareRec.h>
#include <DB/WowClientDB.h>
#include <Gx/Gx.h>

struct CpuHardware {
  UINT farclipIdx;
  UINT animatingDoodadIdx;
  UINT waterLODIdx;
  UINT particleDensityIdx;
  UINT smallCullDistIdx;
  UINT unitDrawDistIdx;
};

struct SoundHardware {
  UINT numChannels;
  BYTE fivePointOne;
};

struct Hardware {
  struct Device {
    WORD  vendorID;
    WORD  deviceID;
    DWORD driverVersionHi;
    DWORD driverVersionLo;
  };

  Device                  videoDevice;
  Device                  soundDevice;
  UINT                    cpuIdx;
  UINT                    videoIdx;
  UINT                    soundIdx;
  UINT                    memIdx;
  const VideoHardwareRec *videoHw;
  const CpuHardware      *cpuHw;
  const SoundHardware    *soundHw;
};

struct DefaultSettings {
  float            farClip;
  float            terrainLODDist;
  UINT             terrainShadowLOD;
  UINT             detailDoodadDensity;
  UINT             detailDoodadAlpha;
  bool             animatingDoodads;
  bool             trilinear;
  UINT             numLights;
  bool             specularity;
  UINT             waterLOD;
  float            particleDensity;
  float            unitDrawDist;
  float            smallCull;
  float            distCull;
  const CGxFormat *format;
  UINT             baseMipLevel;
  UINT             numChannels;
  BYTE             fivePointOne;
};

extern WowClientDB<VideoHardwareRec> g_videoHardwareDB;

void DetectHardware(Hardware &hardware, bool &changed);
void SaveHardware(const Hardware &hardware, bool &changed);
void SetDefaults(DefaultSettings &defaults, const Hardware &hardware);
void SetDefaultsFormat(DefaultSettings &defaults, const Hardware &hardware);
