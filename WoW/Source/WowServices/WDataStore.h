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

  virtual void InternalInitialize(unsigned char *&data, unsigned int &base, unsigned int &alloc);
  virtual void InternalDestroy(unsigned char *&data, unsigned int &base, unsigned int &alloc);
  virtual int  InternalFetchWrite(
      unsigned int    pos,
      unsigned int    bytes,
      unsigned char *&data,
      unsigned int   &base,
      unsigned int   &alloc,
      const char     *fileName,
      int             lineNumber
  );

  static void __fastcall  StaticInitialize();
  static void __fastcall  StaticDestroy();
  static void *__fastcall AllocBuffer(unsigned int size);
  static void __fastcall  FreeBuffer(void *buffer, unsigned int size);

 private:
  void *m_bufferObj;
};

#endif
