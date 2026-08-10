#include "CGxDeviceD3d.h"

#include <stpl.h>

#include <storm.h>

#include <ctype.h>
#include <new>
#include <string.h>

static _D3DFORMAT s_depthFormat[4] = {D3DFMT_D16, D3DFMT_D24X8, D3DFMT_D24S8, D3DFMT_D32};
static _D3DFORMAT s_colorFormat[4] = {D3DFMT_R5G6B5, D3DFMT_X8R8G8B8, D3DFMT_A8R8G8B8, D3DFMT_A2R10G10B10};

template <>
void TSFixedArray<CGxMonitorMode>::ReallocData(UINT count) {
  CGxMonitorMode *oldData = m_data;
  CGxMonitorMode *newData;
  UINT            copyCount;
  UINT            index;

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
UINT TSGrowableArray<CGxMonitorMode>::CalcChunkSize(UINT count) {
  UINT chunk = count;
  UINT next;

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
UINT TSGrowableArray<CGxMonitorMode>::RoundToChunk(UINT count, UINT chunk) const {
  UINT remainder = count % chunk;
  return remainder ? count + chunk - remainder : count;
}

BOOL CGxDevice::D3dEnumFormats(TSGrowableArray<CGxFormat> &formats) {
  CGxFormat      fmt;
  D3DDISPLAYMODE dm;
  UINT           nModes;
  _D3DFORMAT     format;
  HINSTANCE      d3dLib = 0;
  UINT           mode;
  IDirect3D9    *d3d = 0;

  if (!CGxDeviceD3d::ILoadD3dLib(d3dLib, d3d)) {
    return 0;
  }

  for (UINT i = 0; i < 4; ++i) {
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

static WORD HToI(LPCSTR h, UINT count) {
  WORD value = 0;
  char c;
  int  cValue;

  while (count) {
    c = *h++;
    cValue = c;
    value = static_cast<WORD>(value * 16);
    if (isxdigit(cValue)) {
      if (isdigit(cValue)) {
        value = static_cast<WORD>(value + cValue - '0');
      } else {
        value = static_cast<WORD>(value + toupper(cValue) - 'A' + 10);
      }
    }
    --count;
  }

  return value;
}

BOOL CGxDevice::AdapterID(WORD &vendorID, WORD &deviceID, DWORD &driverVersionHi, DWORD &driverVersionLow) {
  D3DADAPTER_IDENTIFIER9 adapterId;
  DISPLAY_DEVICEA  dd;
  HINSTANCE              d3dLib;
  IDirect3D9            *d3d;
  UINT                   displayIndex;
  WORD                   parsedVendorID;
  WORD                   parsedDeviceID;
  int                    retVal;

  vendorID = 0xFFFF;
  deviceID = 0xFFFF;
  driverVersionHi = 0;
  driverVersionLow = 0;

  memset(&dd, 0, sizeof(dd));
  displayIndex = 0;
  retVal = 0;
  dd.cb = sizeof(dd);
  if (!EnumDisplayDevicesA(0, displayIndex, &dd, 0)) {
    goto d3dFallback;
  }
  while (!(dd.StateFlags & 4)) {
    ++displayIndex;
    if (!EnumDisplayDevicesA(0, displayIndex, &dd, 0)) {
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
      vendorID = static_cast<WORD>(adapterId.VendorId);
      deviceID = static_cast<WORD>(adapterId.DeviceId);
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

BOOL CGxDevice::AdapterInfer(WORD &deviceID) {
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
    if ((caps.DevCaps & 0x10000) && caps.MaxSimultaneousTextures > 2 && static_cast<WORD>(caps.PixelShaderVersion) >= 0x101) {
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

BOOL CGxDevice::AdapterMonitorModes(TSGrowableArray<CGxMonitorMode> &modes) {
  DISPLAY_DEVICEA dd;
  DEVMODEA        dm;
  CGxMonitorMode       *mode;
  UINT                  modeIndex;

  modes.SetCount(0);
  dd.cb = sizeof(dd);
  EnumDisplayDevicesA(0, 0, &dd, 0);
  if (!(dd.StateFlags & 1)) {
    return 0;
  }

  modeIndex = 0;
  dm.dmSize = sizeof(dm);
  while (EnumDisplaySettingsA(dd.DeviceName, modeIndex, &dm)) {
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

BOOL CGxDevice::AdapterDesktopMode(CGxMonitorMode &mode) {
  DISPLAY_DEVICEA dd;
  DEVMODEA        dm;

  dd.cb = sizeof(dd);
  EnumDisplayDevicesA(0, 0, &dd, 0);
  if (!(dd.StateFlags & 1)) {
    return 0;
  }

  dm.dmSize = sizeof(dm);
  if (!EnumDisplaySettingsA(dd.DeviceName, 0xFFFFFFFF, &dm)) {
    return 0;
  }

  mode.size.x = dm.dmPelsWidth;
  mode.size.y = dm.dmPelsHeight;
  mode.bpp = dm.dmBitsPerPel;
  mode.refreshRate = dm.dmDisplayFrequency;
  return 1;
}
