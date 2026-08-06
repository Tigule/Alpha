#include <storm.h>
#include <stpl.h>
#include <SZip.h>

#include "../Zlib/zlib.h"

#include <new>
#include <stdio.h>
#include <string.h>

#define ZIP_MAX_COMMENT 0xFFFF
#define ZIP_READ_CHUNK  0x1000

struct CentralDirectoryHeader {
  char         signature[4];
  WORD         thisDiskNumber;
  WORD         directoryStartDiskNumber;
  WORD         directoryEntriesThisDisk;
  WORD         directoryEntriesTotal;
  unsigned int centralDirectorySize;
  unsigned int centralDirectoryOffset;
  WORD         commentLength;

  void EndianCorrect();
};

struct CentralDirectoryFileHeader {
  char         signature[4];
  WORD         versionMadeBy;
  WORD         versionRequired;
  WORD         generalFlags;
  WORD         compressionMethod;
  WORD         modifiedTime;
  WORD         modifiedDate;
  unsigned int z_crc32;
  unsigned int compressedSize;
  unsigned int uncompressedSize;
  WORD         filenameSize;
  WORD         extraFieldSize;
  WORD         commentSize;
  WORD         diskNumberStart;
  WORD         internalFileAttributes;
  unsigned int externalFileAttributes;
  unsigned int localHeaderOffset;

  void EndianCorrect();
};

struct LocalFileHeader {
  char         signature[4];
  WORD         versionRequired;
  WORD         generalFlags;
  WORD         compressionMethod;
  WORD         modifiedTime;
  WORD         modifiedDate;
  unsigned int z_crc32;
  unsigned int compressedSize;
  unsigned int uncompressedSize;
  WORD         filenameSize;
  WORD         extraFieldSize;

  void EndianCorrect();
};

struct DataDescriptor {
  unsigned int z_crc32;
  unsigned int compressedSize;
  unsigned int uncompressedSize;

  void EndianCorrect();
};

struct ZipFileArchive;
struct ZipFileDirEntry;

class Flags {
 public:
  Flags();
  void Set(unsigned int bit);
  void Clear(unsigned int bit);
  int  IsSet(unsigned int bit);
  int  IsClear(unsigned int bit);

 private:
  unsigned int m_value;
};

struct ZipFileDirEntry : TSHashObject<ZipFileDirEntry, HASHKEY_CONSTSTRI> {
  ZipFileArchive *archive;
  char            filename[0x100];
  unsigned int    startOffset;
  unsigned int    compressedSize;
  unsigned int    uncompressedSize;
  unsigned int    compressionMethod;
  ZipFileDirEntry();
  ~ZipFileDirEntry();
};

NODEDECL(ZipFileArchive) {
  FILE         *file;
  char          filename[0x100];
  unsigned int  openFileCount;

  ZipFileArchive();
  ~ZipFileArchive();
  int Open(const char *archivename);
  int GetCentralDirectoryHeader(CentralDirectoryHeader &cdirHeader);
  int ProcessCentralDirectory(CentralDirectoryHeader &cdirHeader);
  int ReadCentralDirectoryFileHeader();
};

struct ZipFileFCB {
  ZipFileDirEntry *dirEntry;
  Flags            flags;
  unsigned int     targetPosition;
  unsigned int     compressedPosition;
  unsigned int     uncompressedPosition;
  z_stream         zlibStream;
  BYTE             compressedData[ZIP_READ_CHUNK];

  ZipFileFCB();
  ~ZipFileFCB();
  int SetFault();
};

typedef TSHashTable<ZipFileDirEntry, HASHKEY_CONSTSTRI>   ZipDirTable;
typedef LISTEXDYN(ZipFileDirEntry)                        ZipDirList;
typedef TSGrowableArray<ZipDirList>                       ZipDirListArray;
static const char                                         centralDirectoryFileSignature[4] = {'P', 'K', 1, 2};
static const char                                         localFileSignature[4] = {'P', 'K', 3, 4};
static const char                                         centralDirectoryHeaderSignature[4] = {'P', 'K', 5, 6};
static WowFileSystem                                      s_fileSystem;
static TestFileSystemProvider                             s_testProvider;
static LISTDECL(ZipFileArchive, s_archives);

