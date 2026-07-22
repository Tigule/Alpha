#include "OsSound.h"

#include "Services/AsyncFileRead.h"

struct ASYNCLOADER {
  CAsyncObject         *asyncLoader;
  TSGrowableArray<char> buffer;

  void Clear();
};

struct _DMUS_VERSION {
  unsigned long dwVersionMS;
  unsigned long dwVersionLS;
};

struct _DMUS_OBJECTDESC {
  unsigned long dwSize;
  unsigned long dwValidData;
  GUID          guidObject;
  GUID          guidClass;
  FILETIME      ftDate;
  _DMUS_VERSION vVersion;
  wchar_t       wszName[64];
  wchar_t       wszCategory[64];
  wchar_t       wszFileName[260];
  __int64       llMemLength;
  unsigned int *pbMemData;
  void         *pStream;
};

typedef _DMUS_OBJECTDESC DMUS_OBJECTDESC;

struct IDirectMusicObject : public IUnknown {};

struct IDirectMusicLoader : public IUnknown {
  virtual long __stdcall GetObjectA(DMUS_OBJECTDESC *myDesc, const GUID &riid, void **ppv) = 0;
  virtual long __stdcall SetSearchDirectory(const GUID &, wchar_t *, int) = 0;
  virtual long __stdcall SetObject(DMUS_OBJECTDESC *) = 0;
  virtual long __stdcall ScanDirectory(const GUID &, wchar_t *, wchar_t *) = 0;
  virtual long __stdcall CacheObject(IDirectMusicObject *) = 0;
  virtual long __stdcall ReleaseObject(IDirectMusicObject *) = 0;
  virtual long __stdcall ClearCache(const GUID &) = 0;
  virtual long __stdcall EnableCache(const GUID &, int) = 0;
  virtual long __stdcall EnumObject(const GUID &, unsigned long, DMUS_OBJECTDESC *) = 0;
};

struct IDirectMusicGetLoader : public IUnknown {
  virtual long __stdcall GetLoader(IDirectMusicLoader **ppLoader) = 0;
};

class MY_DMUS_OBJECTDESC : public DMUS_OBJECTDESC {
 public:
  ASYNCLOADER *asyncLoader;
};

class CMyLoader : public IDirectMusicLoader {
 public:
  CMyLoader();
  ~CMyLoader();
  long                            Init();
  virtual long __stdcall          QueryInterface(const GUID &iid, void **ppv);
  virtual unsigned long __stdcall AddRef();
  virtual unsigned long __stdcall Release();
  virtual long __stdcall          GetObjectA(DMUS_OBJECTDESC *myDesc, const GUID &riid, void **ppv);
  virtual long __stdcall          SetSearchDirectory(const GUID &, wchar_t *, int);
  virtual long __stdcall          SetObject(DMUS_OBJECTDESC *);
  virtual long __stdcall          ScanDirectory(const GUID &, wchar_t *, wchar_t *);
  virtual long __stdcall          CacheObject(IDirectMusicObject *__formal);
  virtual long __stdcall          ReleaseObject(IDirectMusicObject *__formal);
  virtual long __stdcall          ClearCache(const GUID &);
  virtual long __stdcall          EnableCache(const GUID &, int);
  virtual long __stdcall          EnumObject(const GUID &, unsigned long, DMUS_OBJECTDESC *);

 private:
  long    m_cRef;
  wchar_t m_wzSearchPath[260];
};

