#define STORM_SAPIBASE_DECLARATION
#define STORM_STHREAD_METHOD_IMPLEMENTATION

#include <storm.h>
#include <stpl.h>

#include "../Zlib/zlib.h"

#include <limits.h>
#include <stdio.h>
#include <sys/stat.h>

struct ZipFileFCB;

unsigned long ZipFileOpenArchive(const char *archivename);
int ZipFileCloseArchive(unsigned long handle);
ZipFileFCB *ZipFileOpenFile(const char *filename, unsigned long archive);
int ZipFileCloseFile(ZipFileFCB *fcb);
int ZipFileReadFile(ZipFileFCB *fcb, void *buffer, unsigned int bytesToRead, unsigned int *bytesRead);
int ZipFileSetFilePointer(ZipFileFCB *fcb, int offset, int origin);
unsigned long ZipFileGetFilePointer(ZipFileFCB *fcb);
unsigned long ZipFileGetFileSize(ZipFileFCB *fcb);
int ZipFileFileExists(const char *filename);
int ZipFileList(unsigned long archive, int(*cb)(const char *filename, void *param), void *param);
void __cdecl             SOutputDebugString(const char *format, ...);

class ASYNCREAD : public TSLinkedNode<ASYNCREAD> {
 public:
  SFile       *fileptr;
  void        *buffer;
  DWORD        bytestoread;
  SOVERLAPPED *overlapped;
};

struct NoPaqCompHdr {
  unsigned int uncompressedSize;
  char         signature[4];
  MD5          md5;
};

struct FILEMAP : TSHashObject<FILEMAP, HASHKEY_STRI> {
  FILEMAP() : realname(0) {
  }

  ~FILEMAP() {
    FREEIFUSED(realname);
  }

  char      *realname;
  SFILE_TYPE type;
};

bool s_bSFileCheckDisk = true;
bool s_findFileHashInitialized;
bool s_useOldFindFile;

typedef TSHashTable<FILEMAP, HASHKEY_STRI> FileMapTable;

static FileMapTable                             s_fileMap;
static unsigned int                             s_exitReadThread;
static unsigned int                             s_readThreadInitialized;
static SEvent                                   s_readQueueEvent(FALSE, FALSE);
static SCritSect                                s_readQueueLock;
static SThread                                  s_readThread;
static LISTDECL(ASYNCREAD, s_readQueue);
static unsigned long                            s_directaccess;
static char                                     s_basepath[MAX_PATH];
static char                                     s_datapath[MAX_PATH];
static char                                     s_datapath2[MAX_PATH];
static char                                     s_initialbasepath[MAX_PATH];

MD5::MD5() {
}

static void AddDirectoryToHash(const char *top, const char *sub, SDIR *dir) {
  char         namebuf[MAX_PATH];
  struct _stat stats;
  DWORD        toplen;
  DWORD        pathlen;
  char        *relative;
  SDIRENT     *entry;

  toplen = SStrLen(top);
  ASSERT(toplen + SStrLen(sub) < 260 - 1);

  SStrCopy(namebuf, top, MAX_PATH);
  if (namebuf[toplen - 1] != '\\') {
    namebuf[toplen] = '\\';
    namebuf[toplen + 1] = 0;
    ++toplen;
  }
  relative = namebuf + toplen;
  SStrCopy(relative, sub, MAX_PATH - toplen);
  pathlen = SStrLen(namebuf);
  if (namebuf[pathlen - 1] != '\\') {
    namebuf[pathlen] = '\\';
    namebuf[pathlen + 1] = 0;
    ++pathlen;
  }

  while ((entry = SFile::ReadDir(dir)) != NULL) {
    char    *extension;
    char    *actual;
    FILEMAP *mapped;

    if (!SStrCmp(entry->d_name, ".", INT_MAX) || !SStrCmp(entry->d_name, "..", INT_MAX)) {
      continue;
    }
    SStrCopy(namebuf + pathlen, entry->d_name, MAX_PATH - pathlen);
    if (_stat(namebuf, &stats)) {
      continue;
    }

    if (stats.st_mode & 0x4000) {
      SDIR *child;

      if (*s_datapath && !SStrCmp(relative, s_datapath, SStrLen(relative))) {
        continue;
      }
      child = SFile::OpenDir(namebuf);
      if (child) {
        AddDirectoryToHash(top, relative, child);
        SFile::CloseDir(child);
      }
      continue;
    }
    if (!(stats.st_mode & 0x8000)) {
      continue;
    }

    extension = SStrChrR(namebuf, '.');
    actual = SStrDupA(namebuf, __FILE__, __LINE__);
    if (extension && !SStrCmpI(extension, ".bz", INT_MAX)) {
      *extension = 0;
      if (!s_fileMap.Ptr(relative)) {
        mapped = s_fileMap.New(relative, 0, 0);
        mapped->type = SFILE_COMPRESSED;
        mapped->realname = actual;
        continue;
      }
    } else if (extension && !SStrCmpI(extension, ".MPQ", INT_MAX)) {
      *extension = 0;
      if (!s_fileMap.Ptr(relative)) {
        mapped = s_fileMap.New(relative, 0, 0);
        mapped->type = SFILE_PAQ;
        mapped->realname = actual;
        continue;
      }
    }

    if (!s_fileMap.Ptr(relative)) {
      mapped = s_fileMap.New(relative, 0, 0);
      mapped->type = SFILE_PLAIN;
      mapped->realname = actual;
    } else {
      FREE(actual);
    }
  }
}

