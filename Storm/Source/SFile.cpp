#include <storm.h>
#include <stpl.h>
#include <W32/ISThread.h>

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* MPQ names retained by the current client and corroborated by Hellfire. */
#define HASH_INDEX       0
#define HASH_CHECK0      1
#define HASH_CHECK1      2
#define HASH_ENCRYPTKEY  3
#define HASH_ENCRYPTDATA 4

#define MPQ_COMPRESSED_PKWARE 0x00000100
#define MPQ_COMPRESSED_SCOMP  0x00000200
#define MPQ_COMPRESSEDMASK    0x0000FF00
#define MPQ_ENCRYPTED         0x00010000
#define MPQ_ENCRYPTED_FIXLOC  0x00020000
#define MPQ_ALLOCATED         0x80000000

#define DEALLOCATED   0xFFFFFFFE
#define NOBLOCK       0xFFFFFFFF
#define MPQ_SIGNATURE 0x1A51504D
#define READAHEAD     0x1000

#define KEYCONTAINER  "Blizzard_Storm"
#define SIGNATUREFILE "(signature)"
#define LISTFILE      "(listfile)"
#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES ((DWORD)0xFFFFFFFF)
#endif

struct _ARCHIVEHEADER {
  DWORD signature;
  DWORD headersize;
  DWORD archivesize;
  WORD  version;
  WORD  sectorsizeid;
  DWORD hashoffset;
  DWORD blockoffset;
  DWORD hashcount;
  DWORD blockcount;
};

struct _HASHENTRY {
  DWORD hashcheck[2];
  WORD  languageId;
  BYTE  platformId;
  BYTE  reserved;
  DWORD block;
};

struct _BLOCKENTRY {
  DWORD    offset;
  DWORD    sizealloc;
  DWORD    sizefile;
  DWORD    flags;
  FILETIME time;
  DWORD    crc;
  MD5      md5;
};

namespace Storm {
  namespace SFile {

    NODEDECL(ARCHIVEREC) {
      ~ARCHIVEREC();

      BOOL IsReopenedArchive() {
        return parentArchive != NULL;
      }
      BOOL IsSubArchive() {
        return ownerarchivefile != NULL;
      }

      CCritSect      sync;
      LONG           refcount;
      char           archivename[MAX_PATH];
      LPVOID         handle;
      int            dontCheckDisk;
      int            priority;
      _ARCHIVEHEADER header;
      HSFILE         ownerarchivefile;
      HSARCHIVE      parentArchive;
      char           pathPrefix[MAX_PATH];
      LPVOID         sectorfile;
      DWORD          sectorlocation;
      DWORD          sectorsize;
      BYTE          *sectorbuffer;
      DWORD          sectorbytesread;
      DWORD          startinglocation;
      DWORD          endinglocation;
      _BLOCKENTRY   *blocktable;
      _HASHENTRY    *hashtable;
      DWORD          lastlocation;
      int            disableCount;
      DWORD          cdrom;
    };

    NODEDECL(FILEREC) {
      ~FILEREC();

      CCritSect   sync;
      LONG        refcount;
      char        name[MAX_PATH];
      char       *actualName;
      LPVOID      handle;
      ARCHIVEREC *archive;
      _BLOCKENTRY block;
      DWORD       key;
      DWORD       location;
      DWORD       lastlocation;
      DWORD       sectors;
      DWORD      *sectoroffsettable;
      int         sectoroffsettablevalid;
      int         dda;
      LPVOID      readaheadbuffer;
      DWORD       readaheadoffset;
      DWORD       readaheadbytes;
      DWORD       crc;
      int         crcavail;
      DWORD       crcstate;
      DWORD       crcexpected;

      FILEREC() : actualName(0), handle(INVALID_HANDLE_VALUE), archive(0), sectoroffsettable(0), readaheadbuffer(0) {
      }
    };

    NODEDECL(AUDIOSTREAM) {
      ~AUDIOSTREAM();

      FILEREC            *file;
      DWORD               nextwrite;
      DWORD               bytespersecond;
      int                 loop;
      DWORD               fillstatus;
      DWORD               bytespastend;
      DWORD               waveheadersize;
      DWORD               wavedatasize;
      DWORD               startingoffset;
      DWORD               playcursor;
      DWORD               writecursor;
      LONG                volume;
      LONG                pan;
      IDirectSoundBuffer *soundbuffer;
      DWORD               soundbuffersize;
      int                 soundbufferlocal;
      BYTE                fillvalue;
      LONG                refcount;
    };

    struct StormGlobals {
      DWORD s_ioerrormode;
      int(APIENTRY *s_ioerrorproc)(LPCSTR, DWORD, DWORD);
      WORD  s_languageId;
      BYTE  s_platformId;
      DWORD s_dataChunkSize;
      int   s_seekOptimize;
      DWORD WAVECHUNKSIZE;
      UINT  s_asyncBudget;
      DWORD s_directaccess;
      char  s_basepath[MAX_PATH];
      void(APIENTRY *s_loadNotifyProc)(LPCSTR, LPVOID);
      LPVOID s_loadNotifyData;
    };

    struct ArchivePtr {
      ArchivePtr(HSARCHIVE hArchive = 0);
      ~ArchivePtr();
      void        Enter();
      void        Leave();
                  operator ARCHIVEREC *() const;
      ARCHIVEREC *operator->();
      ARCHIVEREC &operator*() {
        return *archive;
      }

     protected:
      ARCHIVEREC *archive;
      int         locked;
    };

    struct ArchivePtrLocked : public ArchivePtr {
      ArchivePtrLocked(HSARCHIVE hArchive);
      ~ArchivePtrLocked();
    };

    struct FilePtr {
      FilePtr(HSFILE hFile);
      ~FilePtr();
      void     Enter();
      void     Leave();
               operator FILEREC *() const;
      FILEREC *operator->();
      FILEREC &operator*() {
        return *file;
      }

     protected:
      FILEREC *file;
      int      locked;
    };

    struct FilePtrLocked : public FilePtr {
      FilePtrLocked(HSFILE hFile);
      ~FilePtrLocked();
    };

    ARCHIVEREC *GetArchivePtr(HSARCHIVE hArchive);
    int         ReleaseArchivePtr(ARCHIVEREC *archive);
    BOOL        IsSubArchive(HSARCHIVE archive);
    BOOL        IsReopenedArchive(HSARCHIVE archive);
    FILEREC    *GetFilePtr(HSFILE hFile);
    int         ReleaseFilePtr(FILEREC *file);
    void        AddArchiveRef(ARCHIVEREC *archive);
    void        AddFileRef(FILEREC *file);
    void        AddStreamRef(AUDIOSTREAM *stream);
    void        RemoveArchiveRef(ARCHIVEREC *archive);
    void        RemoveFileRef(FILEREC *file);
    void        RemoveStreamRef(AUDIOSTREAM *stream);
    int         s_OpenArchive(ARCHIVEREC *archiveptr, DWORD flags, int cdrom, HSARCHIVE *handle);

    struct UseGlob {
      UseGlob();
      ~UseGlob();
      StormGlobals *operator->();
      StormGlobals &operator*() {
        return *globptr;
      }

     protected:
      StormGlobals *globptr;
    };

  }  // namespace SFile
}  // namespace Storm

static int ReadFileChecked(DWORD position, DWORD *currentposition, HANDLE file, LPVOID buffer, DWORD bytestoread, DWORD *bytesread, LPCSTR filename);
static int CheckFileExistsOnDisk(LPCSTR filename, DWORD flags, char *localfilename);

NODEDECL(REQUEST) {
  ~REQUEST();

  LPVOID                     event;
  Storm::SFile::FILEREC     *file;
  DWORD                      location;
  DWORD                      approxarchivelocation;
  DWORD                      requiredcompletiontime;
  int                        urgent;
  LPVOID                     buffer;
  LPVOID                     bufferbegin;
  IDirectSoundBuffer        *soundbuffer;
  DWORD                      soundbufferoffset;
  Storm::SFile::AUDIOSTREAM *stream;
  DWORD                      bytestoread;
  int                        autodelrequest;
  DWORD                      bytesread;
  _TASYNCPARAMBLOCK         *asyncparam;
};

NODEDECL(EVENTREC) {
  LPVOID event;
};

namespace Storm {
  namespace SFile {

    StormGlobals  s_g = {2, 0, 0, 0, 0x20000, 1, 0, 0, 0, "", 0, 0};
    DWORD        *s_hashsource = 0;
    DWORD         s_cdthreadid = 0;
    IDirectSound *s_directsound = 0;
    LPVOID        s_cdthread = 0;
    LPVOID        s_explodebuffer = 0;
    LPVOID        s_soundreadbuffer = 0;
    LPVOID        s_cdevent = 0;
    int           s_cdshutdown = 0;
    REQUEST      *s_cdrequest = 0;
    int           s_closingDirectSound = 0;

    CCritSect s_cdlock;
    LISTDECL(REQUEST, s_cdreqlist);
    LISTDECL(EVENTREC, s_signalList);
    CCritSect s_streamlock;
    LISTDECL(AUDIOSTREAM, s_streamlist);
    CCritSect s_archivelock;
    LISTDECL(ARCHIVEREC, s_archivelist);
    CCritSect s_filelock;
    LISTDECL(FILEREC, s_filelist);
    CCritSect s_lzwcrit;
    CCritSect s_globcritsect;

  }  // namespace SFile
}  // namespace Storm

typedef Storm::SFile::ARCHIVEREC  SFileArchiveRecData;
typedef Storm::SFile::FILEREC     SFileRecData;
typedef Storm::SFile::AUDIOSTREAM SFileAudioStreamData;
typedef REQUEST                   SFileRequestData;
typedef EVENTREC                  SFileEventRecData;
typedef LIST(REQUEST) SFileRequestList;
typedef LIST(EVENTREC) SFileEventRecList;
typedef LIST(SFileAudioStreamData) SFileAudioStreamList;
typedef _ARCHIVEHEADER SFileArchiveHeaderData;
typedef _HASHENTRY     SFileHashEntryData;
typedef _BLOCKENTRY    SFileBlockEntryData;

struct SFileDirectSoundBufferVtbl {
  LPVOID QueryInterface;
  LPVOID AddRef;
  ULONG(STDMETHODCALLTYPE *Release)(IDirectSoundBuffer *buffer);
  LPVOID GetCaps;
  HRESULT(STDMETHODCALLTYPE *GetCurrentPosition)(IDirectSoundBuffer *buffer, DWORD *currentplaycursor, DWORD *currentwritecursor);
  LPVOID GetFormat;
  HRESULT(STDMETHODCALLTYPE *GetVolume)(IDirectSoundBuffer *buffer, LONG *volume);
  HRESULT(STDMETHODCALLTYPE *GetPan)(IDirectSoundBuffer *buffer, LONG *pan);
  LPVOID GetFrequency;
  LPVOID GetStatus;
  LPVOID Initialize;
  HRESULT(STDMETHODCALLTYPE *Lock)(
      IDirectSoundBuffer *buffer,
      DWORD               offset,
      DWORD               bytes,
      LPVOID             *audioPtr1,
      DWORD              *audioBytes1,
      LPVOID             *audioPtr2,
      DWORD              *audioBytes2,
      DWORD               flags
  );
  HRESULT(STDMETHODCALLTYPE *Play)(IDirectSoundBuffer *buffer, DWORD reserved1, DWORD reserved2, DWORD flags);
  LPVOID SetCurrentPosition;
  LPVOID SetFormat;
  HRESULT(STDMETHODCALLTYPE *SetVolume)(IDirectSoundBuffer *buffer, LONG volume);
  HRESULT(STDMETHODCALLTYPE *SetPan)(IDirectSoundBuffer *buffer, LONG pan);
  LPVOID SetFrequency;
  HRESULT(STDMETHODCALLTYPE *Stop)(IDirectSoundBuffer *buffer);
  HRESULT(STDMETHODCALLTYPE *Unlock)(IDirectSoundBuffer *buffer, LPVOID audioPtr1, DWORD audioBytes1, LPVOID audioPtr2, DWORD audioBytes2);
};

struct SFileDirectSoundVtbl {
  LPVOID QueryInterface;
  LPVOID AddRef;
  LPVOID Release;
  HRESULT(STDMETHODCALLTYPE *CreateSoundBuffer)(IDirectSound *sound, LPVOID desc, IDirectSoundBuffer **buffer, LPVOID outer);
};

struct _DSBUFFERDESC {
  DWORD          dwSize;
  DWORD          dwFlags;
  DWORD          dwBufferBytes;
  DWORD          dwReserved;
  tWAVEFORMATEX *lpwfxFormat;
};

struct CKINFO {
  DWORD size;
  DWORD offset;
};

struct _DECOMPRESSIONINFO {
  LPVOID sourcebuffer;
  DWORD  sourceoffset;
  LPVOID destbuffer;
  DWORD  destoffset;
  DWORD  bytes;
};

static const BYTE modulus[0x100] = {
    0x77, 0x64, 0xF8, 0x57, 0x1D, 0xFB, 0xB0, 0x09, 0xC4, 0xE6, 0x28, 0x91, 0x34, 0xE3, 0x55, 0x61, 0x15, 0x8A, 0xE9, 0x07, 0xFC, 0xAA, 0x60, 0xB3,
    0x82, 0xB7, 0xE2, 0xA4, 0x40, 0x15, 0x01, 0x3F, 0xC2, 0x36, 0xA8, 0x9D, 0x95, 0xD0, 0x54, 0x69, 0xAA, 0xF5, 0xED, 0x5C, 0x7F, 0x21, 0xC5, 0x55,
    0x95, 0x56, 0x5B, 0x2F, 0xC6, 0xDD, 0x2C, 0xBD, 0x74, 0xA3, 0x5A, 0x0D, 0x70, 0x98, 0x9A, 0x01, 0x36, 0x51, 0x78, 0x71, 0x9B, 0x8E, 0xCB, 0xB8,
    0x84, 0x67, 0x30, 0xF4, 0x43, 0xB3, 0xA3, 0x50, 0xA3, 0xBA, 0xA4, 0xF7, 0xB1, 0x94, 0xE5, 0x5B, 0x95, 0x8B, 0x1A, 0xE4, 0x04, 0x1D, 0xFB, 0xCF,
    0x0E, 0xE6, 0x97, 0x4C, 0xDC, 0xE4, 0x28, 0x7F, 0xB8, 0x58, 0x4A, 0x45, 0x1B, 0xC8, 0x8C, 0xD0, 0xFD, 0x2E, 0x77, 0xC4, 0x30, 0xD8, 0x3D, 0xD2,
    0xD5, 0xFA, 0xBA, 0x9D, 0x1E, 0x02, 0xF6, 0x7B, 0xBE, 0x08, 0x95, 0xCB, 0xB0, 0x53, 0x3E, 0x1C, 0x41, 0x45, 0xFC, 0x27, 0x6F, 0x63, 0x6A, 0x73,
    0x91, 0xA9, 0x42, 0x00, 0x12, 0x93, 0xF8, 0x5B, 0x83, 0xED, 0x52, 0x77, 0x4E, 0x38, 0x08, 0x16, 0x23, 0x10, 0x85, 0x4C, 0x0B, 0xA9, 0x8C, 0x9C,
    0x40, 0x4C, 0xAF, 0x6E, 0xA7, 0x89, 0x02, 0xC5, 0x06, 0x96, 0x99, 0x41, 0xD4, 0x31, 0x03, 0x4A, 0xA9, 0x2B, 0x17, 0x52, 0xDD, 0x5C, 0x4E, 0x5F,
    0x16, 0xC3, 0x81, 0x0F, 0x2E, 0xE2, 0x17, 0x45, 0x2B, 0x7B, 0x65, 0x7A, 0xA3, 0x18, 0x87, 0xC2, 0xB2, 0xF5, 0xCD, 0x9C, 0xBA, 0xCB, 0xDE, 0x07,
    0x6F, 0x7C, 0x8B, 0x03, 0x68, 0xE6, 0x3C, 0x5A, 0x2C, 0xAE, 0xDC, 0xC3, 0xC8, 0x38, 0x35, 0x82, 0xDA, 0x4D, 0x04, 0xCE, 0x9C, 0x68, 0x47, 0x7D,
    0xB4, 0x1D, 0x98, 0x42, 0x8C, 0xF8, 0x27, 0x7E, 0xC8, 0x87, 0xF6, 0x24, 0xCE, 0x7E, 0x06, 0xB1
};

static const BYTE exponent[4] = {0x01, 0x00, 0x01, 0x00};

typedef BOOL(WINAPI *SFileCryptAcquireContextAProc)(DWORD *, LPCSTR, LPCSTR, DWORD, DWORD);
typedef BOOL(WINAPI *SFileCryptCreateHashProc)(DWORD, DWORD, DWORD, DWORD, DWORD *);
typedef BOOL(WINAPI *SFileCryptDestroyHashProc)(DWORD);
typedef BOOL(WINAPI *SFileCryptDestroyKeyProc)(DWORD);
typedef BOOL(WINAPI *SFileCryptHashDataProc)(DWORD, const BYTE *, DWORD, DWORD);
typedef BOOL(WINAPI *SFileCryptImportKeyProc)(DWORD, const BYTE *, DWORD, DWORD, DWORD, DWORD *);
typedef BOOL(WINAPI *SFileCryptReleaseContextProc)(DWORD, DWORD);
typedef BOOL(WINAPI *SFileCryptSignHashAProc)(DWORD, DWORD, LPCSTR, DWORD, BYTE *, DWORD *);
typedef BOOL(WINAPI *SFileCryptVerifySignatureAProc)(DWORD, const BYTE *, DWORD, DWORD, LPCSTR, DWORD);

