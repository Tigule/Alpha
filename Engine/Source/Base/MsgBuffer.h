#ifndef ENGINE_SOURCE_BASE_MSGBUFFER_H
#define ENGINE_SOURCE_BASE_MSGBUFFER_H

class CMsgBuffer {
  private:
    void ReallocData(unsigned int count);

  protected:
    void Reserve(unsigned int count);

  public:
    CMsgBuffer(unsigned int count = 0);
    ~CMsgBuffer();

    void Reset();
    int Bytes();
    unsigned int GetReadPosition();
    void SetReadPosition(unsigned int position);
    unsigned int GetWritePosition();
    void SetWritePosition(unsigned int position);
    unsigned char *Data();
    void SetData(unsigned char *data, unsigned int count, int freeData);

    void AddChar(char val);
    void AddUchar(unsigned char val);
    void AddByte(unsigned char val);
    void AddTchar(char val);
    void AddTcharArray(const char *str, unsigned int count, int zeroExtra);
    void AddTcharString(const char *str, int compress);
    void AddShort(short val);
    void AddUshort(unsigned short val);
    void AddWord(unsigned short val);
    void AddInt(int val);
    void AddUint(unsigned int val);
    void AddLong(long val);
    void AddUlong(unsigned long val);
    void AddDword(unsigned long val);
    void AddLongLong(__int64 val);
    void AddUlongLong(unsigned __int64 val);
    void AddFloat(float val);
    void AddData(const void *data, unsigned int count);
    void AddData(unsigned char *data, unsigned int count);
    void AddWordArray(const unsigned short *buffer, unsigned int count);
    void AddDwordArray(const unsigned long *buffer, unsigned int count);
    void AddUintArray(const unsigned int *buffer, unsigned int count);
    void AddFloatArray(const float *buffer, unsigned int count);

    void AddArray(const unsigned int *buffer, unsigned int count) { AddUintArray(buffer, count); }
    void AddArray(const float *buffer, unsigned int count) { AddFloatArray(buffer, count); }
    void AddArray(const unsigned long *buffer, unsigned int count) { AddDwordArray(buffer, count); }
    void AddArray(const unsigned short *buffer, unsigned int count) { AddWordArray(buffer, count); }
    void AddArray(const unsigned char *buffer, unsigned int count) { AddData(buffer, count); }

    char GetChar();
    unsigned char GetUchar();
    unsigned char GetByte();
    char GetTchar();
    void GetTcharArray(char *buffer, unsigned int count);
    unsigned int GetTcharStringBufferLength(int *wide);
    void GetTcharString(char *buffer, unsigned int bufferLength, int wide);
    short GetShort();
    unsigned short GetUshort();
    unsigned short GetWord();
    int GetInt();
    unsigned int GetUint();
    long GetLong();
    unsigned long GetUlong();
    unsigned long GetDword();
    __int64 GetLongLong();
    unsigned __int64 GetUlongLong();
    float GetFloat();
    void GetData(void *buffer, int count);
    const void *GetData(int count);
    void GetWordArray(unsigned short *buffer, unsigned int count);
    void GetDwordArray(unsigned long *buffer, unsigned int count);
    void GetFloatArray(float *buffer, unsigned int count);
    void GetUintArray(unsigned int *buffer, unsigned int count);

    void GetArray(unsigned int *buffer, unsigned int count) { GetUintArray(buffer, count); }
    void GetArray(float *buffer, unsigned int count) { GetFloatArray(buffer, count); }
    void GetArray(unsigned long *buffer, unsigned int count) { GetDwordArray(buffer, count); }
    void GetArray(unsigned short *buffer, unsigned int count) { GetWordArray(buffer, count); }
    void GetArray(unsigned char *buffer, unsigned int count) { GetData(buffer, count); }

  private:
    unsigned int m_alloc;
    int m_freeData;

  protected:
    unsigned int m_read;
    unsigned int m_write;
    unsigned char *m_data;
};

#endif
