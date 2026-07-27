#include "CGxDeviceD3d.h"

#include <stpl.h>

#include <storm.h>

#include <ctype.h>
#include <new>
#include <string.h>

struct DISPLAY_DEVICE_TARGET {
  unsigned int cb;
  char         DeviceName[32];
  char         DeviceString[128];
  unsigned int StateFlags;
  char         DeviceID[128];
  char         DeviceKey[128];
};

struct DEVMODE_TARGET {
  char           dmDeviceName[32];
  unsigned short dmSpecVersion;
  unsigned short dmDriverVersion;
  unsigned short dmSize;
  unsigned short dmDriverExtra;
  unsigned int   dmFields;
  union {
    struct {
      short dmOrientation;
      short dmPaperSize;
      short dmPaperLength;
      short dmPaperWidth;
    };
    POINTL dmPosition;
  };
  short          dmScale;
  short          dmCopies;
  short          dmDefaultSource;
  short          dmPrintQuality;
  short          dmColor;
  short          dmDuplex;
  short          dmYResolution;
  short          dmTTOption;
  short          dmCollate;
  char           dmFormName[32];
  unsigned short dmLogPixels;
  unsigned int   dmBitsPerPel;
  unsigned int   dmPelsWidth;
  unsigned int   dmPelsHeight;
  union {
    unsigned int dmDisplayFlags;
    unsigned int dmNup;
  };
  unsigned int dmDisplayFrequency;
  unsigned int dmICMMethod;
  unsigned int dmICMIntent;
  unsigned int dmMediaType;
  unsigned int dmDitherType;
  unsigned int dmReserved1;
  unsigned int dmReserved2;
  unsigned int dmPanningWidth;
  unsigned int dmPanningHeight;
};

typedef BOOL(__stdcall *ENUM_DISPLAY_DEVICES)(void *, unsigned long, void *, unsigned long);

#define EnumDisplayDevicesTarget(device, index, displayDevice, flags)                                             \
  (GetProcAddress(GetModuleHandleA("user32.dll"), "EnumDisplayDevicesA") &&                                       \
   reinterpret_cast<ENUM_DISPLAY_DEVICES>(GetProcAddress(GetModuleHandleA("user32.dll"), "EnumDisplayDevicesA"))( \
       device, index, displayDevice, flags                                                                        \
   ))

static _D3DFORMAT s_depthFormat[4] = {D3DFMT_D16, D3DFMT_D24X8, D3DFMT_D24S8, D3DFMT_D32};
static _D3DFORMAT s_colorFormat[4] = {D3DFMT_R5G6B5, D3DFMT_X8R8G8B8, D3DFMT_A8R8G8B8, D3DFMT_A2R10G10B10};

template <>
void TSFixedArray<CGxMonitorMode>::ReallocData(unsigned int count) {
  CGxMonitorMode *oldData = m_data;
  CGxMonitorMode *newData;
  unsigned int    copyCount;
  unsigned int    index;

  m_alloc = count;
  newData = static_cast<CGxMonitorMode *>(SMemReAlloc(oldData, count * sizeof(*newData), MemFileName(), MemLineNo(), 0x10));
  m_data = newData;
  if (newData) {
    return;
  }

  newData = static_cast<CGxMonitorMode *>(SMemAlloc(count * sizeof(*newData), MemFileName(), MemLineNo(), 0));
  m_data = newData;
  if (!oldData) {
    return;
  }

  copyCount = count < m_count ? count : m_count;
  for (index = 0; index < copyCount; ++index) {
    new (&m_data[index]) CGxMonitorMode(oldData[index]);
  }

  SMemFree(oldData, MemFileName(), MemLineNo(), 0);
}

template <>
unsigned int TSGrowableArray<CGxMonitorMode>::CalcChunkSize(unsigned int count) {
  unsigned int chunk = count;
  unsigned int next;

  if (count >= 0x10) {
    m_chunk = 0x10;
    return 0x10;
  }

  next = (count - 1) & count;
  while (next) {
    chunk = next;
    next = (chunk - 1) & chunk;
  }
  return chunk < 1 ? 1 : chunk;
}

