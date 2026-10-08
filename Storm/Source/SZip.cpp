#include <storm.h>
#include <stpl.h>
#include <SZip.h>

#include "../Zlib/zlib.h"

#include <new>
#include <stdio.h>
#include <string.h>

#define ZIP_MAX_COMMENT 0xFFFF
#define ZIP_READ_CHUNK  0x1000
#define DEFLATED        8

#pragma pack(1)

void ConvertUInt16FromBinary(WORD &value) {
  value = (WORD)((((BYTE *)&value)[1] << 8) | ((BYTE *)&value)[0]);
}

void ConvertUInt32FromBinary(UINT &value) {
  value = ((((((BYTE *)&value)[3] << 8) | ((BYTE *)&value)[2]) << 8 | ((BYTE *)&value)[1]) << 8) | ((BYTE *)&value)[0];
}

struct CentralDirectoryHeader {
  char signature[4];
  WORD thisDiskNumber;
  WORD directoryStartDiskNumber;
  WORD directoryEntriesThisDisk;
  WORD directoryEntriesTotal;
  UINT centralDirectorySize;
  UINT centralDirectoryOffset;
  WORD commentLength;

  void EndianCorrect() {
    ConvertUInt16FromBinary(thisDiskNumber);
    ConvertUInt16FromBinary(directoryStartDiskNumber);
    ConvertUInt16FromBinary(directoryEntriesThisDisk);
    ConvertUInt16FromBinary(directoryEntriesTotal);
    ConvertUInt32FromBinary(centralDirectorySize);
    ConvertUInt32FromBinary(centralDirectoryOffset);
    ConvertUInt16FromBinary(commentLength);
  }
};

struct CentralDirectoryFileHeader {
  char signature[4];
  WORD versionMadeBy;
  WORD versionRequired;
  WORD generalFlags;
  WORD compressionMethod;
  WORD modifiedTime;
  WORD modifiedDate;
  UINT z_crc32;
  UINT compressedSize;
  UINT uncompressedSize;
  WORD filenameSize;
  WORD extraFieldSize;
  WORD commentSize;
  WORD diskNumberStart;
  WORD internalFileAttributes;
  UINT externalFileAttributes;
  UINT localHeaderOffset;

  void EndianCorrect() {
    ConvertUInt16FromBinary(versionMadeBy);
    ConvertUInt16FromBinary(versionRequired);
    ConvertUInt16FromBinary(generalFlags);
    ConvertUInt16FromBinary(compressionMethod);
    ConvertUInt16FromBinary(modifiedTime);
    ConvertUInt16FromBinary(modifiedDate);
    ConvertUInt32FromBinary(z_crc32);
    ConvertUInt32FromBinary(compressedSize);
    ConvertUInt32FromBinary(uncompressedSize);
    ConvertUInt16FromBinary(filenameSize);
    ConvertUInt16FromBinary(extraFieldSize);
    ConvertUInt16FromBinary(commentSize);
    ConvertUInt16FromBinary(diskNumberStart);
    ConvertUInt16FromBinary(internalFileAttributes);
    ConvertUInt32FromBinary(externalFileAttributes);
    ConvertUInt32FromBinary(localHeaderOffset);
  }
};

struct LocalFileHeader {
  char signature[4];
  WORD versionRequired;
  WORD generalFlags;
  WORD compressionMethod;
  WORD modifiedTime;
  WORD modifiedDate;
  UINT z_crc32;
  UINT compressedSize;
  UINT uncompressedSize;
  WORD filenameSize;
  WORD extraFieldSize;

  void EndianCorrect() {
    ConvertUInt16FromBinary(versionRequired);
    ConvertUInt16FromBinary(generalFlags);
    ConvertUInt16FromBinary(compressionMethod);
    ConvertUInt16FromBinary(modifiedTime);
    ConvertUInt16FromBinary(modifiedDate);
    ConvertUInt32FromBinary(z_crc32);
    ConvertUInt32FromBinary(compressedSize);
    ConvertUInt32FromBinary(uncompressedSize);
    ConvertUInt16FromBinary(filenameSize);
    ConvertUInt16FromBinary(extraFieldSize);
  }
};

struct DataDescriptor {
  UINT z_crc32;
  UINT compressedSize;
  UINT uncompressedSize;

  void EndianCorrect() {
    ConvertUInt32FromBinary(z_crc32);
    ConvertUInt32FromBinary(compressedSize);
    ConvertUInt32FromBinary(uncompressedSize);
  }
};

