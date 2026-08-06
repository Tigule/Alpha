#ifndef ENGINE_SOURCE_SRP_SRP6_H
#define ENGINE_SOURCE_SRP_SRP6_H

#include <Base/Base.h>

class SRP6_Random {
 public:
  SRP6_Random(UINT seed);
  void GenerateRandomBytes(BYTE *data, UINT size);

 private:
  BYTE m_randkey1[20];
  BYTE m_randkey2[20];
  BYTE m_randpool[20];
  UINT m_inpool;
};

#endif