class CMyIStream : public IStream, public IDirectMusicGetLoader {
 public:
  CMyIStream() : m_cRef(1), m_pLoader(0), m_cursor(0), m_loader(0) {
  }
  ~CMyIStream() {
    Detach();
  }
  long                            Attach(const char *tzFile, IDirectMusicLoader *pLoader);
  void                            Detach();
  virtual long __stdcall          QueryInterface(const GUID &iid, void **ppv);
  virtual unsigned long __stdcall AddRef();
  virtual unsigned long __stdcall Release();
  virtual long __stdcall          Read(void *pv, unsigned long cb, unsigned long *pcb);
  virtual long __stdcall          Write(const void *, unsigned long, unsigned long *);
  virtual long __stdcall          Seek(LARGE_INTEGER dlibMove, unsigned long dwOrigin, ULARGE_INTEGER *out);
  virtual long __stdcall          SetSize(ULARGE_INTEGER);
  virtual long __stdcall          CopyTo(IStream *, ULARGE_INTEGER, ULARGE_INTEGER *, ULARGE_INTEGER *);
  virtual long __stdcall          Commit(unsigned long __formal);
  virtual long __stdcall          Revert();
  virtual long __stdcall          LockRegion(ULARGE_INTEGER, ULARGE_INTEGER, unsigned long);
  virtual long __stdcall          UnlockRegion(ULARGE_INTEGER, ULARGE_INTEGER, unsigned long);
  virtual long __stdcall          Stat(STATSTG *, unsigned long);
  virtual long __stdcall          Clone(IStream **ppstm);
  virtual long __stdcall          GetLoader(IDirectMusicLoader **ppLoader);

 private:
  long                m_cRef;
  IDirectMusicLoader *m_pLoader;
  __int64             m_cursor;

 public:
  ASYNCLOADER *m_loader;
};

static const GUID CLSID_DirectMusicSegment = {
    0xd2ac2882,
    0xb39b,
    0x11d1,
    {0x87, 0x04, 0x00, 0x60, 0x08, 0x93, 0xb1, 0xbd}
};
static const GUID CLSID_DirectMusicCollection = {
    0x480ff4b0,
    0x28b2,
    0x11d1,
    {0xbe, 0xf7, 0x00, 0xc0, 0x4f, 0xbf, 0x8f, 0xef}
};
static const GUID IID_IDirectMusicSegment8 = {
    0xc6784488,
    0x41a3,
    0x418f,
    {0xaa, 0x15, 0xb3, 0x50, 0x93, 0xba, 0x42, 0xd4}
};
static const GUID IID_IDirectMusicCollection = {
    0xd2ac287c,
    0xb39b,
    0x11d1,
    {0x87, 0x04, 0x00, 0x60, 0x08, 0x93, 0xb1, 0xbd}
};
static const GUID GUID_Download = {
    0x1db1ae6b,
    0xe92e,
    0x11d1,
    {0xa8, 0xc5, 0x00, 0xc0, 0x4f, 0xa3, 0x72, 0x6e}
};
static const GUID IID_IDirectMusicObject = {
    0xd2ac28b5,
    0xb39b,
    0x11d1,
    {0x87, 0x04, 0x00, 0x60, 0x08, 0x93, 0xb1, 0xbd}
};
static const GUID IID_IDirectMusicGetLoader = {
    0x68a04844,
    0xd13d,
    0x11d1,
    {0xaf, 0xa6, 0x00, 0xaa, 0x00, 0x24, 0xd8, 0xb6}
};
static const GUID IID_IDirectMusicLoader = {
    0x2ffaaca2,
    0x5dca,
    0x11d2,
    {0xaf, 0xa6, 0x00, 0xaa, 0x00, 0x24, 0xd8, 0xb6}
};
static const GUID CLSID_DirectMusicSegmentState = {
    0xd2ac288a,
    0xb39b,
    0x11d1,
    {0x87, 0x04, 0x00, 0x60, 0x08, 0x93, 0xb1, 0xbd}
};
static const GUID CLSID_DirectMusicWaveTrack = {
    0x8a667154,
    0xf9cb,
    0x11d2,
    {0xad, 0x8a, 0x00, 0x60, 0xb0, 0x57, 0x5a, 0xbc}
};

static CMyLoader s_loader;

static void        *s_dmusicPerformance;
static void        *s_dmusicSegment;
static void        *s_dmusicPath;
static void        *s_dmusicCollection;
static unsigned int s_initialized;
static ASYNCLOADER  s_MID;
static ASYNCLOADER  s_DLS;