static void BuildFileSystemHash() {
  const char *base;
  SDIR       *basedir;
  SDIR       *dir;
  char        datapath[MAX_PATH];

  base = s_basepath;
  basedir = SFile::OpenDir(base);
  if (!basedir) {
    base = ".";
    basedir = SFile::OpenDir(base);
  }

  SStrCopy(s_initialbasepath, s_basepath, MAX_PATH);
  ASSERT(basedir);
  AddDirectoryToHash(base, "", basedir);
  SFile::CloseDir(basedir);

  SStrCopy(datapath, base, MAX_PATH);
  SStrPack(datapath, "\\", MAX_PATH);
  SStrPack(datapath, s_datapath, MAX_PATH);
  dir = SFile::OpenDir(datapath);
  if (dir) {
    AddDirectoryToHash(datapath, "", dir);
    SFile::CloseDir(dir);
  }

  if (*s_datapath2) {
    SStrCopy(datapath, s_datapath2, MAX_PATH);
    dir = SFile::OpenDir(datapath);
    if (dir) {
      AddDirectoryToHash(datapath, "", dir);
      SFile::CloseDir(dir);
    }
  }
  s_findFileHashInitialized = true;
}

static int OldFindFile(const char *filename, char *realname, int len, DWORD flags, SFILE_TYPE *type);

static int FindFile(const char *filename, char *realname, int len, DWORD flags, SFILE_TYPE *type) {
  const char *base;
  FILEMAP    *mapped;

  if (s_useOldFindFile || (filename[0] == '\\' && filename[1] == '\\') || filename[1] == ':') {
    return OldFindFile(filename, realname, len, flags, type);
  }

  if (!s_findFileHashInitialized) {
    BuildFileSystemHash();
  }

  base = SStrChrR(filename, '\\');
  base = base ? base + 1 : filename;
  mapped = s_fileMap.Ptr(base);
  if (mapped) {
    SStrCopy(realname, mapped->realname, len);
    *type = mapped->type;
    return TRUE;
  }

  mapped = s_fileMap.Ptr(filename);
  if (mapped) {
    SStrCopy(realname, mapped->realname, len);
    *type = mapped->type;
    return TRUE;
  }

  if (SStrCmp(s_initialbasepath, s_basepath, INT_MAX)) {
    char fuckedWithPath[MAX_PATH];

    SStrCopy(fuckedWithPath, s_basepath, INT_MAX);
    SStrPack(fuckedWithPath, "\\", INT_MAX);
    SStrPack(fuckedWithPath, filename, INT_MAX);
    mapped = s_fileMap.Ptr(fuckedWithPath);
    if (mapped) {
      SStrCopy(realname, mapped->realname, len);
      *type = mapped->type;
    }
  }

  if (!s_bSFileCheckDisk) {
    flags &= ~3u;
  }
  if (SFileFileExistsEx(NULL, filename, flags)) {
    SStrCopy(realname, filename, len);
    *type = SFILE_OLD_SFILE;
    return TRUE;
  }
  return FALSE;
}

void APIENTRY SFile::EnableHash(bool enable) {
  s_useOldFindFile = !enable;
}

void APIENTRY SFile::RebuildHash() {
  s_fileMap.Clear();
  s_findFileHashInitialized = false;
}

