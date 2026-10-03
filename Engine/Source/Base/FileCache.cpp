#include <Base/Base.h>

#include "FileCache.h"

#include <storm.h>
#include <stpl.h>

#include <new>

struct PrefetchNode;

static void IBaseFileWaitForLoad(PrefetchNode *theFile);

struct PrefetchNode : public TSHashObject<PrefetchNode, HASHKEY_STRI> {
  LPVOID      buffer;
  UINT        size;
  SOVERLAPPED overlapped;
  int         refCount;

  PrefetchNode() : buffer(0), size(0), refCount(0) {
    SFile::CreateOverlapped(&overlapped);
  }

  PrefetchNode &operator=(const PrefetchNode &);

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

typedef LISTEXDYN(PrefetchNode) PrefetchList;
typedef LISTEXDYN(UncachableNode) UncachableList;

static PrefetchNode *IBaseFileStartLoad(LPCSTR fileName);

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

static PrefetchNode *IBaseFileStartLoad(LPCSTR fileName) {
  PrefetchNode *theFile = s_activeFiles.Ptr(fileName);

  if (theFile) {
    s_activeFiles.Unlink(theFile);
    s_activeFiles.Insert(theFile, fileName);
    return theFile;
  }

  if (s_numActiveFiles == 128) {
    s_activeFiles.Delete(s_activeFiles.Head()->GetString());
    --s_numActiveFiles;
  }

  theFile = s_activeFiles.New(fileName, 0, 0);

  LPVOID localBuffer;
  DWORD  localBytes;

  if (!SFile::LoadFile(fileName, &localBuffer, &localBytes, 1, &theFile->overlapped)) {
    s_activeFiles.Delete(fileName);
    return 0;
  }

  theFile->buffer = localBuffer;
  theFile->size = localBytes;
  theFile->refCount = 0;
  ++s_numActiveFiles;
  return theFile;
}

BOOL IBaseFileLoad(LPCSTR fileName, LPCVOID *fileBuffer, DWORD *fileSize) {
  BOOL          success = 0;
  PrefetchNode *theFile = IBaseFileStartLoad(fileName);
  if (theFile) {
    IBaseFileWaitForLoad(theFile);
    ASSERT(theFile->refCount >= 0);
    ++theFile->refCount;
    *fileBuffer = theFile->buffer;
    if (fileSize) {
      *fileSize = theFile->size;
    }
    success = 1;
  }

  return success;
}

void IBaseFileUnload(LPCSTR fileName) {
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

BOOL BaseFilePrefetch(LPCSTR fileName) {
  VALIDATEBEGIN;
  VALIDATE(fileName);
  VALIDATEEND;

  s_critSect.Enter();
  int success = 0;
  if (!s_uncachableFiles.Ptr(fileName)) {
    success = IBaseFileStartLoad(fileName) != 0;
  }
  s_critSect.Leave();
  return success;
}

int BaseFileIsFetched(LPCSTR fileName) {
  VALIDATEBEGIN;
  VALIDATE(fileName);
  VALIDATEEND;

  s_critSect.Enter();
  int           success = 0;
  PrefetchNode *theFile = s_activeFiles.Ptr(fileName);
  if (theFile) {
    success = SFile::PollOverlapped(&theFile->overlapped);
  }
  s_critSect.Leave();
  return success;
}

int BaseFileLoad(LPCSTR fileName, LPVOID *fileBuffer, DWORD *fileSize) {
  VALIDATEBEGIN;
  VALIDATE(fileName);
  VALIDATEEND;
  FATALASSERT(fileBuffer);

  LPCVOID tempBuffer;
  DWORD   tempSize;

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

void BaseFileRegisterUncachable(LPCSTR fileName) {
  VALIDATEBEGIN;
  VALIDATE(fileName);
  VALIDATEENDVOID;

  s_critSect.Enter();
  if (!s_uncachableFiles.Ptr(fileName)) {
    s_uncachableFiles.New(fileName, 0, 0);
  }
  s_critSect.Leave();
}

void BaseFileUnregisterUncachable(LPCSTR fileName) {
  VALIDATEBEGIN;
  VALIDATE(fileName);
  VALIDATEENDVOID;

  s_critSect.Enter();
  if (s_uncachableFiles.Ptr(fileName)) {
    s_uncachableFiles.Delete(fileName);
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