void __fastcall PostLoadCallback(void *userArg);

CMyLoader::CMyLoader() : m_cRef(1) {
  wcscpy(m_wzSearchPath, L"");
}

CMyLoader::~CMyLoader() {
}

long CMyLoader::Init() {
  return S_OK;
}

long __stdcall CMyLoader::QueryInterface(const GUID &iid, void **ppv) {
  *ppv = 0;
  if (memcmp(&iid, &IID_IUnknown, sizeof(iid)) && memcmp(&iid, &IID_IDirectMusicLoader, sizeof(iid))) {
    return E_NOINTERFACE;
  }

  *ppv = static_cast<IDirectMusicLoader *>(this);
  AddRef();
  return S_OK;
}

unsigned long __stdcall CMyLoader::AddRef() {
  return InterlockedIncrement(&m_cRef);
}

unsigned long __stdcall CMyLoader::Release() {
  long ref = InterlockedDecrement(&m_cRef);
  if (ref <= 0) {
    delete this;
  }
  return ref;
}

long __stdcall CMyLoader::SetSearchDirectory(const GUID &, wchar_t *, int) {
  return E_NOTIMPL;
}

long __stdcall CMyLoader::SetObject(DMUS_OBJECTDESC *) {
  return E_NOTIMPL;
}

long __stdcall CMyLoader::ScanDirectory(const GUID &, wchar_t *, wchar_t *) {
  return E_NOTIMPL;
}

long __stdcall CMyLoader::CacheObject(IDirectMusicObject *__formal) {
  return E_NOTIMPL;
}

long __stdcall CMyLoader::ReleaseObject(IDirectMusicObject *__formal) {
  return E_NOTIMPL;
}

long __stdcall CMyLoader::ClearCache(const GUID &) {
  return E_NOTIMPL;
}

long __stdcall CMyLoader::EnableCache(const GUID &, int) {
  return E_NOTIMPL;
}

long __stdcall CMyLoader::EnumObject(const GUID &, unsigned long, DMUS_OBJECTDESC *) {
  return E_NOTIMPL;
}

long CMyIStream::Attach(const char *tzFile, IDirectMusicLoader *pLoader) {
  m_pLoader = pLoader;
  m_pLoader->AddRef();
  return S_OK;
}

void CMyIStream::Detach() {
  if (m_pLoader) {
    m_pLoader->Release();
  }
  m_pLoader = 0;
}

long __stdcall CMyIStream::Read(void *pv, unsigned long cb, unsigned long *pcb) {
  if (!m_loader->asyncLoader->buffer || m_loader->buffer.Count() < m_cursor + cb) {
    return E_FAIL;
  }

  unsigned char *source = reinterpret_cast<unsigned char *>(m_loader->buffer.Ptr()) + static_cast<unsigned long>(m_cursor);
  unsigned char *destination = static_cast<unsigned char *>(pv);
  for (unsigned long i = 0; i < cb; ++i) {
    *destination++ = *source++;
  }
  if (pcb) {
    *pcb = cb;
  }
  m_cursor += cb;
  return S_OK;
}

long __stdcall CMyIStream::Write(const void *, unsigned long, unsigned long *) {
  return E_NOTIMPL;
}

long __stdcall CMyIStream::Seek(LARGE_INTEGER dlibMove, unsigned long dwOrigin, ULARGE_INTEGER *out) {
  unsigned __int64 origin = 0;
  if (dwOrigin == STREAM_SEEK_CUR) {
    origin = m_cursor;
  } else if (dwOrigin == STREAM_SEEK_END) {
    origin = m_loader->buffer.Count();
  } else if (dwOrigin != STREAM_SEEK_SET) {
    FATALASSERT(0);
  }

  m_cursor = origin + dlibMove.QuadPart;
  if (out) {
    out->QuadPart = m_cursor;
  }
  return m_cursor <= m_loader->buffer.Count() ? S_OK : E_FAIL;
}

