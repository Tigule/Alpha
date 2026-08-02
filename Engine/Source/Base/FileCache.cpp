#include <Base/Base.h>

#include "FileCache.h"

#include <storm.h>
#include <stpl.h>

#include <new>

struct PrefetchNode;

static void IBaseFileWaitForLoad(PrefetchNode *theFile);

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

typedef LISTEXDYN(PrefetchNode)   PrefetchList;
typedef LISTEXDYN(UncachableNode) UncachableList;

static PrefetchNode* IBaseFileStartLoad(const char* fileName);

static void IBaseFileWaitForLoad(PrefetchNode *theFile) {
  ASSERT(theFile != 0);

  SFile::WaitOverlapped(&theFile->overlapped);
}

typedef TSHashTable<PrefetchNode, HASHKEY_STRI>   PrefetchTable;
typedef TSHashTable<UncachableNode, HASHKEY_STRI> UncachableTable;

static PrefetchTable   s_activeFiles;
static UncachableTable s_uncachableFiles;
static int             s_numActiveFiles;
static SCritSect       s_critSect;

static PrefetchNode* IBaseFileStartLoad(const char* fileName) {
  PrefetchNode *theFile = s_activeFiles.Ptr(fileName);

  if (theFile) {
    s_activeFiles.Insert(theFile, fileName);
    return theFile;
  }

  if (s_numActiveFiles == 128) {
    s_activeFiles.Delete(s_activeFiles.Head());
    --s_numActiveFiles;
  }

  theFile = s_activeFiles.New(fileName, 0, 0);
  if (!SFile::LoadFile(fileName, &theFile->buffer, reinterpret_cast<unsigned long *>(&theFile->size), 1, &theFile->overlapped)) {
    s_activeFiles.Delete(theFile);
    return 0;
  }

  ++s_numActiveFiles;
  return theFile;
}

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

int IBaseFileLoad(const char* fileName, const void** fileBuffer, unsigned long* fileSize) {
  PrefetchNode *theFile = IBaseFileStartLoad(fileName);
  if (!theFile) {
    return 0;
  }

  IBaseFileWaitForLoad(theFile);
  ASSERT(theFile->refCount >= 0);
  ++theFile->refCount;
  *fileBuffer = theFile->buffer;
  if (fileSize) {
    *fileSize = theFile->size;
  }
  return 1;
}

void IBaseFileUnload(const char* fileName) {
  PrefetchNode *theFile = s_activeFiles.Ptr(fileName);
  ASSERT(theFile);

  if (theFile) {
    ASSERT(theFile->refCount > 0);
    --theFile->refCount;
  }
}

void BaseFileInitialize() {
}

void BaseFileDestroy() {
  s_critSect.Enter();
  s_activeFiles.Clear();
  s_uncachableFiles.Clear();
  s_critSect.Leave();
}

int BaseFilePrefetch(const char* fileName) {
  FATALASSERT(fileName);

  s_critSect.Enter();
  int success = 0;
  if (!s_uncachableFiles.Ptr(fileName)) {
    success = IBaseFileStartLoad(fileName) != 0;
  }
  s_critSect.Leave();
  return success;
}

int BaseFileIsFetched(const char* fileName) {
  FATALASSERT(fileName);

  s_critSect.Enter();
  int success = 0;
  PrefetchNode *theFile = s_activeFiles.Ptr(fileName);
  if (theFile) {
    success = SFile::PollOverlapped(&theFile->overlapped);
  }
  s_critSect.Leave();
  return success;
}

int BaseFileLoad(const char* fileName, void** fileBuffer, unsigned long* fileSize) {
  FATALASSERT(fileName);
  FATALASSERT(fileBuffer);

  const void   *tempBuffer;
  unsigned long tempSize;

  *fileBuffer = 0;
  if (fileSize) {
    *fileSize = 0;
  }

  s_critSect.Enter();
  int success = 0;

  if (s_uncachableFiles.Ptr(fileName)) {
    success = SFile::LoadFile(fileName, fileBuffer, fileSize, 1, 0);
  } else {
    tempBuffer = 0;
    tempSize = 0;
    if (IBaseFileLoad(fileName, &tempBuffer, &tempSize)) {
      *fileBuffer = SMemAlloc(tempSize + 1, __FILE__, __LINE__, 0);
      memcpy(*fileBuffer, tempBuffer, tempSize + 1);
      if (fileSize) {
        *fileSize = tempSize;
      }
      IBaseFileUnload(fileName);
      success = 1;
    }
  }

  s_critSect.Leave();
  return success;
}

void BaseFileFlush() {
  s_critSect.Enter();
  s_activeFiles.Clear();
  s_numActiveFiles = 0;
  s_critSect.Leave();
}

void BaseFileRegisterUncachable(const char *fileName) {
  FATALASSERT(fileName);

  s_critSect.Enter();
  if (!s_uncachableFiles.Ptr(fileName)) {
    s_uncachableFiles.New(fileName, 0, 0);
  }
  s_critSect.Leave();
}

void BaseFileUnregisterUncachable(const char* fileName) {
  FATALASSERT(fileName);

  s_critSect.Enter();
  UncachableNode *theFile = s_uncachableFiles.Ptr(fileName);
  if (theFile) {
    s_uncachableFiles.Delete(theFile);
  }
  s_critSect.Leave();
}

void BaseFileDumpStats() {
  HSLOG log;
  if (SLogCreate("BaseFileCacheDump.txt", 0, &log)) {
    SLogSetTimestamp(log, 0);
    SLogWrite(log, "-----------------------------------------------------------------");
    SLogWrite(log, "Base File Cache:");

    int count = 0;
    int bytes = 0;
    ITERATELIST(PrefetchNode, s_activeFiles, theFile) {
      SLogWrite(log, "%8d: %s (%d)", theFile->size, theFile->GetString(), theFile->refCount);
      bytes += theFile->size;
      ++count;
    }

    SLogWrite(log, "TOTAL - %d bytes for %d files", bytes, count);
    SLogClose(log);
  }
}