struct SFileCryptoApi {
  SFileCryptAcquireContextAProc  acquireContext;
  SFileCryptCreateHashProc       createHash;
  SFileCryptDestroyHashProc      destroyHash;
  SFileCryptDestroyKeyProc       destroyKey;
  SFileCryptHashDataProc         hashData;
  SFileCryptImportKeyProc        importKey;
  SFileCryptReleaseContextProc   releaseContext;
  SFileCryptSignHashAProc        signHash;
  SFileCryptVerifySignatureAProc verifySignature;
};

struct _AUTHCOMPANYINFO {
  LPCSTR keyname;
  DWORD  authresult;
};

static const _AUTHCOMPANYINFO s_authcompany[1] = {
    {"BLIZZARDKEY", SFILE_AUTH_AUTHENTICBLIZZARD}
};

typedef LIST(SFileArchiveRecData) SFileArchiveRecList;
typedef LIST(SFileRecData) SFileRecList;

Storm::SFile::AUDIOSTREAM::~AUDIOSTREAM() {
  Storm::SFile::RemoveFileRef(file);
  if (soundbuffer && soundbufferlocal) {
    SFileDirectSoundBufferVtbl *vtbl = *(SFileDirectSoundBufferVtbl **)soundbuffer;
    vtbl->Release(soundbuffer);
  }
  Unlink();
}

REQUEST::~REQUEST() {
  Storm::SFile::RemoveFileRef(file);
  Storm::SFile::RemoveStreamRef(stream);
}

Storm::SFile::FILEREC::~FILEREC() {
  if (handle != INVALID_HANDLE_VALUE) {
    CloseHandle(handle);
  }
  if (archive) {
    archive->sectorfile = NULL;
  }
  if (sectoroffsettable) {
    FREE(sectoroffsettable);
  }
  if (readaheadbuffer) {
    FREE(readaheadbuffer);
  }
  if (actualName) {
    FREE(actualName);
  }

  Storm::SFile::s_archivelock.Enter();
  Storm::SFile::RemoveArchiveRef(archive);
  Storm::SFile::s_archivelock.Leave();
}

Storm::SFile::ArchivePtr::~ArchivePtr() {
  Leave();
  ReleaseArchivePtr(archive);
}

Storm::SFile::ArchivePtr::ArchivePtr(HSARCHIVE hArchive) {
  archive = GetArchivePtr(hArchive);
  locked = 0;
}

void Storm::SFile::ArchivePtr::Enter() {
  if (!locked && archive) {
    archive->sync.Enter();
  }
  locked = 1;
}

Storm::SFile::ArchivePtr::operator Storm::SFile::ARCHIVEREC *() const {
  return archive;
}

void Storm::SFile::ArchivePtr::Leave() {
  if (locked && archive) {
    archive->sync.Leave();
  }
  locked = 0;
}

Storm::SFile::ARCHIVEREC *Storm::SFile::ArchivePtr::operator->() {
  return archive;
}

Storm::SFile::ArchivePtrLocked::~ArchivePtrLocked() {
}

Storm::SFile::ArchivePtrLocked::ArchivePtrLocked(HSARCHIVE hArchive) : ArchivePtr(hArchive) {
  Enter();
}

Storm::SFile::FilePtr::~FilePtr() {
  Leave();
  ReleaseFilePtr(file);
}

void Storm::SFile::FilePtr::Enter() {
  if (!locked && file) {
    file->sync.Enter();
  }
  locked = 1;
}

void Storm::SFile::FilePtr::Leave() {
  if (locked && file) {
    file->sync.Leave();
  }
  locked = 0;
}

Storm::SFile::FilePtr::operator Storm::SFile::FILEREC *() const {
  return file;
}

Storm::SFile::FILEREC *Storm::SFile::FilePtr::operator->() {
  return file;
}

Storm::SFile::FilePtr::FilePtr(HSFILE hFile) {
  file = GetFilePtr(hFile);
  locked = 0;
}

Storm::SFile::FilePtrLocked::~FilePtrLocked() {
}

Storm::SFile::FilePtrLocked::FilePtrLocked(HSFILE hFile) : FilePtr(hFile) {
  Enter();
}

Storm::SFile::UseGlob::~UseGlob() {
  Storm::SFile::s_globcritsect.Leave();
}

Storm::SFile::StormGlobals *Storm::SFile::UseGlob::operator->() {
  return globptr;
}

Storm::SFile::ARCHIVEREC::~ARCHIVEREC() {
  if (!IsReopenedArchive()) {
    if (handle != INVALID_HANDLE_VALUE && !IsSubArchive()) {
      CloseHandle(handle);
    }
    if (sectorbuffer) {
      FREE(sectorbuffer);
    }
    if (blocktable) {
      FREE(blocktable);
    }
    if (hashtable) {
      FREE(hashtable);
    }
  }

  Unlink();
}

Storm::SFile::UseGlob::UseGlob() {
  Storm::SFile::s_globcritsect.Enter();
  globptr = &Storm::SFile::s_g;
}

Storm::SFile::ARCHIVEREC *Storm::SFile::GetArchivePtr(HSARCHIVE hArchive) {
  SFileArchiveRecData *found;
  SFileArchiveRecData *record;

  if (!hArchive) {
    return NULL;
  }

  record = (SFileArchiveRecData *)hArchive;
  found = NULL;
  s_archivelock.Enter();
  ITERATELIST(SFileArchiveRecData, s_archivelist, archive) {
    if (archive == record) {
      found = archive;
      AddArchiveRef(archive);
      break;
    }
  }
  s_archivelock.Leave();

  if (!found) {
    SErrSetLastError(ERROR_INVALID_HANDLE);
  }
  return found;
}

int Storm::SFile::ReleaseArchivePtr(ARCHIVEREC *archive) {
  if (!archive) {
    return FALSE;
  }
  s_archivelock.Enter();
  RemoveArchiveRef(archive);
  s_archivelock.Leave();
  return TRUE;
}

int Storm::SFile::IsSubArchive(HSARCHIVE archive) {
  ArchivePtr base(archive);
  return base && base->IsSubArchive();
}

int Storm::SFile::IsReopenedArchive(HSARCHIVE archive) {
  ArchivePtr base(archive);
  return base && base->IsReopenedArchive();
}

Storm::SFile::FILEREC *Storm::SFile::GetFilePtr(HSFILE hFile) {
  SFileRecData *found;
  SFileRecData *record;

  if (!hFile) {
    return NULL;
  }

  record = (SFileRecData *)hFile;
  found = NULL;
  s_filelock.Enter();
  ITERATELIST(SFileRecData, s_filelist, file) {
    if (file == record) {
      found = file;
      AddFileRef(file);
      break;
    }
  }
  s_filelock.Leave();

  if (!found) {
    SErrSetLastError(ERROR_INVALID_HANDLE);
  }
  return found;
}

int Storm::SFile::ReleaseFilePtr(FILEREC *file) {
  if (!file) {
    return FALSE;
  }
  s_filelock.Enter();
  RemoveFileRef(file);
  s_filelock.Leave();
  return TRUE;
}

UINT __cdecl DecompressLzw_BufferRead(char *buffer, UINT *size, LPVOID param) {
  _DECOMPRESSIONINFO *info = static_cast<_DECOMPRESSIONINFO *>(param);
  UINT                bytes = info->bytes - info->sourceoffset;

  if (*size < bytes) {
    bytes = *size;
  }
  memcpy(buffer, static_cast<BYTE *>(info->sourcebuffer) + info->sourceoffset, bytes);
  info->sourceoffset += bytes;
  return bytes;
}

void __cdecl DecompressLzw_BufferWrite(char *buffer, UINT *size, LPVOID param) {
  _DECOMPRESSIONINFO *info = static_cast<_DECOMPRESSIONINFO *>(param);

  memcpy(static_cast<BYTE *>(info->destbuffer) + info->destoffset, buffer, *size);
  info->destoffset += *size;
}

static int DecompressLzw(BYTE *dest, BYTE *source, DWORD sourcebytes) {
  (void)dest;
  (void)source;
  (void)sourcebytes;
  return FALSE;
}

static void Decrypt(DWORD *data, DWORD bytes, DWORD key) {
  DWORD seed;

  bytes >>= 2;
  if (bytes) {
    seed = 0xEEEEEEEE;
    do {
      seed += Storm::SFile::s_hashsource[0x400 + (BYTE)key];
      *data ^= key + seed;
      seed = seed * 0x21 + *data + 3;
      key = (key >> 0x0B) | ((~key << 0x15) + 0x11111111);
      ++data;
    } while (--bytes);
  }
}

static DWORD Hash(LPCSTR filename, int hashtype) {
  DWORD hash;
  DWORD seed;

  hash = 0x7FED7FED;
  seed = 0xEEEEEEEE;
  while (filename && *filename) {
    int ch = (BYTE)toupper((signed char)*filename++);
    if (ch == '/') {
      ch = '\\';
    }
    hash = Storm::SFile::s_hashsource[(hashtype << 8) + ch] ^ (hash + seed);
    seed = seed * 0x21 + ch + hash + 3;
  }
  return hash;
}

static void InitializeHashSource(DWORD seed) {
  DWORD i;
  DWORD j;
  DWORD index;
  DWORD value1;
  DWORD value2;

  if (!Storm::SFile::s_hashsource) {
    Storm::SFile::s_hashsource = (DWORD *)ALLOC(0x1400);
  }

  for (i = 0; i < 0x100; ++i) {
    index = i;
    for (j = 0; j < 5; ++j, index += 0x100) {
      seed = (seed * 125 + 3) % 0x2AAAAB;
      value1 = seed & 0xFFFF;
      seed = (seed * 125 + 3) % 0x2AAAAB;
      value2 = seed & 0xFFFF;
      Storm::SFile::s_hashsource[index] = (value1 << 16) | value2;
    }
  }
}

typedef BOOL(APIENTRY *SFileIoErrorProc)(LPCSTR filename, DWORD error, DWORD retryCount);

static DWORD InternalReadAligned(SFileRecData *file, DWORD location, LPVOID buffer, DWORD bytes) {
  SFileArchiveRecData *archive;
  DWORD                sectorSize;
  DWORD                flags;
  DWORD                compression;
  DWORD                bytesRead;
  DWORD                tableBytes;
  DWORD                savedLocation;
  DWORD                i;
  DWORD                firstSector;
  DWORD                sectorCount;
  DWORD                compressedSpan;
  DWORD                compressedStart;
  DWORD                readAmount;
  DWORD                outOffset;
  DWORD                inOffset;
  DWORD                sectorIndex;
  DWORD                sectorBytes;
  DWORD                expectedBytes;
  BYTE                *workBuffer;
  BYTE                *dest;

  archive = file->archive;
  sectorSize = archive->sectorsize;
  FATALASSERT(!(location & (file->archive->sectorsize - 1)));
  if (!bytes) {
    return 0;
  }

  flags = file->block.flags;
  compression = flags & MPQ_COMPRESSEDMASK;

  if (compression && !file->sectoroffsettablevalid) {
    bytesRead = 0;
    savedLocation = archive->lastlocation;
    tableBytes = (file->sectors * sizeof(DWORD)) + sizeof(DWORD);
    if (!ReadFileChecked(
            file->block.offset, &savedLocation, (HANDLE)archive->handle, file->sectoroffsettable, tableBytes, &bytesRead, archive->archivename
        ))
    {
      return 0;
    }
    archive->lastlocation = savedLocation;
    if (flags & MPQ_ENCRYPTED) {
      Decrypt((DWORD *)file->sectoroffsettable, bytesRead, file->key - 1);
    }
    for (i = 0; i < file->sectors; ++i) {
      DWORD *offsets = (DWORD *)file->sectoroffsettable;
      if (offsets[i] > offsets[i + 1]) {
        return 0;
      }
    }
    file->sectoroffsettablevalid = TRUE;
  }

  firstSector = location / sectorSize;
  sectorCount = (bytes + sectorSize - 1) / sectorSize;
  compressedStart = location;
  compressedSpan = bytes;

  if (compression) {
    DWORD *offsets = (DWORD *)file->sectoroffsettable;
    compressedStart = offsets[firstSector];
    compressedSpan = offsets[firstSector + sectorCount] - compressedStart;
  }

  workBuffer = (BYTE *)buffer;
  if (compression) {
    workBuffer = (BYTE *)ALLOC(compressedSpan + 4);
    if (!workBuffer) {
      SErrSetLastError(ERROR_NOT_ENOUGH_MEMORY);
      return 0;
    }
  }

  bytesRead = 0;
  savedLocation = archive->lastlocation;
  ReadFileChecked(
      file->block.offset + compressedStart, &savedLocation, (HANDLE)archive->handle, workBuffer, compressedSpan, &bytesRead, archive->archivename
  );
  archive->lastlocation = savedLocation;

  readAmount = bytes;
  if (bytesRead < compressedSpan) {
    if (compression) {
      DWORD *offsets = (DWORD *)file->sectoroffsettable;
      DWORD  completeBytes = 0;
      for (i = firstSector + 1; i <= file->sectors; ++i) {
        if (bytesRead < offsets[i] - compressedStart) {
          break;
        }
        completeBytes += sectorSize;
      }
      if (completeBytes < readAmount) {
        readAmount = completeBytes;
      }
    } else {
      readAmount = bytesRead;
    }
  }

  if (flags & MPQ_ENCRYPTED) {
    inOffset = 0;
    for (i = 0; i < sectorCount && i + firstSector < file->sectors; ++i) {
      sectorIndex = firstSector + i;
      if (compression) {
        DWORD *offsets = (DWORD *)file->sectoroffsettable;
        sectorBytes = offsets[sectorIndex + 1] - offsets[sectorIndex];
      } else {
        sectorBytes = readAmount - inOffset;
        if (sectorBytes > sectorSize) {
          sectorBytes = sectorSize;
        }
        if (sectorIndex == file->sectors - 1) {
          DWORD finalBytes = file->block.sizefile & (sectorSize - 1);
          if (finalBytes && sectorBytes >= finalBytes) {
            sectorBytes = finalBytes;
          }
        }
      }
      Decrypt((DWORD *)(workBuffer + inOffset), sectorBytes & ~3, file->key + sectorIndex);
      inOffset += sectorBytes;
      if (inOffset >= bytesRead) {
        break;
      }
    }
  }

  if (compression) {
    dest = (BYTE *)buffer;
    inOffset = 0;
    outOffset = 0;
    for (i = 0; i < sectorCount && i + firstSector < file->sectors && outOffset < readAmount; ++i) {
      sectorIndex = firstSector + i;
      {
        DWORD *offsets = (DWORD *)file->sectoroffsettable;
        sectorBytes = offsets[sectorIndex + 1] - offsets[sectorIndex];
      }

      expectedBytes = sectorSize;
      if (sectorIndex == file->sectors - 1) {
        expectedBytes = file->block.sizefile & (sectorSize - 1);
        if (!expectedBytes) {
          expectedBytes = sectorSize;
        }
      }

      if (expectedBytes > sectorBytes) {
        if (compression == MPQ_COMPRESSED_PKWARE) {
          DecompressLzw(dest + outOffset, workBuffer + inOffset, sectorBytes);
        } else if (compression == MPQ_COMPRESSED_SCOMP) {
          DWORD outSize = expectedBytes;
          if (file->crcavail && file->crcstate != 1 && (workBuffer[inOffset] & 0xC0)) {
            file->crcstate = 1;
          }
          if (!SCompDecompress2(dest + outOffset, &outSize, workBuffer + inOffset, sectorBytes, archive->archivename)) {
            SErrDisplayError(0x85100083, archive->archivename, -4, NULL, FALSE, 1);
          }
        }
      } else if (workBuffer != dest) {
        memcpy(dest + outOffset, workBuffer + inOffset, expectedBytes);
      }

      outOffset += expectedBytes;
      inOffset += sectorBytes;
    }
    FREE(workBuffer);
  }

  return readAmount;
}

static DWORD InternalReadAlignedSector(SFileRecData *file, DWORD location) {
  DWORD bytes;

  if (file->archive->sectorfile == file && file->archive->sectorlocation == location) {
    return file->archive->sectorbytesread;
  }

  bytes = file->archive->sectorsize;
  if (location + bytes > file->block.sizefile && (file->block.flags & MPQ_COMPRESSEDMASK) == 0) {
    bytes = file->block.sizefile - location;
  }

  file->archive->sectorfile = file;
  file->archive->sectorlocation = location;
  file->archive->sectorbytesread = InternalReadAligned(file, location, file->archive->sectorbuffer, bytes);
  return file->archive->sectorbytesread;
}