static int OldFindFile(const char *filename, char *realname, int len, DWORD flags, SFILE_TYPE *type) {
  struct _stat stats;
  const char  *backslash;

  SStrCopy(realname, s_basepath, len);
  SStrPack(realname, filename, len);
  if (!_stat(realname, &stats) && stats.st_mode >= 0x8000) {
    *type = SFILE_PLAIN;
    return TRUE;
  }

  backslash = SStrChrR(filename, '\\');
  if (backslash) {
    SStrCopy(realname, s_basepath, len);
    SStrPack(realname, backslash + 1, len);
    if (!_stat(realname, &stats) && stats.st_mode >= 0x8000) {
      *type = SFILE_PLAIN;
      return TRUE;
    }
  }

  SStrCopy(realname, s_basepath, len);
  SStrPack(realname, s_datapath, len);
  SStrPack(realname, filename, len);
  if (!_stat(realname, &stats)) {
    if (stats.st_mode < 0x8000) {
      return FALSE;
    }
    *type = SFILE_PLAIN;
    return TRUE;
  }

  SStrPack(realname, ".bz", len);
  if (!_stat(realname, &stats)) {
    if (stats.st_mode < 0x8000) {
      return FALSE;
    }
    *type = SFILE_COMPRESSED;
    return TRUE;
  }

  if (*s_basepath) {
    SStrCopy(realname, s_datapath, len);
    SStrPack(realname, filename, len);
    if (!_stat(realname, &stats) && stats.st_mode >= 0x8000) {
      *type = SFILE_PLAIN;
      return TRUE;
    }
    SStrPack(realname, ".bz", len);
    if (!_stat(realname, &stats) && stats.st_mode >= 0x8000) {
      *type = SFILE_COMPRESSED;
      return TRUE;
    }
  }

  SStrCopy(realname, s_basepath, len);
  SStrPack(realname, s_datapath, len);
  SStrPack(realname, filename, len);
  SStrPack(realname, ".MPQ", len);
  if (!_stat(realname, &stats)) {
    if (stats.st_mode < 0x8000) {
      return FALSE;
    }
    *type = SFILE_PAQ;
    return TRUE;
  }

  if (*s_datapath2) {
    SStrCopy(realname, s_basepath, len);
    SStrPack(realname, s_datapath2, len);
    SStrPack(realname, filename, len);
    if (!_stat(realname, &stats)) {
      if (stats.st_mode < 0x8000) {
        return FALSE;
      }
      *type = SFILE_PLAIN;
      return TRUE;
    }
  }

  if (ZipFileFileExists(filename)) {
    SStrCopy(realname, filename, len);
    *type = SFILE_ZIP_FILE;
    return TRUE;
  }

  if (!s_bSFileCheckDisk) {
    flags &= ~3u;
  }
  if (SFileFileExistsEx(NULL, filename, flags)) {
    SStrCopy(realname, filename, len);
    *type = SFILE_OLD_SFILE;
    return TRUE;
  }
  return FALSE;
}

SFile::SFile(SFILE_TYPE type) {
  m_type = type;
  m_fileptr = NULL;
  m_archive = NULL;
  m_filename = NULL;
  m_zbuffer = NULL;
  m_hsfile = NULL;
  m_zipFile = NULL;
  m_haveMD5 = 0;
  m_closeAfterLoad = 0;
  m_actualname = NULL;
  m_asyncCount = 0;
  m_zstream = NULL;
}

SFile::~SFile() {
  ASSERT(!m_asyncCount);
  if (m_zstream) {
    inflateEnd(m_zstream);
    FREE(m_zstream);
  }
  if (m_filename) {
    FREE(m_filename);
  }
  if (m_actualname) {
    FREE(m_actualname);
  }
  if (m_zbuffer) {
    FREE(m_zbuffer);
  }
}

static DWORD BuildDefaultOpenFlags() {
  DWORD flags;

  flags = 0;
  if (s_directaccess & 1) {
    flags |= 1;
  }
  if (s_directaccess & 2) {
    flags |= 2;
  }
  if (!s_directaccess) {
    flags |= 1;
  }
  return flags;
}

void SFile::DoAsyncRead(ASYNCREAD *ptr) {
  DWORD savedOffset;
  int   closeAfterLoad;

  if (!(ptr->fileptr->m_filename != (char *)0xdddddddd)) {
    SErrDisplayError(STORM_ERROR_ASSERTION, __FILE__, __LINE__, "ptr->fileptr->m_filename != (char *)0xdddddddd", FALSE, 1);
  }

  ptr->fileptr->m_lock.Enter();
  if (!(ptr->fileptr->m_asyncCount == 1)) {
    SErrDisplayError(STORM_ERROR_ASSERTION, __FILE__, __LINE__, "ptr->fileptr->m_asyncCount == 1", FALSE, 1);
  }
  closeAfterLoad = ptr->fileptr->m_closeAfterLoad;

  switch (ptr->fileptr->m_type) {
    case 0:
      savedOffset = (DWORD)ftell((FILE *)ptr->fileptr->m_fileptr);
      fseek((FILE *)ptr->fileptr->m_fileptr, (long)ptr->overlapped->Offset, SEEK_SET);
      fread(ptr->buffer, 1, ptr->bytestoread, (FILE *)ptr->fileptr->m_fileptr);
      fseek((FILE *)ptr->fileptr->m_fileptr, (long)savedOffset, SEEK_SET);
      break;

    case 1:
      savedOffset = ptr->fileptr->m_curOffset;
      if (ptr->overlapped->Offset) {
        SFile::SetFilePointer(ptr->fileptr, (LONG)ptr->overlapped->Offset, NULL, FILE_BEGIN);
      }
      DoZRead(ptr->fileptr, ptr->buffer, ptr->bytestoread, NULL);
      SFile::SetFilePointer(ptr->fileptr, (LONG)savedOffset, NULL, FILE_BEGIN);
      break;

    case 2:
    case 3:
      savedOffset = SFileSetFilePointer((HSFILE)ptr->fileptr->m_hsfile, 0, NULL, FILE_CURRENT);
      if (ptr->overlapped->Offset) {
        SFileSetFilePointer((HSFILE)ptr->fileptr->m_hsfile, (LONG)ptr->overlapped->Offset, NULL, FILE_BEGIN);
      }
      SFileReadFileEx2((HSFILE)ptr->fileptr->m_hsfile, ptr->buffer, ptr->bytestoread, NULL, NULL, 0, NULL);
      SFileSetFilePointer((HSFILE)ptr->fileptr->m_hsfile, (LONG)savedOffset, NULL, FILE_BEGIN);
      break;

    case 4:
      savedOffset = ZipFileGetFilePointer((ZipFileFCB *)ptr->fileptr->m_zipFile);
      if (ptr->overlapped->Offset) {
        ZipFileSetFilePointer((ZipFileFCB *)ptr->fileptr->m_zipFile, (int)ptr->overlapped->Offset, FILE_BEGIN);
      }
      ZipFileReadFile((ZipFileFCB *)ptr->fileptr->m_zipFile, ptr->buffer, ptr->bytestoread, NULL);
      ZipFileSetFilePointer((ZipFileFCB *)ptr->fileptr->m_zipFile, (int)savedOffset, FILE_BEGIN);
      break;
  }

  if (!(ptr->fileptr->m_asyncCount == 1)) {
    SErrDisplayError(STORM_ERROR_ASSERTION, __FILE__, __LINE__, "ptr->fileptr->m_asyncCount == 1", FALSE, 1);
  }
  --ptr->fileptr->m_asyncCount;
  ptr->fileptr->m_lock.Leave();
  ptr->overlapped->hEvent->Set();
  if (closeAfterLoad) {
    SFile::Close(ptr->fileptr);
  }
}

