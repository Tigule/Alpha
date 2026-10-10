#include <Base/Base.h>

#include "OsMemory.h"

#include <windows.h>

COsSharedMemory::COsSharedMemory() : m_data(0), m_size(0) {
  *(HANDLE *)m_opaqueData = 0;
}

COsSharedMemory::~COsSharedMemory() {
  Destroy();
}

bool COsSharedMemory::Initialize(LPCSTR name, UINT size, int mode) {
  DWORD access = FILE_MAP_WRITE;

  switch (mode) {
    case 0:
    case 2:
      *(HANDLE *)m_opaqueData = CreateFileMappingA(INVALID_HANDLE_VALUE, 0, 0x08000004, 0, size, name);
      break;

    case 3:
      access = FILE_MAP_READ;

    case 1:
      *(HANDLE *)m_opaqueData = OpenFileMappingA(access, FALSE, name);
      break;

    default:
      return true;
  }

  if (!*(HANDLE *)m_opaqueData) {
    return true;
  }

  if (GetLastError() == ERROR_ALREADY_EXISTS && mode == 0) {
    CloseHandle(*(HANDLE *)m_opaqueData);
    *(HANDLE *)m_opaqueData = 0;
    return true;
  }

  m_data = MapViewOfFile(*(HANDLE *)m_opaqueData, access, 0, 0, 0);
  if (!m_data) {
    CloseHandle(*(HANDLE *)m_opaqueData);
    *(HANDLE *)m_opaqueData = 0;
    return true;
  }

  m_size = size;
  return false;
}

bool COsSharedMemory::ChangeAccess(int newAccess) {
  DWORD oldAccess;
  DWORD protection;

  if (!*(HANDLE *)m_opaqueData) {
    return true;
  }
  if (!m_data) {
    return true;
  }
  if (!m_size) {
    return true;
  }

  switch (newAccess) {
    case 0:
      protection = PAGE_READONLY;
      break;

    case 1:
      protection = PAGE_READWRITE;
      break;

    default:
      return true;
  }

  return !VirtualProtect(m_data, m_size, protection, &oldAccess);
}

void COsSharedMemory::Destroy() {
  HANDLE mapping;

  if (m_data) {
    UnmapViewOfFile(m_data);
    m_data = 0;
  }

  mapping = *(HANDLE *)m_opaqueData;
  if (mapping) {
    CloseHandle(mapping);
    *(HANDLE *)m_opaqueData = 0;
  }
}