long __stdcall CMyIStream::SetSize(ULARGE_INTEGER) {
  return E_NOTIMPL;
}

long __stdcall CMyIStream::CopyTo(IStream *, ULARGE_INTEGER, ULARGE_INTEGER *, ULARGE_INTEGER *) {
  return E_NOTIMPL;
}

long __stdcall CMyIStream::Commit(unsigned long __formal) {
  return E_NOTIMPL;
}

long __stdcall CMyIStream::Revert() {
  return E_NOTIMPL;
}

long __stdcall CMyIStream::LockRegion(ULARGE_INTEGER, ULARGE_INTEGER, unsigned long) {
  return E_NOTIMPL;
}

long __stdcall CMyIStream::UnlockRegion(ULARGE_INTEGER, ULARGE_INTEGER, unsigned long) {
  return E_NOTIMPL;
}

long __stdcall CMyIStream::Stat(STATSTG *, unsigned long) {
  return E_NOTIMPL;
}

long __stdcall CMyIStream::Clone(IStream **ppstm) {
  CMyIStream *stream = new CMyIStream;
  if (!stream) {
    return E_OUTOFMEMORY;
  }

  *ppstm = stream;
  return S_OK;
}

unsigned long __stdcall CMyIStream::Release() {
  long ref = InterlockedDecrement(&m_cRef);
  if (!ref) {
    delete this;
    return 0;
  }
  return ref;
}

unsigned long __stdcall CMyIStream::AddRef() {
  return InterlockedIncrement(&m_cRef);
}

long __stdcall CMyIStream::QueryInterface(const GUID &iid, void **ppv) {
  *ppv = 0;
  if (!memcmp(&iid, &IID_IUnknown, sizeof(iid)) || !memcmp(&iid, &IID_ISequentialStream, sizeof(iid)) || !memcmp(&iid, &IID_IStream, sizeof(iid))) {
    *ppv = static_cast<IStream *>(this);
  } else if (!memcmp(&iid, &IID_IDirectMusicGetLoader, sizeof(iid))) {
    *ppv = static_cast<IDirectMusicGetLoader *>(this);
  } else {
    return E_NOINTERFACE;
  }

  AddRef();
  return S_OK;
}

long __stdcall CMyIStream::GetLoader(IDirectMusicLoader **ppLoader) {
  m_pLoader->AddRef();
  *ppLoader = m_pLoader;
  return S_OK;
}

long __stdcall CMyLoader::GetObjectA(DMUS_OBJECTDESC *myDesc, const GUID &riid, void **ppv) {
  wchar_t         wzExt[256];
  DMUS_OBJECTDESC DESC;
  wchar_t         wzFileName[260];
  char            name[260];
  IUnknown       *pObject = 0;
  IPersistStream *pPersistStream = 0;
  const GUID     *pGUID = &myDesc->guidClass;

  long status = CoCreateInstance(*pGUID, 0, CLSCTX_INPROC_SERVER, IID_IDirectMusicObject, reinterpret_cast<void **>(&pObject));
  if (status >= 0) {
    if (myDesc->dwValidData & 0x400) {
      _wmakepath(wzFileName, 0, m_wzSearchPath, myDesc->wszFileName, 0);
      _wsplitpath(wzFileName, 0, 0, 0, wzExt);

      CMyIStream *stream = new CMyIStream;
      if (!stream) {
        status = E_OUTOFMEMORY;
      }
      stream->m_loader = static_cast<MY_DMUS_OBJECTDESC *>(myDesc)->asyncLoader;
      if (status >= 0) {
        wcstombs(name, wzFileName, sizeof(name));
        status = stream->Attach(name, this);
      }
      if (status >= 0) {
        status = pObject->QueryInterface(IID_IPersistStream, reinterpret_cast<void **>(&pPersistStream));
        if (status >= 0) {
          status = pPersistStream->Load(stream);
        }
      }
      stream->Release();
      if (pPersistStream) {
        pPersistStream->Release();
      }

      if (status >= 0) {
        if (!memcmp(pGUID, &CLSID_DirectMusicSegmentState, sizeof(*pGUID)) || !memcmp(pGUID, &CLSID_DirectMusicWaveTrack, sizeof(*pGUID)) ||
            !memcmp(pGUID, &CLSID_DirectMusicCollection, sizeof(*pGUID)))
        {
          memset(&DESC, 0, sizeof(DESC));
          DESC.dwSize = sizeof(DESC);
          typedef long(__stdcall * GetDescriptorFn)(void *, DMUS_OBJECTDESC *);
          GetDescriptorFn getDescriptor = *reinterpret_cast<GetDescriptorFn *>(*reinterpret_cast<unsigned char **>(pObject) + 12);
          getDescriptor(pObject, &DESC);
        }
        status = pObject->QueryInterface(riid, ppv);
      }
    } else {
      status = E_FAIL;
    }
    pObject->Release();
  }
  return status;
}

