#include "FileCache.h"

#include <storm.h>
#include <stpl.h>

#include <new>

struct PrefetchNode;

static void __fastcall IBaseFileWaitForLoad(PrefetchNode *theFile);

struct PrefetchNode : public TSHashObject<PrefetchNode, HASHKEY_STRI> {
  void        *buffer;
  unsigned int size;
  SOVERLAPPED  overlapped;
  int          refCount;

  PrefetchNode() : buffer(0), size(0), refCount(0) {
    SFile::CreateOverlapped(&overlapped);
  }

  ~PrefetchNode() {
    ASSERT(refCount == 0);

    if (buffer) {
      IBaseFileWaitForLoad(this);
      FREE(buffer);
    }

    SFile::DestroyOverlapped(&overlapped);
  }
};

struct UncachableNode : public TSHashObject<UncachableNode, HASHKEY_STRI> {};

typedef TSExplicitList<PrefetchNode, -572662307>   PrefetchList;
typedef TSExplicitList<UncachableNode, -572662307> UncachableList;

static PrefetchNode* IBaseFileStartLoad(const char* fileName) {
    // TODO: implement
    return 0;
}

static void __fastcall IBaseFileWaitForLoad(PrefetchNode *theFile) {
  ASSERT(theFile != 0);

  SFile::WaitOverlapped(&theFile->overlapped);
}

typedef TSHashTable<PrefetchNode, HASHKEY_STRI>   PrefetchTable;
typedef TSHashTable<UncachableNode, HASHKEY_STRI> UncachableTable;

static PrefetchTable   s_activeFiles;
static UncachableTable s_uncachableFiles;
static SCritSect       s_critSect;

HASHKEY_STR::~HASHKEY_STR() {
  if (m_str) {
    SMemFree(m_str, __FILE__, __LINE__, 0);
  }
}

HASHKEY_STR &HASHKEY_STR::operator=(const char *str) {
  if (m_str != str) {
    if (m_str) {
      SMemFree(m_str, __FILE__, __LINE__, 0);
    }
    m_str = SStrDupA(str, __FILE__, __LINE__);
  }
  return *this;
}

int __fastcall IBaseFileLoad(const char* fileName, const void** fileBuffer, unsigned long* fileSize) {
    // TODO: implement
    return 0;
}

void __fastcall IBaseFileUnload(const char* fileName) {
    // TODO: implement
}

void __fastcall BaseFileInitialize() {
}

void __fastcall BaseFileDestroy() {
  s_critSect.Enter();
  s_activeFiles.Clear();
  s_uncachableFiles.Clear();
  s_critSect.Leave();
}

int __fastcall BaseFilePrefetch(const char* fileName) {
    // TODO: implement
    return 0;
}

int __fastcall BaseFileIsFetched(const char* fileName) {
    // TODO: implement
    return 0;
}

int __fastcall BaseFileLoad(const char* fileName, void** fileBuffer, unsigned long* fileSize) {
    // TODO: implement
    return 0;
}

void __fastcall BaseFileFlush() {
    // TODO: implement
}

void __fastcall BaseFileRegisterUncachable(const char *fileName) {
  FATALASSERT(fileName);

  s_critSect.Enter();
  if (!s_uncachableFiles.Ptr(fileName)) {
    s_uncachableFiles.New(fileName, 0, 0);
  }
  s_critSect.Leave();
}

void __fastcall BaseFileUnregisterUncachable(const char* fileName) {
    // TODO: implement
}

void __fastcall BaseFileDumpStats() {
    // TODO: implement
}
