#include <Base/Base.h>

#include "OsSound.h"

#include "Gx/Gx.h"
#include "Services/AsyncFileRead.h"

typedef DWORD DWORD_PTR;

// embeds guids into object file
#include <initguid.h>
#include <dmusici.h>

struct ASYNCLOADER {
  ASYNCLOADER() : asyncLoader(0) {
  }
  ASYNCLOADER(const ASYNCLOADER &);
  ~ASYNCLOADER();
  void Clear();
  CAsyncObject         *asyncLoader;
  TSGrowableArray<char> buffer;
};



class MY_DMUS_OBJECTDESC : public DMUS_OBJECTDESC {
 public:
  ASYNCLOADER *asyncLoader;
};

class CMyLoader : public IDirectMusicLoader {
 public:
  CMyLoader();
  ~CMyLoader();
  long                    Init();
  virtual long __stdcall  QueryInterface(const GUID &iid, LPVOID *ppv);
  virtual DWORD __stdcall AddRef();
  virtual DWORD __stdcall Release();
  virtual long __stdcall  GetObjectA(DMUS_OBJECTDESC *myDesc, const GUID &riid, LPVOID *ppv);
  virtual long __stdcall  SetSearchDirectory(const GUID &, wchar_t *, int);
  virtual long __stdcall  SetObject(DMUS_OBJECTDESC *);
  virtual long __stdcall  ScanDirectory(const GUID &, wchar_t *, wchar_t *);
  virtual long __stdcall  CacheObject(IDirectMusicObject *);
  virtual long __stdcall  ReleaseObject(IDirectMusicObject *);
  virtual long __stdcall  ClearCache(const GUID &);
  virtual long __stdcall  EnableCache(const GUID &, int);
  virtual long __stdcall  EnumObject(const GUID &, DWORD, DMUS_OBJECTDESC *);

 private:
  long    m_cRef;
  wchar_t m_wzSearchPath[MAX_PATH];
};

class CMyIStream : public IStream, public IDirectMusicGetLoader {
 public:
  CMyIStream() : m_cRef(1), m_pLoader(0), m_cursor(0) {
  }
  ~CMyIStream() {
    Detach();
  }
  long                    Attach(LPCSTR tzFile, IDirectMusicLoader *pLoader);
  void                    Detach();
  virtual long __stdcall  QueryInterface(const GUID &iid, LPVOID *ppv);
  virtual DWORD __stdcall AddRef();
  virtual DWORD __stdcall Release();
  virtual long __stdcall  GetLoader(IDirectMusicLoader **ppLoader);
  virtual long __stdcall  Read(LPVOID pv, DWORD cb, DWORD *pcb);
  virtual long __stdcall  Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *out);
  virtual long __stdcall  Clone(IStream **ppstm);
  virtual long __stdcall  Write(LPCVOID, DWORD, DWORD *);
  virtual long __stdcall  SetSize(ULARGE_INTEGER);
  virtual long __stdcall  CopyTo(IStream *, ULARGE_INTEGER, ULARGE_INTEGER *, ULARGE_INTEGER *);
  virtual long __stdcall  Commit(DWORD);
  virtual long __stdcall  Revert();
  virtual long __stdcall  LockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD);
  virtual long __stdcall  UnlockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD);
  virtual long __stdcall  Stat(STATSTG *, DWORD);

 private:
  long                m_cRef;
  IDirectMusicLoader *m_pLoader;
  LONGLONG            m_cursor;

 public:
  ASYNCLOADER *m_loader;
};

static CMyLoader s_loader;

static IDirectMusicPerformance8 *s_dmusicPerformance;
static IDirectMusicSegment8     *s_dmusicSegment;
static IDirectMusicAudioPath    *s_dmusicPath;
static IDirectMusicCollection   *s_dmusicCollection;
static BYTE                      s_comInitialized;
static BYTE                      s_initialized;
static ASYNCLOADER               s_MID;
static ASYNCLOADER               s_DLS;

static void PostLoadCallback(LPVOID userArg);