static DWORD InternalReadUnaligned(SFileRecData *file, DWORD location, LPVOID buffer, DWORD bytes) {
  SFileArchiveRecData *archive;
  BYTE                *cursor;
  DWORD                sectorSize;
  DWORD                sectorMask;
  DWORD                totalRead;
  DWORD                requested;
  DWORD                read;
  DWORD                offset;
  DWORD                copyBytes;
  DWORD                alignedBytes;

  if (file->block.sizefile <= location) {
    return 0;
  }
  if (bytes >= file->block.sizefile - location) {
    bytes = file->block.sizefile - location;
  }

  if (file->crcavail && location == 0) {
    CrcBuffer(NULL, 0, &file->crc, 1);
    file->crcexpected = 0;
    file->crcstate = 2;
  }

  archive = file->archive;
  sectorSize = archive->sectorsize;
  sectorMask = sectorSize - 1;
  cursor = (BYTE *)buffer;
  totalRead = 0;

  if (location & sectorMask) {
    if (file->crcavail && file->crcstate != 1 && file->crcexpected != location) {
      file->crcstate = 1;
    }
    offset = location & sectorMask;
    read = InternalReadAlignedSector(file, location & ~sectorMask);
    copyBytes = sectorSize - offset;
    if (copyBytes > bytes) {
      copyBytes = bytes;
    }
    memcpy(cursor, (BYTE *)archive->sectorbuffer + offset, copyBytes);
    if (file->crcavail && file->crcstate == 2) {
      CrcBuffer(cursor, copyBytes, &file->crc, 2);
      file->crcexpected += copyBytes;
    }

    bytes -= copyBytes;
    cursor += copyBytes;
    location += copyBytes;
    totalRead = copyBytes;

    if (read != sectorSize) {
      if (read < offset) {
        return 0;
      }
      read -= offset;
      return totalRead < read ? totalRead : read;
    }
    if (!bytes) {
      return totalRead;
    }
  }

  alignedBytes = bytes & ~sectorMask;
  if (alignedBytes) {
    if (file->crcavail && file->crcstate != 1 && file->crcexpected != location) {
      file->crcstate = 1;
    }
    read = InternalReadAligned(file, location, cursor, alignedBytes);
    if (file->crcavail && file->crcstate == 2) {
      CrcBuffer(cursor, read, &file->crc, 2);
      file->crcexpected += read;
    }

    cursor += alignedBytes;
    location += alignedBytes;
    bytes -= alignedBytes;
    totalRead += read;

    if (read != alignedBytes || !bytes) {
      return totalRead;
    }
  }

  if (bytes) {
    if (file->crcavail && file->crcstate != 1 && file->crcexpected != location) {
      file->crcstate = 1;
    }
    read = InternalReadAlignedSector(file, location);
    requested = bytes;
    memcpy(cursor, archive->sectorbuffer, requested);
    if (requested > read) {
      requested = read;
    }
    if (file->crcavail && file->crcstate == 2) {
      CrcBuffer(cursor, requested, &file->crc, 2);
      file->crcexpected += requested;
    }
    totalRead += requested;
  }

  return totalRead;
}

static int ReadFileChecked(DWORD position, DWORD *currentposition, HANDLE file, LPVOID buffer, DWORD bytestoread, DWORD *bytesread, LPCSTR filename) {
  SFileIoErrorProc retryProc;
  DWORD            mode;
  DWORD            numcalls;
  DWORD            error;
  BOOL             result;

  numcalls = 0;
  for (;;) {
    if (position != 0xFFFFFFFF) {
      SetFilePointer(file, position, NULL, FILE_BEGIN);
    }

    result = ReadFile(file, buffer, bytestoread, bytesread, NULL);
    *currentposition = position + *bytesread;
    if (result) {
      break;
    }
    error = GetLastError();

    {
      Storm::SFile::UseGlob glob;
      mode = glob->s_ioerrormode;
      retryProc = glob->s_ioerrorproc;
    }

    switch (mode) {
      case SFILE_ERRORMODE_RETURNCODE:
        return FALSE;
      case SFILE_ERRORMODE_CUSTOM:
        if (!retryProc || !retryProc(filename, error, numcalls++)) {
          return FALSE;
        }
        break;
      case SFILE_ERRORMODE_FATAL:
        SErrDisplayError(error, filename, -4, NULL, FALSE, 1);
        break;
    }
  }

  if (*bytesread == bytestoread) {
    return TRUE;
  }
  SErrSetLastError(ERROR_HANDLE_EOF);
  return FALSE;
}

static BOOL ReadFileWin32(SFileRecData *fileptr, DWORD offset, LPVOID buffer, DWORD bytestoread, DWORD *bytesread) {
  DWORD localbytesread;
  DWORD newlocation;

  localbytesread = 0;
  newlocation = bytestoread;
  ReadFileChecked(offset, &newlocation, (HANDLE)fileptr->handle, buffer, bytestoread, &localbytesread, fileptr->name);
  if (bytesread) {
    *bytesread = localbytesread;
  }
  return localbytesread == bytestoread;
}

static SFileBlockEntryData *GetBlockEntry(SFileArchiveRecData *archive, DWORD index) {
  return &archive->blocktable[archive->hashtable[index].block];
}

static DWORD SearchHashTable(SFileArchiveRecData *archive, LPCSTR filename, WORD languageId, BYTE platformId) {
  SFileHashEntryData *table;
  SFileHashEntryData *entry;
  DWORD               mask;
  DWORD               firstindex;
  DWORD               index;
  DWORD               hashIndex;
  DWORD               hashcheck0;
  DWORD               hashcheck1;
  DWORD               found;

  hashIndex = Hash(filename, HASH_INDEX);
  hashcheck0 = Hash(filename, HASH_CHECK0);
  hashcheck1 = Hash(filename, HASH_CHECK1);
  table = archive->hashtable;
  mask = archive->header.hashcount - 1;
  index = hashIndex & mask;
  firstindex = index;
  found = NOBLOCK;

  for (;;) {
    entry = &table[index];
    if (entry->block == NOBLOCK) {
      return found;
    }
    if (entry->hashcheck[0] == hashcheck0 && entry->hashcheck[1] == hashcheck1 && entry->block != DEALLOCATED) {
      if (entry->languageId == languageId && entry->platformId == platformId) {
        return index;
      }
      if ((entry->languageId == 0 || entry->languageId == languageId) && (entry->platformId == 0 || entry->platformId == platformId)) {
        found = index;
      }
    }
    index = (index + 1) & mask;
    if (index == firstindex) {
      return found;
    }
  }
}

static void BlockEntryFileToMem(SFileBlockEntryData *pBlockTbl, DWORD dwBlockTblEntries) {
  DWORD i;

  if (dwBlockTblEntries > 1) {
    for (i = dwBlockTblEntries - 1; i != 0; --i) {
      memmove(&pBlockTbl[i], ((BYTE *)pBlockTbl) + (i * 0x10), 0x10);
      pBlockTbl[i].time.dwLowDateTime = 0;
      pBlockTbl[i].time.dwHighDateTime = 0;
      pBlockTbl[i].crc = 0;
      memset(&pBlockTbl[i].md5, 0, sizeof(pBlockTbl[i].md5));
    }
  }
  if (dwBlockTblEntries) {
    pBlockTbl[0].time.dwLowDateTime = 0;
    pBlockTbl[0].time.dwHighDateTime = 0;
    pBlockTbl[0].crc = 0;
    memset(&pBlockTbl[0].md5, 0, sizeof(pBlockTbl[0].md5));
  }
}

static DWORD InternalReadAligned(SFileRecData *file, DWORD location, LPVOID buffer, DWORD bytes);
static void  Initialize();

static int ReadAdditionalAttributes(HSARCHIVE archive, SFileBlockEntryData *pBlockTbl, DWORD dwBlockTblEntries) {
  HSFILE hfile;
  DWORD *pBuf;
  BYTE  *cursor;
  DWORD  flags;
  DWORD  expectedSize;
  DWORD  i;
  int    ret;

  ret = FALSE;
  if (!SFileOpenFileEx(archive, "(attributes)", 0, &hfile)) {
    return FALSE;
  }
  pBuf = (DWORD *)ALLOC(SFileGetFileSize(hfile, NULL));
  if (SFileReadFile(hfile, pBuf, SFileGetFileSize(hfile, NULL), NULL, NULL) && SFileGetFileSize(hfile, NULL) >= 8) {
    flags = pBuf[1];
    cursor = (BYTE *)(pBuf + 2);
    expectedSize = 8;
    if (flags & 1) {
      expectedSize += dwBlockTblEntries * 4;
    }
    if (flags & 2) {
      expectedSize += dwBlockTblEntries * 8;
    }
    if (flags & 4) {
      expectedSize += dwBlockTblEntries * 16;
    }

    if (SFileGetFileSize(hfile, NULL) == expectedSize) {
      if (flags & 1) {
        for (i = 0; i < dwBlockTblEntries; ++i) {
          pBlockTbl[i].crc = *(DWORD *)cursor;
          cursor += 4;
        }
      }
      if (flags & 2) {
        for (i = 0; i < dwBlockTblEntries; ++i) {
          pBlockTbl[i].time.dwLowDateTime = *(DWORD *)cursor;
          pBlockTbl[i].time.dwHighDateTime = *(DWORD *)(cursor + 4);
          cursor += 8;
        }
      }
      if (flags & 4) {
        for (i = 0; i < dwBlockTblEntries; ++i) {
          memcpy(&pBlockTbl[i].md5, cursor, sizeof(pBlockTbl[i].md5));
          cursor += 16;
        }
      }
      ret = TRUE;
    }
  }
  FREE(pBuf);

  SFileCloseFile(hfile);
  return ret;
}

void Storm::SFile::AddArchiveRef(ARCHIVEREC *archive) {
  if (archive) {
    archive->refcount++;
  }
}

void Storm::SFile::AddFileRef(FILEREC *file) {
  if (file) {
    file->refcount++;
  }
}

void Storm::SFile::AddStreamRef(AUDIOSTREAM *stream) {
  if (stream) {
    stream->refcount++;
  }
}

static void              FillSoundBuffer(SFileRequestData *request);
static SFileRequestData *IssueRequest(
    SFileRecData         *file,
    DWORD                 offset,
    LPVOID                buffer,
    LPVOID                bufferbegin,
    IDirectSoundBuffer   *soundbuffer,
    DWORD                 soundbufferoffset,
    SFileAudioStreamData *stream,
    DWORD                 bytestoread,
    DWORD                 requiredms,
    int                   urgent,
    LPVOID                event,
    int                   autodelrequest,
    int                   triggerreadthread,
    _TASYNCPARAMBLOCK    *asyncparam
);

void Storm::SFile::RemoveArchiveRef(ARCHIVEREC *archive) {
  if (archive) {
    archive->refcount--;
    if (archive->refcount == 0) {
      delete archive;
    }
  }
}

void Storm::SFile::RemoveFileRef(FILEREC *file) {
  if (file) {
    file->refcount--;
    if (file->refcount == 0) {
      Storm::SFile::s_filelist.UnlinkNode(file);
      delete file;
    }
  }
}

void Storm::SFile::RemoveStreamRef(AUDIOSTREAM *stream) {
  if (stream) {
    stream->refcount--;
    if (stream->refcount == 0) {
      delete stream;
    }
  }
}

static int CancelRequest(LPVOID buffer, IDirectSoundBuffer *soundbuffer) {
  int cancelled;

  if (!buffer && !soundbuffer) {
    return FALSE;
  }

  cancelled = FALSE;

  Storm::SFile::s_cdlock.Enter();
  ITERATELIST(SFileRequestData, Storm::SFile::s_cdreqlist, request) {
    if ((buffer && request->bufferbegin == buffer) || (soundbuffer && request->soundbuffer == soundbuffer)) {
      if (request->event) {
        SFileEventRecData *eventrec = Storm::SFile::s_signalList.NewNode(LIST_TAIL, 0, 0);
        eventrec->event = request->event;
      }
      cancelled = TRUE;
      ITERATE_DELETE;
    }
  }
  Storm::SFile::s_cdlock.Leave();

  return cancelled;
}

static BOOL CanProcessRequest(SFileRequestData *request) {
  if (request->bufferbegin) {
    if (request->Prev() && request->Prev()->bufferbegin == request->bufferbegin) {
      return FALSE;
    }
  }
  return TRUE;
}

static DWORD UpdateAudioStreamPos(SFileAudioStreamData *curr) {
  DWORD playpos;
  DWORD writepos;
  DWORD ringOffset;
  DWORD distance;
  DWORD position;

  if (curr->fillstatus != 2 && curr->fillstatus != 3) {
    SErrDisplayError(STORM_ERROR_ASSERTION, __FILE__, __LINE__, "curr->fillstatus == 2 || curr->fillstatus == 3", TRUE, 1);
  }

  playpos = 0;
  writepos = 0;
  if ((*(SFileDirectSoundBufferVtbl **)curr->soundbuffer)->GetCurrentPosition(curr->soundbuffer, &playpos, &writepos) < 0) {
    return 0xFFFFFFFF;
  }

  ringOffset = curr->writecursor % curr->soundbuffersize;
  distance = ringOffset - playpos;
  if ((LONG)distance <= 0) {
    distance += curr->soundbuffersize;
  }

  position = curr->writecursor - distance;
  if (position >= curr->wavedatasize) {
    position = curr->wavedatasize;
  }
  curr->playcursor = position;
  return playpos;
}

static void CheckAudioStreams(int &parent_header) {
  DWORD  timeoffset;
  LPVOID buffer;
  DWORD  bytes;
  DWORD  maxoffset;
  DWORD  WAVECHUNKSIZE;

  (void)parent_header;
  if (Storm::SFile::s_closingDirectSound || Storm::SFile::s_streamlist.IsEmpty()) {
    return;
  }

  {
    Storm::SFile::UseGlob glob;
    WAVECHUNKSIZE = glob->WAVECHUNKSIZE;
  }
  Storm::SFile::s_streamlock.Enter();
  ITERATELIST(SFileAudioStreamData, Storm::SFile::s_streamlist, stream) {
    {
      Storm::SFile::FilePtrLocked    fileptr((HSFILE)stream->file);
      SFileRecData                  *file = fileptr.operator->();
      Storm::SFile::ArchivePtrLocked archiveptr((HSARCHIVE)file->archive);

      if (stream->fillstatus == 3 || (stream->fillstatus == 2 && file->location >= stream->waveheadersize + stream->wavedatasize)) {
        if (stream->loop) {
          file->location = stream->waveheadersize;
          stream->writecursor = 0;
          stream->bytespastend = 0;
        } else {
          DWORD playpos = UpdateAudioStreamPos(stream);
          if (playpos != 0xFFFFFFFF) {
            if (stream->playcursor >= stream->wavedatasize) {
              (*(SFileDirectSoundBufferVtbl **)stream->soundbuffer)->Stop(stream->soundbuffer);
              stream->fillstatus = 4;
            } else {
              DWORD soundoffset = stream->nextwrite;
              if ((playpos - soundoffset) % stream->soundbuffersize >= WAVECHUNKSIZE) {
                DWORD endbytes = stream->soundbuffersize - soundoffset;
                if (endbytes < WAVECHUNKSIZE) {
                  SErrDisplayError(STORM_ERROR_ASSERTION, __FILE__, __LINE__, "curr->soundbuffersize - curr->nextwritepos >= WAVECHUNKSIZE", TRUE, 1);
                }
                bytes = WAVECHUNKSIZE;
                if (bytes > endbytes) {
                  bytes = endbytes;
                }
                if (bytes != WAVECHUNKSIZE) {
                  SErrDisplayError(STORM_ERROR_ASSERTION, __FILE__, __LINE__, "bytes == WAVECHUNKSIZE", TRUE, 1);
                }

                buffer = NULL;
                if ((*(SFileDirectSoundBufferVtbl **)stream->soundbuffer)
                        ->Lock(stream->soundbuffer, soundoffset, bytes, &buffer, &bytes, NULL, NULL, 0) >= 0)
                {
                  if (buffer && bytes) {
                    memset(buffer, stream->fillvalue, bytes);
                  }
                  (*(SFileDirectSoundBufferVtbl **)stream->soundbuffer)->Unlock(stream->soundbuffer, buffer, bytes, NULL, 0);
                  stream->writecursor += bytes;
                  stream->bytespastend += bytes;
                  stream->nextwrite += bytes;
                  if (stream->nextwrite >= stream->soundbuffersize) {
                    stream->nextwrite -= stream->soundbuffersize;
                  }
                }
              }
              stream->fillstatus = 3;
            }
          }
        }
      }

      if (stream->fillstatus == 2) {
        DWORD playpos = UpdateAudioStreamPos(stream);
        if (playpos != 0xFFFFFFFF && (playpos - stream->nextwrite) % stream->soundbuffersize >= WAVECHUNKSIZE) {
          bytes = stream->soundbuffersize - stream->nextwrite;
          if (bytes < WAVECHUNKSIZE) {
            SErrDisplayError(STORM_ERROR_ASSERTION, __FILE__, __LINE__, "curr->soundbuffersize - curr->nextwritepos >= WAVECHUNKSIZE", TRUE, 1);
          }
          if (bytes > WAVECHUNKSIZE) {
            bytes = WAVECHUNKSIZE;
          }
          DWORD remaining = stream->waveheadersize + stream->wavedatasize - file->location;
          if (bytes > remaining) {
            bytes = remaining;
          }
          if (g_opt.alignstreamingwavedata) {
            DWORD remainder = (file->archive->sectorsize - 1) & (file->location + bytes);
            if (remainder && remainder < bytes) {
              bytes -= remainder;
            }
          }
          IssueRequest(file, file->location, NULL, NULL, stream->soundbuffer, stream->nextwrite, stream, bytes, 0, 0, NULL, 1, 0, NULL);
          file->location += bytes;
          stream->nextwrite += WAVECHUNKSIZE;
          if (stream->nextwrite >= stream->soundbuffersize) {
            stream->nextwrite -= stream->soundbuffersize;
          }
        }
      } else if (stream->fillstatus == 0) {
        maxoffset = stream->wavedatasize - stream->startingoffset;
        if (maxoffset > stream->soundbuffersize) {
          maxoffset = stream->soundbuffersize;
        }
        file->location = stream->waveheadersize + stream->startingoffset;

        DWORD offset = 0;
        timeoffset = 0;
        if (g_opt.alignstreamingwavedata) {
          bytes = WAVECHUNKSIZE;
          if (bytes > maxoffset) {
            bytes = maxoffset;
          }
          DWORD remainder = (file->archive->sectorsize - 1) & (file->location + bytes);
          if (remainder && remainder < bytes) {
            bytes -= remainder;
          }
          IssueRequest(file, file->location, NULL, NULL, stream->soundbuffer, 0, stream, bytes, 0, 0, NULL, 1, 0, NULL);
          file->location += bytes;
          offset = bytes;
          timeoffset = 1;
        }

        while (offset < maxoffset) {
          bytes = maxoffset - offset;
          if (bytes > WAVECHUNKSIZE) {
            bytes = WAVECHUNKSIZE;
          }
          IssueRequest(file, file->location, NULL, NULL, stream->soundbuffer, offset, stream, bytes, timeoffset, 0, NULL, 1, 0, NULL);
          file->location += bytes;
          timeoffset++;
          offset += WAVECHUNKSIZE;
        }

        stream->fillstatus = 1;
        stream->nextwrite = offset % stream->soundbuffersize;
      }
    }
  }
  Storm::SFile::s_streamlock.Leave();
}

