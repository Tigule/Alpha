#pragma once

class COsSharedMemory {
 public:
  COsSharedMemory();
  ~COsSharedMemory();

  bool Initialize(const char *name, unsigned int size, int mode);
  bool ChangeAccess(int newAccess);
  void Destroy();

  void         *m_data;
  unsigned int  m_size;
  unsigned char m_opaqueData[4];
};
