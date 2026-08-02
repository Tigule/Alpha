#include <Base/Base.h>

#include "ConsoleDetect.h"

#include <Os/W32/OsGui.h>
#include <Os/OsTime.h>
#include <storm.h>

struct _PCI_VENTABLE {
  unsigned short VenId;
  char          *VenShort;
  char          *VenFull;
};

struct _PCI_DEVTABLE {
  unsigned short VenId;
  unsigned short DevId;
  char          *Chip;
  char          *ChipDesc;
};

#include "ConsoleDetectPciData.inc"

WowClientDB<VideoHardwareRec> g_videoHardwareDB;

static CpuHardware s_cpuHwSettings[2] = {
    {0, 0, 0, 0, 0, 0},
    {1, 1, 1, 1, 1, 1}
};

static SoundHardware s_soundHwSettings[1] = {
    {1, 0}
};

static CGxFormat s_formats[7] = {
    CGxFormat(false, NTempest::C2iVector(640, 480), CGxFormat::Fmt_Rgb565, CGxFormat::Fmt_Ds160, 60, true, true, false),
    CGxFormat(false, NTempest::C2iVector(800, 600), CGxFormat::Fmt_Rgb565, CGxFormat::Fmt_Ds160, 60, true, true, false),
    CGxFormat(false, NTempest::C2iVector(640, 480), CGxFormat::Fmt_ArgbX888, CGxFormat::Fmt_Ds24X, 60, true, true, false),
    CGxFormat(false, NTempest::C2iVector(800, 600), CGxFormat::Fmt_ArgbX888, CGxFormat::Fmt_Ds24X, 60, true, true, false),
    CGxFormat(false, NTempest::C2iVector(1024, 768), CGxFormat::Fmt_ArgbX888, CGxFormat::Fmt_Ds24X, 60, true, true, false),
    CGxFormat(false, NTempest::C2iVector(1280, 1024), CGxFormat::Fmt_ArgbX888, CGxFormat::Fmt_Ds24X, 60, true, true, false),
    CGxFormat(false, NTempest::C2iVector(1600, 1200), CGxFormat::Fmt_ArgbX888, CGxFormat::Fmt_Ds24X, 60, true, true, false)
};

static float s_terrainLODDist[4] = {80.0f, 80.0f, 100.0f, 100.0f};

static unsigned int s_detailDoodadDensity[4] = {8, 12, 16, 24};

static unsigned int s_animatingDoodads[2][2] = {
    {0, 0},
    {0, 1}
};

static unsigned int s_waterLOD[2][2] = {
    {0, 0},
    {0, 1}
};

static float s_particleDensity[4][2] = {
    {0.3f, 0.4f},
    {0.5f, 0.6f},
    {0.9f, 1.0f},
    {1.0f, 1.0f}
};

static float s_unitDrawDist[4][2] = {
    { 35.0f,  50.0f},
    { 50.0f,  75.0f},
    { 75.0f, 125.0f},
    {120.0f, 150.0f}
};

static float s_smallCull[4][2] = {
    {0.08f, 0.08f},
    {0.07f, 0.07f},
    {0.07f, 0.07f},
    {0.04f, 0.04f}
};

static float s_distCull[4][2] = {
    {350.0f, 350.0f},
    {400.0f, 400.0f},
    {450.0f, 450.0f},
    {500.0f, 500.0f}
};

static float s_farClip[4][2] = {
    {200.0f, 250.0f},
    {300.0f, 350.0f},
    {350.0f, 400.0f},
    {450.0f, 500.0f}
};

static const char  REGKEY[11] = "WoW\\Client";
static const char *HWCPUIDX = "HWCpuIdx";
static const char *HWMEMIDX = "HWMemIdx";
static const char *HWVIDEOIDX = "HWVideoIdx";
static const char *HWSOUNDIDX = "HWSoundIdx";