unsigned int APIENTRY SFile::ReadProc(void *__formal) {
  ASYNCREAD *request;

  for (;;) {
    do {
    } while (s_readQueueEvent.Wait(INFINITE) != WAIT_OBJECT_0);

    s_readQueueLock.Enter();
    if (s_exitReadThread) {
      s_exitReadThread = FALSE;
      return 0;
    }

    request = s_readQueue.Head();
    while (request) {
      s_readQueue.UnlinkNode(request);
      s_readQueueLock.Leave();
      DoAsyncRead(request);
      delete request;
      s_readQueueLock.Enter();
      request = s_readQueue.Head();
    }
    s_readQueueLock.Leave();
  }
}

void SFile::InitializeReadThread() {
  if (!s_readThreadInitialized) {
    SThread::Create(ReadProc, NULL, s_readThread, NULL);
    s_readThreadInitialized = TRUE;
  }
}

void SFile::QueueReadRequest(SFile *fileptr, void *buffer, DWORD bytestoread, SOVERLAPPED *overlapped) {
  ASYNCREAD *request;

  ASSERT(!fileptr->m_asyncCount);
  ++fileptr->m_asyncCount;
  s_readQueueLock.Enter();
  InitializeReadThread();
  request = s_readQueue.NewNode(LIST_TAIL, 0, 0);
  request->bytestoread = bytestoread;
  request->fileptr = fileptr;
  request->buffer = buffer;
  request->overlapped = overlapped;
  s_readQueueEvent.Set();
  s_readQueueLock.Leave();
}

int SFile::DoZRead(SFile *fileptr, void *buffer, DWORD bytestoread, DWORD *bytesread) {
  int result;
  int read;

  fileptr->m_zstream->avail_out = bytestoread;
  fileptr->m_zstream->next_out = (Bytef *)buffer;
  for (;;) {
    if (!fileptr->m_zstream->avail_in) {
      fileptr->m_zstream->next_in = (Bytef *)fileptr->m_zbuffer;
      read = (int)fread(fileptr->m_zstream->next_in, 1, 0x1000, (FILE *)fileptr->m_fileptr);
      if (read > 0) {
        fileptr->m_zstream->avail_in = (uInt)read;
      }
    }

    result = inflate(fileptr->m_zstream, Z_SYNC_FLUSH);
    if (!fileptr->m_zstream->avail_out) {
      if (bytesread) {
        *bytesread = bytestoread;
      }
      fileptr->m_curOffset += bytestoread;
      return TRUE;
    }
    if (result == Z_STREAM_END) {
      if (bytesread) {
        *bytesread = bytestoread - fileptr->m_zstream->avail_out;
      }
      fileptr->m_curOffset += bytestoread - fileptr->m_zstream->avail_out;
      return bytestoread - fileptr->m_zstream->avail_out != 0;
    }
    if (result != Z_OK) {
      return FALSE;
    }
  }
}

DWORD APIENTRY SFile::Open(const char *filename, SFile **file) {
  return SFile::OpenEx(NULL, filename, BuildDefaultOpenFlags(), file);
}