long __stdcall CMyIStream::Write(LPCVOID, DWORD, DWORD *) {
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

long __stdcall CMyIStream::Commit(DWORD) {
  return E_NOTIMPL;
}

long __stdcall CMyIStream::Revert() {
  return E_NOTIMPL;
}

long __stdcall CMyIStream::LockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD) {
  return E_NOTIMPL;
}

long __stdcall CMyIStream::UnlockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD) {
  return E_NOTIMPL;
}

long __stdcall CMyIStream::Stat(STATSTG *, DWORD) {
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

long __stdcall CMyLoader::CacheObject(IDirectMusicObject *) {
  return E_NOTIMPL;
}

long __stdcall CMyLoader::ReleaseObject(IDirectMusicObject *) {
  return E_NOTIMPL;
}

long __stdcall CMyLoader::ClearCache(const GUID &) {
  return E_NOTIMPL;
}

long __stdcall CMyLoader::EnableCache(const GUID &, int) {
  return E_NOTIMPL;
}

long __stdcall CMyLoader::EnumObject(const GUID &, DWORD, DMUS_OBJECTDESC *) {
  return E_NOTIMPL;
}

static void MIDI_CleanupSegment() {
  if (s_dmusicPerformance && s_dmusicSegment) {
    s_dmusicSegment->Unload(s_dmusicPerformance);

    if (s_dmusicCollection) {
      s_dmusicCollection->Release();
    }
    s_dmusicCollection = 0;

    if (s_dmusicSegment) {
      s_dmusicSegment->Release();
    }
    s_dmusicSegment = 0;
  }
}

static void PostLoadCallback(LPVOID userArg) {
  if (!s_MID.asyncLoader || !s_MID.asyncLoader->isLoaded || !s_DLS.asyncLoader || !s_DLS.asyncLoader->isLoaded) {
    return;
  }

  HRESULT            status;
  MY_DMUS_OBJECTDESC objDesc;
  objDesc.dwSize = sizeof(DMUS_OBJECTDESC);
  objDesc.dwValidData = 0x402;
  objDesc.guidClass = CLSID_DirectMusicSegment;
  objDesc.asyncLoader = &s_MID;
  objDesc.pbMemData = (BYTE *)s_MID.buffer.Ptr();
  objDesc.llMemLength = s_MID.buffer.Count();
  status = s_loader.GetObjectA(&objDesc, IID_IDirectMusicSegment8, (LPVOID *)&s_dmusicSegment);
  if (status) {
    goto finallylabel;
  }

  objDesc.guidClass = CLSID_DirectMusicCollection;
  objDesc.asyncLoader = &s_DLS;
  objDesc.pbMemData = (BYTE *)s_DLS.buffer.Ptr();
  objDesc.llMemLength = s_DLS.buffer.Count();
  status = s_loader.GetObjectA(&objDesc, IID_IDirectMusicCollection, (LPVOID *)&s_dmusicCollection);
  if (status) {
    goto finallylabel;
  }

  status = s_dmusicSegment->SetParam(GUID_ConnectToDLSCollection, -1, 0x80000000, 0, s_dmusicCollection);
  if (status) {
    goto finallylabel;
  }

  status = s_dmusicSegment->Download(s_dmusicPerformance);
  if (status) {
    goto finallylabel;
  }

  status = s_dmusicSegment->SetRepeats(-1);
  if (status) {
    goto finallylabel;
  }

  status = s_dmusicPerformance->PlaySegmentEx(s_dmusicSegment, 0, 0, 0, 0, 0, 0, s_dmusicPath);

finallylabel:
  if (status) {
    MIDI_CleanupSegment();
  }
}

static void InitLoader(ASYNCLOADER &loader, LPCSTR fileName) {
  loader.Clear();

  SFile *file = 0;
  if (!SFile::Open(fileName, &file)) {
    return;
  }

  CAsyncObject *asyncLoader = AsyncFileReadCreateObject();
  if (!asyncLoader) {
    SFile::Close(file);
    return;
  }

  loader.buffer.SetCount(SFile::GetFileSize(file, 0));
  asyncLoader->isLoaded = 0;
  asyncLoader->userPostloadCallback = PostLoadCallback;
  asyncLoader->file = file;
  asyncLoader->offset = 0;
  asyncLoader->size = SFile::GetFileSize(file, 0);
  asyncLoader->buffer = loader.buffer.Ptr();
  asyncLoader->userArg = 0;
  loader.asyncLoader = asyncLoader;
  AsyncFileReadObject(asyncLoader);
}

BOOL Sound::MIDI_Initialize() {
  if (CoInitialize(0) == S_OK) {
    s_comInitialized = 1;
  }

  HRESULT result = CoCreateInstance(
      CLSID_DirectMusicPerformance, 0, CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER, IID_IDirectMusicPerformance8,
      (LPVOID *)&s_dmusicPerformance
  );
  if (result == S_OK) {
    result = s_dmusicPerformance->InitAudio(0, 0, (HWND)GxDevWindow(), 0, 0, 63, 0);
    if (result == S_OK) {
      result = s_dmusicPerformance->CreateStandardAudioPath(8, 16, true, &s_dmusicPath);
    }
  }
  s_initialized = result == S_OK;
  return s_initialized;
}

void Sound::MIDI_Shutdown() {
  MIDI_Stop();
  MIDI_CleanupSegment();
  if (s_dmusicPerformance) {
    s_dmusicPerformance->CloseDown();
    if (s_dmusicPerformance) {
      s_dmusicPerformance->Release();
    }
    s_dmusicPerformance = 0;
  }
  if (s_comInitialized) {
    CoUninitialize();
  }
  s_MID.Clear();
  s_DLS.Clear();
}

void Sound::MIDI_Play(LPCSTR midiFilename, LPCSTR dlsFilename) {
  if (s_initialized && midiFilename && *midiFilename && dlsFilename && *dlsFilename) {
    InitLoader(s_MID, midiFilename);
    InitLoader(s_DLS, dlsFilename);
    MIDI_CleanupSegment();
  }
}

void Sound::MIDI_Stop() {
  if (s_initialized && s_dmusicSegment) {
    s_dmusicPerformance->StopEx(s_dmusicSegment, 0, 0);
    MIDI_CleanupSegment();
  }
}

void Sound::MIDI_SetVolume(float volume) {
  if (s_initialized) {
    ASSERT(volume >= 0.0f && volume <= 1.0f);
    s_dmusicPath->SetVolume(-9600 - (long)(volume * -9600.0f), 0);
  }
}

bool Sound::MIDI_Playing() {
  return s_dmusicPerformance && s_dmusicPerformance->IsPlaying(0, 0);
}

CMyLoader::CMyLoader() : m_cRef(1) {
  wcscpy(m_wzSearchPath, L"");
}

long CMyLoader::Init() {
  return S_OK;
}

long __stdcall CMyLoader::GetObjectA(DMUS_OBJECTDESC *myDesc, const GUID &riid, LPVOID *ppv) {
  IDirectMusicObject *pObject = 0;
  const GUID         *pGUID = &myDesc->guidClass;

  long status = CoCreateInstance(*pGUID, 0, CLSCTX_INPROC_SERVER, IID_IDirectMusicObject, (LPVOID *)&pObject);
  if (status >= 0) {
    if (myDesc->dwValidData & 0x400) {
      wchar_t wzFileName[MAX_PATH];
      wchar_t wzExt[256];

      _wmakepath(wzFileName, 0, m_wzSearchPath, myDesc->wszFileName, 0);
      _wsplitpath(wzFileName, 0, 0, 0, wzExt);

      CMyIStream *stream = new CMyIStream;
      if (!stream) {
        status = E_OUTOFMEMORY;
      }
      stream->m_loader = ((MY_DMUS_OBJECTDESC *)myDesc)->asyncLoader;
      if (status >= 0) {
        char name[MAX_PATH];

        wcstombs(name, wzFileName, sizeof(name));
        status = stream->Attach(name, this);
      }

      IPersistStream *pPersistStream = 0;
      if (status >= 0) {
        status = pObject->QueryInterface(IID_IPersistStream, (LPVOID *)&pPersistStream);
        if (status >= 0) {
          status = pPersistStream->Load(stream);
        }
      }
      stream->Release();
      if (pPersistStream) {
        pPersistStream->Release();
      }
    } else {
      status = E_FAIL;
    }

    if (status >= 0) {
      if (!memcmp(pGUID, &CLSID_DirectMusicStyle, sizeof(*pGUID)) || !memcmp(pGUID, &CLSID_DirectSoundWave, sizeof(*pGUID)) ||
          !memcmp(pGUID, &CLSID_DirectMusicCollection, sizeof(*pGUID)))
      {
        DMUS_OBJECTDESC DESC;

        memset(&DESC, 0, sizeof(DESC));
        DESC.dwSize = sizeof(DESC);
        pObject->GetDescriptor(&DESC);
      }
      status = pObject->QueryInterface(riid, ppv);
    }
    if (pObject) {
      pObject->Release();
    }
  }
  return status;
}

long CMyIStream::Attach(LPCSTR tzFile, IDirectMusicLoader *pLoader) {
  m_pLoader = pLoader;
  m_pLoader->AddRef();
  return S_OK;
}

DWORD __stdcall CMyIStream::Release() {
  if (!InterlockedDecrement(&m_cRef)) {
    delete this;
    return 0;
  }
  return m_cRef;
}

DWORD __stdcall CMyIStream::AddRef() {
  return InterlockedIncrement(&m_cRef);
}

long __stdcall CMyIStream::QueryInterface(const GUID &iid, LPVOID *ppv) {
  *ppv = 0;
  if (!memcmp(&iid, &IID_IUnknown, sizeof(iid)) || !memcmp(&iid, &IID_ISequentialStream, sizeof(iid)) || !memcmp(&iid, &IID_IStream, sizeof(iid))) {
    *ppv = this;
  } else if (!memcmp(&iid, &IID_IDirectMusicGetLoader, sizeof(iid))) {
    *ppv = (IDirectMusicGetLoader *)this;
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

long __stdcall CMyIStream::Read(LPVOID pv, DWORD cb, DWORD *pcb) {
  if (!m_loader->asyncLoader->buffer || m_loader->buffer.Count() < (DWORDLONG)(m_cursor + cb)) {
    return E_FAIL;
  }

  BYTE *destination = (BYTE *)pv;
  BYTE *source = (BYTE *)m_loader->buffer.Ptr() + (DWORD)m_cursor;
  for (DWORD i = 0; i < cb; ++i) {
    *destination++ = *source++;
  }
  if (pcb) {
    *pcb = cb;
  }
  m_cursor += cb;
  return S_OK;
}

long __stdcall CMyIStream::Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *out) {
  DWORDLONG origin = 0;
  if (dwOrigin != STREAM_SEEK_SET) {
    if (dwOrigin == STREAM_SEEK_CUR) {
      origin = m_cursor;
    } else if (dwOrigin == STREAM_SEEK_END) {
      origin = m_loader->buffer.Count();
    } else {
      FATALASSERT(0);
    }
  }

  m_cursor = origin + dlibMove.QuadPart;
  if (out) {
    out->QuadPart = m_cursor;
  }
  if ((int)m_loader->buffer.Count() < m_cursor) {
    return E_FAIL;
  }
  return S_OK;
}

long __stdcall CMyIStream::Clone(IStream **ppstm) {
  CMyIStream *stream = new CMyIStream;
  if (!stream) {
    return E_OUTOFMEMORY;
  }

  *ppstm = stream;
  return S_OK;
}

DWORD __stdcall CMyLoader::Release() {
  long ref = InterlockedDecrement(&m_cRef);
  if (ref <= 0) {
    delete this;
  }
  return ref;
}

DWORD __stdcall CMyLoader::AddRef() {
  return InterlockedIncrement(&m_cRef);
}

long __stdcall CMyLoader::QueryInterface(const GUID &iid, LPVOID *ppv) {
  *ppv = 0;
  if (memcmp(&iid, &IID_IUnknown, sizeof(iid)) && memcmp(&iid, &IID_IDirectMusicLoader, sizeof(iid))) {
    return E_NOINTERFACE;
  }

  *ppv = this;
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

ASYNCLOADER::~ASYNCLOADER() {
  buffer.Clear();
  Clear();
}
