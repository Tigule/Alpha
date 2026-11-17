#include <Base/Base.h>

#include "Os/W32/OsMemory.h"

#include <storm.h>

#include <sys/ipc.h>
#include <sys/shm.h>

COsSharedMemory::COsSharedMemory() : m_data(0), m_size(0) {
  *(int *)m_opaqueData = -1;
}

COsSharedMemory::~COsSharedMemory() {
  Destroy();
}

bool COsSharedMemory::Initialize(LPCSTR name, UINT size, int mode) {
  key_t key = (key_t)SStrHashHT(name);
  int   flags = 0600;
  int   atFlags = 0;
  int   shmid;

  switch (mode) {
    case SMEM_OPEN_NEW:
    case SMEM_OPEN_ALWAYS:
      shmid = shmget(key, 0, 0);
      if (shmid >= 0) {
        shmctl(shmid, IPC_RMID, 0);
      }
      flags |= IPC_CREAT | IPC_EXCL;
      break;

    case SMEM_OPEN_EXISTING:
      break;

    case SMEM_OPEN_READONLY:
      atFlags = SHM_RDONLY;
      break;

    default:
      return true;
  }

  shmid = shmget(key, size, flags);
  if (shmid >= 0) {
    m_data = shmat(shmid, 0, atFlags);
    if (m_data == (LPVOID)-1) {
      m_data = 0;
    }

    if (flags & IPC_CREAT) {
      if (m_data) {
        *(int *)m_opaqueData = shmid;
      } else {
        shmctl(shmid, IPC_RMID, 0);
      }
    }
  }

  if (!m_data) {
    return true;
  }

  m_size = size;
  return false;
}

void COsSharedMemory::Destroy() {
  if (m_data) {
    if (*(int *)m_opaqueData >= 0) {
      shmctl(*(int *)m_opaqueData, IPC_RMID, 0);
    }
    shmdt(m_data);
    m_data = 0;
  }

  m_size = 0;
}