void ASYNCLOADER::Clear() {
  if (asyncLoader) {
    SFile *file = asyncLoader->file;
    AsyncFileReadDestroyObject(asyncLoader);
    asyncLoader = 0;
    if (file) {
      SFile::Close(file);
    }
  }
}

static void __fastcall InitLoader(ASYNCLOADER &loader, const char *fileName) {
  loader.Clear();

  SFile *file = 0;
  if (SFile::Open(fileName, &file)) {
    CAsyncObject *asyncLoader = AsyncFileReadCreateObject();
    if (asyncLoader) {
      unsigned int size = SFile::GetFileSize(file, 0);
      loader.buffer.SetCount(size);
      asyncLoader->isLoaded = 0;
      asyncLoader->userPostloadCallback = PostLoadCallback;
      asyncLoader->file = file;
      asyncLoader->offset = 0;
      asyncLoader->size = SFile::GetFileSize(file, 0);
      asyncLoader->buffer = loader.buffer.Ptr();
      asyncLoader->userArg = 0;
      loader.asyncLoader = asyncLoader;
      AsyncFileReadObject(asyncLoader);
    } else {
      SFile::Close(file);
    }
  }
}

static void __fastcall MIDI_CleanupSegment() {
  if (s_dmusicPerformance && s_dmusicSegment) {
    typedef long(__stdcall * UnloadFn)(void *, void *);
    UnloadFn unload = *reinterpret_cast<UnloadFn *>(*reinterpret_cast<unsigned char **>(s_dmusicSegment) + 120);
    unload(s_dmusicSegment, s_dmusicPerformance);

    typedef unsigned long(__stdcall * ReleaseFn)(void *);
    if (s_dmusicCollection) {
      ReleaseFn releaseCollection = *reinterpret_cast<ReleaseFn *>(*reinterpret_cast<unsigned char **>(s_dmusicCollection) + 8);
      releaseCollection(s_dmusicCollection);
      s_dmusicCollection = 0;
    }

    ReleaseFn releaseSegment = *reinterpret_cast<ReleaseFn *>(*reinterpret_cast<unsigned char **>(s_dmusicSegment) + 8);
    releaseSegment(s_dmusicSegment);
    s_dmusicSegment = 0;
  }
}

