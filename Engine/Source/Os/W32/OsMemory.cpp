#include <Base/Base.h>

#include "OsMemory.h"

#include <windows.h>

COsSharedMemory::COsSharedMemory() : m_data(0), m_size(0) {
  *reinterpret_cast<HANDLE *>(m_opaqueData) = 0;
}

COsSharedMemory::~COsSharedMemory() {
  Destroy();
}

bool COsSharedMemory::Initialize(const char *name, unsigned int size, int mode) {
  DWORD  access = FILE_MAP_WRITE;
  HANDLE mapping = 0;

  switch (mode) {
    case 0:
    case 2:
      mapping = CreateFileMappingA(INVALID_HANDLE_VALUE, 0, 0x08000004, 0, size, name);
      break;

    case 1:
      mapping = OpenFileMappingA(access, FALSE, name);
      break;

    case 3:
      access = FILE_MAP_READ;
      mapping = OpenFileMappingA(access, FALSE, name);
      break;

    default:
      return true;
  }

  *reinterpret_cast<HANDLE *>(m_opaqueData) = mapping;
  if (!mapping) {
    return true;
  }

  if (GetLastError() == ERROR_ALREADY_EXISTS && mode == 0) {
    CloseHandle(mapping);
    *reinterpret_cast<HANDLE *>(m_opaqueData) = 0;
    return true;
  }

  m_data = MapViewOfFile(mapping, access, 0, 0, 0);
  if (!m_data) {
    CloseHandle(mapping);
    *reinterpret_cast<HANDLE *>(m_opaqueData) = 0;
    return true;
  }

  m_size = size;
  return false;
}

bool COsSharedMemory::ChangeAccess(int newAccess) {
  DWORD oldAccess;
  DWORD protection;

  if (!*reinterpret_cast<HANDLE *>(m_opaqueData) || !m_data || !m_size) {
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

  mapping = *reinterpret_cast<HANDLE *>(m_opaqueData);
  if (mapping) {
    CloseHandle(mapping);
    *reinterpret_cast<HANDLE *>(m_opaqueData) = 0;
  }
}
