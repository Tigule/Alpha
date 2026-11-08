#include <Base/Base.h>

#include "ConsoleDetect.h"

#include <Os/W32/OsGui.h>
#include <Os/OsTime.h>
#include <storm.h>

struct _PCI_VENTABLE {
  WORD  VenId;
  char *VenShort;
  char *VenFull;
};

struct _PCI_DEVTABLE {
  WORD  VenId;
  WORD  DevId;
  char *Chip;
  char *ChipDesc;
};

struct _PCI_CLASSCODETABLE {
  BYTE  BaseClass;
  BYTE  SubClass;
  BYTE  ProgIf;
  char *BaseDesc;
  char *SubDesc;
  char *ProgDesc;
};

#include "ConsoleDetectPciData.inc"

_PCI_CLASSCODETABLE PciClassCodeTable[] = {
    {0x00, 0x00, 0x00, "Pre-2.0 PCI Specification Device", "Non-VGA", ""},
    {0x00, 0x01, 0x00, "Pre-2.0 PCI Specification Device", "VGA Compatible", ""},
    {0x01, 0x00, 0x00, "Mass Storage Controller", "SCSI", ""},
    {0x01, 0x01, 0x00, "Mass Storage Controller", "IDE", ""},
    {0x01, 0x02, 0x00, "Mass Storage Controller", "Floppy", ""},
    {0x01, 0x03, 0x00, "Mass Storage Controller", "IPI", ""},
    {0x01, 0x04, 0x00, "Mass Storage Controller", "RAID", ""},
    {0x01, 0x80, 0x00, "Mass Storage Controller", "Other", ""},
    {0x02, 0x00, 0x00, "Network Controller", "Ethernet", ""},
    {0x02, 0x01, 0x00, "Network Controller", "Token Ring", ""},
    {0x02, 0x02, 0x00, "Network Controller", "FDDI", ""},
    {0x02, 0x03, 0x00, "Network Controller", "ATM", ""},
    {0x02, 0x04, 0x00, "Network Controller", "ISDN", ""},
    {0x02, 0x80, 0x00, "Network Controller", "Other", ""},
    {0x03, 0x00, 0x00, "Display Controller", "PC Compatible", "VGA"},
    {0x03, 0x00, 0x01, "Display Controller", "PC Compatible", "8514"},
    {0x03, 0x01, 0x00, "Display Controller", "XGA", ""},
    {0x03, 0x02, 0x00, "Display Controller", "3D", ""},
    {0x03, 0x80, 0x00, "Display Controller", "Other", ""},
    {0x04, 0x00, 0x00, "Multimedia Device", "Video", ""},
    {0x04, 0x01, 0x00, "Multimedia Device", "Audio", ""},
    {0x04, 0x02, 0x00, "Multimedia Device", "Telephony", ""},
    {0x04, 0x80, 0x00, "Multimedia Device", "Other", ""},
    {0x05, 0x00, 0x00, "Memory Controller", "RAM", ""},
    {0x05, 0x01, 0x00, "Memory Controller", "Flash", ""},
    {0x05, 0x80, 0x00, "Memory Controller", "Other", ""},
    {0x06, 0x00, 0x00, "Bridge Device", "Host/PCI", ""},
    {0x06, 0x01, 0x00, "Bridge Device", "PCI/ISA", ""},
    {0x06, 0x02, 0x00, "Bridge Device", "PCI/EISA", ""},
    {0x06, 0x03, 0x00, "Bridge Device", "PCI/MCA", ""},
    {0x06, 0x04, 0x00, "Bridge Device", "PCI/PCI", ""},
    {0x06, 0x04, 0x01, "Bridge Device", "PCI/PCI", "Subtractive"},
    {0x06, 0x05, 0x00, "Bridge Device", "PCI/PCMCIA", ""},
    {0x06, 0x06, 0x00, "Bridge Device", "PCI/NuBus", ""},
    {0x06, 0x07, 0x00, "Bridge Device", "PCI/CardBus", ""},
    {0x06, 0x08, 0x00, "Bridge Device", "PCI/RACEway", "Transparent"},
    {0x06, 0x08, 0x01, "Bridge Device", "PCI/RACEway", "End-Point"},
    {0x06, 0x80, 0x00, "Bridge Device", "Other", ""},
    {0x07, 0x00, 0x00, "Simple Communications Controller", "Serial", "Generic XT Compatible"},
    {0x07, 0x00, 0x01, "Simple Communications Controller", "Serial", "16450 Compatible"},
    {0x07, 0x00, 0x02, "Simple Communications Controller", "Serial", "16550 Compatible"},
    {0x07, 0x00, 0x03, "Simple Communications Controller", "Serial", "16650 Compatible"},
    {0x07, 0x00, 0x04, "Simple Communications Controller", "Serial", "16750 Compatible"},
    {0x07, 0x00, 0x05, "Simple Communications Controller", "Serial", "16850 Compatible"},
    {0x07, 0x00, 0x06, "Simple Communications Controller", "Serial", "16950 Compatible"},
    {0x07, 0x01, 0x00, "Simple Communications Controller", "Parallel", "Standard"},
    {0x07, 0x01, 0x01, "Simple Communications Controller", "Parallel", "Bidirectional"},
    {0x07, 0x01, 0x02, "Simple Communications Controller", "Parallel", "ECP 1.X Compliant"},
    {0x07, 0x01, 0x03, "Simple Communications Controller", "Parallel", "IEEE 1284 controller"},
    {0x07, 0x01, 0xFE, "Simple Communications Controller", "Parallel", "IEEE 1284 target"},
    {0x07, 0x02, 0x00, "Simple Communications Controller", "Multiport Serial", ""},
    {0x07, 0x03, 0x00, "Simple Communications Controller", "Generic Modem", ""},
    {0x07, 0x03, 0x01, "Simple Communications Controller", "Hayes-Compatible Modem", "16450"},
    {0x07, 0x03, 0x02, "Simple Communications Controller", "Hayes-Compatible Modem", "16550"},
    {0x07, 0x03, 0x03, "Simple Communications Controller", "Hayes-Compatible Modem", "16650"},
    {0x07, 0x03, 0x04, "Simple Communications Controller", "Hayes-Compatible Modem", "16750"},
    {0x07, 0x80, 0x02, "Simple Communications Controller", "Other", ""},
    {0x08, 0x00, 0x00, "Base Systems Peripheral", "PIC (Programmable Interrupt Controller)", "Generic 8259"},
    {0x08, 0x00, 0x01, "Base Systems Peripheral", "PIC (Programmable Interrupt Controller)", "ISA"},
    {0x08, 0x00, 0x02, "Base Systems Peripheral", "PIC (Programmable Interrupt Controller)", "EISA"},
    {0x08, 0x00, 0x10, "Base Systems Peripheral", "I/O APIC", ""},
    {0x08, 0x00, 0x20, "Base Systems Peripheral", "I/O(x) APIC", ""},
    {0x08, 0x01, 0x00, "Base Systems Peripheral", "DMA (Direct Memory Access)", "Generic 8237"},
    {0x08, 0x01, 0x01, "Base Systems Peripheral", "DMA (Direct Memory Access)", "ISA"},
    {0x08, 0x01, 0x02, "Base Systems Peripheral", "DMA (Direct Memory Access)", "EISA"},
    {0x08, 0x02, 0x00, "Base Systems Peripheral", "System Timer", "Generic 8254"},
    {0x08, 0x02, 0x01, "Base Systems Peripheral", "System Timer", "ISA"},
    {0x08, 0x02, 0x02, "Base Systems Peripheral", "System Timer", "EISA"},
    {0x08, 0x03, 0x00, "Base Systems Peripheral", "RTC (Real Time Clock)", "Generic"},
    {0x08, 0x03, 0x01, "Base Systems Peripheral", "RTC (Real Time Clock)", "ISA"},
    {0x08, 0x04, 0x00, "Base Systems Peripheral", "PCI Hot-Plug Controller", "Generic"},
    {0x08, 0x80, 0x00, "Base Systems Peripheral", "Other", ""},
    {0x09, 0x00, 0x00, "Input Device", "Keyboard", ""},
    {0x09, 0x01, 0x00, "Input Device", "Digitizer (Pen)", ""},
    {0x09, 0x02, 0x00, "Input Device", "Mouse", ""},
    {0x09, 0x03, 0x00, "Input Device", "Scanner", ""},
    {0x09, 0x04, 0x00, "Input Device", "Game Port", "Generic"},
    {0x09, 0x04, 0x10, "Input Device", "Game Port", "Legacy"},
    {0x09, 0x80, 0x00, "Input Device", "Other", ""},
    {0x0A, 0x00, 0x00, "Docking Station", "Generic", ""},
    {0x0A, 0x80, 0x00, "Docking Station", "Other", ""},
    {0x0B, 0x00, 0x00, "Processor", "i386", ""},
    {0x0B, 0x01, 0x00, "Processor", "i486", ""},
    {0x0B, 0x02, 0x00, "Processor", "Pentium", ""},
    {0x0B, 0x10, 0x00, "Processor", "Alpha", ""},
    {0x0B, 0x20, 0x00, "Processor", "Power PC", ""},
    {0x0B, 0x30, 0x00, "Processor", "MIPS", ""},
    {0x0B, 0x40, 0x00, "Processor", "Co-processor", ""},
    {0x0C, 0x00, 0x00, "Serial Bus Controller", "IEEE 1394", "Firewire"},
    {0x0C, 0x00, 0x10, "Serial Bus Controller", "IEEE 1394", "OpenHCI"},
    {0x0C, 0x01, 0x00, "Serial Bus Controller", "ACCESS.bus", ""},
    {0x0C, 0x02, 0x00, "Serial Bus Controller", "SSA (Serial Storage Archetecture)", ""},
    {0x0C, 0x03, 0x00, "Serial Bus Controller", "USB (Universal Serial Bus)", "Universal Host Controller"},
    {0x0C, 0x03, 0x10, "Serial Bus Controller", "USB (Universal Serial Bus)", "Open Host Controller"},
    {0x0C, 0x03, 0x80, "Serial Bus Controller", "USB (Universal Serial Bus)", "Non-specific Controller"},
    {0x0C, 0x03, 0xFE, "Serial Bus Controller", "USB (Universal Serial Bus)", "Device"},
    {0x0C, 0x04, 0x00, "Serial Bus Controller", "Fibre Channel", ""},
    {0x0C, 0x05, 0x00, "Serial Bus Controller", "SMBus", ""},
    {0x0D, 0x00, 0x00, "Wireless", "iRDA-compatible Controller", ""},
    {0x0D, 0x01, 0x00, "Wireless", "Consumer IR Controller", ""},
    {0x0D, 0x10, 0x00, "Wireless", "RF Controller", ""},
    {0x0D, 0x80, 0x00, "Wireless", "Other", ""},
    {0x0E, 0x00, 0x00, "Intelligent IO", "I2O", ""},
    {0x0F, 0x01, 0x00, "Satellite", "TV", ""},
    {0x0F, 0x02, 0x00, "Satellite", "Audio", ""},
    {0x0F, 0x03, 0x00, "Satellite", "Voice", ""},
    {0x0F, 0x04, 0x00, "Satellite", "Data", ""},
    {0x10, 0x00, 0x00, "Encryption", "Network/Computing", ""},
    {0x10, 0x10, 0x00, "Encryption", "Entertainment", ""},
    {0x10, 0x80, 0x00, "Encryption", "Other", ""},
    {0x11, 0x00, 0x00, "Signal Processing", "DPIO", ""},
    {0x11, 0x80, 0x00, "Signal Processing", "Other", ""},
    {0xFF, 0x00, 0x00, "Unknown", "Device Does Not Fit in a Defined Class", "UDF"},
};

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