template <>
TSFixedArray<ZipDirList>::~TSFixedArray() {
  unsigned int index;
  ZipDirList  *data;

  for (index = 0; index < m_count; ++index) {
    m_data[index].~ZipDirList();
  }
  data = m_data;
  if (data) {
    SMemFree(data, MemFileName(), MemLineNo(), 0);
  }
}

template <>
void TSFixedArray<ZipDirList>::ReallocData(unsigned int count) {
  ZipDirList  *oldData;
  unsigned int index;

  oldData = m_data;
  if (count < m_count) {
    for (index = count; index < m_count; ++index) {
      oldData[index].~ZipDirList();
    }
  }

  m_alloc = count;
  m_data = static_cast<ZipDirList *>(SMemReAlloc(oldData, count * sizeof(ZipDirList), MemFileName(), MemLineNo(), 0x10));
  if (m_data) {
    return;
  }

  m_data = static_cast<ZipDirList *>(SMemAlloc(count * sizeof(ZipDirList), MemFileName(), MemLineNo(), 0));
  if (!oldData) {
    return;
  }

  for (index = 0; index < (count < m_count ? count : m_count); ++index) {
    if (m_data) {
      new (&m_data[index]) ZipDirList(oldData[index]);
    }
    oldData[index].~ZipDirList();
  }

  SMemFree(oldData, MemFileName(), MemLineNo(), 0);
}

template <>
void ZipDirTable::Initialize() {
  int          linkoffset;
  unsigned int index;

  m_slotmask = 3;
  m_slotlistarray.SetCount(4);
  linkoffset = GetLinkOffset();
  for (index = 0; index <= m_slotmask; ++index) {
    m_slotlistarray[index].ChangeLinkOffset(linkoffset);
  }
}

template <>
ZipFileDirEntry *ZipDirTable::InternalNew(ZipDirList *listptr, unsigned long extrabytes, unsigned long flags) {
  return listptr->NewNode(LIST_HEAD, extrabytes, flags);
}

template <>
int ZipDirTable::MonitorFullness(unsigned int slot) {
  if (m_slotmask >= 0x1FFF) {
    return 0;
  }

  if (m_fullnessIndicator > 3) {
    m_fullnessIndicator -= 3;
  } else {
    m_fullnessIndicator = 0;
  }

  ITERATELIST(ZipFileDirEntry, m_slotlistarray[slot], ptr) {
    ++m_fullnessIndicator;
    if (m_fullnessIndicator > 13) {
      m_fullnessIndicator = 0;
      GrowListArray((m_slotmask + 1) * 2);
      return 1;
    }
  }
  return 0;
}

template <>
ZipDirTable::~TSHashTable() {
  InternalClear(1);
}

template <>
ZipFileDirEntry *ZipDirTable::Ptr(const char *str) {
  unsigned int     hashval;
  unsigned int     slot;

  if (!Initialized()) {
    return 0;
  }
  hashval = SStrHashHT(str);
  slot = hashval & m_slotmask;
  ITERATELIST(ZipFileDirEntry, m_slotlistarray[slot], ptr) {
    if (ptr->m_hashval == hashval && ptr->m_key == str) {
      return ptr;
    }
  }
  return 0;
}

void ZipFileUnloadFile(void *buffer);

static ZipDirTable s_directory;

static void ConvertUInt16FromBinary(WORD &value) {
  BYTE *bytes;

  bytes = (BYTE *)&value;
  value = (WORD)(((WORD)bytes[1] << 8) | bytes[0]);
}

static void ConvertUInt32FromBinary(unsigned int &value) {
  BYTE *bytes;

  bytes = (BYTE *)&value;
  value = ((DWORD)bytes[3] << 24) | ((DWORD)bytes[2] << 16) | ((DWORD)bytes[1] << 8) | bytes[0];
}