#pragma pack()

struct ZipFileArchive;
struct ZipFileDirEntry;

class Flags {
  UINT m_value;

 public:
  Flags() {
    m_value = 0;
  }
  void Set(UINT bit) {
    m_value |= bit;
  }
  void Clear(UINT bit) {
    m_value &= ~bit;
  }
  BOOL IsSet(UINT bit) {
    return m_value & bit;
  }
  BOOL IsClear(UINT bit) {
    return (m_value & bit) == 0;
  }
};

struct ZipFileDirEntry : TSHashObject<ZipFileDirEntry, HASHKEY_CONSTSTRI> {
  ZipFileArchive *archive;
  char            filename[0x100];
  UINT            startOffset;
  UINT            compressedSize;
  UINT            uncompressedSize;
  UINT            compressionMethod;
  ZipFileDirEntry();
  ~ZipFileDirEntry();
};

NODEDECL(ZipFileArchive) {
  FILE *file;
  char  filename[0x100];
  UINT  openFileCount;

  ZipFileArchive();
  ~ZipFileArchive();
  int Open(LPCSTR archivename);
  int GetCentralDirectoryHeader(CentralDirectoryHeader & cdirHeader);
  int ProcessCentralDirectory(CentralDirectoryHeader & cdirHeader);
  int ReadCentralDirectoryFileHeader();
};

struct ZipFileFCB {
  ZipFileDirEntry *dirEntry;
  Flags            flags;
  UINT             targetPosition;
  UINT             compressedPosition;
  UINT             uncompressedPosition;
  z_stream         zlibStream;
  BYTE             compressedData[ZIP_READ_CHUNK];

  ZipFileFCB() {
  }
  ~ZipFileFCB() {
    if (flags.IsSet(4)) {
      inflateEnd(&zlibStream);
    }
  }
  BOOL SetFault() {
    if (flags.IsSet(4)) {
      inflateEnd(&zlibStream);
      flags.Clear(4);
    }
    flags.Set(1);
    return 0;
  }
};

typedef TSHashTable<ZipFileDirEntry, HASHKEY_CONSTSTRI> ZipDirTable;
typedef LISTEXDYN(ZipFileDirEntry) ZipDirList;
typedef TSGrowableArray<ZipDirList> ZipDirListArray;
static const char                   centralDirectoryFileSignature[4] = {'P', 'K', 1, 2};
static const char                   localFileSignature[4] = {'P', 'K', 3, 4};
static const char                   centralDirectoryHeaderSignature[4] = {'P', 'K', 5, 6};
static LISTDECL(ZipFileArchive, s_archives);

void ZipFileUnloadFile(LPVOID buffer);

static ZipDirTable s_directory;

ZipFileArchive::ZipFileArchive() {
  file = NULL;
  openFileCount = 0;
}

ZipFileArchive::~ZipFileArchive() {
  FATALASSERT(openFileCount == 0);
  if (file) {
    fclose(file);
    file = NULL;
  }

  ZipFileDirEntry *next;
  for (ZipFileDirEntry *entry = s_directory.Head(); (int)entry > 0; entry = next) {
    next = s_directory.RawNext(entry);
    if (entry->archive == this) {
      s_directory.DeleteNode(entry);
    }
  }
}

BOOL ZipFileArchive::Open(LPCSTR archivename) {
  FATALASSERT(archivename);
  file = fopen(archivename, "rb");
  if (!file) {
    return 0;
  }
  SStrCopy(filename, archivename, sizeof(filename));
  return 1;
}

BOOL ZipFileArchive::GetCentralDirectoryHeader(CentralDirectoryHeader &cdirHeader) {
  if (fseek(file, 0, SEEK_END)) {
    return 0;
  }

  UINT fileSize = ftell(file);
  if (fileSize == static_cast<UINT>(-1)) {
    return 0;
  }

  UINT offset;
  if (fileSize > ZIP_MAX_COMMENT + sizeof(CentralDirectoryHeader) + 1) {
    offset = fileSize - ZIP_MAX_COMMENT - sizeof(CentralDirectoryHeader) - 1;
  } else {
    offset = 0;
  }
  if (fseek(file, offset, SEEK_SET)) {
    return 0;
  }

  UINT signatureOffset = 0;
  while (!feof(file)) {
    if (static_cast<char>(fgetc(file)) == centralDirectoryHeaderSignature[signatureOffset]) {
      ++signatureOffset;
      if (signatureOffset == sizeof(centralDirectoryHeaderSignature)) {
        break;
      }
    } else {
      signatureOffset = 0;
    }
  }
  if (signatureOffset < sizeof(centralDirectoryHeaderSignature)) {
    return 0;
  }

  UINT headerOffset = ftell(file);
  if (headerOffset == static_cast<UINT>(-1)) {
    return 0;
  }
  if (fseek(file, headerOffset - sizeof(cdirHeader.signature), SEEK_SET)) {
    return 0;
  }
  if (fread(&cdirHeader, sizeof(cdirHeader), 1, file) != 1) {
    return 0;
  }

  cdirHeader.EndianCorrect();
  if (cdirHeader.thisDiskNumber != cdirHeader.directoryStartDiskNumber) {
    return 0;
  }
  return cdirHeader.directoryEntriesThisDisk == cdirHeader.directoryEntriesTotal;
}