static void CheckRequests(
    SFileArchiveRecData *lastarchive,
    DWORD                lastarchivelocation,
    SFileRequestData   **nextreq,
    SFileRequestData   **urgentreq,
    LONG                *lowcompletetime
) {
  DWORD                s_seekOptimize;
  DWORD                currtime;
  DWORD                bestDistance;
  DWORD                distance;
  DWORD                archiveBase;
  SFileArchiveRecData *requestArchive;
  LONG                 timeout;

  *nextreq = NULL;
  *urgentreq = NULL;
  *lowcompletetime = 0x7FFFFFFF;

  {
    Storm::SFile::UseGlob glob;
    s_seekOptimize = glob->s_seekOptimize;
  }

  bestDistance = 0xFFFFFFFF;
  Storm::SFile::s_cdlock.Enter();
  currtime = GetTickCount();
  ITERATELIST(SFileRequestData, Storm::SFile::s_cdreqlist, request) {
    if (CanProcessRequest(request)) {
      distance = 0;
      if (s_seekOptimize && !request->stream) {
        requestArchive = NULL;
        archiveBase = 0;
        if (request->file) {
          requestArchive = request->file->archive;
          if (requestArchive && lastarchivelocation > requestArchive->sectorsize) {
            archiveBase = lastarchivelocation - requestArchive->sectorsize;
          }
        }
        if (requestArchive == lastarchive) {
          distance = request->approxarchivelocation;
          if (distance < archiveBase) {
            distance += 0x08000000;
          } else {
            distance -= archiveBase;
          }
        } else {
          distance = 0xFFFFFFFE;
        }
      }

      if (distance <= bestDistance) {
        bestDistance = distance;
        *nextreq = request;
      }

      timeout = (LONG)(request->requiredcompletiontime - currtime);
      if (request->urgent && timeout >= 1000) {
        timeout = 1000;
      }
      if (timeout < *lowcompletetime) {
        *lowcompletetime = timeout;
        *urgentreq = request;
      }
    }
  }
  Storm::SFile::s_cdlock.Leave();
}

static DWORD WINAPI CdThreadProc(LPVOID) {
  typedef void(APIENTRY * SFileAsyncNotifyProc)(LPVOID buffer, DWORD completedOffset, _TASYNCPARAMBLOCK * asyncparam);

  LONG                 lowcompletetime;
  SFileRequestData    *urgentreq;
  DWORD                s_dataChunkSize;
  SFileAsyncNotifyProc asyncproc;
  SFileRequestData    *nextreq;
  DWORD                lastarchivelocation;
  SFileArchiveRecData *lastarchive;
  int                  audio_header;
  DWORD                lastReadTime;
  DWORD                currtime;
  DWORD                budgetInterval;
  DWORD                s_asyncBudget;
  SFileEventRecData   *eventrec;
  LPVOID               event;

  lastarchive = NULL;
  lastarchivelocation = 0;
  lastReadTime = GetTickCount();
  audio_header = 0;
  SErrRegisterThread((HANDLE)Storm::SFile::s_cdthread, Storm::SFile::s_cdthreadid);

  while (!Storm::SFile::s_cdshutdown) {
    {
      Storm::SFile::UseGlob glob;
      s_asyncBudget = glob->s_asyncBudget;
      s_dataChunkSize = glob->s_dataChunkSize;
    }
    currtime = GetTickCount();
    budgetInterval = 0;
    if (s_asyncBudget) {
      budgetInterval = (s_dataChunkSize * 1000) / s_asyncBudget;
    }

    Storm::SFile::s_cdlock.Enter();
    eventrec = Storm::SFile::s_signalList.Head();
    while (eventrec) {
      if (eventrec->event) {
        SetEvent((HANDLE)eventrec->event);
      }
      eventrec = Storm::SFile::s_signalList.DeleteNode(eventrec);
    }
    Storm::SFile::s_cdlock.Leave();

    audio_header = 0;
    CheckAudioStreams(audio_header);
    CheckRequests(lastarchive, lastarchivelocation, &nextreq, &urgentreq, &lowcompletetime);

    Storm::SFile::s_cdlock.Enter();
    if (urgentreq && lowcompletetime <= 1000) {
      nextreq = urgentreq;
    } else if (!nextreq || currtime - lastReadTime < budgetInterval) {
      Storm::SFile::s_cdlock.Leave();
      LONG sleeptime;
      if (nextreq) {
        sleeptime = (LONG)(budgetInterval - (currtime - lastReadTime));
        if ((DWORD)sleeptime > 5000) {
          SErrDisplayError(STORM_ERROR_ASSERTION, __FILE__, __LINE__, "sleeptime <= 5000", TRUE, 1);
        }
      } else {
        sleeptime = 250;
      }
      if (Storm::SFile::s_cdevent) {
        WaitForSingleObject((HANDLE)Storm::SFile::s_cdevent, sleeptime);
      } else {
        Sleep(sleeptime);
      }
      continue;
    }

    Storm::SFile::s_cdrequest = nextreq;
    Storm::SFile::s_cdreqlist.UnlinkNode(nextreq);
    Storm::SFile::s_cdlock.Leave();

    {
      Storm::SFile::FilePtrLocked    fileptr((HSFILE)nextreq->file);
      SFileRecData                  *file = fileptr.operator->();
      Storm::SFile::ArchivePtrLocked archiveptr((HSARCHIVE)file->archive);

      lastarchive = file->archive;
      lastarchivelocation = nextreq->location;
      lastReadTime = currtime;

      BYTE *buffer = nextreq->stream ? (BYTE *)Storm::SFile::s_soundreadbuffer : (BYTE *)nextreq->buffer;
      if (buffer) {
        if (file->handle == INVALID_HANDLE_VALUE) {
          nextreq->bytesread = InternalReadUnaligned(file, nextreq->location, buffer, nextreq->bytestoread);
          if (file->crcavail && file->crcstate == 2 && file->crcexpected == file->block.sizefile) {
            CrcBuffer(NULL, 0, &file->crc, 4);
            file->crcstate = 4;
          }
        } else {
          ReadFileWin32(file, nextreq->location, buffer, nextreq->bytestoread, &nextreq->bytesread);
        }
      } else {
        nextreq->bytesread = 0;
      }

      if (nextreq->soundbuffer) {
        if (nextreq->bytesread) {
          FillSoundBuffer(nextreq);
        } else if (nextreq->bytestoread) {
          (*(SFileDirectSoundBufferVtbl **)nextreq->soundbuffer)->Stop(nextreq->soundbuffer);
          if (nextreq->stream) {
            ((SFileAudioStreamData *)nextreq->stream)->fillstatus = 4;
          }
        }
      }

      if (nextreq->asyncparam && *(LPVOID *)nextreq->asyncparam) {
        if (file->crcavail && file->crcstate == 4 && file->crc != file->block.crc) {
          SErrDisplayError(0x85100083, file->name, -4, NULL, FALSE, 1);
        }
        asyncproc = (SFileAsyncNotifyProc) * (LPVOID *)nextreq->asyncparam;
        DWORD completedOffset = (DWORD)((BYTE *)nextreq->buffer + nextreq->bytesread - (BYTE *)nextreq->bufferbegin);
        archiveptr.Leave();
        fileptr.Leave();
        asyncproc(nextreq->bufferbegin, completedOffset, nextreq->asyncparam);
      }

      archiveptr.Leave();
      fileptr.Leave();

      event = nextreq->event;
      Storm::SFile::s_cdlock.Enter();
      if (nextreq->autodelrequest) {
        delete nextreq;
      }
      Storm::SFile::s_cdrequest = NULL;
      Storm::SFile::s_cdlock.Leave();
      if (event) {
        SetEvent((HANDLE)event);
      }
    }
  }

  SErrUnregisterThread((HANDLE)Storm::SFile::s_cdthread, Storm::SFile::s_cdthreadid);
  Storm::SFile::s_cdshutdown = FALSE;
  return 0;
}

static void CreateCdThread() {
  HANDLE thread;
  HANDLE event;

  if (Storm::SFile::s_cdthread) {
    return;
  }

  event = CreateEventA(NULL, FALSE, FALSE, NULL);
  Storm::SFile::s_cdevent = event;
  thread = CreateThread(NULL, 0, CdThreadProc, NULL, 0, &Storm::SFile::s_cdthreadid);
  Storm::SFile::s_cdthread = thread;
  SetThreadPriority(thread, THREAD_PRIORITY_ABOVE_NORMAL);
}

static void DestroyCdThread() {
  HANDLE thread;
  HANDLE event;

  thread = (HANDLE)Storm::SFile::s_cdthread;
  if (thread) {
    Storm::SFile::s_cdshutdown = 1;
    event = (HANDLE)Storm::SFile::s_cdevent;
    if (event) {
      SetEvent(event);
    }
    WaitForSingleObject(thread, 1000);
    thread = (HANDLE)Storm::SFile::s_cdthread;
    if (thread) {
      CloseHandle(thread);
      Storm::SFile::s_cdthread = NULL;
    }
    event = (HANDLE)Storm::SFile::s_cdevent;
    if (event) {
      CloseHandle(event);
      Storm::SFile::s_cdevent = NULL;
    }
  }
}

void StormOptCdThread(DWORD *threadId, LPVOID *hThread) {
  CreateCdThread();
  *threadId = Storm::SFile::s_cdthreadid;
  *hThread = Storm::SFile::s_cdthread;
}

static void FillSoundBuffer(SFileRequestData *request) {
  DWORD                 WAVECHUNKSIZE;
  DWORD                 locksize;
  DWORD                 copyBytes;
  SFileAudioStreamData *stream;

  {
    Storm::SFile::UseGlob glob;
    WAVECHUNKSIZE = glob->WAVECHUNKSIZE;
  }
  stream = (SFileAudioStreamData *)request->stream;
  if (stream->soundbuffersize - request->soundbufferoffset < WAVECHUNKSIZE) {
    SErrDisplayError(
        STORM_ERROR_ASSERTION, __FILE__, __LINE__, "request->stream->soundbuffersize - request->soundbufferoffset >= WAVECHUNKSIZE", TRUE, 1
    );
  }

  locksize = 0;
  if ((*(SFileDirectSoundBufferVtbl **)request->soundbuffer)
              ->Lock(request->soundbuffer, request->soundbufferoffset, WAVECHUNKSIZE, &request->buffer, &locksize, NULL, NULL, 0) < 0 ||
      locksize != WAVECHUNKSIZE)
  {
    return;
  }

  copyBytes = request->bytesread;
  if (copyBytes > locksize) {
    copyBytes = locksize;
  }

  if (copyBytes && Storm::SFile::s_soundreadbuffer && request->buffer) {
    memcpy(request->buffer, Storm::SFile::s_soundreadbuffer, copyBytes);
  }

  if (copyBytes < locksize && request->buffer) {
    memset((BYTE *)request->buffer + copyBytes, stream->fillvalue, locksize - copyBytes);
  }

  (*(SFileDirectSoundBufferVtbl **)request->soundbuffer)->Unlock(request->soundbuffer, request->buffer, locksize, NULL, 0);

  if (request->location - stream->waveheadersize && stream->writecursor > request->location - stream->waveheadersize + locksize) {
    SErrDisplayError(
        STORM_ERROR_ASSERTION, __FILE__, __LINE__, "request->location - request->stream->dataoffset + bytes <= request->stream->bytesbuffered", TRUE,
        1
    );
  }

  stream->writecursor = request->location - stream->waveheadersize + locksize;
  if (stream->writecursor > stream->wavedatasize) {
    stream->bytespastend = stream->writecursor - stream->wavedatasize;
  }

  if (stream->fillstatus == 1) {
    DWORD playthreshold = stream->wavedatasize;
    if (playthreshold > 0x10000) {
      playthreshold = 0x10000;
    }
    if (playthreshold > stream->soundbuffersize) {
      playthreshold = stream->soundbuffersize;
    }

    if (request->location - stream->waveheadersize + request->bytestoread >= playthreshold) {
      (*(SFileDirectSoundBufferVtbl **)stream->soundbuffer)->Play(stream->soundbuffer, 0, 0, 1);
      stream->fillstatus = 2;
    }
  }
}

static SFileRequestData *IssueRequest(
    SFileRecData         *file,
    DWORD                 offset,
    LPVOID                buffer,
    LPVOID                bufferbegin,
    IDirectSoundBuffer   *soundbuffer,
    DWORD                 soundbufferoffset,
    SFileAudioStreamData *stream,
    DWORD                 bytestoread,
    DWORD                 requiredms,
    int                   urgent,
    LPVOID                event,
    int                   autodelrequest,
    int                   triggerreadthread,
    _TASYNCPARAMBLOCK    *asyncparam
) {
  SFileRequestData *request;

  FATALASSERT(buffer || soundbuffer);

  Storm::SFile::s_cdlock.Enter();
  request = Storm::SFile::s_cdreqlist.NewNode(LIST_UNLINKED, 0, 0);
  Storm::SFile::s_cdlock.Leave();

  request->event = event;
  request->file = file;
  request->location = offset;
  if (file->handle == INVALID_HANDLE_VALUE) {
    offset = file->block.offset + (offset >> 1);
  }
  request->approxarchivelocation = offset;
  request->requiredcompletiontime = GetTickCount() + requiredms;
  request->urgent = urgent;
  request->buffer = buffer;
  request->bufferbegin = bufferbegin;
  request->soundbuffer = soundbuffer;
  request->soundbufferoffset = soundbufferoffset;
  request->bytestoread = bytestoread;
  request->stream = stream;
  request->autodelrequest = autodelrequest;
  request->asyncparam = asyncparam;

  Storm::SFile::AddStreamRef(stream);
  Storm::SFile::s_filelock.Enter();
  Storm::SFile::AddFileRef(file);
  Storm::SFile::s_filelock.Leave();

  Storm::SFile::s_cdlock.Enter();
  Storm::SFile::s_cdreqlist.LinkNode(request, LIST_TAIL, NULL);
  Storm::SFile::s_cdlock.Leave();

  if (triggerreadthread && Storm::SFile::s_cdevent) {
    SetEvent((HANDLE)Storm::SFile::s_cdevent);
  }

  return request;
}

static void MarkRequestUrgent(LPVOID buffer, int urgent) {
  SFileRequestList *requests;

  if (!buffer) {
    return;
  }

  requests = &Storm::SFile::s_cdreqlist;
  Storm::SFile::s_cdlock.Enter();
  ITERATELISTPTR(SFileRequestData, requests, request) {
    if (request->bufferbegin == buffer) {
      request->urgent = urgent;
    }
  }
  Storm::SFile::s_cdlock.Leave();
}

