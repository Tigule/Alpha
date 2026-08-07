#ifndef WOW_SOURCE_WOWSERVICES_WDATASTORE_H
#define WOW_SOURCE_WOWSERVICES_WDATASTORE_H

#include "Base/CDataStore.h"

class WDataStore : public CDataStore {
 public:
  WDataStore() {
    Initialize();
  }

  virtual ~WDataStore() {
    Destroy();
  }

  virtual void InternalInitialize(BYTE *&data, UINT &base, UINT &alloc);
  virtual void InternalDestroy(BYTE *&data, UINT &base, UINT &alloc);
  virtual BOOL InternalFetchWrite(UINT pos, UINT bytes, BYTE *&data, UINT &base, UINT &alloc, LPCSTR fileName, int lineNumber);

  static void   StaticInitialize();
  static void   StaticDestroy();
  static LPVOID AllocBuffer(UINT size);
  static void   FreeBuffer(LPVOID buffer, UINT size);

 private:
  LPVOID m_bufferObj;
};

#endif