BOOL ZipFileArchive::ProcessCentralDirectory(CentralDirectoryHeader &cdirHeader) {
  DWORD i;

  if (fseek(file, (long)cdirHeader.centralDirectoryOffset, SEEK_SET) != 0) {
    return 0;
  }

  for (i = 0; i < cdirHeader.directoryEntriesTotal; ++i) {
    if (!ReadCentralDirectoryFileHeader()) {
      return 0;
    }
  }
  return 1;
}
void ConvertFromZip(char *str) {
  if (*str) {
    do {
      if (*str == '/') {
        *str = '\\';
      }
    } while (*++str);
  }
}

BOOL ZipFileArchive::ReadCentralDirectoryFileHeader() {
  char                       localFilename[0x100];
  char                       cdirFilename[0x100];
  CentralDirectoryFileHeader cdirFileHeader;
  LocalFileHeader            localFileHeader;
  DataDescriptor             trailer;
  DWORD                      compressedDataOffset;
  DWORD                      saveOffset;
  ZipFileDirEntry           *entry;
  if (fread(&cdirFileHeader, sizeof(cdirFileHeader), 1, file) != 1) {
    return 0;
  }
  cdirFileHeader.EndianCorrect();
  if (memcmp(cdirFileHeader.signature, centralDirectoryFileSignature, sizeof(cdirFileHeader.signature)) != 0) {
    return 0;
  }

  FATALASSERT(cdirFileHeader.filenameSize < sizeof(cdirFilename));
  if (fread(cdirFilename, cdirFileHeader.filenameSize, 1, file) != 1) {
    return 0;
  }
  cdirFilename[cdirFileHeader.filenameSize] = 0;
  ConvertFromZip(cdirFilename);

  fseek(file, cdirFileHeader.extraFieldSize, SEEK_CUR);
  fseek(file, cdirFileHeader.commentSize, SEEK_CUR);
  if (cdirFileHeader.compressionMethod != 0 && cdirFileHeader.compressionMethod != 8) {
    return 0;
  }
  if (s_directory.Ptr(filename)) {
    return 0;
  }

  saveOffset = (DWORD)ftell(file);
  fseek(file, (long)cdirFileHeader.localHeaderOffset, SEEK_SET);
  if (fread(&localFileHeader, sizeof(localFileHeader), 1, file) != 1) {
    return 0;
  }
  localFileHeader.EndianCorrect();
  if (memcmp(localFileHeader.signature, localFileSignature, sizeof(localFileHeader.signature)) != 0) {
    return 0;
  }

  FATALASSERT(localFileHeader.filenameSize < sizeof(localFilename));
  if (fread(localFilename, localFileHeader.filenameSize, 1, file) != 1) {
    return 0;
  }
  localFilename[localFileHeader.filenameSize] = 0;
  ConvertFromZip(localFilename);
  if (SStrCmp(cdirFilename, localFilename, 0x7FFFFFFF) != 0) {
    return 0;
  }

  fseek(file, localFileHeader.extraFieldSize, SEEK_CUR);
  if (localFileHeader.generalFlags & 8) {
    if (fread(&trailer, sizeof(trailer), 1, file) != 1) {
      return 0;
    }
    trailer.EndianCorrect();
    localFileHeader.z_crc32 = trailer.z_crc32;
    localFileHeader.compressedSize = trailer.compressedSize;
    localFileHeader.uncompressedSize = trailer.uncompressedSize;
  }
  compressedDataOffset = (DWORD)ftell(file);

  entry = (ZipFileDirEntry *)SMemAlloc(sizeof(ZipFileDirEntry), __FILE__, __LINE__, 0);
  if (entry) {
    new (entry) ZipFileDirEntry;
  }
  entry->archive = this;
  SStrCopy(entry->filename, cdirFilename, sizeof(entry->filename));
  entry->startOffset = compressedDataOffset;
  entry->compressedSize = cdirFileHeader.compressedSize;
  entry->uncompressedSize = cdirFileHeader.uncompressedSize;
  entry->compressionMethod = cdirFileHeader.compressionMethod;
  s_directory.Insert(entry, entry->filename);

  fseek(file, (long)saveOffset, SEEK_SET);
  return 1;
}