static void BuildDefaultBasePath(char *basePath, DWORD maxPath) {
  char *slash;

  GetModuleFileNameA(GetModuleHandleA(NULL), basePath, maxPath);
  slash = SStrChrR(basePath, '\\');
  if (slash) {
    *slash = 0;
  }
  SStrPack(basePath, "\\", maxPath);
}

static DWORD BuildDefaultOpenFlags() {
  DWORD                 flags;
  Storm::SFile::UseGlob glob;

  flags = 0;
  if (glob->s_directaccess & 1) {
    flags |= 1;
  }
  if (glob->s_directaccess & 2) {
    flags |= 2;
  }
  if (!glob->s_directaccess && Storm::SFile::s_archivelist.IsEmpty()) {
    flags |= 1;
  }
  return flags;
}

static DWORD
GetFileBlockEntry(HSARCHIVE archivehandle, LPCSTR filename, DWORD flags, SFileArchiveRecData **archive, SFileBlockEntryData **block, char *diskname) {
  char                 localfilename[MAX_PATH];
  char                 mungedname[MAX_PATH];
  UINT                 s_platformId;
  WORD                 s_languageId;
  DWORD                exists;
  SFileArchiveRecData *archiveNode;
  SFileArchiveRecData *searchArchive;
  SFileBlockEntryData *blockentry;
  LPCSTR               searchName;
  DWORD                index;

  if (archive) {
    *archive = NULL;
  }
  if (block) {
    *block = NULL;
  }

  {
    Storm::SFile::UseGlob glob;
    s_languageId = glob->s_languageId;
    s_platformId = glob->s_platformId;
  }

  Storm::SFile::ArchivePtr baseArchive(archivehandle);
  Storm::SFile::s_archivelock.Enter();
  exists = 0;
  if (!archivehandle || baseArchive) {
    archiveNode = Storm::SFile::s_archivelist.Head();
    for (;;) {
      if (!baseArchive || archiveNode == baseArchive) {
        if (!archiveNode || archiveNode->disableCount <= 0) {
          searchArchive = archiveNode;
          if (archiveNode && archiveNode->parentArchive) {
            searchArchive = (SFileArchiveRecData *)archiveNode->parentArchive;
          }

          searchName = filename;
          if (archiveNode && archiveNode->pathPrefix[0]) {
            SStrCopy(mungedname, archiveNode->pathPrefix, MAX_PATH);
            SStrPack(mungedname, "\\", MAX_PATH);
            SStrPack(mungedname, filename, MAX_PATH);
            searchName = mungedname;
          }

          if ((flags & 3) && (!searchArchive || !searchArchive->dontCheckDisk)) {
            if (CheckFileExistsOnDisk(searchName, flags, localfilename)) {
              if (diskname) {
                SStrCopy(diskname, localfilename, MAX_PATH);
              }
              exists = 2;
              break;
            }
          }

          if (archiveNode) {
            index = SearchHashTable(searchArchive, searchName, s_languageId, (BYTE)s_platformId);
            if (index != 0xFFFFFFFF) {
              blockentry = GetBlockEntry(searchArchive, index);
              exists = 1;
              if ((LONG)blockentry->flags >= 0 || ((flags & 4) && (blockentry->flags & 0x1FF00))) {
                SErrSetLastError(0x3EE);
                exists = 0;
                break;
              }
              if (archive) {
                Storm::SFile::AddArchiveRef(searchArchive);
                *archive = searchArchive;
              }
              if (block) {
                *block = blockentry;
              }
              break;
            }
          }
        }
      }

      if (!archiveNode) {
        SErrSetLastError(ERROR_FILE_NOT_FOUND);
        break;
      }
      archiveNode = archiveNode->Next();
    }
  }

  Storm::SFile::s_archivelock.Leave();
  return exists;
}

static int CheckFileExists(LPCSTR filename) {
  return !(GetFileAttributesA(filename) & FILE_ATTRIBUTE_DIRECTORY);
}

static int CheckForCdRom(LPCSTR path) {
  char  rootpath[4];
  char  fsname[MAX_PATH];
  DWORD driveType;
  DWORD fsflags;
  DWORD sectorspercluster;
  DWORD bytespersector;
  DWORD freeclusters;
  DWORD totalclusters;
  DWORD value;

  SStrCopy(rootpath, path, sizeof(rootpath));
  driveType = GetDriveTypeA(rootpath);
  memset(fsname, 0, sizeof(fsname));
  fsflags = 0;
  if (!GetVolumeInformationA(rootpath, NULL, 0, NULL, NULL, &fsflags, fsname, sizeof(fsname))) {
    return FALSE;
  }

  sectorspercluster = 0;
  bytespersector = 0;
  freeclusters = 0;
  totalclusters = 0;
  if (!GetDiskFreeSpaceA(rootpath, &sectorspercluster, &bytespersector, &freeclusters, &totalclusters)) {
    return FALSE;
  }

  value = (fsflags & 4) ^ *(DWORD *)fsname ^ bytespersector ^ sectorspercluster ^ driveType;
  value = ((value >> 16) ^ value) & 0xFFFF;
  return value == 0x1F00 || value == 0x0805;
}

static void ConvertRelativePathName(LPCSTR inputpath, char *outputpath, int strippath) {
  char                  absolutepath[MAX_PATH];
  Storm::SFile::UseGlob glob;

  if (!glob->s_basepath[0]) {
    BuildDefaultBasePath(glob->s_basepath, MAX_PATH);
  }
  if (strippath) {
    LPCSTR slash = SStrChrR(inputpath, '\\');
    if (slash) {
      inputpath = slash + 1;
    }
  }
  wsprintfA(absolutepath, "%s%s", glob->s_basepath, inputpath);
  _fullpath(outputpath, absolutepath, MAX_PATH);
}

static int CheckFileExistsOnDisk(LPCSTR filename, DWORD flags, char *localfilename) {
  char localonly[MAX_PATH];
  int  result = FALSE;

  if (!localfilename) {
    localfilename = localonly;
  }

  if (flags & 3) {
    if (flags & 2) {
      ConvertRelativePathName(filename, localfilename, TRUE);
      result = CheckFileExists(localfilename);
      if (result) {
        goto done;
      }
    }
    if (flags & 1) {
      if (filename[0] == '\\' || strstr(filename, ":\\") || strstr(filename, "\\\\")) {
        SStrCopy(localfilename, filename, MAX_PATH);
      } else {
        ConvertRelativePathName(filename, localfilename, FALSE);
      }
      result = CheckFileExists(localfilename);
    }
  }

done:
  return result;
}

static int FindChunk(HSFILE handle, DWORD ckid, CKINFO *pck) {
  DWORD ckhdr[2];

  for (;;) {
    if (!SFileReadFile(handle, ckhdr, sizeof(ckhdr), NULL, NULL)) {
      return FALSE;
    }
    if (ckhdr[0] == ckid) {
      pck->size = ckhdr[1];
      pck->offset = SFileSetFilePointer(handle, 0, NULL, FILE_CURRENT);
      return pck->offset != 0xFFFFFFFF;
    }
    if (SFileSetFilePointer(handle, (LONG)ckhdr[1], NULL, FILE_CURRENT) == 0xFFFFFFFF) {
      return FALSE;
    }
  }
}

static void Initialize() {
  if (!Storm::SFile::s_hashsource) {
    InitializeHashSource(0x100001);
  }
}

extern "C" BOOL APIENTRY SFileAuthenticateArchive(HSARCHIVE handle, DWORD *extendedresult) {
  if (SFileAuthenticateArchiveEx(handle, extendedresult, NULL, 0, NULL, 0)) {
    return TRUE;
  }

  SFileBlockEntryData *block;
  SFileCryptoApi      *crypto;
  HMODULE              library;
  HANDLE               mapping;
  LPVOID               view;
  HRSRC                resource;
  HGLOBAL              resourceHandle;
  LPVOID               resourceData;
  DWORD                provider;
  DWORD                key;
  DWORD                hash;
  DWORD                companyId;
  DWORD                signatureIndex;
  DWORD                signatureEnd;
  BYTE                *zeros;
  BOOL                 hashResult;
  BOOL                 result;

  if (extendedresult) {
    *extendedresult = SFILE_AUTH_UNABLETOAUTHENTICATE;
  }

  Storm::SFile::ArchivePtr ptr(handle);
  if (!ptr) {
    return FALSE;
  }

  result = FALSE;
  signatureIndex = SearchHashTable(ptr, SIGNATUREFILE, 0, 0);
  if (signatureIndex == NOBLOCK) {
    if (extendedresult) {
      *extendedresult = SFILE_AUTH_NOSIGNATURE;
    }
    SErrSetLastError(0x4DC);
    return FALSE;
  }

  crypto = NULL;
  mapping = NULL;
  view = NULL;
  provider = 0;
  key = 0;
  hash = 0;

  library = LoadLibraryA("advapi32.dll");
  if (!library) {
    goto cleanup;
  }

  crypto = (SFileCryptoApi *)ALLOC(sizeof(SFileCryptoApi));

  crypto->acquireContext = (SFileCryptAcquireContextAProc)GetProcAddress(library, "CryptAcquireContextA");
  if (!crypto->acquireContext) {
    goto cleanup;
  }
  crypto->createHash = (SFileCryptCreateHashProc)GetProcAddress(library, "CryptCreateHash");
  if (!crypto->createHash) {
    goto cleanup;
  }
  crypto->destroyHash = (SFileCryptDestroyHashProc)GetProcAddress(library, "CryptDestroyHash");
  if (!crypto->destroyHash) {
    goto cleanup;
  }
  crypto->destroyKey = (SFileCryptDestroyKeyProc)GetProcAddress(library, "CryptDestroyKey");
  if (!crypto->destroyKey) {
    goto cleanup;
  }
  crypto->hashData = (SFileCryptHashDataProc)GetProcAddress(library, "CryptHashData");
  if (!crypto->hashData) {
    goto cleanup;
  }
  crypto->importKey = (SFileCryptImportKeyProc)GetProcAddress(library, "CryptImportKey");
  if (!crypto->importKey) {
    goto cleanup;
  }
  crypto->releaseContext = (SFileCryptReleaseContextProc)GetProcAddress(library, "CryptReleaseContext");
  if (!crypto->releaseContext) {
    goto cleanup;
  }
  crypto->signHash = (SFileCryptSignHashAProc)GetProcAddress(library, "CryptSignHashA");
  if (!crypto->signHash) {
    goto cleanup;
  }
  crypto->verifySignature = (SFileCryptVerifySignatureAProc)GetProcAddress(library, "CryptVerifySignatureA");
  if (!crypto->verifySignature) {
    goto cleanup;
  }

  if (!crypto->acquireContext(&provider, KEYCONTAINER, "Microsoft Base Cryptographic Provider v1.0", 1, 0) &&
      !crypto->acquireContext(&provider, KEYCONTAINER, "Microsoft Base Cryptographic Provider v1.0", 1, 8))
  {
    goto cleanup;
  }

  mapping = CreateFileMappingA((HANDLE)ptr->handle, NULL, 0x08000002, 0, 0, NULL);
  if (!mapping) {
    goto cleanup;
  }
  view = MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0);
  if (!view) {
    goto cleanup;
  }

  signatureIndex = SearchHashTable(ptr, SIGNATUREFILE, 0, 0);
  if (signatureIndex == NOBLOCK) {
    goto cleanup;
  }
  block = GetBlockEntry(ptr, signatureIndex);
  if ((block->flags & 0x1FF00) || block->sizefile <= 8 || !block->offset) {
    goto badsignature;
  }

  companyId = *(DWORD *)((BYTE *)view + block->offset);
  if (companyId >= sizeof(s_authcompany) / sizeof(s_authcompany[0])) {
    if (extendedresult) {
      *extendedresult = SFILE_AUTH_UNKNOWNSIGNATURE;
    }
    goto cleanup;
  }

  resource = FindResourceA(StormGetInstance(), s_authcompany[companyId].keyname, "#256");
  resourceHandle = LoadResource(StormGetInstance(), resource);
  resourceData = LockResource(resourceHandle);
  if (resourceData) {
    crypto->importKey(provider, (const BYTE *)resourceData, SizeofResource(StormGetInstance(), resource), 0, 0, &key);
  }
  FreeResource(resourceHandle);
  if (!key) {
    goto cleanup;
  }

  if (!crypto->createHash(provider, 0x8003, 0, 0, &hash)) {
    goto cleanup;
  }
  if (!crypto->hashData(hash, (const BYTE *)view + ptr->startinglocation, block->offset - ptr->startinglocation, 0)) {
    goto cleanup;
  }

  zeros = (BYTE *)ALLOCZERO(block->sizefile);
  hashResult = crypto->hashData(hash, zeros, block->sizefile, 0);
  FREE(zeros);
  if (!hashResult) {
    goto cleanup;
  }

  signatureEnd = block->offset + block->sizefile;
  if (signatureEnd < ptr->startinglocation + ptr->header.archivesize &&
      !crypto->hashData(hash, (const BYTE *)view + signatureEnd, ptr->startinglocation + ptr->header.archivesize - signatureEnd, 0))
  {
    goto cleanup;
  }

  if (!crypto->verifySignature(hash, (const BYTE *)view + block->offset + 8, block->sizefile - 8, key, NULL, 0)) {
    goto badsignature;
  }

  result = TRUE;
  if (extendedresult) {
    *extendedresult = s_authcompany[companyId].authresult;
  }
  goto cleanup;

badsignature:
  if (extendedresult) {
    *extendedresult = SFILE_AUTH_BADSIGNATURE;
  }

cleanup:
  if (hash) {
    crypto->destroyHash(hash);
  }
  if (key) {
    crypto->destroyKey(key);
  }
  if (view) {
    UnmapViewOfFile(view);
  }
  if (mapping) {
    CloseHandle(mapping);
  }
  if (provider) {
    crypto->releaseContext(provider, 0);
    crypto->acquireContext(&provider, KEYCONTAINER, "Microsoft Base Cryptographic Provider v1.0", 1, 0x10);
  }
  if (crypto) {
    FREE(crypto);
  }
  if (library) {
    FreeLibrary(library);
  }
  if (!result) {
    SErrSetLastError(0x4DC);
  }
  return result;
}

extern "C" BOOL APIENTRY SFileAuthenticateArchiveEx(
    HSARCHIVE   handle,
    DWORD      *extendedresult,
    const BYTE *modulus,
    DWORD       modulusSize,
    const BYTE *exponent,
    DWORD       exponentSize
) {
  SSignatureData *token;
  BYTE           *buffer;
  DWORD           internalextendedresult;
  DWORD           originalreadcursor;
  DWORD           remaining;

  if (!modulus && !exponent) {
    modulus = ::modulus;
    modulusSize = sizeof(::modulus);
    exponent = ::exponent;
    exponentSize = sizeof(::exponent);
  }

  if (!extendedresult) {
    extendedresult = &internalextendedresult;
  }
  *extendedresult = SFILE_AUTH_UNABLETOAUTHENTICATE;

  Storm::SFile::ArchivePtrLocked archiveptr(handle);
  if (!archiveptr) {
    return FALSE;
  }

  token = NULL;
  SSignatureVerifyStream_Begin(&token, modulusSize, exponentSize);
  ASSERT(token);

  buffer = (BYTE *)ALLOC(0x10000);
  originalreadcursor = SetFilePointer((HANDLE)archiveptr->handle, 0, NULL, FILE_CURRENT);
  if (originalreadcursor == 0xFFFFFFFF) {
    goto cleanup;
  }
  if (SetFilePointer((HANDLE)archiveptr->handle, archiveptr->startinglocation, NULL, FILE_BEGIN) == 0xFFFFFFFF) {
    goto cleanup;
  }

  remaining = archiveptr->header.archivesize + SSignatureVerifyStream_GetSignatureLength(token);
  while (remaining) {
    DWORD chunk = remaining > 0x10000 ? 0x10000 : remaining;
    DWORD actualreadbytes;

    remaining -= chunk;
    actualreadbytes = 0;
    if (!ReadFile((HANDLE)archiveptr->handle, buffer, chunk, &actualreadbytes, NULL)) {
      goto cleanup;
    }
    if (actualreadbytes != chunk) {
      goto cleanup;
    }
    SSignatureVerifyStream_ProvideData(token, buffer, chunk);
  }
  SetFilePointer((HANDLE)archiveptr->handle, originalreadcursor, NULL, FILE_BEGIN);

cleanup:
  FREE(buffer);
  *extendedresult = SSignatureVerifyStream_Finish(token, modulus, exponent) ? 5 : 1;
  return *extendedresult == 5;
}