void CentralDirectoryHeader::EndianCorrect() {
  ConvertUInt16FromBinary(thisDiskNumber);
  ConvertUInt16FromBinary(directoryStartDiskNumber);
  ConvertUInt16FromBinary(directoryEntriesThisDisk);
  ConvertUInt16FromBinary(directoryEntriesTotal);
  ConvertUInt32FromBinary(centralDirectorySize);
  ConvertUInt32FromBinary(centralDirectoryOffset);
  ConvertUInt16FromBinary(commentLength);
}

void LocalFileHeader::EndianCorrect() {
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

void DataDescriptor::EndianCorrect() {
  ConvertUInt32FromBinary(z_crc32);
  ConvertUInt32FromBinary(compressedSize);
  ConvertUInt32FromBinary(uncompressedSize);
}

void CentralDirectoryFileHeader::EndianCorrect() {
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

Flags::Flags() {
  m_value = 0;
}

void Flags::Set(unsigned int bit) {
  m_value |= bit;
}

void Flags::Clear(unsigned int bit) {
  m_value &= ~bit;
}

int Flags::IsSet(unsigned int bit) {
  return m_value & bit;
}

int Flags::IsClear(unsigned int bit) {
  return (m_value & bit) == 0;
}

ZipFileFCB::ZipFileFCB() {
}

ZipFileFCB::~ZipFileFCB() {
  if (flags.IsSet(4)) {
    inflateEnd(&zlibStream);
  }
}

int ZipFileFCB::SetFault() {
  if (flags.IsSet(4)) {
    inflateEnd(&zlibStream);
    flags.Clear(4);
  }
  flags.Set(1);
  return 0;
}

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

  ITERATELIST(ZipFileDirEntry, s_directory, entry) {
    if (entry->archive == this) {
      ITERATE_DELETE;
    }
  }
}

int ZipFileArchive::GetCentralDirectoryHeader(CentralDirectoryHeader &cdirHeader) {
  if (fseek(file, 0, SEEK_END)) {
    return 0;
  }

  unsigned int fileSize = ftell(file);
  if (fileSize == static_cast<unsigned int>(-1)) {
    return 0;
  }

  unsigned int offset = 0;
  if (fileSize > ZIP_MAX_COMMENT + sizeof(CentralDirectoryHeader) + 1) {
    offset = fileSize - ZIP_MAX_COMMENT - sizeof(CentralDirectoryHeader) - 1;
  }
  if (fseek(file, offset, SEEK_SET)) {
    return 0;
  }

  unsigned int signatureOffset = 0;
  while (!feof(file)) {
    if (static_cast<unsigned char>(fgetc(file)) == centralDirectoryHeaderSignature[signatureOffset]) {
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

  unsigned int headerOffset = ftell(file);
  if (headerOffset == static_cast<unsigned int>(-1)) {
    return 0;
  }
  if (fseek(file, headerOffset - sizeof(cdirHeader.signature), SEEK_SET)) {
    return 0;
  }
  if (fread(&cdirHeader, sizeof(cdirHeader), 1, file) != 1) {
    return 0;
  }

  cdirHeader.EndianCorrect();
  return cdirHeader.thisDiskNumber == cdirHeader.directoryStartDiskNumber && cdirHeader.directoryEntriesThisDisk == cdirHeader.directoryEntriesTotal;
}

int ZipFileArchive::ProcessCentralDirectory(CentralDirectoryHeader &cdirHeader) {
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
static void ConvertFromZip(char *str) {
  while (*str) {
    if (*str == '/') {
      *str = '\\';
    }
    ++str;
  }
}

int ZipFileArchive::ReadCentralDirectoryFileHeader() {
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

static int GetDirEntry(const char *filename, ZipFileDirEntry **dirEntry) {
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

static void *zalloc(void *opaque, unsigned int count, unsigned int size) {
  (void)opaque;
  return SMemAlloc(count * size, __FILE__, __LINE__, 8);
}

static void zfree(void *opaque, void *ptr) {
  (void)opaque;
  SMemFree(ptr, __FILE__, __LINE__, 0);
}

unsigned long ZipFileOpenArchive(const char *archivename) {
  ZipFileArchive        *archive;
  CentralDirectoryHeader cdirHeader;

  archive = s_archives.NewNode(LIST_TAIL, 0, 0);
  if (archive && archive->Open(archivename) && archive->GetCentralDirectoryHeader(cdirHeader) && archive->ProcessCentralDirectory(cdirHeader)) {
    return (unsigned long)archive;
  }
  if (archive) {
    s_archives.DeleteNode(archive);
  }
  return 0;
}

int ZipFileCloseArchive(unsigned long handle) {
  FATALASSERT(((ZipFileArchive *)handle)->openFileCount == 0);
  s_archives.DeleteNode((ZipFileArchive *)handle);
  return 1;
}

int ZipFileFileExists(const char *filename) {
  return GetDirEntry(filename, NULL);
}

ZipFileFCB *ZipFileOpenFile(const char *filename, unsigned long archive) {
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

  fcb = (ZipFileFCB *)SMemAlloc(sizeof(ZipFileFCB), __FILE__, __LINE__, 0);
  FATALASSERT(fcb);
  new (fcb) ZipFileFCB;
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
      fcb->~ZipFileFCB();
      SMemFree(fcb, "delete", -1, 0);
      return NULL;
    }
    fcb->flags.Set(4);
  }
  ++dirEntry->archive->openFileCount;
  return fcb;
}

int ZipFileCloseFile(ZipFileFCB *fcb) {
  --fcb->dirEntry->archive->openFileCount;
  delete fcb;
  return 1;
}

int ZipFileSetFilePointer(ZipFileFCB *fcb, int offset, int origin) {
  DWORD target;

  FATALASSERT(fcb);
  FATALASSERT(origin >= FILE_BEGIN && origin <= FILE_END);
  if (fcb->flags.IsSet(1)) {
    return 0;
  }

  target = 0;
  switch (origin) {
    case FILE_BEGIN:
      target = offset;
      break;
    case FILE_CURRENT:
      target = fcb->targetPosition + offset;
      break;
    case FILE_END:
      target = fcb->dirEntry->uncompressedSize + offset;
      break;
  }

  if (target > fcb->dirEntry->uncompressedSize) {
    return fcb->SetFault();
  }

  switch (fcb->dirEntry->compressionMethod) {
    case 0:
      break;

    default:
      FATALASSERT(fcb->dirEntry->compressionMethod == 0 || fcb->dirEntry->compressionMethod == Z_DEFLATED);

    case 8:
      if (target < fcb->targetPosition) {
        if (fcb->flags.IsSet(4) && inflateEnd(&fcb->zlibStream)) {
          return fcb->SetFault();
        }
        fcb->compressedPosition = 0;
        fcb->uncompressedPosition = 0;
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
      break;
  }

  fcb->targetPosition = target;
  return 1;
}

unsigned long ZipFileGetFilePointer(ZipFileFCB *fcb) {
  return fcb->targetPosition;
}

unsigned long ZipFileGetFileSize(ZipFileFCB *fcb) {
  return fcb->dirEntry->uncompressedSize;
}

int ZipFileReadFile(ZipFileFCB *fcb, void *buffer, unsigned int bytesToRead, unsigned int *bytesRead) {
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
  switch (fcb->dirEntry->compressionMethod) {
    case 0:
      if (fseek(file, (long)(fcb->dirEntry->startOffset + fcb->targetPosition), SEEK_SET)) {
        return fcb->SetFault();
      }
      bytesProduced = (DWORD)fread(buffer, 1, bytesToRead, file);
      if (bytesProduced != bytesToRead && ferror(file)) {
        return fcb->SetFault();
      }
      break;

    default:
      FATALASSERT(fcb->dirEntry->compressionMethod == Z_DEFLATED);

    case 8:
      bytesSkipped = fcb->targetPosition - fcb->uncompressedPosition;
      fcb->zlibStream.next_out = (BYTE *)buffer;
      fcb->zlibStream.avail_out = bytesToRead;
      if (bytesSkipped && bytesSkipped < bytesToRead) {
        fcb->zlibStream.avail_out = bytesSkipped;
      }
      if (fseek(file, (long)(fcb->dirEntry->startOffset + fcb->compressedPosition), SEEK_SET)) {
        return fcb->SetFault();
      }

      while (fcb->zlibStream.avail_out) {
        if (!fcb->zlibStream.avail_in && fcb->flags.IsSet(2)) {
          fcb->zlibStream.next_in = fcb->compressedData;
          inputBytes = fcb->dirEntry->compressedSize - fcb->compressedPosition;
          if (inputBytes > sizeof(fcb->compressedData)) {
            inputBytes = sizeof(fcb->compressedData);
          } else if (!inputBytes) {
            bytesProduced = bytesToRead - fcb->zlibStream.avail_out;
            if (bytesRead) {
              *bytesRead = bytesProduced;
            }
            return bytesProduced != 0;
          }
          fcb->zlibStream.avail_in = (DWORD)fread(fcb->compressedData, 1, inputBytes, file);
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
          if (!bytesSkipped || bytesSkipped >= bytesToRead) {
            fcb->zlibStream.avail_out = bytesToRead;
          } else {
            fcb->zlibStream.avail_out = bytesSkipped;
          }
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
      break;
  }

  fcb->targetPosition += bytesProduced;
  if (bytesRead) {
    *bytesRead = bytesProduced;
  }
  return 1;
}

int ZipFileLoadFile(const char *filename, void **buffer, unsigned int *bytes) {
  z_stream         stream;
  ZipFileDirEntry *dirEntry;
  BYTE            *compressedData;
  BYTE            *uncompressedData;
  int              err;

  FATALASSERT(filename);
  FATALASSERT(buffer);

  dirEntry = NULL;
  if (!GetDirEntry(filename, &dirEntry)) {
    return 0;
  }
  if (fseek(dirEntry->archive->file, (long)dirEntry->startOffset, SEEK_SET)) {
    return 0;
  }

  stream.zalloc = (alloc_func)zalloc;
  stream.zfree = (free_func)zfree;
  compressedData = (BYTE *)SMemAlloc(dirEntry->compressedSize, __FILE__, __LINE__, 0);
  FATALASSERT(compressedData);
  stream.next_in = compressedData;
  stream.avail_in = dirEntry->compressedSize;
  if (fread(compressedData, dirEntry->compressedSize, 1, dirEntry->archive->file) != 1) {
    SMemFree(compressedData, __FILE__, __LINE__, 0);
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

void ZipFileUnloadFile(void *buffer) {
  SMemFree(buffer, __FILE__, __LINE__, 0);
}

int ZipFileList(unsigned long archive, int(*cb)(const char *, void *), void *param) {
  ZipFileArchive *archiveptr = (ZipFileArchive *)archive;

  ITERATELIST(ZipFileDirEntry, s_directory, entry) {
    if (entry->archive == archiveptr && !cb(entry->filename, param)) {
      break;
    }
  }
  return 1;
}

TestFile::TestFile(WowFileSystemProvider *provider, FILE *f) : WowFile(provider), m_f(f) {
}

TestFileSystemProvider::TestFileSystemProvider() {
}

int ZipFileArchive::Open(const char *archivename) {
  FATALASSERT(archivename);
  file = fopen(archivename, "rb");
  if (!file) {
    return 0;
  }
  SStrCopy(filename, archivename, sizeof(filename));
  return 1;
}

TestFileSystemProvider::~TestFileSystemProvider() {
}

WowFile *TestFileSystemProvider::Open(const char *filename) {
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

WowFile *WowFileSystem::Open(const char *filename) {
  if (!m_providerList) {
    return NULL;
  }
  return m_providerList->Open(filename);
}

void FSTest() {
  s_fileSystem.RegisterProvider(s_testProvider);
  WowFile *file = s_fileSystem.Open("c:\\boot.ini");
  file->Close();
  s_fileSystem.UnregisterProvider(s_testProvider);
}