static void PrintUnknownHardware(const Hardware &hardware) {
  char         caption[512];
  char         msg[1024];
  unsigned int i;

  SStrPrintf(msg, sizeof(msg), "Vendor id = 0x%04X\nDevice id = 0x%04X\n\n", hardware.videoDevice.vendorID, hardware.videoDevice.deviceID);

  for (i = 0; i < sizeof(PciVenTable) / sizeof(PciVenTable[0]); ++i) {
    if (PciVenTable[i].VenId == hardware.videoDevice.vendorID) {
      SStrPack(msg, "Vendor name = ", sizeof(msg));
      SStrPack(msg, PciVenTable[i].VenShort, sizeof(msg));
      SStrPack(msg, "\n", sizeof(msg));
      break;
    }
  }

  for (i = 0; i < sizeof(PciDevTable) / sizeof(PciDevTable[0]); ++i) {
    if (PciDevTable[i].VenId == hardware.videoDevice.vendorID && PciDevTable[i].DevId == hardware.videoDevice.deviceID) {
      SStrPack(msg, "Device name = ", sizeof(msg));
      SStrPack(msg, PciDevTable[i].Chip, sizeof(msg));
      SStrPack(msg, "\n", sizeof(msg));
      break;
    }
  }

  SStrCopy(caption, "Send to Tim", 0x7FFFFFFF);
  OsGuiMessageBox(OsGuiGetWindow(2), 0, msg, caption);
}

static void SetVideoIdx(Hardware &hardware) {
  int numRecords = g_videoHardwareDB.GetNumRecords();
  int i;

  for (i = 1; i < numRecords; ++i) {
    const VideoHardwareRec *video = g_videoHardwareDB.GetRecordByIndex(i);

    if (hardware.videoDevice.vendorID == video->m_vendorID && hardware.videoDevice.deviceID == video->m_deviceID) {
      hardware.videoIdx = i;
      break;
    }
  }
}

void DetectHardware(Hardware &hardware, bool &changed) {
  char str[1024];

  g_videoHardwareDB.Load();

  if (OsGetAsyncClocksPerSecond() * 0.000001f > 1500.0f) {
    hardware.cpuIdx = 1;
  } else {
    hardware.cpuIdx = 0;
  }
  hardware.memIdx = 0;
  hardware.videoIdx = 0;

  if (GxAdapterID(
          hardware.videoDevice.vendorID, hardware.videoDevice.deviceID, hardware.videoDevice.driverVersionHi, hardware.videoDevice.driverVersionLo
      ))
  {
    SetVideoIdx(hardware);

#if 0
    // tigule: annoying popup disabled for now
    if (!hardware.videoIdx && hardware.videoDevice.vendorID && hardware.videoDevice.deviceID) {
      PrintUnknownHardware(hardware);
    }
#endif
  }

  if (!hardware.videoIdx) {
    hardware.videoDevice.vendorID = 0xFFFF;

    if (GxAdapterInfer(hardware.videoDevice.deviceID)) {
      SetVideoIdx(hardware);
    }
  }

  hardware.cpuHw = &s_cpuHwSettings[hardware.cpuIdx];
  hardware.soundIdx = 0;
  hardware.videoHw = g_videoHardwareDB.GetRecordByIndex(hardware.videoIdx);
  hardware.soundHw = &s_soundHwSettings[hardware.soundIdx];

  SaveHardware(hardware, changed);

  GxLog("DetectHardware():");

  SStrPrintf(str, sizeof(str), "\tcpuIdx: %d", hardware.cpuIdx);
  GxLog(str);

  SStrPrintf(str, sizeof(str), "\tvideoIdx: %d", hardware.videoIdx);
  GxLog(str);

  SStrPrintf(str, sizeof(str), "\tsoundIdx: %d", hardware.soundIdx);
  GxLog(str);

  SStrPrintf(str, sizeof(str), "\tmemIdx: %d", hardware.memIdx);
  GxLog(str);
}