extern "C" DWORD APIENTRY SFileCalcFileCrc(LPCSTR filename) {
  BYTE  *buffer;
  DWORD  crc;
  HANDLE filehandle;
  DWORD  filesize;
  DWORD  remaining;
  DWORD  chunk;
  DWORD  actual;

  FATALASSERT(filename);
  FATALASSERT(*filename);

  filehandle = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
  if (filehandle == INVALID_HANDLE_VALUE) {
    return 0;
  }

  crc = 0;
  filesize = GetFileSize(filehandle, NULL);
  if (filesize == 0xFFFFFFFF) {
    goto close_file;
  }

  buffer = (BYTE *)ALLOC(0x8000);

  CrcBuffer(NULL, 0, &crc, 1);
  remaining = filesize;
  while (remaining) {
    chunk = remaining < 0x8000 ? remaining : 0x8000;
    ReadFile(filehandle, buffer, chunk, &actual, NULL);
    if (actual != chunk) {
      if (buffer) {
        FREE(buffer);
      }
      CloseHandle(filehandle);
      return 0;
    }
    CrcBuffer(buffer, chunk, &crc, 2);
    remaining -= chunk;
  }
  CrcBuffer(NULL, 0, &crc, 4);

  if (buffer) {
    FREE(buffer);
  }
close_file:
  CloseHandle(filehandle);
  return crc;
}

extern "C" void APIENTRY SFileCancelRequest(LPVOID buffer) {
  FATALASSERT(buffer);

  CancelRequest(buffer, NULL);
}

extern "C" BOOL APIENTRY SFileCancelRequestEx(LPVOID buffer) {
  FATALASSERT(buffer);

  return CancelRequest(buffer, NULL);
}

extern "C" BOOL APIENTRY SFileCloseArchive(HSARCHIVE handle) {
  SFileArchiveRecData *archive;

  archive = Storm::SFile::GetArchivePtr(handle);
  if (!archive) {
    return FALSE;
  }

  if (Storm::SFile::IsSubArchive(handle)) {
    {
      Storm::SFile::FilePtr fileptr((HSFILE)archive->ownerarchivefile);
      if (!fileptr) {
        SErrDisplayError(STORM_ERROR_ASSERTION, __FILE__, __LINE__, "\"SFileCloseArchive-child archive file is closed!\"", FALSE, 1);
      }
    }
    SFileCloseFile((HSFILE)archive->ownerarchivefile);
  } else if (Storm::SFile::IsReopenedArchive(handle)) {
    Storm::SFile::ArchivePtr parent((HSARCHIVE)archive->parentArchive);
    if (parent) {
      Storm::SFile::s_archivelock.Enter();
      Storm::SFile::RemoveArchiveRef(parent);
      Storm::SFile::s_archivelock.Leave();
    }
  }

  Storm::SFile::s_archivelock.Enter();
  Storm::SFile::s_archivelist.UnlinkNode(archive);
  Storm::SFile::RemoveArchiveRef(archive);
  Storm::SFile::s_archivelock.Leave();
  Storm::SFile::ReleaseArchivePtr(archive);
  return TRUE;
}

extern "C" BOOL APIENTRY SFileCloseFile(HSFILE handle) {
  Storm::SFile::FilePtrLocked fileptr(handle);
  if (!fileptr) {
    return FALSE;
  }
  Storm::SFile::RemoveFileRef(fileptr);
  return TRUE;
}

extern "C" void APIENTRY SFileRegisterLoadNotifyProc(void(APIENTRY *f)(LPCSTR, LPVOID), LPVOID opaqueData) {
  Storm::SFile::UseGlob glob;
  glob->s_loadNotifyProc = f;
  glob->s_loadNotifyData = opaqueData;
}

extern "C" BOOL APIENTRY SFileDdaBegin(HSFILE handle, DWORD buffersize, DWORD flags) {
  return SFileDdaBeginEx(handle, buffersize, flags, 0, 0x7FFFFFFF, 0x7FFFFFFF, NULL);
}

extern "C" BOOL APIENTRY
SFileDdaBeginEx(HSFILE handle, DWORD buffersize, DWORD flags, DWORD offset, LONG volume, LONG pan, IDirectSoundBuffer *soundbuffer) {
  _MMCKINFO             mmck;
  tWAVEFORMATEX         format;
  _DSBUFFERDESC         desc;
  CKINFO                info;
  SFileAudioStreamData *stream;
  int                   soundbufferlocal;
  DWORD                 remainder;

  FATALASSERT(buffersize);

  if (!Storm::SFile::s_directsound) {
    SErrSetLastError(0x85100071);
    return FALSE;
  }

  Storm::SFile::FilePtr fileptr(handle);
  if (!fileptr) {
    return FALSE;
  }

  if (fileptr->dda) {
    SFileDdaEnd(handle);
  }

  fileptr.Enter();
  {
    Storm::SFile::UseGlob glob;

    if (!((glob->s_directaccess | flags) & 1) && (fileptr->handle != INVALID_HANDLE_VALUE || !fileptr->archive)) {
      SErrSetLastError(0x8510006F);
      return FALSE;
    }

    if (!g_opt.wavechunksize) {
      g_opt.wavechunksize = 0x4000;
    }
    glob->WAVECHUNKSIZE = g_opt.wavechunksize;

    if (!Storm::SFile::s_soundreadbuffer) {
      Storm::SFile::s_soundreadbuffer = (BYTE *)ALLOC(glob->WAVECHUNKSIZE);
    }

    if (fileptr->archive) {
      if (fileptr->archive->cdrom == 0 || fileptr->archive->cdrom == 1) {
        fileptr->archive->cdrom = 2;
      }
    }

    remainder = buffersize & (glob->WAVECHUNKSIZE - 1);
    if (remainder) {
      buffersize += glob->WAVECHUNKSIZE - remainder;
    }
    if (buffersize < glob->WAVECHUNKSIZE * 2) {
      buffersize = glob->WAVECHUNKSIZE * 2;
    }
  }
  fileptr.Leave();

  SFileSetFilePointer(handle, 0, NULL, FILE_BEGIN);
  if (!SFileReadFileEx2(handle, &mmck, 12, NULL, NULL, 0, NULL)) {
    goto invalidData;
  }
  if (mmck.ckid != 'FFIR' || mmck.fccType != 'EVAW') {
    goto invalidData;
  }

  memset(&info, 0, sizeof(info));
  if (!FindChunk(handle, ' tmf', &info)) {
    goto invalidData;
  }
  {
    pcmwaveformat_tag pcm;

    if (info.size < sizeof(pcm)) {
      goto invalidData;
    }
    if (!SFileReadFileEx2(handle, &pcm, sizeof(pcm), NULL, NULL, 0, NULL)) {
      goto invalidData;
    }
    if (SFileSetFilePointer(handle, (LONG)(info.size - sizeof(pcm)), NULL, FILE_CURRENT) == 0xFFFFFFFF) {
      goto invalidData;
    }
    memcpy(&format, &pcm, sizeof(pcm));
    format.cbSize = 0;
  }
  if (!FindChunk(handle, 'atad', &info)) {
    goto invalidData;
  }

  if (offset) {
    offset %= info.size;
  }

  soundbufferlocal = soundbuffer ? 0 : 1;
  if (!soundbuffer) {
    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = (flags & 0x20) ? 0xA0 : 0x80;
    if (flags & 0x40) {
      desc.dwFlags |= 0x40;
    }
    desc.dwBufferBytes = buffersize;
    desc.lpwfxFormat = &format;

    Storm::SFile::s_cdlock.Enter();
    (*(SFileDirectSoundVtbl **)Storm::SFile::s_directsound)->CreateSoundBuffer(Storm::SFile::s_directsound, &desc, &soundbuffer, NULL);
    Storm::SFile::s_cdlock.Leave();
    if (!soundbuffer) {
      return FALSE;
    }
  }

  if (volume && volume != 0x7FFFFFFF) {
    (*(SFileDirectSoundBufferVtbl **)soundbuffer)->SetVolume(soundbuffer, volume);
  }
  if (pan && pan != 0x7FFFFFFF) {
    (*(SFileDirectSoundBufferVtbl **)soundbuffer)->SetPan(soundbuffer, pan);
  }

  Storm::SFile::s_streamlock.Enter();
  stream = Storm::SFile::s_streamlist.NewNode(LIST_TAIL, 0, 0);
  stream->file = fileptr;
  stream->soundbuffersize = buffersize;
  stream->waveheadersize = info.offset;
  stream->bytespersecond = format.nAvgBytesPerSec;
  stream->wavedatasize = info.size;
  stream->startingoffset = offset;
  stream->playcursor = offset;
  stream->writecursor = offset;
  stream->loop = (flags & SFILE_DDA_LOOP) != 0;
  stream->fillstatus = 0;
  stream->volume = volume;
  stream->pan = pan;
  stream->soundbufferlocal = soundbufferlocal;
  stream->soundbuffer = soundbuffer;
  stream->fillvalue = (format.wBitsPerSample == 8) ? 0x80 : 0;
  Storm::SFile::AddStreamRef(stream);
  (*(SFileDirectSoundBufferVtbl **)soundbuffer)->Play(soundbuffer, 0, 0, 0);
  Storm::SFile::s_streamlock.Leave();

  Storm::SFile::s_filelock.Enter();
  Storm::SFile::AddFileRef(fileptr);
  Storm::SFile::s_filelock.Leave();

  fileptr.Enter();
  fileptr->dda = 1;
  fileptr.Leave();

  CreateCdThread();
  SetEvent((HANDLE)Storm::SFile::s_cdevent);
  return TRUE;

invalidData:
  SErrSetLastError(ERROR_INVALID_DATA);
  return FALSE;
}

extern "C" BOOL APIENTRY SFileDdaDestroy() {
  Storm::SFile::s_closingDirectSound = 1;
  if (Storm::SFile::s_cdthread) {
    do {
      if (!Storm::SFile::s_cdrequest && Storm::SFile::s_cdreqlist.IsEmpty()) {
        break;
      }
      if (WaitForSingleObject((HANDLE)Storm::SFile::s_cdthread, 10) != WAIT_TIMEOUT) {
        break;
      }
    } while (Storm::SFile::s_cdthread);
  }

  Storm::SFile::s_streamlock.Enter();
  SFileAudioStreamData *stream = Storm::SFile::s_streamlist.Head();
  if (stream) {
    do {
      SFileAudioStreamData *next = stream->Next();
      Storm::SFile::s_streamlist.UnlinkNode(stream);
      Storm::SFile::RemoveStreamRef(stream);
      stream = next;
    } while (stream);
  }
  Storm::SFile::s_streamlock.Leave();

  Storm::SFile::s_cdlock.Enter();
  Storm::SFile::s_directsound = NULL;
  Storm::SFile::s_cdlock.Leave();
  Storm::SFile::s_closingDirectSound = 0;
  return TRUE;
}

extern "C" BOOL APIENTRY SFileDdaEnd(HSFILE handle) {
  SFileAudioStreamData *stream;
  SFileAudioStreamData *next;
  SFileRequestData     *cdrequest;

  Storm::SFile::FilePtr fileptr(handle);
  if (!fileptr) {
    return FALSE;
  }

  Storm::SFile::s_streamlock.Enter();
  stream = Storm::SFile::s_streamlist.Head();
  while (stream) {
    next = stream->Next();
    if (stream->file == (SFileRecData *)fileptr) {
      cdrequest = Storm::SFile::s_cdrequest;
      if (cdrequest && cdrequest->soundbuffer && cdrequest->soundbuffer == stream->soundbuffer) {
        cdrequest->soundbuffer = NULL;
      }
      CancelRequest(NULL, stream->soundbuffer);
      Storm::SFile::s_streamlist.UnlinkNode(stream);
      Storm::SFile::RemoveStreamRef(stream);
    }
    stream = next;
  }
  Storm::SFile::s_streamlock.Leave();

  fileptr.Enter();
  fileptr->dda = 0;
  return TRUE;
}

extern "C" BOOL APIENTRY SFileDdaGetPos(HSFILE handle, DWORD *position, DWORD *maxposition) {
  SFileAudioStreamData *stream;
  DWORD                 currentPosition;
  DWORD                 ddamaxpos;
  BOOL                  result;

  currentPosition = 0;
  ddamaxpos = 0;
  result = FALSE;

  Storm::SFile::s_streamlock.Enter();
  {
    Storm::SFile::FilePtr fileptr(handle);
    if (fileptr) {
      stream = Storm::SFile::s_streamlist.Head();
      while (stream && stream->file != (SFileRecData *)fileptr) {
        stream = stream->Next();
      }
      if (stream) {
        ddamaxpos = stream->wavedatasize;
        if (stream->fillstatus == 4) {
          SErrSetLastError(0x85100072);
        } else {
          result = TRUE;
          if (stream->fillstatus != 0 && stream->fillstatus != 1) {
            UpdateAudioStreamPos(stream);
            currentPosition = stream->playcursor;
          }
        }
      } else {
        SErrSetLastError(0x85100072);
      }
    }
  }

  if (position) {
    *position = currentPosition;
  }
  if (maxposition) {
    *maxposition = ddamaxpos;
  }
  Storm::SFile::s_streamlock.Leave();
  return result;
}

extern "C" BOOL APIENTRY SFileDdaGetVolume(HSFILE handle, LONG *volume, LONG *pan) {
  SFileAudioStreamData *stream;

  if (volume) {
    *volume = 0;
  }
  if (pan) {
    *pan = 0;
  }

  Storm::SFile::FilePtr fileptr(handle);
  if (!fileptr) {
    return FALSE;
  }

  Storm::SFile::s_streamlock.Enter();
  stream = Storm::SFile::s_streamlist.Head();
  while (stream && stream->file != (SFileRecData *)fileptr) {
    stream = stream->Next();
  }
  if (!stream) {
    Storm::SFile::s_streamlock.Leave();
    SErrSetLastError(0x85100072);
    return FALSE;
  }

  if (stream->volume == 0x7FFFFFFF) {
    (*(SFileDirectSoundBufferVtbl **)stream->soundbuffer)->GetVolume(stream->soundbuffer, &stream->volume);
  }
  if (stream->pan == 0x7FFFFFFF) {
    (*(SFileDirectSoundBufferVtbl **)stream->soundbuffer)->GetPan(stream->soundbuffer, &stream->pan);
  }
  if (volume) {
    *volume = stream->volume;
  }
  if (pan) {
    *pan = stream->pan;
  }
  Storm::SFile::s_streamlock.Leave();
  return TRUE;
}

extern "C" BOOL APIENTRY SFileDdaInitialize(IDirectSound *directsound) {
  FATALASSERT(directsound);

  Storm::SFile::s_cdlock.Enter();
  Storm::SFile::s_directsound = directsound;
  Storm::SFile::s_cdlock.Leave();
  return TRUE;
}

extern "C" BOOL APIENTRY SFileDdaSetVolume(HSFILE handle, LONG volume, LONG pan) {
  SFileAudioStreamData *stream;

  Storm::SFile::FilePtr fileptr(handle);
  if (!fileptr) {
    return FALSE;
  }

  Storm::SFile::s_streamlock.Enter();
  stream = Storm::SFile::s_streamlist.Head();
  while (stream && stream->file != (SFileRecData *)fileptr) {
    stream = stream->Next();
  }
  if (!stream) {
    Storm::SFile::s_streamlock.Leave();
    SErrSetLastError(0x85100072);
    return FALSE;
  }

  if (volume != 0x7FFFFFFF && volume != stream->volume) {
    stream->volume = volume;
    (*(SFileDirectSoundBufferVtbl **)stream->soundbuffer)->SetVolume(stream->soundbuffer, volume);
  }
  if (pan != 0x7FFFFFFF && pan != stream->pan) {
    stream->pan = pan;
    (*(SFileDirectSoundBufferVtbl **)stream->soundbuffer)->SetPan(stream->soundbuffer, pan);
  }
  Storm::SFile::s_streamlock.Leave();
  return TRUE;
}

extern "C" BOOL APIENTRY SFileDestroy() {
  SFileRequestData     *request;
  SFileAudioStreamData *stream;
  SFileRecData         *file;
  SFileArchiveRecData  *archive;
  LPVOID                buffer;

  SFileDdaDestroy();
  DestroyCdThread();

  request = Storm::SFile::s_cdrequest;
  if (request) {
    delete request;
    Storm::SFile::s_cdrequest = NULL;
  }

  Storm::SFile::s_cdlock.Enter();
  request = Storm::SFile::s_cdreqlist.Head();
  while (request) {
    delete request;
    request = Storm::SFile::s_cdreqlist.Head();
  }
  Storm::SFile::s_cdlock.Leave();

  Storm::SFile::s_streamlock.Enter();
  stream = Storm::SFile::s_streamlist.Head();
  while (stream) {
    SErrReportResourceLeak("AUDIOSTREAM");
    delete stream;
    stream = Storm::SFile::s_streamlist.Head();
  }
  Storm::SFile::s_streamlock.Leave();

  Storm::SFile::s_filelock.Enter();
  file = Storm::SFile::s_filelist.Head();
  while (file) {
    SErrReportNamedResourceLeak("HSFILE", Storm::SFile::s_filelist.Head()->name);
    delete file;
    file = Storm::SFile::s_filelist.Head();
  }
  Storm::SFile::s_filelock.Leave();

  Storm::SFile::s_archivelock.Enter();
  archive = Storm::SFile::s_archivelist.Head();
  while (archive) {
    SErrReportNamedResourceLeak("HSARCHIVE", Storm::SFile::s_archivelist.Head()->archivename);
    delete archive;
    archive = Storm::SFile::s_archivelist.Head();
  }
  Storm::SFile::s_archivelock.Leave();

  buffer = Storm::SFile::s_hashsource;
  if (buffer) {
    FREE(buffer);
    Storm::SFile::s_hashsource = NULL;
  }
  buffer = Storm::SFile::s_explodebuffer;
  if (buffer) {
    FREE(buffer);
    Storm::SFile::s_explodebuffer = NULL;
  }
  buffer = Storm::SFile::s_soundreadbuffer;
  if (buffer) {
    FREE(buffer);
    Storm::SFile::s_soundreadbuffer = NULL;
  }
  return TRUE;
}