template <>
unsigned int TSGrowableArray<CGxMonitorMode>::RoundToChunk(unsigned int count, unsigned int chunk) const {
  unsigned int remainder = count % chunk;
  return remainder ? count + chunk - remainder : count;
}

int CGxDevice::D3dEnumFormats(TSGrowableArray<CGxFormat> &formats) {
  CGxFormat      fmt;
  D3DDISPLAYMODE dm;
  unsigned int   nModes;
  _D3DFORMAT     format;
  HINSTANCE      d3dLib = 0;
  unsigned int   mode;
  IDirect3D9    *d3d = 0;

  if (!CGxDeviceD3d::ILoadD3dLib(d3dLib, d3d)) {
    return 0;
  }

  for (unsigned int i = 0; i < 4; ++i) {
    format = s_colorFormat[i];
    nModes = d3d->GetAdapterModeCount(0, format);
    for (mode = 0; mode < nModes; ++mode) {
      if (d3d->EnumAdapterModes(0, format, mode, &dm) < 0 || dm.Width < 640 || dm.Height < 480) {
        continue;
      }

      memset(&fmt, 0, sizeof(fmt));
      fmt.size.x = dm.Width;
      fmt.size.y = dm.Height;
      fmt.apiSpecificModeID = mode;
      fmt.refreshRate = dm.RefreshRate;
      formats.Add(1, &fmt);
    }
  }

  CGxDeviceD3d::IUnloadD3dLib(d3dLib, d3d);
  return formats.Count() != 0;
}

CGxDevice *CGxDevice::NewD3d() {
  return NEW(CGxDeviceD3d);
}

static unsigned short HToI(const char *h, unsigned int count) {
  unsigned short value = 0;
  char           c;
  int            cValue;

  while (count) {
    c = *h++;
    cValue = c;
    value = static_cast<unsigned short>(value * 16);
    if (isxdigit(cValue)) {
      if (isdigit(cValue)) {
        value = static_cast<unsigned short>(value + cValue - '0');
      } else {
        value = static_cast<unsigned short>(value + toupper(cValue) - 'A' + 10);
      }
    }
    --count;
  }

  return value;
}

int CGxDevice::AdapterID(
    unsigned short &vendorID,
    unsigned short &deviceID,
    unsigned long  &driverVersionHi,
    unsigned long  &driverVersionLow
) {
  D3DADAPTER_IDENTIFIER9 adapterId;
  DISPLAY_DEVICE_TARGET  dd;
  HINSTANCE              d3dLib;
  IDirect3D9            *d3d;
  unsigned int           displayIndex;
  unsigned short         parsedVendorID;
  unsigned short         parsedDeviceID;
  int                    retVal;

  vendorID = 0xFFFF;
  deviceID = 0xFFFF;
  driverVersionHi = 0;
  driverVersionLow = 0;

  memset(&dd, 0, sizeof(dd));
  displayIndex = 0;
  retVal = 0;
  dd.cb = sizeof(dd);
  if (!EnumDisplayDevicesTarget(0, displayIndex, &dd, 0)) {
    goto d3dFallback;
  }
  while (!(dd.StateFlags & 4)) {
    ++displayIndex;
    if (!EnumDisplayDevicesTarget(0, displayIndex, &dd, 0)) {
      goto d3dFallback;
    }
  }

  if (strlen(dd.DeviceID) >= 0x15) {
    parsedVendorID = HToI(&dd.DeviceID[8], 4);
    parsedDeviceID = HToI(&dd.DeviceID[17], 4);
    if (parsedVendorID && parsedDeviceID) {
      vendorID = parsedVendorID;
      deviceID = parsedDeviceID;
      retVal = 1;
      goto done;
    }
  }

d3dFallback:
  d3dLib = 0;
  d3d = 0;
  if (CGxDeviceD3d::ILoadD3dLib(d3dLib, d3d)) {
    if (d3d->GetAdapterIdentifier(0, 0, &adapterId) >= 0) {
      vendorID = static_cast<unsigned short>(adapterId.VendorId);
      deviceID = static_cast<unsigned short>(adapterId.DeviceId);
      driverVersionHi = adapterId.DriverVersion.HighPart;
      driverVersionLow = adapterId.DriverVersion.LowPart;
      retVal = 1;
    }
    CGxDeviceD3d::IUnloadD3dLib(d3dLib, d3d);
  }

done:
  Log("CGxDevice::DeviceAdapterID(): RET: %d, VID: %x, DID: %x, DVER: %x.%x", retVal, vendorID, deviceID, driverVersionHi, driverVersionLow);
  return retVal;
}