void SaveHardware(const Hardware &hardware, bool &changed) {
  int memIdx = -1;
  int soundIdx = -1;
  int videoIdx = -1;
  int cpuIdx = -1;

  changed = false;

  if (SRegLoadValue(REGKEY, HWCPUIDX, 0, reinterpret_cast<unsigned long *>(&cpuIdx)) &&
      SRegLoadValue(REGKEY, HWMEMIDX, 0, reinterpret_cast<unsigned long *>(&memIdx)) &&
      SRegLoadValue(REGKEY, HWVIDEOIDX, 0, reinterpret_cast<unsigned long *>(&videoIdx)) &&
      SRegLoadValue(REGKEY, HWSOUNDIDX, 0, reinterpret_cast<unsigned long *>(&soundIdx)))
  {
    if (hardware.cpuIdx != static_cast<unsigned int>(cpuIdx) || hardware.videoIdx != static_cast<unsigned int>(videoIdx) ||
        hardware.soundIdx != static_cast<unsigned int>(soundIdx) || hardware.memIdx != static_cast<unsigned int>(memIdx))
    {
      if (!OsGuiMessageBox(OsGuiGetWindow(2), 2, "Hardware changed.  Reload default settings?", "")) {
        changed = true;
      }
    }
  }

  SRegSaveValue(REGKEY, HWCPUIDX, 0, hardware.cpuIdx);
  SRegSaveValue(REGKEY, HWMEMIDX, 0, hardware.memIdx);
  SRegSaveValue(REGKEY, HWVIDEOIDX, 0, hardware.videoIdx);
  SRegSaveValue(REGKEY, HWSOUNDIDX, 0, hardware.soundIdx);
}

void SetDefaults(DefaultSettings &defaults, const Hardware &hardware) {
  defaults.farClip = s_farClip[hardware.videoHw->m_farclipIdx][hardware.cpuHw->farclipIdx];
  defaults.terrainLODDist = s_terrainLODDist[hardware.videoHw->m_terrainLODDistIdx];
  defaults.terrainShadowLOD = hardware.videoHw->m_terrainShadowLOD;
  defaults.detailDoodadDensity = s_detailDoodadDensity[hardware.videoHw->m_detailDoodadDensityIdx];
  defaults.detailDoodadAlpha = hardware.videoHw->m_detailDoodadAlpha;
  defaults.animatingDoodads = s_animatingDoodads[hardware.videoHw->m_animatingDoodadIdx][hardware.cpuHw->animatingDoodadIdx];
  defaults.trilinear = hardware.videoHw->m_trilinear != 0;
  defaults.numLights = hardware.videoHw->m_numLights;
  defaults.specularity = hardware.videoHw->m_specularity != 0;
  defaults.waterLOD = s_waterLOD[hardware.videoHw->m_waterLODIdx][hardware.cpuHw->waterLODIdx];
  defaults.particleDensity = s_particleDensity[hardware.videoHw->m_particleDensityIdx][hardware.cpuHw->particleDensityIdx];
  defaults.unitDrawDist = s_unitDrawDist[hardware.videoHw->m_unitDrawDistIdx][hardware.cpuHw->unitDrawDistIdx];
  defaults.smallCull = s_smallCull[hardware.videoHw->m_smallCullDistIdx][hardware.cpuHw->smallCullDistIdx];
  defaults.distCull = s_distCull[hardware.videoHw->m_smallCullDistIdx][hardware.cpuHw->smallCullDistIdx];
  defaults.format = &s_formats[hardware.videoHw->m_resolutionIdx];
  defaults.baseMipLevel = hardware.videoHw->m_baseMipLevel;
  defaults.numChannels = 0;
  defaults.fivePointOne = 0;
}

void SetDefaultsFormat(DefaultSettings &defaults, const Hardware &hardware) {
  defaults.format = &s_formats[hardware.videoHw->m_resolutionIdx];
}