DWORD APIENTRY SFile::OpenEx(SArchive *archive, const char *filename, DWORD flags, SFile **file) {
  SArchive   *archiveData;
  HSFILE      sfile;
  SFILE_TYPE  type;
  char        realname[MAX_PATH];
  char       *extension;
  const char *basename;
  ZipFileFCB *md5file;

  *file = NULL;
  if (!archive) {
    if (!FindFile(filename, realname, MAX_PATH, flags, &type)) {
      return FALSE;
    }

    *file = NEW(SFile)(type);
    switch (type) {
      case SFILE_PLAIN:
        (*file)->m_fileptr = fopen(realname, "rb");
        if (!(*file)->m_fileptr) {
          goto open_failed;
        }
        (*file)->m_filename = SStrDupA(realname, __FILE__, __LINE__);
        return 2;

      case SFILE_COMPRESSED: {
        (*file)->m_fileptr = fopen(realname, "rb");
        if (!(*file)->m_fileptr) {
          delete *file;
          *file = NULL;
          return FALSE;
        }
        (*file)->m_filename = SStrDupA(realname, __FILE__, __LINE__);
        (*file)->m_actualname = SStrDupA(filename, __FILE__, __LINE__);
        extension = SStrChrR((*file)->m_filename, '.');
        if (extension && !SStrCmpI(extension, ".bz", INT_MAX)) {
          *extension = 0;
        }

        NoPaqCompHdr header;

        fread(&header, 1, sizeof(header), (FILE *)(*file)->m_fileptr);
        if (*(DWORD *)header.signature != *(const DWORD *)"BZ00") {
          goto open_failed;
        }
        (*file)->m_size = header.uncompressedSize;
        (*file)->m_md5 = header.md5;
        (*file)->m_haveMD5 = TRUE;
        (*file)->m_curOffset = 0;
        (*file)->m_zbuffer = (BYTE *)ALLOC(0x1000);
        ASSERT(!(*file)->m_zstream);
        (*file)->m_zstream = (z_stream *)ALLOC(sizeof(z_stream));
        (*file)->m_zstream->avail_in = 0;
        (*file)->m_zstream->zalloc = NULL;
        (*file)->m_zstream->zfree = NULL;
        (*file)->m_zstream->opaque = NULL;
        ASSERT(inflateInit_((*file)->m_zstream, "1.1.3", sizeof(z_stream)) == 0);
        return 3;
      }

      case SFILE_PAQ:
        archiveData = NEW(SArchive);
        (*file)->m_archive = archiveData;
        if (!SFileOpenArchive(realname, 0, 0, (HSARCHIVE *)&archiveData->m_archive)) {
          goto open_failed;
        }
        basename = SStrChrR(filename, '\\');
        basename = basename ? basename + 1 : filename;
        if (!SFileOpenFileEx((HSARCHIVE)archiveData->m_archive, basename, 0, (HSFILE *)&(*file)->m_hsfile)) {
          SFileCloseArchive((HSARCHIVE)archiveData->m_archive);
          goto open_failed;
        }
        (*file)->m_filename = SStrDupA(filename, __FILE__, __LINE__);
        if (flags & 0x10000) {
          if (SFileGetFileMD5((HSFILE)(*file)->m_hsfile, (DWORD *)&(*file)->m_md5) && !((*file)->m_md5 == MD5(0, 0, 0, 0))) {
            (*file)->m_haveMD5 = TRUE;
          } else {
            (*file)->m_haveMD5 = FALSE;
          }
        }
        return TRUE;

      case SFILE_OLD_SFILE:
        if (!SFileOpenFileEx(NULL, filename, flags, (HSFILE *)&(*file)->m_hsfile)) {
          goto open_failed;
        }
        (*file)->m_filename = SStrDupA(filename, __FILE__, __LINE__);
        if (flags & 0x10000) {
          if (SFileGetFileMD5((HSFILE)(*file)->m_hsfile, (DWORD *)&(*file)->m_md5) && !((*file)->m_md5 == MD5(0, 0, 0, 0))) {
            (*file)->m_haveMD5 = TRUE;
          } else {
            (*file)->m_haveMD5 = FALSE;
          }
        }
        return TRUE;

      case SFILE_ZIP_FILE:
        (*file)->m_zipFile = ZipFileOpenFile(filename, 0);
        if (!(*file)->m_zipFile) {
          goto open_failed;
        }
        (*file)->m_filename = SStrDupA(filename, __FILE__, __LINE__);
        if (flags & 0x10000) {
          char md5filename[MAX_PATH];

          SStrCopy(md5filename, filename, INT_MAX);
          SStrPack(md5filename, ".md5", INT_MAX);
          md5file = ZipFileOpenFile(md5filename, 0);
          if (md5file) {
            (*file)->m_haveMD5 = ZipFileReadFile(md5file, &(*file)->m_md5, sizeof((*file)->m_md5), NULL);
            ZipFileCloseFile(md5file);
          }
        }
        return 4;
    }
    return FALSE;
  }

  archiveData = archive;
  if (archiveData->m_type == SARCHIVE_MPQ) {
    sfile = NULL;
    if (!SFileOpenFileEx((HSARCHIVE)archiveData->m_archive, filename, 0, &sfile)) {
      return FALSE;
    }
    *file = NEW(SFile)(SFILE_OLD_SFILE);
    (*file)->m_hsfile = sfile;
    (*file)->m_filename = SStrDupA(filename, __FILE__, __LINE__);
    if (flags & 0x10000) {
      if (SFileGetFileMD5((HSFILE)(*file)->m_hsfile, (DWORD *)&(*file)->m_md5) && !((*file)->m_md5 == MD5(0, 0, 0, 0))) {
        (*file)->m_haveMD5 = TRUE;
      } else {
        (*file)->m_haveMD5 = FALSE;
      }
    }
    return TRUE;
  }
  if (archiveData->m_type == SARCHIVE_ZIP) {
    *file = NEW(SFile)(SFILE_ZIP_FILE);
    (*file)->m_zipFile = ZipFileOpenFile(filename, (DWORD)archiveData->m_archive);
    if (!(*file)->m_zipFile) {
      goto open_failed;
    }
    (*file)->m_filename = SStrDupA(filename, __FILE__, __LINE__);
    if (flags & 0x10000) {
      char md5filename[MAX_PATH];

      SStrCopy(md5filename, filename, INT_MAX);
      SStrPack(md5filename, ".md5", INT_MAX);
      md5file = ZipFileOpenFile(md5filename, (DWORD)archiveData->m_archive);
      if (md5file) {
        (*file)->m_haveMD5 = ZipFileReadFile(md5file, &(*file)->m_md5, sizeof((*file)->m_md5), NULL);
        ZipFileCloseFile(md5file);
      }
    }
    return 4;
  }
  return FALSE;

open_failed:
  if (*file) {
    delete *file;
  }
  *file = NULL;
  return FALSE;
}

