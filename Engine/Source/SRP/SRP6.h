#ifndef ENGINE_SOURCE_SRP_SRP6_H
#define ENGINE_SOURCE_SRP_SRP6_H

class SRP6_Random {
  public:
    SRP6_Random(unsigned int seed);
    void GenerateRandomBytes(unsigned char *data, unsigned int size);

  private:
    unsigned char m_randkey1[20];
    unsigned char m_randkey2[20];
    unsigned char m_randpool[20];
    unsigned int  m_inpool;
};

#endif