static UINT s_detailDoodadDensity[4] = {8, 12, 16, 24};

static BYTE s_animatingDoodads[2][2] = {
    {0, 0},
    {0, 1}
};

static UINT s_waterLOD[2][2] = {
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

static const char REGKEY[11] = "WoW\\Client";
static LPCSTR     HWCPUIDX = "HWCpuIdx";
static LPCSTR     HWMEMIDX = "HWMemIdx";
static LPCSTR     HWVIDEOIDX = "HWVideoIdx";
static LPCSTR     HWSOUNDIDX = "HWSoundIdx";

static void PrintUnknownHardware(const Hardware &hardware) {
  char caption[512];
  char msg[1024];
  UINT i;

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

  if (SRegLoadValue(REGKEY, HWCPUIDX, 0, (DWORD *)&cpuIdx) &&
      SRegLoadValue(REGKEY, HWMEMIDX, 0, (DWORD *)&memIdx) &&
      SRegLoadValue(REGKEY, HWVIDEOIDX, 0, (DWORD *)&videoIdx) &&
      SRegLoadValue(REGKEY, HWSOUNDIDX, 0, (DWORD *)&soundIdx))
  {
    if (hardware.cpuIdx != (UINT)cpuIdx || hardware.videoIdx != (UINT)videoIdx ||
        hardware.soundIdx != (UINT)soundIdx || hardware.memIdx != (UINT)memIdx)
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