DWORD APIENTRY
SFile::Read(SFile *fileptr, void *buffer, DWORD bytestoread, DWORD *bytesread, SOVERLAPPED *overlapped, _TASYNCPARAMBLOCK *asyncparam) {
  SCritSect *lock;
  int        result;

  (void)asyncparam;
  ASSERT(fileptr);
  if (!bytestoread) {
    if (overlapped) {
      overlapped->hEvent->Set();
    }
    if (bytesread) {
      *bytesread = 0;
      return TRUE;
    }
  } else if (!overlapped) {
    lock = &fileptr->m_lock;
    lock->Enter();
    switch (fileptr->m_type) {
      case SFILE_PLAIN:
        result = (int)fread(buffer, 1, bytestoread, (FILE *)fileptr->m_fileptr);
        if (result < 0) {
          return FALSE;
        }
        if (bytesread) {
          *bytesread = result;
        }
        result = result > 0;
        lock->Leave();
        return result;

      case SFILE_COMPRESSED:
        result = DoZRead(fileptr, buffer, bytestoread, bytesread);
        lock->Leave();
        return result;

      case SFILE_PAQ:
      case SFILE_OLD_SFILE: {
        DWORD localBytesRead;

        localBytesRead = 0;
        result = SFileReadFileEx2((HSFILE)fileptr->m_hsfile, buffer, bytestoread, &localBytesRead, NULL, 0, NULL) || localBytesRead > 0;
        if (bytesread) {
          *bytesread = localBytesRead;
        }
        lock->Leave();
        return result;
      }

      case SFILE_ZIP_FILE: {
        DWORD localBytesRead;

        localBytesRead = 0;
        result = ZipFileReadFile((ZipFileFCB *)fileptr->m_zipFile, buffer, bytestoread, (unsigned int *)&localBytesRead) || localBytesRead > 0;
        if (bytesread) {
          *bytesread = localBytesRead;
        }
        lock->Leave();
        return result;
      }

      default:
        ASSERT(0);
        lock->Leave();
        return FALSE;
    }
  } else {
    QueueReadRequest(fileptr, buffer, bytestoread, overlapped);
  }
  return TRUE;
}

DWORD APIENTRY SFile::LoadFile(const char *filename, void **buffer, DWORD *bytes, DWORD extraBytes, SOVERLAPPED *overlapped) {
  return SFile::Load(NULL, filename, buffer, bytes, extraBytes, BuildDefaultOpenFlags(), overlapped);
}

DWORD APIENTRY
SFile::Load(SArchive *archive, const char *filename, void **buffer, DWORD *bytes, DWORD extraBytes, DWORD flags, SOVERLAPPED *overlapped) {
  SFile *file;
  BYTE  *target;
  DWORD  size;
  BOOL   ok;

  if (!SFile::OpenEx(archive, filename, flags, &file)) {
    return FALSE;
  }

  size = SFile::GetFileSize(file, NULL);
  target = (BYTE *)ALLOC(size + extraBytes);
  if (overlapped) {
    file->m_closeAfterLoad = TRUE;
  }

  ok = SFile::Read(file, target, size, bytes, overlapped, NULL);
  if (!ok) {
    SFile::Close(file);
    return FALSE;
  }

  if (extraBytes) {
    memset(target + size, 0, extraBytes);
  }
  *buffer = target;
  if (bytes) {
    *bytes = size;
  }
  if (!overlapped) {
    SFile::Close(file);
  }
  return TRUE;
}

int APIENTRY SFile::Unload(void *buffer) {
  FREE(buffer);
  return TRUE;
}

DWORD APIENTRY SFile::Close(SFile *file) {
  ASSERT(file);
  ASSERT(file->m_asyncCount == 0);
  if (file->m_fileptr) {
    fclose((FILE *)file->m_fileptr);
  }
  if (file->m_hsfile) {
    SFileCloseFile((HSFILE)file->m_hsfile);
  }
  if (file->m_zipFile) {
    ZipFileCloseFile((ZipFileFCB *)file->m_zipFile);
  }
  if (file->m_archive) {
    SArchive *archive = file->m_archive;
    if (archive->m_archive) {
      SFileCloseArchive((HSARCHIVE)archive->m_archive);
    }
    delete archive;
  }
  delete file;
  return TRUE;
}

