#ifndef ENGINE_SOURCE_BASE_MSGBUFFER_H
#define ENGINE_SOURCE_BASE_MSGBUFFER_H

#include <storm.h>

class CMsgBuffer {
 private:
  void ReallocData(UINT count);

 protected:
  void Reserve(UINT count);

 public:
  CMsgBuffer(UINT count = 0);
  ~CMsgBuffer();

  void  Reset();
  int   Bytes() const;
  UINT  GetReadPosition();
  void  SetReadPosition(UINT position);
  UINT  GetWritePosition();
  void  SetWritePosition(UINT position);
  BYTE *Data();
  void  SetData(BYTE *data, UINT count, int freeData);

  void AddChar(char val);
  void AddUchar(BYTE val);
  void AddByte(BYTE val);
  void AddTchar(char val);
  void AddTcharArray(LPCSTR str, UINT count, int zeroExtra);
  void AddTcharString(LPCSTR str, int compress);
  void AddShort(short val);
  void AddUshort(WORD val);
  void AddWord(WORD val);
  void AddInt(int val);
  void AddUint(UINT val);
  void AddLong(long val);
  void AddUlong(DWORD val);
  void AddDword(DWORD val);
  void AddLongLong(LONGLONG val);
  void AddUlongLong(DWORDLONG val);
  void AddFloat(float val);
  void AddData(LPCVOID data, UINT count);
  void AddData(BYTE *data, UINT count);
  void AddWordArray(const WORD *buffer, UINT count);
  void AddDwordArray(const DWORD *buffer, UINT count);
  void AddUintArray(const UINT *buffer, UINT count);
  void AddFloatArray(const float *buffer, UINT count);

  void AddArray(const UINT *buffer, UINT count) {
    AddUintArray(buffer, count);
  }
  void AddArray(const float *buffer, UINT count) {
    AddFloatArray(buffer, count);
  }
  void AddArray(const DWORD *buffer, UINT count) {
    AddDwordArray(buffer, count);
  }
  void AddArray(const WORD *buffer, UINT count) {
    AddWordArray(buffer, count);
  }
  void AddArray(const BYTE *buffer, UINT count) {
    AddData(buffer, count);
  }

  char      GetChar();
  BYTE      GetUchar();
  BYTE      GetByte();
  char      GetTchar();
  void      GetTcharArray(char *buffer, UINT count);
  UINT      GetTcharStringBufferLength(int *wide);
  void      GetTcharString(char *buffer, UINT bufferLength, int wide);
  short     GetShort();
  WORD      GetUshort();
  WORD      GetWord();
  int       GetInt();
  UINT      GetUint();
  long      GetLong();
  DWORD     GetUlong();
  DWORD     GetDword();
  LONGLONG  GetLongLong();
  DWORDLONG GetUlongLong();
  float     GetFloat();
  void      GetData(LPVOID buffer, int count);
  LPCVOID   GetData(int count);
  void      GetWordArray(WORD *buffer, UINT count);
  void      GetDwordArray(DWORD *buffer, UINT count);
  void      GetFloatArray(float *buffer, UINT count);
  void      GetUintArray(UINT *buffer, UINT count);

  void GetArray(UINT *buffer, UINT count) {
    GetUintArray(buffer, count);
  }
  void GetArray(float *buffer, UINT count) {
    GetFloatArray(buffer, count);
  }
  void GetArray(DWORD *buffer, UINT count) {
    GetDwordArray(buffer, count);
  }
  void GetArray(WORD *buffer, UINT count) {
    GetWordArray(buffer, count);
  }
  void GetArray(BYTE *buffer, UINT count) {
    GetData(buffer, count);
  }

 private:
  UINT m_alloc;
  int  m_freeData;

 protected:
  UINT  m_read;
  UINT  m_write;
  BYTE *m_data;
};

inline CMsgBuffer::CMsgBuffer(UINT count)
    : m_alloc(count), m_freeData(1), m_read(0), m_write(0), m_data(count ? static_cast<BYTE *>(SMemAlloc(count, __FILE__, __LINE__, 0)) : 0) {
}

inline CMsgBuffer::~CMsgBuffer() {
  if (m_freeData && m_data) {
    SMemFree(m_data, __FILE__, __LINE__, 0);
  }
}

inline void CMsgBuffer::Reserve(UINT count) {
  if (m_write + count > m_alloc) {
    ReallocData(m_write + count);
  }
}

inline void CMsgBuffer::Reset() {
  m_read = 0;
  m_write = 0;
}

inline int CMsgBuffer::Bytes() const {
  return m_write - m_read;
}

inline UINT CMsgBuffer::GetReadPosition() {
  return m_read;
}

inline void CMsgBuffer::SetReadPosition(UINT position) {
  m_read = position;
}

inline UINT CMsgBuffer::GetWritePosition() {
  return m_write;
}

inline void CMsgBuffer::SetWritePosition(UINT position) {
  m_write = position;
}

inline BYTE *CMsgBuffer::Data() {
  return m_data;
}

inline void CMsgBuffer::SetData(BYTE *data, UINT count, int freeData) {
  if (m_freeData && m_data) {
    SMemFree(m_data, __FILE__, __LINE__, 0);
  }
  m_alloc = count;
  m_freeData = freeData;
  m_read = 0;
  m_write = count;
  m_data = data;
}

#endif
