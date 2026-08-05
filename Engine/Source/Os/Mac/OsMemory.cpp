#include <Base/Base.h>

#include "Os/W32/OsMemory.h"

#include <storm.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

struct OSSHAREDMEMORY {
  int  descriptor;
  char name[0x100];
};

COsSharedMemory::COsSharedMemory() : m_data(0), m_size(0) {
  OSSHAREDMEMORY *shared = reinterpret_cast<OSSHAREDMEMORY *>(m_opaqueData);

  shared->descriptor = -1;
  shared->name[0] = 0;
}

COsSharedMemory::~COsSharedMemory() {
  Destroy();
}

bool COsSharedMemory::Initialize(const char *name, unsigned int size, int mode) {
  OSSHAREDMEMORY *shared = reinterpret_cast<OSSHAREDMEMORY *>(m_opaqueData);
  struct stat     stats;
  int             flags;
  int             protection = mode == SMEM_OPEN_READONLY ? PROT_READ : PROT_READ | PROT_WRITE;

  switch (mode) {
    case SMEM_OPEN_NEW:
    case SMEM_OPEN_ALWAYS:
      flags = O_RDWR | O_CREAT;
      break;

    case SMEM_OPEN_EXISTING:
      flags = O_RDWR;
      break;

    case SMEM_OPEN_READONLY:
      flags = O_RDONLY;
      break;

    default:
      return true;
  }

  SStrPrintf(shared->name, sizeof(shared->name), "/%s", name);

  shared->descriptor = shm_open(shared->name, flags, 0666);
  if (shared->descriptor < 0) {
    shared->descriptor = -1;
    return true;
  }

  if (mode == SMEM_OPEN_NEW || mode == SMEM_OPEN_ALWAYS) {
    if (ftruncate(shared->descriptor, size)) {
      Destroy();
      return true;
    }
  } else if (!size) {
    if (fstat(shared->descriptor, &stats)) {
      Destroy();
      return true;
    }

    size = static_cast<unsigned int>(stats.st_size);
  }

  if (!size) {
    Destroy();
    return true;
  }

  m_data = mmap(0, size, protection, MAP_SHARED, shared->descriptor, 0);
  if (m_data == MAP_FAILED) {
    m_data = 0;
    Destroy();
    return true;
  }

  m_size = size;
  return false;
}

bool COsSharedMemory::ChangeAccess(int newAccess) {
  return mprotect(m_data, m_size, newAccess == SMEM_ACCESS_WRITE ? PROT_READ | PROT_WRITE : PROT_READ) == 0;
}

void COsSharedMemory::Destroy() {
  OSSHAREDMEMORY *shared = reinterpret_cast<OSSHAREDMEMORY *>(m_opaqueData);

  if (m_data) {
    munmap(m_data, m_size);
    m_data = 0;
  }

  if (shared->descriptor >= 0) {
    close(shared->descriptor);
    shm_unlink(shared->name);
    shared->descriptor = -1;
  }

  m_size = 0;
}