int APIENTRY SFile::GetActualFileName(SFile *file, char *buffer, DWORD bufferchars) {
  ASSERT(file);
  SStrCopy(buffer, file->m_actualname ? file->m_actualname : file->m_filename, bufferchars);
  return TRUE;
}

DWORD APIENTRY SFile::GetFileSize(SFile *file, DWORD *filesizehigh) {
  struct _stat stats;

  ASSERT(file);
  ASSERT(file->m_filename != (char *)0xdddddddd);

  switch (file->m_type) {
    case SFILE_PLAIN:
      if (!file->m_fileptr) {
        return 0;
      }
      if (filesizehigh) {
        *filesizehigh = 0;
      }
      return _fstat(_fileno((FILE *)file->m_fileptr), &stats) ? 0 : (DWORD)stats.st_size;

    case SFILE_COMPRESSED:
      return file->m_size;

    case SFILE_PAQ:
    case SFILE_OLD_SFILE:
      return SFileGetFileSize((HSFILE)file->m_hsfile, filesizehigh);

    case SFILE_ZIP_FILE:
      return ZipFileGetFileSize((ZipFileFCB *)file->m_zipFile);
  }
  return 0;
}

int APIENTRY SFile::GetBasePath(char *buffer, DWORD bufferchars) {
  SStrCopy(buffer, s_basepath, bufferchars);
  return TRUE;
}

int APIENTRY SFile::SetBasePath(const char *path) {
  SStrCopy(s_basepath, path, MAX_PATH);
  if (*s_basepath && s_basepath[SStrLen(s_basepath) - 1] != '\\') {
    SStrPack(s_basepath, "\\", MAX_PATH);
  }
  SFileSetBasePath(path);
  return TRUE;
}

int APIENTRY SFile::SetDataPath(const char *path) {
  SStrCopy(s_datapath, path, MAX_PATH);
  if (*s_datapath && s_datapath[SStrLen(s_datapath) - 1] != '\\') {
    SStrPack(s_datapath, "\\", MAX_PATH);
  }
  return TRUE;
}

int APIENTRY SFile::SetDataPathAlternate(const char *path) {
  SStrCopy(s_datapath2, path, MAX_PATH);
  if (*s_datapath2 && s_datapath[SStrLen(s_datapath2) - 1] != '\\') {
    SStrPack(s_datapath2, "\\", MAX_PATH);
  }
  return TRUE;
}

int APIENTRY SFile::FileExists(const char *filename) {
  char       realname[MAX_PATH];
  SFILE_TYPE type;

  return FindFile(filename, realname, MAX_PATH, BuildDefaultOpenFlags(), &type);
}

DWORD APIENTRY SFile::SetFilePointer(SFile *file, LONG distancetomove, LONG *distancetomovehigh, DWORD movemethod) {
  DWORD result;

  (void)distancetomovehigh;
  ASSERT(file);

  file->m_lock.Enter();
  switch (file->m_type) {
    case SFILE_PLAIN:
      switch (movemethod) {
        case FILE_BEGIN:
          fseek((FILE *)file->m_fileptr, distancetomove, SEEK_SET);
          break;
        case FILE_CURRENT:
          fseek((FILE *)file->m_fileptr, distancetomove, SEEK_CUR);
          break;
        case FILE_END:
          fseek((FILE *)file->m_fileptr, distancetomove, SEEK_END);
          break;
        default:
          ASSERT(0);
          break;
      }
      result = (DWORD)ftell((FILE *)file->m_fileptr);
      break;

    case SFILE_COMPRESSED: {
      BYTE  buffer[0x1000];
      LONG  skip;
      LONG  skipped;
      DWORD amount;
      DWORD bytesRead;

      switch (movemethod) {
        default:
          result = file->m_curOffset;
          file->m_lock.Leave();
          return result;

        case FILE_BEGIN:
          if (distancetomove < (LONG)file->m_curOffset) {
            inflateEnd(file->m_zstream);
            file->m_zstream->avail_in = 0;
            file->m_zstream->next_in = (Bytef *)file->m_zbuffer;
            ASSERT(inflateInit_(file->m_zstream, "1.1.3", sizeof(z_stream)) == 0);
            fseek((FILE *)file->m_fileptr, sizeof(NoPaqCompHdr), SEEK_SET);
            file->m_curOffset = 0;
            skip = distancetomove;
          } else {
            skip = distancetomove - (LONG)file->m_curOffset;
          }
          break;

        case FILE_CURRENT:
          skip = distancetomove;
          break;
      }

      skipped = 0;
      while (skipped < skip) {
        amount = (DWORD)(skip - skipped);
        if (amount > sizeof(buffer)) {
          amount = sizeof(buffer);
        }
        if (!DoZRead(file, buffer, amount, &bytesRead)) {
          file->m_lock.Leave();
          return 0;
        }
        skipped += bytesRead;
      }
      result = file->m_curOffset;
      break;
    }

    case SFILE_PAQ:
    case SFILE_OLD_SFILE:
      result = SFileSetFilePointer((HSFILE)file->m_hsfile, distancetomove, distancetomovehigh, movemethod);
      break;

    case SFILE_ZIP_FILE:
      result = (DWORD)ZipFileSetFilePointer((ZipFileFCB *)file->m_zipFile, distancetomove, movemethod);
      break;

    default:
      ASSERT(0);
      result = 0;
      break;
  }
  file->m_lock.Leave();
  return result;
}