ZipFileDirEntry::ZipFileDirEntry() {
  startOffset = 0;
}

static BOOL GetDirEntry(LPCSTR filename, ZipFileDirEntry **dirEntry) {
  ZipFileDirEntry *found = s_directory.Ptr(filename);

  if (!found) {
    return 0;
  }
  if (dirEntry) {
    *dirEntry = found;
  }
  return 1;
}

ZipFileDirEntry::~ZipFileDirEntry() {
}

LPVOID zalloc(LPVOID opaque, UINT count, UINT size) {
  (void)opaque;
  return SMemAlloc(count * size, __FILE__, __LINE__, 8);
}

void zfree(LPVOID opaque, LPVOID ptr) {
  (void)opaque;
  SMemFree(ptr, __FILE__, __LINE__, 0);
}

DWORD ZipFileOpenArchive(LPCSTR archivename) {
  ZipFileArchive        *archive;
  CentralDirectoryHeader cdirHeader;

  archive = s_archives.NewNode(LIST_TAIL, 0, 0);
  if (archive && (!archive->Open(archivename) || !archive->GetCentralDirectoryHeader(cdirHeader) || !archive->ProcessCentralDirectory(cdirHeader))) {
    s_archives.DeleteNode(archive);
    archive = NULL;
  }
  return (DWORD)archive;
}

BOOL ZipFileCloseArchive(DWORD handle) {
  ZipFileArchive *archive = (ZipFileArchive *)handle;
  FATALASSERT(archive->openFileCount == 0);
  s_archives.DeleteNode(archive);
  return 1;
}

int ZipFileFileExists(LPCSTR filename) {
  return GetDirEntry(filename, NULL);
}

ZipFileFCB *ZipFileOpenFile(LPCSTR filename, DWORD archive) {
  ZipFileArchive  *archiveptr;
  ZipFileDirEntry *dirEntry;
  ZipFileFCB      *fcb;

  FATALASSERT(filename);
  dirEntry = NULL;
  if (!GetDirEntry(filename, &dirEntry)) {
    return NULL;
  }
  archiveptr = (ZipFileArchive *)archive;
  if (archiveptr && dirEntry->archive != archiveptr) {
    return NULL;
  }

  fcb = NEW(ZipFileFCB);
  FATALASSERT(fcb);
  fcb->dirEntry = dirEntry;
  fcb->targetPosition = 0;
  if (dirEntry->compressionMethod == 8) {
    fcb->compressedPosition = 0;
    fcb->uncompressedPosition = 0;
    fcb->zlibStream.next_in = fcb->compressedData;
    fcb->zlibStream.avail_in = 0;
    fcb->zlibStream.next_out = NULL;
    fcb->zlibStream.avail_out = 0;
    fcb->zlibStream.zalloc = (alloc_func)zalloc;
    fcb->zlibStream.zfree = (free_func)zfree;
    fcb->flags.Set(2);
    if (inflateInit2(&fcb->zlibStream, -15)) {
      DEL(fcb);
      return NULL;
    }
    fcb->flags.Set(4);
  }
  ++dirEntry->archive->openFileCount;
  return fcb;
}

BOOL ZipFileCloseFile(ZipFileFCB *fcb) {
  --fcb->dirEntry->archive->openFileCount;
  delete fcb;
  return 1;
}

