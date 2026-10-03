#ifndef WOW_SOURCE_WOWSERVICES_WDATASTORE_H
#define WOW_SOURCE_WOWSERVICES_WDATASTORE_H

#include "Base/CDataStore.h"

class WDataStore : public CDataStore {
  LPVOID m_bufferObj;

 public:
  WDataStore() {
    Initialize();
  }

  virtual ~WDataStore() {
    Destroy();
  }
  virtual BOOL InternalFetchWrite(UINT pos, UINT bytes, BYTE *&data, UINT &base, UINT &alloc, LPCSTR fileName, int lineNumber);

  virtual void InternalInitialize(BYTE *&data, UINT &base, UINT &alloc);
  virtual void InternalDestroy(BYTE *&data, UINT &base, UINT &alloc);

  static void   StaticInitialize();
  static void   StaticDestroy();
  static LPVOID AllocBuffer(UINT size);
  static void   FreeBuffer(LPVOID buffer, UINT size);
};

#endif