int CGxDevice::AdapterInfer(unsigned short &deviceID) {
  D3DCAPS9    caps;
  HINSTANCE   d3dLib;
  IDirect3D9 *d3d;
  int         retVal = 0;

  deviceID = 0;
  d3dLib = 0;
  d3d = 0;
  if (!CGxDeviceD3d::ILoadD3dLib(d3dLib, d3d)) {
    return 0;
  }

  if (d3d->GetDeviceCaps(0, D3DDEVTYPE_HAL, &caps) >= 0) {
    if ((caps.DevCaps & 0x10000) && caps.MaxSimultaneousTextures > 2 && static_cast<unsigned short>(caps.PixelShaderVersion) >= 0x101) {
      deviceID = 2;
    } else if ((caps.DevCaps & 0x10000) && caps.MaxSimultaneousTextures >= 2) {
      deviceID = 1;
    } else {
      deviceID = 0;
    }
    retVal = 1;
  }

  CGxDeviceD3d::IUnloadD3dLib(d3dLib, d3d);
  Log("CGxDevice::DeviceAdapterInfer(): RET: %d, DID: %x", retVal, deviceID);
  return retVal;
}

int CGxDevice::AdapterMonitorModes(TSGrowableArray<CGxMonitorMode> &modes) {
  DISPLAY_DEVICE_TARGET dd;
  DEVMODE_TARGET        dm;
  CGxMonitorMode       *mode;
  unsigned int          modeIndex;

  modes.SetCount(0);
  dd.cb = sizeof(dd);
  EnumDisplayDevicesTarget(0, 0, &dd, 0);
  if (!(dd.StateFlags & 1)) {
    return 0;
  }

  modeIndex = 0;
  dm.dmSize = sizeof(dm);
  while (EnumDisplaySettingsA(dd.DeviceName, modeIndex, reinterpret_cast<DEVMODEA *>(&dm))) {
    if (dm.dmPelsWidth >= 0x280 && dm.dmPelsHeight >= 0x1E0 && dm.dmBitsPerPel >= 0x10 && dm.dmDisplayFrequency >= 0x3C) {
      mode = modes.New();
      mode->size.x = dm.dmPelsWidth;
      mode->size.y = dm.dmPelsHeight;
      mode->bpp = dm.dmBitsPerPel;
      mode->refreshRate = dm.dmDisplayFrequency;
    }
    ++modeIndex;
  }

  return modes.Count() != 0;
}

int CGxDevice::AdapterDesktopMode(CGxMonitorMode &mode) {
  DISPLAY_DEVICE_TARGET dd;
  DEVMODE_TARGET        dm;

  dd.cb = sizeof(dd);
  EnumDisplayDevicesTarget(0, 0, &dd, 0);
  if (!(dd.StateFlags & 1)) {
    return 0;
  }

  dm.dmSize = sizeof(dm);
  if (!EnumDisplaySettingsA(dd.DeviceName, 0xFFFFFFFF, reinterpret_cast<DEVMODEA *>(&dm))) {
    return 0;
  }

  mode.size.x = dm.dmPelsWidth;
  mode.size.y = dm.dmPelsHeight;
  mode.bpp = dm.dmBitsPerPel;
  mode.refreshRate = dm.dmDisplayFrequency;
  return 1;
}
