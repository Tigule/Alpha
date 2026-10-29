#pragma once

typedef DWORD SWAIT;

class SSyncObject {
  friend DWORD WaitMultiplePtr(UINT, SSyncObject *[], int, SWAIT);

 protected:
  BYTE m_opaqueData[0x04];

  void Copy(const SSyncObject &rhs);

 public:
  SSyncObject();
  SSyncObject(const SSyncObject &rhs);
  ~SSyncObject();
  SSyncObject &operator=(const SSyncObject &rhs);
  BOOL         Valid();
  void         Close();
  DWORD        Wait(SWAIT timeoutMs);
};

class SInitCritSect {
 private:
  LONG       m_spinLock;
  SCritSect *m_critsect;
  BYTE       m_critsectData[0x18];

 public:
  BOOL Enter();
  void Leave();
};

class SEvent : public SSyncObject {
 public:
  SEvent(int manualReset = 0, int initialValue = 0);
  ~SEvent() {}
  SEvent &operator=(const SEvent &rhs);
  BOOL    Set();
  BOOL    Reset();
};

class SSemaphore : public SSyncObject {
 public:
  SSemaphore(UINT initialCount, UINT maximumCount);
  ~SSemaphore() {
  }
  SSemaphore &operator=(const SSemaphore &rhs);
  BOOL        Signal(UINT count);
};

class SMutex : public SSyncObject {
 public:
  SMutex();
  SMutex(int initialOwner, LPCSTR name);
  SMutex(LPCSTR name);
  ~SMutex();
  SMutex &operator=(const SMutex &rhs);
  void    Create(int initialOwner, LPCSTR name);
  void    Open(LPCSTR name);
  int     Release();
};

DWORD WaitMultiple(UINT count, SSyncObject objects[], int waitAll, SWAIT timeoutMs);
DWORD WaitMultiplePtr(UINT count, SSyncObject *objectPtrs[], int waitAll, SWAIT timeoutMs);
void  SServerInitialize();
void  SServerDestroy();
int   STryEnterCriticalSection(LPVOID opaqueData);

typedef UINT(APIENTRY *STHREADPROC)(LPVOID);

class SThread : public SSyncObject {
 public:
  SThread() {}
  ~SThread() {}

  SThread    &operator=(const SThread &rhs);
  static BOOL Create(STHREADPROC proc, LPVOID param, SThread &thread, char *name);
};

LPVOID SCreateThread(DWORD stackSize, STHREADPROC proc, LPVOID param, DWORD flags, UINT *threadId, char *name);
LPVOID SCreateThread(STHREADPROC lpStartAddress, LPVOID lpParameter, UINT *lpThreadId, LPVOID linuxData, char *threadName);
DWORD  SGetCurrentThreadId();
int    SGetCurrentThreadPriority();
void   SSetCurrentThreadPriority(int priority);
