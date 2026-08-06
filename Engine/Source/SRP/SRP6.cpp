#include "SRP6.h"
#include "SHA.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

SRP6_Random::SRP6_Random(UINT seed) {
  struct {
    time_t ltime;
    UINT   msec;
    int    random;
  } preseed;
  SHA1_CONTEXT context;

  time(&preseed.ltime);
  preseed.msec = seed;
  srand(seed);
  preseed.random = rand();

  SHA1_Init(&context);
  SHA1_Update(&context, reinterpret_cast<BYTE *>(&preseed), sizeof(preseed));
  SHA1_Final(this->m_randkey1, &context);

  memcpy(this->m_randkey2, this->m_randkey1, sizeof(this->m_randkey2));
  memset(this->m_randpool, 0, sizeof(this->m_randpool));
  this->m_inpool = 0;
}

void SRP6_Random::GenerateRandomBytes(BYTE *data, UINT size) {
  SHA1_CONTEXT context;

  while (size > this->m_inpool) {
    memcpy(data, &this->m_randpool[sizeof(this->m_randpool) - this->m_inpool], this->m_inpool);
    data += this->m_inpool;
    size -= this->m_inpool;

    SHA1_Init(&context);
    SHA1_Update(&context, this->m_randkey1, sizeof(this->m_randkey1));
    SHA1_Update(&context, this->m_randpool, sizeof(this->m_randpool));
    SHA1_Update(&context, this->m_randkey2, sizeof(this->m_randkey2));
    SHA1_Final(this->m_randpool, &context);

    this->m_inpool = sizeof(this->m_randpool);
  }

  if (size) {
    memcpy(data, &this->m_randpool[sizeof(this->m_randpool) - this->m_inpool], size);
    this->m_inpool -= size;
  }
}