int APIENTRY SFile::EnableDirectAccess(DWORD access) {
  s_directaccess = access;
  SFileEnableDirectAccess(access);
  return TRUE;
}

void APIENTRY SFile::DisableSFileCheckDisk() {
  s_bSFileCheckDisk = false;
}

void APIENTRY SFile::CreateOverlapped(SOVERLAPPED *overlapped) {
  ASSERT(overlapped);
  overlapped->hEvent = NEW(SEvent)(TRUE, FALSE);
}

void APIENTRY SFile::DestroyOverlapped(SOVERLAPPED *overlapped) {
  ASSERT(overlapped);
  if (overlapped->hEvent) {
    delete overlapped->hEvent;
    overlapped->hEvent = NULL;
  }
}

void APIENTRY SFile::ResetOverlapped(SOVERLAPPED *overlapped) {
  ASSERT(overlapped);
  overlapped->hEvent->Reset();
}

void APIENTRY SFile::WaitOverlapped(SOVERLAPPED *overlapped) {
  ASSERT(overlapped);
  overlapped->hEvent->Wait(INFINITE);
}

int APIENTRY SFile::PollOverlapped(SOVERLAPPED *overlapped) {
  ASSERT(overlapped);
  return overlapped->hEvent->Wait(0) == WAIT_OBJECT_0;
}

void SFile::Destroy() {
  s_readQueueLock.Enter();
  s_exitReadThread = TRUE;
  s_readQueueEvent.Set();
  s_readQueueLock.Leave();
  if (s_readThreadInitialized && s_readThread.Wait(5000) != WAIT_OBJECT_0) {
    SOutputDebugString("SFile read thread did not exit");
  }
  s_fileMap.Clear();
  s_findFileHashInitialized = false;
}

int APIENTRY SFile::OpenArchive(const char *archivename, int priority, DWORD flags, SArchive **handle) {
  const char *dot;
  int         result;

  *handle = NEW(SArchive);
  (*handle)->m_archive = NULL;
  dot = SStrChrR(archivename, '.');
  if (dot && !SStrCmpI(dot, ".mpq", INT_MAX)) {
    (*handle)->m_type = SARCHIVE_MPQ;
    result = SFileOpenArchive(archivename, priority, flags, (HSARCHIVE *)&(*handle)->m_archive);
  } else {
    (*handle)->m_type = SARCHIVE_ZIP;
    (*handle)->m_archive = (void *)ZipFileOpenArchive(archivename);
    result = (*handle)->m_archive != NULL;
  }
  if (!result) {
    delete *handle;
    *handle = NULL;
  }
  return result;
}

int APIENTRY SFile::CloseArchive(SArchive *archive) {
  int result;

  ASSERT(archive);
  if (archive->m_type == SARCHIVE_ZIP) {
    result = ZipFileCloseArchive((DWORD)archive->m_archive);
  } else {
    result = SFileCloseArchive((HSARCHIVE)archive->m_archive);
  }
  delete archive;
  return result;
}

int APIENTRY SFile::GetMD5(SFile *file, MD5 &sum) {
  ASSERT(file);
  if (file->m_type <= SFILE_PLAIN || file->m_type > SFILE_ZIP_FILE || !file->m_haveMD5) {
    return FALSE;
  }
  sum = file->m_md5;
  return TRUE;
}

int APIENTRY SFile::List(SArchive *archive, int(*cb)(const char *filename, void *param), void *param) {
  unsigned int *list;
  BYTE         *cursor;
  BYTE         *end;
  DWORD         size;
  char          line[MAX_PATH];
  char         *output;
  const char   *extension;

  ASSERT(archive);
  switch (archive->m_type) {
    case SARCHIVE_ZIP:
      return ZipFileList((DWORD)archive->m_archive, cb, param);
    case SARCHIVE_MPQ:
      break;
    default:
      goto list_failed;
  }
  if (!SFile::Load(archive, "(listfile)", (void **)&list, &size, 0, 0, NULL)) {
    goto list_failed;
  }

  cursor = (BYTE *)list;
  end = cursor + size;
  output = line;
  line[0] = 0;
  while (cursor < end) {
    if (*cursor == '\r' || *cursor == '\n') {
      *output = 0;
      if (line[0]) {
        extension = SStrChrR(line, '.');
        if ((!extension || SStrCmpI(extension, ".md5", INT_MAX)) && !cb(line, param)) {
          break;
        }
      }
      output = line;
    } else if (output < line + sizeof(line) - 1) {
      *output++ = *cursor;
    }
    ++cursor;
  }
  SFile::Unload(list);
  return TRUE;

list_failed:
  return FALSE;
}
