#pragma once

class COsSharedMemory {
 public:
  enum {
    SMEM_OPEN_NEW = 0,
    SMEM_OPEN_EXISTING = 1,
    SMEM_OPEN_ALWAYS = 2,
    SMEM_OPEN_READONLY = 3
  };

  enum {
    SMEM_ACCESS_READ = 0,
    SMEM_ACCESS_WRITE = 1
  };

  COsSharedMemory();
  ~COsSharedMemory();

  bool Initialize(LPCSTR name, UINT size, int mode);
  bool ChangeAccess(int newAccess);
  void Destroy();

  LPVOID Data() const {
    return m_data;
  }

 protected:
  LPVOID m_data;
  UINT   m_size;
  BYTE   m_opaqueData[4];
};