extern "C" BOOL APIENTRY SFileEnableArchive(HSARCHIVE archive, int enable) {
  Storm::SFile::ArchivePtrLocked archiveptr(archive);
  if (!archiveptr) {
    return FALSE;
  }
  archiveptr->disableCount += enable ? -1 : 1;
  return TRUE;
}

extern "C" BOOL APIENTRY SFileEnableDirectAccess(DWORD access) {
  Storm::SFile::UseGlob glob;
  glob->s_directaccess = access;
  return TRUE;
}

extern "C" void APIENTRY SFileEnableSeekOptimization(int enable) {
  Storm::SFile::UseGlob glob;
  glob->s_seekOptimize = enable;
}

extern "C" DWORD APIENTRY SFileFileExists(LPCSTR filename) {
  return SFileFileExistsEx(NULL, filename, BuildDefaultOpenFlags());
}

extern "C" DWORD APIENTRY SFileFileExistsEx(HSARCHIVE archivehandle, LPCSTR filename, DWORD flags) {
  FATALASSERT(filename);
  FATALASSERT(*filename);

  Initialize();
  return GetFileBlockEntry(archivehandle, filename, flags, NULL, NULL, NULL);
}

extern "C" BOOL APIENTRY SFileGetArchiveInfo(HSARCHIVE archive, int *priority, int *cdrom) {
  if (priority) {
    *priority = 0;
  }
  if (cdrom) {
    *cdrom = 0;
  }
  Storm::SFile::ArchivePtr archiveptr(archive);
  if (!archiveptr) {
    return FALSE;
  }
  if (priority) {
    *priority = archiveptr->priority;
  }
  if (cdrom) {
    *cdrom = archiveptr->cdrom == 3;
  }
  return TRUE;
}

extern "C" BOOL APIENTRY SFileGetArchiveName(HSARCHIVE archive, char *buffer, DWORD bufferchars) {
  FATALASSERT(buffer);
  *buffer = 0;
  Storm::SFile::ArchivePtr archiveptr(archive);
  if (!archiveptr) {
    return FALSE;
  }
  SStrCopy(buffer, archiveptr->archivename, bufferchars);
  return TRUE;
}

extern "C" BOOL APIENTRY SFileGetBasePath(char *buffer, DWORD bufferchars) {
  Storm::SFile::UseGlob glob;

  if (!glob->s_basepath[0]) {
    BuildDefaultBasePath(glob->s_basepath, MAX_PATH);
  }
  SStrCopy(buffer, glob->s_basepath, bufferchars);
  return TRUE;
}

extern "C" BOOL APIENTRY SFileGetFileArchive(HSFILE file, HSARCHIVE *archive) {
  FATALASSERT(archive);
  *archive = NULL;
  Storm::SFile::FilePtr fileptr(file);
  if (!fileptr) {
    return FALSE;
  }
  *archive = (HSARCHIVE)fileptr->archive;
  return TRUE;
}

extern "C" DWORD APIENTRY SFileGetFileCrc(HSFILE handle) {
  Storm::SFile::FilePtr fileptr(handle);
  if (!fileptr) {
    return 0;
  }
  if (fileptr->handle != INVALID_HANDLE_VALUE) {
    return 0;
  }
  return fileptr->block.crc;
}

extern "C" BOOL APIENTRY SFileGetFileMD5(HSFILE handle, BYTE *md5) {
  Storm::SFile::FilePtr fileptr(handle);
  if (!fileptr) {
    return FALSE;
  }
  if (fileptr->handle != INVALID_HANDLE_VALUE) {
    return FALSE;
  }
  memcpy(md5, &fileptr->block.md5, sizeof(fileptr->block.md5));
  return TRUE;
}

extern "C" BOOL APIENTRY SFileGetFileName(HSFILE file, char *buffer, DWORD bufferchars) {
  FATALASSERT(buffer);
  *buffer = 0;
  Storm::SFile::FilePtr fileptr(file);
  if (!fileptr) {
    return FALSE;
  }
  SStrCopy(buffer, fileptr->name, bufferchars);
  return TRUE;
}

extern "C" BOOL APIENTRY SFileGetActualFileName(HSFILE file, char *buffer, DWORD bufferchars) {
  FATALASSERT(buffer);
  *buffer = 0;
  Storm::SFile::FilePtr fileptr(file);
  if (!fileptr) {
    return FALSE;
  }
  SStrCopy(buffer, fileptr->actualName ? fileptr->actualName : fileptr->name, bufferchars);
  return TRUE;
}

extern "C" DWORD APIENTRY SFileGetFileCompressedSize(HSFILE handle, DWORD *fileSizeHigh) {
  DWORD size;

  size = 0;
  if (fileSizeHigh) {
    *fileSizeHigh = 0;
  }
  Storm::SFile::FilePtr fileptr(handle);
  if (fileptr && fileptr->handle == INVALID_HANDLE_VALUE) {
    size = fileptr->block.sizealloc;
  }
  return size;
}

extern "C" DWORD APIENTRY SFileGetFileSize(HSFILE handle, DWORD *filesizehigh) {
  if (filesizehigh) {
    *filesizehigh = 0;
  }
  Storm::SFile::FilePtr fileptr(handle);
  if (!fileptr) {
    return 0xFFFFFFFF;
  }
  if (fileptr->handle != INVALID_HANDLE_VALUE) {
    return GetFileSize((HANDLE)fileptr->handle, filesizehigh);
  }
  return fileptr->block.sizefile;
}

extern "C" BOOL APIENTRY SFileGetFileTime(HSFILE handle, FILETIME *filetime) {
  FATALASSERT(filetime);

  Storm::SFile::FilePtr fileptr(handle);
  if (!fileptr) {
    return FALSE;
  }
  if (fileptr->handle != INVALID_HANDLE_VALUE) {
    return GetFileTime(fileptr->handle, NULL, NULL, filetime);
  }
  SFileRecData *fileRec = fileptr.operator->();
  filetime->dwLowDateTime = fileRec->block.time.dwLowDateTime;
  filetime->dwHighDateTime = fileRec->block.time.dwHighDateTime;
  return TRUE;
}

extern "C" BOOL APIENTRY SFileLoadFile(LPCSTR filename, LPVOID *buffer, DWORD *bytes, DWORD extraBytes, OVERLAPPED *overlapped) {
  return SFileLoadFileEx2(NULL, filename, buffer, bytes, extraBytes, BuildDefaultOpenFlags(), overlapped, 0);
}

extern "C" BOOL APIENTRY
SFileLoadFileEx(HSARCHIVE archive, LPCSTR filename, LPVOID *buffer, DWORD *bytes, DWORD extraBytes, DWORD flags, OVERLAPPED *overlapped) {
  return SFileLoadFileEx2(archive, filename, buffer, bytes, extraBytes, flags, overlapped, 0);
}

extern "C" BOOL APIENTRY SFileLoadFileEx2(
    HSARCHIVE   archive,
    LPCSTR      filename,
    LPVOID     *buffer,
    DWORD      *bytes,
    DWORD       extraBytes,
    DWORD       flags,
    OVERLAPPED *overlapped,
    LONG        overlappedpriority
) {
  DWORD  sizeLow;
  LPVOID target;

  FATALASSERT(filename);
  FATALASSERT(buffer);

  *buffer = NULL;
  if (bytes) {
    *bytes = 0;
  }

  HSFILE file = NULL;
  if (SFileOpenFileEx(archive, filename, flags, &file)) {
    sizeLow = SFileGetFileSize(file, NULL);
    target = ALLOC(sizeLow + extraBytes);
    if (SFileReadFileEx2(file, target, sizeLow, NULL, overlapped, overlappedpriority, NULL)) {
      if (extraBytes) {
        memset((BYTE *)target + sizeLow, 0, extraBytes);
      }
      *buffer = target;
      if (bytes) {
        *bytes = sizeLow;
      }
    } else if (target) {
      FREE(target);
    }
  }

  if (file) {
    SFileCloseFile(file);
  }
  return *buffer != NULL;
}

