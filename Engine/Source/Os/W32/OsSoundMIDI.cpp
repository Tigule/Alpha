#include "OsSound.h"

#include "Gx/Gx.h"
#include "Services/AsyncFileRead.h"

typedef unsigned long DWORD_PTR;

#include <dmusici.h>

struct ASYNCLOADER {
  CAsyncObject         *asyncLoader;
  TSGrowableArray<char> buffer;

  void Clear();
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
  virtual long __stdcall          SetObject(DMUS_OBJECTDESC *);
  virtual long __stdcall          SetSearchDirectory(const GUID &, wchar_t *, int);
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

static CMyLoader s_loader;

static IDirectMusicPerformance8 *s_dmusicPerformance;
static IDirectMusicSegment8     *s_dmusicSegment;
static IDirectMusicAudioPath    *s_dmusicPath;
static IDirectMusicCollection   *s_dmusicCollection;
static unsigned char s_comInitialized;
static unsigned char s_initialized;
static ASYNCLOADER  s_MID;
static ASYNCLOADER  s_DLS;

void __fastcall PostLoadCallback(void *userArg);

long __stdcall CMyIStream::Write(const void *, unsigned long, unsigned long *) {
  return E_NOTIMPL;
}

CMyLoader::~CMyLoader() {
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

static void __fastcall MIDI_CleanupSegment() {
  if (s_dmusicPerformance && s_dmusicSegment) {
    s_dmusicSegment->Unload(s_dmusicPerformance);

    if (s_dmusicCollection) {
      s_dmusicCollection->Release();
      s_dmusicCollection = 0;
    }

    s_dmusicSegment->Release();
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
  objDesc.pbMemData = reinterpret_cast<unsigned char *>(s_MID.buffer.Ptr());
  objDesc.pStream = 0;
  if (s_loader.GetObjectA(&objDesc, IID_IDirectMusicSegment8, reinterpret_cast<void **>(&s_dmusicSegment))) {
    MIDI_CleanupSegment();
    return;
  }

  objDesc.guidClass = CLSID_DirectMusicCollection;
  objDesc.asyncLoader = &s_DLS;
  objDesc.llMemLength = s_DLS.buffer.Count();
  objDesc.pbMemData = reinterpret_cast<unsigned char *>(s_DLS.buffer.Ptr());
  objDesc.pStream = 0;
  if (s_loader.GetObjectA(&objDesc, IID_IDirectMusicCollection, reinterpret_cast<void **>(&s_dmusicCollection))) {
    MIDI_CleanupSegment();
    return;
  }

  if (s_dmusicSegment->SetParam(GUID_ConnectToDLSCollection, -1, 0x80000000, 0, s_dmusicCollection)) {
    MIDI_CleanupSegment();
    return;
  }

  if (s_dmusicSegment->GetAudioPathConfig(reinterpret_cast<IUnknown **>(&s_dmusicPath))) {
    MIDI_CleanupSegment();
    return;
  }

  if (s_dmusicSegment->SetRepeats(-1)) {
    MIDI_CleanupSegment();
    return;
  }

  if (s_dmusicPerformance->PlaySegmentEx(s_dmusicSegment, 0, 0, 0, 0, 0, 0, s_dmusicPath)) {
    MIDI_CleanupSegment();
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

int __fastcall Sound::MIDI_Initialize() {
  if (CoInitialize(0) == S_OK) {
    s_comInitialized = 1;
  }

  HRESULT result = CoCreateInstance(
      CLSID_DirectMusicPerformance,
      0,
      CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
      IID_IDirectMusicPerformance8,
      reinterpret_cast<void **>(&s_dmusicPerformance)
  );
  if (result == S_OK) {
    result = s_dmusicPerformance->InitAudio(
        0,
        0,
        reinterpret_cast<HWND>(GxDevWindow()),
        0,
        0,
        63,
        0
    );
    if (result == S_OK) {
      result = s_dmusicPerformance->CreateStandardAudioPath(8, 16, true, &s_dmusicPath);
    }
  }
  s_initialized = result == S_OK;
  return s_initialized;
}

void __fastcall Sound::MIDI_Shutdown() {
  MIDI_Stop();
  MIDI_CleanupSegment();
  if (s_dmusicPerformance) {
    s_dmusicPerformance->CloseDown();
    s_dmusicPerformance->Release();
    s_dmusicPerformance = 0;
  }
  if (s_comInitialized) {
    CoUninitialize();
  }
  s_MID.Clear();
  s_DLS.Clear();
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
    s_dmusicPerformance->StopEx(s_dmusicSegment, 0, 0);
    MIDI_CleanupSegment();
  }
}

void __fastcall Sound::MIDI_SetVolume(float volume) {
  if (s_initialized) {
    ASSERT(volume >= 0.0f && volume <= 1.0f);
    s_dmusicPath->SetVolume(-9600 - static_cast<long>(volume * -9600.0f), 0);
  }
}

bool __fastcall Sound::MIDI_Playing() {
  return s_dmusicPerformance && s_dmusicPerformance->IsPlaying(0, 0);
}

CMyLoader::CMyLoader() : m_cRef(1) {
  wcscpy(m_wzSearchPath, L"");
}

long CMyLoader::Init() {
  return S_OK;
}

long __stdcall CMyLoader::GetObjectA(DMUS_OBJECTDESC *myDesc, const GUID &riid, void **ppv) {
  wchar_t         wzExt[256];
  DMUS_OBJECTDESC DESC;
  wchar_t         wzFileName[260];
  char            name[260];
  IDirectMusicObject *pObject = 0;
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
        if (!memcmp(pGUID, &CLSID_DirectMusicStyle, sizeof(*pGUID)) || !memcmp(pGUID, &CLSID_DirectSoundWave, sizeof(*pGUID)) ||
            !memcmp(pGUID, &CLSID_DirectMusicCollection, sizeof(*pGUID)))
        {
          memset(&DESC, 0, sizeof(DESC));
          DESC.dwSize = sizeof(DESC);
          pObject->GetDescriptor(&DESC);
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

long CMyIStream::Attach(const char *tzFile, IDirectMusicLoader *pLoader) {
  m_pLoader = pLoader;
  m_pLoader->AddRef();
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

long __stdcall CMyIStream::Clone(IStream **ppstm) {
  CMyIStream *stream = new CMyIStream;
  if (!stream) {
    return E_OUTOFMEMORY;
  }

  *ppstm = stream;
  return S_OK;
}

unsigned long __stdcall CMyLoader::Release() {
  long ref = InterlockedDecrement(&m_cRef);
  if (ref <= 0) {
    delete this;
  }
  return ref;
}

unsigned long __stdcall CMyLoader::AddRef() {
  return InterlockedIncrement(&m_cRef);
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

long __stdcall CMyIStream::GetLoader(IDirectMusicLoader **ppLoader) {
  m_pLoader->AddRef();
  *ppLoader = m_pLoader;
  return S_OK;
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
