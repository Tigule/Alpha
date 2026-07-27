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

  bool Initialize(const char *name, unsigned int size, int mode);
  bool ChangeAccess(int newAccess);
  void Destroy();

  void *Data() const {
    return m_data;
  }

 protected:
  void         *m_data;
  unsigned int  m_size;
  unsigned char m_opaqueData[4];
};