int ZipFileSetFilePointer(ZipFileFCB *fcb, int offset, int origin) {
  DWORD target;

  FATALASSERT(fcb);
  FATALASSERT(origin >= 0 && origin <= 2);
  if (fcb->flags.IsSet(1)) {
    return 0;
  }

  target = 0;
  switch (origin) {
    case FILE_BEGIN:
      target = offset;
      break;
    case FILE_CURRENT:
      target = fcb->uncompressedPosition + offset;
      break;
    case FILE_END:
      target = fcb->dirEntry->uncompressedSize + offset;
      break;
  }

  if (target > fcb->dirEntry->uncompressedSize) {
    return fcb->SetFault();
  }

  if (fcb->dirEntry->compressionMethod != 0) {
    FATALASSERT(fcb->dirEntry->compressionMethod == DEFLATED);
    if (target < fcb->targetPosition) {
      if (fcb->flags.IsSet(4) && inflateEnd(&fcb->zlibStream)) {
        return fcb->SetFault();
      }
      fcb->uncompressedPosition = 0;
      fcb->compressedPosition = 0;
      fcb->zlibStream.next_in = fcb->compressedData;
      fcb->zlibStream.avail_in = 0;
      fcb->zlibStream.next_out = NULL;
      fcb->zlibStream.avail_out = 0;
      fcb->flags.Set(2);
      if (inflateInit2(&fcb->zlibStream, -15)) {
        return fcb->SetFault();
      }
      fcb->flags.Set(4);
    }
  }

  fcb->targetPosition = target;
  return 1;
}

DWORD ZipFileGetFilePointer(ZipFileFCB *fcb) {
  return fcb->targetPosition;
}

DWORD ZipFileGetFileSize(ZipFileFCB *fcb) {
  return fcb->dirEntry->uncompressedSize;
}

int ZipFileReadFile(ZipFileFCB *fcb, LPVOID buffer, UINT bytesToRead, UINT *bytesRead) {
  FILE *file;
  DWORD bytesSkipped;
  DWORD bytesProduced;
  DWORD inputBytes;
  DWORD outputBefore;
  int   zresult;

  FATALASSERT(fcb);
  FATALASSERT(buffer);
  if (fcb->flags.IsSet(1)) {
    return 0;
  }
  if (fcb->flags.IsClear(4)) {
    return 0;
  }

  file = fcb->dirEntry->archive->file;
  if (fcb->dirEntry->compressionMethod == 0) {
    if (fseek(file, fcb->dirEntry->startOffset + fcb->targetPosition, SEEK_SET)) {
      return fcb->SetFault();
    }
    bytesProduced = fread(buffer, 1, bytesToRead, file);
    if (bytesProduced != bytesToRead && ferror(file)) {
      return fcb->SetFault();
    }
  } else {
    FATALASSERT(fcb->dirEntry->compressionMethod == DEFLATED);
    fcb->zlibStream.next_out = (BYTE *)buffer;
    fcb->zlibStream.avail_out = bytesToRead;
    bytesSkipped = fcb->targetPosition - fcb->uncompressedPosition;
    if (bytesSkipped && bytesSkipped < bytesToRead) {
      fcb->zlibStream.avail_out = bytesSkipped;
    }
    if (fseek(file, fcb->dirEntry->startOffset + fcb->compressedPosition, SEEK_SET)) {
      return fcb->SetFault();
    }

    while (fcb->zlibStream.avail_out) {
      if (!fcb->zlibStream.avail_in && fcb->flags.IsSet(2)) {
        fcb->zlibStream.next_in = fcb->compressedData;
        inputBytes = min(sizeof(fcb->compressedData), fcb->dirEntry->compressedSize - fcb->compressedPosition);
        if (!inputBytes) {
          bytesProduced = bytesToRead - fcb->zlibStream.avail_out;
          if (bytesRead) {
            *bytesRead = bytesProduced;
          }
          return bytesProduced > 0;
        }
        fcb->zlibStream.avail_in = fread(fcb->zlibStream.next_in, 1, inputBytes, file);
        if (fcb->zlibStream.avail_in != inputBytes) {
          return fcb->SetFault();
        }
        fcb->compressedPosition += fcb->zlibStream.avail_in;
      }

      outputBefore = fcb->zlibStream.avail_out;
      zresult = inflate(&fcb->zlibStream, Z_SYNC_FLUSH);
      if (zresult != Z_OK && zresult != Z_STREAM_END) {
        return fcb->SetFault();
      }
      bytesProduced = outputBefore - fcb->zlibStream.avail_out;
      fcb->uncompressedPosition += bytesProduced;

      if (bytesSkipped) {
        bytesSkipped -= bytesProduced;
        fcb->zlibStream.next_out = (BYTE *)buffer;
        fcb->zlibStream.avail_out = (bytesSkipped && bytesSkipped < bytesToRead) ? bytesSkipped : bytesToRead;
      }

      if (zresult == Z_STREAM_END) {
        fcb->flags.Clear(2);
        inflateEnd(&fcb->zlibStream);
        fcb->flags.Clear(4);
        break;
      }
      if (fcb->zlibStream.avail_out) {
        fcb->flags.Set(2);
      } else {
        fcb->flags.Clear(2);
      }
    }
    bytesProduced = bytesToRead - fcb->zlibStream.avail_out;
  }

  fcb->targetPosition += bytesProduced;
  if (bytesRead) {
    *bytesRead = bytesProduced;
  }
  return 1;
}