void __fastcall PostLoadCallback(void *userArg) {
  if (!s_MID.asyncLoader || !s_MID.asyncLoader->isLoaded || !s_DLS.asyncLoader || !s_DLS.asyncLoader->isLoaded) {
    return;
  }

  MY_DMUS_OBJECTDESC objDesc;
  objDesc.dwSize = sizeof(DMUS_OBJECTDESC);
  objDesc.dwValidData = 0x402;
  objDesc.guidClass = CLSID_DirectMusicSegment;
  objDesc.asyncLoader = &s_MID;
  objDesc.llMemLength = s_MID.buffer.Count();
  objDesc.pbMemData = reinterpret_cast<unsigned int *>(s_MID.buffer.Ptr());
  objDesc.pStream = 0;
  if (s_loader.GetObjectA(&objDesc, IID_IDirectMusicSegment8, &s_dmusicSegment)) {
    MIDI_CleanupSegment();
    return;
  }

  objDesc.guidClass = CLSID_DirectMusicCollection;
  objDesc.asyncLoader = &s_DLS;
  objDesc.llMemLength = s_DLS.buffer.Count();
  objDesc.pbMemData = reinterpret_cast<unsigned int *>(s_DLS.buffer.Ptr());
  objDesc.pStream = 0;
  if (s_loader.GetObjectA(&objDesc, IID_IDirectMusicCollection, &s_dmusicCollection)) {
    MIDI_CleanupSegment();
    return;
  }

  typedef long(__stdcall * SetParamFn)(void *, const GUID *, long, unsigned long, unsigned long, void *);
  SetParamFn setParam = *reinterpret_cast<SetParamFn *>(*reinterpret_cast<unsigned char **>(s_dmusicSegment) + 76);
  if (setParam(s_dmusicSegment, &GUID_Download, -1, 0x80000000, 0, s_dmusicCollection)) {
    MIDI_CleanupSegment();
    return;
  }

  typedef long(__stdcall * GetAudioPathConfigFn)(void *, void **);
  GetAudioPathConfigFn getAudioPathConfig = *reinterpret_cast<GetAudioPathConfigFn *>(*reinterpret_cast<unsigned char **>(s_dmusicSegment) + 116);
  if (getAudioPathConfig(s_dmusicSegment, &s_dmusicPath)) {
    MIDI_CleanupSegment();
    return;
  }

  typedef long(__stdcall * SetRepeatsFn)(void *, long);
  SetRepeatsFn setRepeats = *reinterpret_cast<SetRepeatsFn *>(*reinterpret_cast<unsigned char **>(s_dmusicSegment) + 24);
  if (setRepeats(s_dmusicSegment, -1)) {
    MIDI_CleanupSegment();
    return;
  }

  typedef long(__stdcall * PlaySegmentExFn)(void *, void *, void *, void *, unsigned long, __int64, void *, void *, void *);
  PlaySegmentExFn playSegment = *reinterpret_cast<PlaySegmentExFn *>(*reinterpret_cast<unsigned char **>(s_dmusicPerformance) + 180);
  if (playSegment(s_dmusicPerformance, s_dmusicSegment, 0, 0, 0, 0, 0, 0, s_dmusicPath)) {
    MIDI_CleanupSegment();
  }
}

void __fastcall Sound::MIDI_Play(const char *midiFilename, const char *dlsFilename) {
  if (s_initialized && midiFilename && *midiFilename && dlsFilename && *dlsFilename) {
    InitLoader(s_MID, midiFilename);
    InitLoader(s_DLS, dlsFilename);
    MIDI_CleanupSegment();
  }
}

void __fastcall Sound::MIDI_Stop() {
  if (s_initialized && s_dmusicSegment) {
    typedef long(__stdcall * StopFn)(void *, void *, void *, unsigned long, unsigned long);
    StopFn stop = *reinterpret_cast<StopFn *>(*reinterpret_cast<unsigned char **>(s_dmusicPerformance) + 184);
    stop(s_dmusicPerformance, s_dmusicSegment, 0, 0, 0);
    MIDI_CleanupSegment();
  }
}

void __fastcall Sound::MIDI_SetVolume(float volume) {
  if (s_initialized) {
    ASSERT(volume >= 0.0f && volume <= 1.0f);
    typedef long(__stdcall * SetVolumeFn)(void *, long, unsigned long);
    SetVolumeFn setVolume = *reinterpret_cast<SetVolumeFn *>(*reinterpret_cast<unsigned char **>(s_dmusicPath) + 20);
    setVolume(s_dmusicPath, -9600 - static_cast<long>(volume * -9600.0f), 0);
  }
}