extern "C" BOOL APIENTRY SFileOpenArchive(LPCSTR archivename, int priority, DWORD flags, HSARCHIVE *handle) {
  SFileArchiveRecData *archive;
  HANDLE               file;
  int                  checkCdRom;
  char                 localarchivename[MAX_PATH];

  FATALASSERT(handle);
  *handle = NULL;
  FATALASSERT(archivename);
  FATALASSERT(*archivename);

  Initialize();
  SStrCopy(localarchivename, archivename, sizeof(localarchivename));
  localarchivename[MAX_PATH - 1] = 0;
  if (!CheckFileExists(localarchivename)) {
    if (archivename[0] == '\\' || strstr(archivename, ":\\") || strstr(archivename, "\\\\")) {
      SStrCopy(localarchivename, archivename, sizeof(localarchivename));
    } else {
      ConvertRelativePathName(archivename, localarchivename, FALSE);
    }
  }

  checkCdRom = CheckForCdRom(localarchivename);
  if ((flags & 1) && !checkCdRom) {
    SErrSetLastError(ERROR_INVALID_DRIVE);
    return FALSE;
  }

  file = CreateFileA(localarchivename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
  if (file == INVALID_HANDLE_VALUE) {
    return FALSE;
  }

  Storm::SFile::s_archivelock.Enter();
  archive = Storm::SFile::s_archivelist.NewNode(LIST_UNLINKED, 0, 0);
  Storm::SFile::AddArchiveRef(archive);
  Storm::SFile::s_archivelock.Leave();

  SStrCopy(archive->archivename, localarchivename, sizeof(archive->archivename));
  archive->handle = file;
  archive->priority = priority;
  if (checkCdRom) {
    archive->cdrom = 3;
  } else if (flags & 2) {
    archive->cdrom = 1;
  } else {
    archive->cdrom = 0;
  }
  archive->dontCheckDisk = (flags >> 2) & 1;
  archive->startinglocation = 0;
  archive->endinglocation = GetFileSize(file, NULL);
  archive->disableCount = 0;

  return Storm::SFile::s_OpenArchive(archive, flags, checkCdRom, handle);
}

extern "C" BOOL APIENTRY SFileOpenPathAsArchive(HSARCHIVE ownerarchive, LPCSTR pathPrefix, int priority, DWORD flags, HSARCHIVE *handle) {
  SFileArchiveRecData *node;
  SFileArchiveRecData *position;

  (void)flags;
  FATALASSERT(handle);
  *handle = NULL;
  FATALASSERT(pathPrefix);
  FATALASSERT(*pathPrefix);

  Initialize();
  Storm::SFile::ArchivePtr parent(ownerarchive);
  if (!parent) {
    SErrSetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
  }

  Storm::SFile::s_archivelock.Enter();
  node = Storm::SFile::s_archivelist.NewNode(LIST_UNLINKED, 0, 0);
  Storm::SFile::AddArchiveRef(node);
  Storm::SFile::AddArchiveRef(parent);
  Storm::SFile::s_archivelock.Leave();

  SStrCopy(node->archivename, parent->archivename, sizeof(node->archivename));
  node->handle = parent->handle;
  node->parentArchive = ownerarchive;
  node->priority = priority;
  node->disableCount = 0;
  SStrCopy(node->pathPrefix, pathPrefix, 0x7FFFFFFF);

  Storm::SFile::s_archivelock.Enter();
  position = Storm::SFile::s_archivelist.Head();
  while (position && position->priority > node->priority) {
    position = position->Next();
  }
  Storm::SFile::s_archivelist.LinkNode(node, LIST_LINK_BEFORE, position);
  Storm::SFile::s_archivelock.Leave();

  *handle = (HSARCHIVE)node;
  return TRUE;
}

int Storm::SFile::s_OpenArchive(ARCHIVEREC *archiveptr, DWORD flags, int cdrom, HSARCHIVE *handle) {
  BYTE                *archivebuffer;
  DWORD                bufferlocation;
  DWORD                bytesread;
  DWORD                bufferbytes;
  DWORD                i;
  SFileArchiveRecData *position;

  archivebuffer = (BYTE *)ALLOC(READAHEAD);

  bufferlocation = 0;
  bytesread = 0;
  for (;;) {
    if (archiveptr->startinglocation >= bufferlocation + bytesread) {
      bufferlocation = archiveptr->startinglocation;
      SetFilePointer((HANDLE)archiveptr->handle, archiveptr->startinglocation, NULL, FILE_BEGIN);
      bytesread = 0;
      ReadFile((HANDLE)archiveptr->handle, archivebuffer, READAHEAD, &bytesread, NULL);
      if (bytesread < sizeof(SFileArchiveHeaderData) || archiveptr->startinglocation + sizeof(SFileArchiveHeaderData) >= archiveptr->endinglocation) {
        delete archiveptr;
        SErrSetLastError(0x8510006C);
        FREE(archivebuffer);
        return FALSE;
      }
    }

    memcpy(&archiveptr->header, archivebuffer + (archiveptr->startinglocation - bufferlocation), sizeof(archiveptr->header));
    if (archiveptr->header.signature == MPQ_SIGNATURE && archiveptr->header.headersize >= sizeof(SFileArchiveHeaderData)) {
      break;
    }
    archiveptr->startinglocation += 0x200;
  }

  FREE(archivebuffer);

  archiveptr->sectorsize = 0x200 << archiveptr->header.sectorsizeid;
  archiveptr->sectorbuffer = (BYTE *)ALLOC(archiveptr->sectorsize);

  bufferbytes = archiveptr->header.hashcount << 4;
  archiveptr->hashtable = (SFileHashEntryData *)ALLOC(bufferbytes);
  bytesread = 0;
  ReadFileChecked(
      archiveptr->startinglocation + archiveptr->header.hashoffset, &archiveptr->lastlocation, (HANDLE)archiveptr->handle, archiveptr->hashtable,
      bufferbytes, &bytesread, archiveptr->archivename
  );
  Decrypt((DWORD *)archiveptr->hashtable, bufferbytes, Hash("(hash table)", HASH_ENCRYPTKEY));

  bufferbytes = archiveptr->header.blockcount * 0x2C;
  archiveptr->blocktable = (SFileBlockEntryData *)ALLOC(bufferbytes);
  bytesread = 0;
  ReadFileChecked(
      archiveptr->startinglocation + archiveptr->header.blockoffset, &archiveptr->lastlocation, (HANDLE)archiveptr->handle, archiveptr->blocktable,
      archiveptr->header.blockcount << 4, &bytesread, archiveptr->archivename
  );
  Decrypt((DWORD *)archiveptr->blocktable, archiveptr->header.blockcount << 4, Hash("(block table)", HASH_ENCRYPTKEY));
  BlockEntryFileToMem(archiveptr->blocktable, archiveptr->header.blockcount);

  if (archiveptr->startinglocation) {
    for (i = 0; i < archiveptr->header.blockcount; ++i) {
      if (archiveptr->blocktable[i].offset) {
        archiveptr->blocktable[i].offset += archiveptr->startinglocation;
      }
    }
  }

  Storm::SFile::s_archivelock.Enter();
  position = Storm::SFile::s_archivelist.Head();
  while (position && position->priority > archiveptr->priority) {
    position = position->Next();
  }
  Storm::SFile::s_archivelist.LinkNode(archiveptr, LIST_LINK_BEFORE, position);
  Storm::SFile::s_archivelock.Leave();

  if (archiveptr->cdrom) {
    CreateCdThread();
  }

  if (!archiveptr->cdrom && (flags & 1) && !cdrom) {
    SFileCloseArchive((HSARCHIVE)archiveptr);
    SErrSetLastError(ERROR_INVALID_DRIVE);
    return FALSE;
  }

  *handle = (HSARCHIVE)archiveptr;
  ReadAdditionalAttributes((HSARCHIVE)archiveptr, archiveptr->blocktable, archiveptr->header.blockcount);
  return TRUE;
}

extern "C" DWORD APIENTRY SFileOpenFile(LPCSTR filename, HSFILE *handle) {
  return SFileOpenFileEx(NULL, filename, BuildDefaultOpenFlags(), handle);
}

extern "C" BOOL APIENTRY SFileOpenFileAsArchive(HSARCHIVE ownerarchive, LPCSTR filename, int priority, DWORD flags, HSARCHIVE *handle) {
  HSFILE               filehandle;
  SFileArchiveRecData *newarchive;
  BOOL                 ondisk;
  BOOL                 unsupported;
  int                  result;

  if (!SFileOpenFileEx(ownerarchive, filename, BuildDefaultOpenFlags(), &filehandle)) {
    return FALSE;
  }

  ondisk = TRUE;
  unsupported = TRUE;
  {
    Storm::SFile::FilePtr fileptr(filehandle);

    if (fileptr) {
      ondisk = fileptr->handle != INVALID_HANDLE_VALUE;
      unsupported = (fileptr->block.flags & MPQ_COMPRESSEDMASK) != 0 || (fileptr->block.flags & MPQ_ENCRYPTED) != 0;
    }
  }

  if (ondisk) {
    SFileCloseFile(filehandle);
    if (CheckFileExistsOnDisk(filename, BuildDefaultOpenFlags(), NULL) && !SFileOpenArchive(filename, priority, flags, handle)) {
      return FALSE;
    }
    return TRUE;
  }

  if (unsupported) {
    SFileCloseFile(filehandle);
    SErrSetLastError(0x3EE);
    return FALSE;
  }

  {
    Storm::SFile::FilePtrLocked fileptr(filehandle);

    ASSERT(fileptr);

    Storm::SFile::s_archivelock.Enter();
    newarchive = Storm::SFile::s_archivelist.NewNode(LIST_UNLINKED, 0, 0);
    Storm::SFile::AddArchiveRef(newarchive);
    Storm::SFile::s_archivelock.Leave();

    SStrCopy(newarchive->archivename, filename, sizeof(newarchive->archivename));
    newarchive->handle = fileptr->archive->handle;
    newarchive->cdrom = fileptr->archive->cdrom;
    newarchive->dontCheckDisk = (flags >> 2) & 1;
    newarchive->priority = priority;
    newarchive->disableCount = 0;
    newarchive->ownerarchivefile = filehandle;
    newarchive->startinglocation = fileptr->block.offset;
    newarchive->endinglocation = fileptr->block.offset + fileptr->block.sizefile;

    result = Storm::SFile::s_OpenArchive(newarchive, flags, newarchive->cdrom, handle);
  }
  return result;
}

extern "C" DWORD APIENTRY SFileOpenFileEx(HSARCHIVE archivehandle, LPCSTR filename, DWORD flags, HSFILE *handle) {
  typedef void(APIENTRY * SFileLoadNotifyProc)(LPCSTR, LPVOID);
  char                 localfilename[MAX_PATH];
  BOOL                 result = FALSE;
  DWORD                sectors;
  DWORD               *sectoroffsettable;
  SFileArchiveRecData *archiveptr;
  LPVOID               s_loadNotifyData;
  DWORD                key;

  FATALASSERT(handle);
  *handle = NULL;
  FATALASSERT(filename);
  FATALASSERT(*filename);

  Initialize();
  archiveptr = NULL;
  DWORD source =
      GetFileBlockEntry(archivehandle, filename, flags, &archiveptr, reinterpret_cast<SFileBlockEntryData **>(&archivehandle), localfilename);

  if (source) {
    if (source == 2) {
      HANDLE osFile =
          CreateFileA(localfilename, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
      if (osFile == INVALID_HANDLE_VALUE) {
        result = 0;
      } else if (flags & 4) {
        *handle = (HSFILE)osFile;
        result = 2;
      } else {
        Storm::SFile::s_filelock.Enter();
        SFileRecData *fileptr = Storm::SFile::s_filelist.NewNode(LIST_HEAD, 0, 0);
        Storm::SFile::AddFileRef(fileptr);
        Storm::SFile::s_filelock.Leave();
        SStrCopy(fileptr->name, filename, sizeof(fileptr->name));
        fileptr->actualName = SStrDupA(localfilename, __FILE__, __LINE__);
        fileptr->handle = osFile;
        fileptr->crcavail = 0;
        *handle = (HSFILE)fileptr;
        result = 2;
      }
    } else {
      if (flags & 4) {
        HANDLE osFile = CreateFileA(archiveptr->archivename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
        *handle = (HSFILE)osFile;
        result = osFile != INVALID_HANDLE_VALUE;
        if (result) {
          SetFilePointer(osFile, reinterpret_cast<SFileBlockEntryData *>(archivehandle)->offset, NULL, FILE_BEGIN);
        } else {
          SErrSetLastError(ERROR_FILE_NOT_FOUND);
        }
      } else {
        LPCSTR keyname = filename;
        LPCSTR separator = SStrChrR(keyname, ':');
        if (separator) {
          keyname = separator + 1;
        }
        separator = SStrChrR(keyname, '\\');
        if (separator) {
          keyname = separator + 1;
        }

        key = Hash(keyname, HASH_ENCRYPTKEY);
        if (reinterpret_cast<SFileBlockEntryData *>(archivehandle)->flags & MPQ_ENCRYPTED_FIXLOC) {
          key = (key + reinterpret_cast<SFileBlockEntryData *>(archivehandle)->offset - archiveptr->startinglocation) ^
                reinterpret_cast<SFileBlockEntryData *>(archivehandle)->sizefile;
        }
        sectors = (reinterpret_cast<SFileBlockEntryData *>(archivehandle)->sizefile + archiveptr->sectorsize - 1) / archiveptr->sectorsize;
        sectoroffsettable = NULL;
        if (reinterpret_cast<SFileBlockEntryData *>(archivehandle)->flags & MPQ_COMPRESSEDMASK) {
          sectoroffsettable = (DWORD *)ALLOC((sectors + 1) * sizeof(DWORD));
        }

        Storm::SFile::s_filelock.Enter();
        SFileRecData *fileptr = Storm::SFile::s_filelist.NewNode(LIST_HEAD, 0, 0);
        Storm::SFile::AddFileRef(fileptr);
        Storm::SFile::s_filelock.Leave();
        SStrCopy(fileptr->name, filename, sizeof(fileptr->name));
        fileptr->archive = archiveptr;
        fileptr->handle = INVALID_HANDLE_VALUE;
        memcpy(&fileptr->block, reinterpret_cast<SFileBlockEntryData *>(archivehandle), sizeof(fileptr->block));
        fileptr->key = key;
        fileptr->sectors = sectors;
        fileptr->sectoroffsettable = sectoroffsettable;
        fileptr->readaheadbuffer = ALLOC(READAHEAD);
        fileptr->crcavail = reinterpret_cast<SFileBlockEntryData *>(archivehandle)->crc ? g_opt.crcenabled : 0;
        fileptr->crcstate = 1;
        result = 1;

        Storm::SFile::s_archivelock.Enter();
        Storm::SFile::AddArchiveRef(archiveptr);
        Storm::SFile::s_archivelock.Leave();
        *handle = (HSFILE)fileptr;
      }
    }
  }

  Storm::SFile::ReleaseArchivePtr(archiveptr);

  SFileLoadNotifyProc s_loadNotifyProc;
  {
    Storm::SFile::UseGlob glob;
    s_loadNotifyProc = glob->s_loadNotifyProc;
    s_loadNotifyData = glob->s_loadNotifyData;
  }
  if (result && s_loadNotifyProc) {
    s_loadNotifyProc(filename, s_loadNotifyData);
  }
  return result;
}

extern "C" void APIENTRY SFilePrioritizeRequest(LPVOID buffer, LONG overlappedpriority) {
  FATALASSERT(buffer);
  MarkRequestUrgent(buffer, overlappedpriority > 0);
  if (overlappedpriority >= 1 && Storm::SFile::s_cdevent) {
    SetEvent((HANDLE)Storm::SFile::s_cdevent);
  }
}

extern "C" BOOL APIENTRY SFileReadFile(HSFILE handle, LPVOID buffer, DWORD bytestoread, DWORD *bytesread, OVERLAPPED *overlapped) {
  return SFileReadFileEx2(handle, buffer, bytestoread, bytesread, overlapped, 0, NULL);
}

extern "C" BOOL APIENTRY
SFileReadFileEx(HSFILE handle, LPVOID buffer, DWORD bytestoread, DWORD *bytesread, OVERLAPPED *overlapped, _TASYNCPARAMBLOCK *asyncparam) {
  return SFileReadFileEx2(handle, buffer, bytestoread, bytesread, overlapped, 0, asyncparam);
}

extern "C" BOOL APIENTRY SFileReadFileEx2(
    HSFILE             handle,
    LPVOID             buffer,
    DWORD              bytestoread,
    DWORD             *bytesread,
    OVERLAPPED        *overlapped,
    LONG               overlappedpriority,
    _TASYNCPARAMBLOCK *asyncparam
) {
  SFileRequestData *request;
  BYTE             *output;
  BYTE             *readBuffer;
  HANDLE            event;
  DWORD             remaining;
  DWORD             cached;
  DWORD             readBytes;
  DWORD             location;
  DWORD             dataChunkSize;
  DWORD             chunkCount;
  DWORD             chunkIndex;
  DWORD             chunkOffset;
  DWORD             chunkBytes;
  DWORD             totalRead;
  DWORD             callerRead;
  BOOL              lastChunk;
  BOOL              autoDelete;

  (void)overlappedpriority;
  if (bytesread) {
    *bytesread = 0;
  }

  Storm::SFile::FilePtrLocked file(handle);
  if (!file) {
    return FALSE;
  }
  if (!bytestoread) {
    if (overlapped && overlapped->hEvent) {
      SetEvent(overlapped->hEvent);
    }
    return TRUE;
  }
  FATALASSERT(buffer);

  if (file->handle != INVALID_HANDLE_VALUE && !overlapped) {
    return ReadFileWin32(file, 0xFFFFFFFF, buffer, bytestoread, bytesread);
  }

  output = (BYTE *)buffer;
  remaining = bytestoread;
  if (file->readaheadbuffer && file->readaheadoffset < file->readaheadbytes && !overlapped) {
    cached = file->readaheadbytes - file->readaheadoffset;
    if (cached > remaining) {
      cached = remaining;
    }
    memcpy(output, (BYTE *)file->readaheadbuffer + file->readaheadoffset, cached);
    file->location += cached;
    file->readaheadoffset += cached;
    output += cached;
    remaining -= cached;
    if (bytesread) {
      *bytesread = cached;
    }
    if (!remaining) {
      goto validateCrc;
    }
  }

  readBuffer = output;
  readBytes = remaining;
  if (!overlapped) {
    DWORD sectorSize;
    DWORD readAheadBytes;

    sectorSize = file->archive->sectorsize;
    readAheadBytes = sectorSize - (file->location & (sectorSize - 1));
    if (readAheadBytes > READAHEAD) {
      readAheadBytes = READAHEAD;
    }
    if (remaining < readAheadBytes) {
      readBuffer = (BYTE *)file->readaheadbuffer;
      readBytes = readAheadBytes;
    }
  }

  if (!readBytes) {
    if (overlapped && overlapped->hEvent) {
      SetEvent(overlapped->hEvent);
    }
    return TRUE;
  }

  {
    Storm::SFile::UseGlob glob;
    dataChunkSize = glob->s_dataChunkSize;
  }

  location = overlapped ? overlapped->Offset : file->location;
  totalRead = 0;
  if (file->handle == INVALID_HANDLE_VALUE && !file->archive->cdrom) {
    {
      Storm::SFile::ArchivePtrLocked archive((HSARCHIVE)file->archive);
      totalRead = InternalReadUnaligned(file, location, readBuffer, readBytes);
      archive.Leave();
      if (file->crcavail && file->crcstate == 2 && file->crcexpected == file->block.sizefile) {
        CrcBuffer(NULL, 0, &file->crc, 4);
        file->crcstate = 4;
      }
      if (overlapped && overlapped->hEvent) {
        SetEvent(overlapped->hEvent);
      }
    }
  } else {
    CreateCdThread();
    event = overlapped ? overlapped->hEvent : CreateEventA(NULL, TRUE, FALSE, NULL);
    chunkCount = (readBytes + dataChunkSize - 1) / dataChunkSize;
    request = NULL;
    chunkOffset = 0;
    for (chunkIndex = 0; chunkIndex < chunkCount; ++chunkIndex) {
      lastChunk = chunkIndex == chunkCount - 1;
      chunkBytes = readBytes - chunkOffset;
      if (chunkBytes > dataChunkSize) {
        chunkBytes = dataChunkSize;
      }
      autoDelete = overlapped || !lastChunk;
      request = IssueRequest(
          file, location + chunkOffset, readBuffer + chunkOffset, readBuffer, NULL, 0, NULL, chunkBytes, 0x7FFFFFFF, overlapped ? FALSE : TRUE,
          lastChunk ? event : NULL, autoDelete, lastChunk, lastChunk ? asyncparam : NULL
      );
      if (!lastChunk) {
        totalRead += chunkBytes;
      }
      chunkOffset += dataChunkSize;
    }

    if (overlapped) {
      return TRUE;
    }

    file.Leave();
    WaitForSingleObject(event, INFINITE);
    file.Enter();
    CloseHandle(event);
    totalRead += request->bytesread;
    delete request;
  }

  callerRead = totalRead;
  if (callerRead > remaining) {
    callerRead = remaining;
  }
  if (readBuffer != output) {
    memcpy(output, readBuffer, callerRead);
    file->readaheadoffset = callerRead;
    file->readaheadbytes = totalRead;
  }
  if (!overlapped) {
    file->location += callerRead;
  }
  if (bytesread) {
    *bytesread += callerRead;
  }
  if (callerRead != remaining) {
    SErrSetLastError(ERROR_HANDLE_EOF);
    return FALSE;
  }

validateCrc:
  if (file->crcavail && file->crcstate == 4 && file->crc != file->block.crc) {
    SErrDisplayError(0x85100083, file->name, -4, NULL, FALSE, 1);
  }
  return TRUE;
}

extern "C" void APIENTRY SFileSetAsyncBudget(DWORD bytesPerSec) {
  Storm::SFile::UseGlob glob;
  glob->s_asyncBudget = bytesPerSec;
}

extern "C" BOOL APIENTRY SFileSetBasePath(LPCSTR path) {
  DWORD len;
  DWORD required;
  int   terminated;

  FATALASSERT(path);

  Storm::SFile::UseGlob glob;
  if (!path[0]) {
    glob->s_basepath[0] = 0;
    return TRUE;
  }
  terminated = path[SStrLen(path) - 1] == '\\';
  len = SStrLen(path);
  required = len + (terminated ? 0 : 1) + 1;
  if (required > MAX_PATH) {
    SErrSetLastError(ERROR_BAD_PATHNAME);
    return FALSE;
  }
  SStrCopy(glob->s_basepath, path, MAX_PATH);
  if (!terminated) {
    SStrPack(glob->s_basepath, "\\", MAX_PATH);
  }
  return TRUE;
}

extern "C" void APIENTRY SFileSetDataChunkSize(DWORD bytes) {
  FATALASSERT(!(bytes & (bytes - 1)));
  Storm::SFile::UseGlob glob;
  glob->s_dataChunkSize = bytes;
}

extern "C" DWORD APIENTRY SFileSetFilePointer(HSFILE handle, LONG distancetomove, LONG *distancetomovehigh, DWORD movemethod) {
  if (distancetomovehigh && *distancetomovehigh != 0) {
    SErrSetLastError(ERROR_INVALID_PARAMETER);
    return 0xFFFFFFFF;
  }

  Storm::SFile::FilePtrLocked fileptr(handle);
  if (!fileptr) {
    return 0xFFFFFFFF;
  }
  if (fileptr->handle != INVALID_HANDLE_VALUE) {
    return SetFilePointer((HANDLE)fileptr->handle, distancetomove, NULL, movemethod);
  }

  switch (movemethod) {
    case FILE_BEGIN:
      fileptr->location = (DWORD)distancetomove;
      break;

    case FILE_CURRENT:
      if (distancetomove < 0) {
        fileptr->location = fileptr->location < (DWORD)-distancetomove ? 0 : fileptr->location + distancetomove;
      } else {
        fileptr->location += (DWORD)distancetomove;
      }
      break;

    case FILE_END:
      if (distancetomove < 0) {
        fileptr->location = fileptr->block.sizefile < (DWORD)-distancetomove ? 0 : fileptr->block.sizefile + distancetomove;
      } else {
        fileptr->location = fileptr->block.sizefile + (DWORD)distancetomove;
      }
      break;
  }
  fileptr->readaheadoffset = 0;
  fileptr->readaheadbytes = 0;
  return fileptr->location;
}

extern "C" BOOL APIENTRY SFileSetIoErrorMode(DWORD errormode, SFileIoErrorProc errorproc) {
  Storm::SFile::UseGlob glob;
  glob->s_ioerrormode = errormode;
  glob->s_ioerrorproc = errorproc;
  return TRUE;
}

extern "C" void APIENTRY SFileSetLocale(DWORD lcid) {
  Storm::SFile::UseGlob glob;
  glob->s_languageId = (WORD)lcid;
}

extern "C" WORD APIENTRY SFileGetLocale() {
  WORD lcid;

  Storm::SFile::UseGlob glob;
  lcid = glob->s_languageId;
  return lcid;
}

extern "C" void APIENTRY SFileSetPlatform(DWORD platformId) {
  Storm::SFile::UseGlob glob;
  glob->s_platformId = (BYTE)platformId;
}

extern "C" BOOL APIENTRY SFileUnloadFile(LPVOID buffer) {
  FATALASSERT(buffer);
  FREE(buffer);
  return TRUE;
}

extern "C" void APIENTRY SFileLoadDump() {
}

extern "C" void APIENTRY SFileArchiveDump() {
}