BOOL ZipFileLoadFile(LPCSTR filename, LPVOID *buffer, UINT *bytes) {
  z_stream         stream;
  ZipFileDirEntry *dirEntry;
  ZipFileArchive  *archive;
  BYTE            *compressedData;
  BYTE            *uncompressedData;
  int              err;

  FATALASSERT(filename);
  FATALASSERT(buffer);

  dirEntry = NULL;
  if (!GetDirEntry(filename, &dirEntry)) {
    return 0;
  }
  archive = dirEntry->archive;
  if (fseek(archive->file, (long)dirEntry->startOffset, SEEK_SET)) {
    return 0;
  }

  stream.zalloc = (alloc_func)zalloc;
  stream.zfree = (free_func)zfree;
  compressedData = (BYTE *)SMemAlloc(dirEntry->compressedSize, __FILE__, __LINE__, 0);
  FATALASSERT(compressedData);
  stream.next_in = compressedData;
  stream.avail_in = dirEntry->compressedSize;
  if (fread(compressedData, dirEntry->compressedSize, 1, archive->file) != 1) {
    SMemFree(stream.next_in, __FILE__, __LINE__, 0);
    return 0;
  }

  uncompressedData = (BYTE *)SMemAlloc(dirEntry->uncompressedSize, __FILE__, __LINE__, 0);
  FATALASSERT(uncompressedData);
  stream.next_out = uncompressedData;
  stream.avail_out = dirEntry->uncompressedSize;
  if (inflateInit2(&stream, -15)) {
    SMemFree(uncompressedData, __FILE__, __LINE__, 0);
    SMemFree(compressedData, __FILE__, __LINE__, 0);
    return 0;
  }

  err = inflate(&stream, Z_FINISH);
  if (err != Z_STREAM_END) {
    inflateEnd(&stream);
    SMemFree(compressedData, __FILE__, __LINE__, 0);
    SMemFree(uncompressedData, __FILE__, __LINE__, 0);
    return 0;
  }

  err = inflateEnd(&stream);
  SMemFree(compressedData, __FILE__, __LINE__, 0);
  if (!err && stream.total_out == dirEntry->uncompressedSize) {
    *buffer = uncompressedData;
    if (bytes) {
      *bytes = dirEntry->uncompressedSize;
    }
    return 1;
  }

  SMemFree(uncompressedData, __FILE__, __LINE__, 0);
  return 0;
}

void ZipFileUnloadFile(LPVOID buffer) {
  SMemFree(buffer, __FILE__, __LINE__, 0);
}

BOOL ZipFileList(DWORD archive, int (*cb)(LPCSTR, LPVOID), LPVOID param) {
  ZipFileArchive *archiveptr = (ZipFileArchive *)archive;

  ITERATELIST(ZipFileDirEntry, s_directory, entry) {
    if (entry->archive == archiveptr && !cb(entry->filename, param)) {
      break;
    }
  }
  return 1;
}

WowFile *TestFileSystemProvider::Open(LPCSTR filename) {
  FILE *f = fopen(filename, "rb");
  if (f) {
    return new TestFile(this, f);
  }
  return NULL;
}

bool TestFileSystemProvider::Close(WowFile *f) {
  fclose(static_cast<TestFile *>(f)->m_f);
  delete f;
  return true;
}

void WowFileSystem::RegisterProvider(WowFileSystemProvider &provider) {
  FATALASSERT(m_providerList == 0);
  m_providerList = &provider;
}

void WowFileSystem::UnregisterProvider(WowFileSystemProvider &provider) {
  FATALASSERT(m_providerList == &provider);
  m_providerList = NULL;
}

WowFile *WowFileSystem::Open(LPCSTR filename) {
  if (!m_providerList) {
    return NULL;
  }
  return m_providerList->Open(filename);
}

static WowFileSystem          s_fileSystem;
static TestFileSystemProvider s_testProvider;

void FSTest() {
  s_fileSystem.RegisterProvider(s_testProvider);
  WowFile *file = s_fileSystem.Open("c:\\boot.ini");
  file->Close();
  s_fileSystem.UnregisterProvider(s_testProvider);
}
