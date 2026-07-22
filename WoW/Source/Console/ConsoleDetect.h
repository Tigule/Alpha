#pragma once

#include <DB/DBClient/AutoCode/VideoHardwareRec.h>
#include <DB/WowClientDB.h>
#include <Gx/Gx.h>

struct CpuHardware {
  unsigned int farclipIdx;
  unsigned int animatingDoodadIdx;
  unsigned int waterLODIdx;
  unsigned int particleDensityIdx;
  unsigned int smallCullDistIdx;
  unsigned int unitDrawDistIdx;
};

struct SoundHardware {
  unsigned int  numChannels;
  unsigned char fivePointOne;
};

struct Hardware {
  struct Device {
    unsigned short vendorID;
    unsigned short deviceID;
    unsigned long  driverVersionHi;
    unsigned long  driverVersionLo;
  };

  Device                  videoDevice;
  Device                  soundDevice;
  unsigned int            cpuIdx;
  unsigned int            videoIdx;
  unsigned int            soundIdx;
  unsigned int            memIdx;
  const VideoHardwareRec *videoHw;
  const CpuHardware      *cpuHw;
  const SoundHardware    *soundHw;
};

struct DefaultSettings {
  float            farClip;
  float            terrainLODDist;
  unsigned int     terrainShadowLOD;
  unsigned int     detailDoodadDensity;
  unsigned int     detailDoodadAlpha;
  bool             animatingDoodads;
  bool             trilinear;
  unsigned int     numLights;
  bool             specularity;
  unsigned int     waterLOD;
  float            particleDensity;
  float            unitDrawDist;
  float            smallCull;
  float            distCull;
  const CGxFormat *format;
  unsigned int     baseMipLevel;
  unsigned int     numChannels;
  unsigned char    fivePointOne;
};

extern WowClientDB<VideoHardwareRec> g_videoHardwareDB;

void __fastcall DetectHardware(Hardware &hardware, bool &changed);
void __fastcall SaveHardware(const Hardware &hardware, bool &changed);
void __fastcall SetDefaults(DefaultSettings &defaults, const Hardware &hardware);
void __fastcall SetDefaultsFormat(DefaultSettings &defaults, const Hardware &hardware);
