#include <winsock2.h>

#include "WowConnection.h"

#include "Base/CDataStore.h"
#include "Os/OsTime.h"
#include "WDataStore.h"
#include "WowConnectionNet.h"

#include <new>
#include <string.h>

const float PI = 3.14159265358979323846f;
const float TWO_PI = PI + PI;
const float OO_TWO_PI = 1.0f / TWO_PI;

static unsigned int s_destroyed;
bool(__fastcall *WowConnection::m_verifyAddr)(const NETADDR *);
static WowConnectionNet *s_network;

class WowConnectionInitializer {
  static unsigned int count;

 public:
  WowConnectionInitializer() {
    if (++count == 1) {
      Initialize();
    }
  }

  ~WowConnectionInitializer() {
    if (--count == 0) {
      Destroy();
    }
  }

  static void __fastcall Initialize();
  static void __fastcall Destroy();
};

unsigned int                    WowConnectionInitializer::count;
static WowConnectionInitializer s_initializer;

int WowConnection::CreateSocket() {
  return socket(AF_INET, SOCK_STREAM, 0);
}
void WowConnection::CloseSocket(int sock) {
  closesocket(sock);
}

void __fastcall RegisterSocket(int sock) {
  (void)sock;
}
WowConnection::WowConnection(int sock, sockaddr_in *addr, WowConnectionResponse *response) {
  int len;

  Init(response, 0);

  len = sizeof(sockaddr_in);
  getpeername(sock, reinterpret_cast<sockaddr *>(&m_peer), &len);
  len = sizeof(sockaddr_in);
  getsockname(sock, reinterpret_cast<sockaddr *>(&m_peer.selfAddr), &len);
  m_sock = sock;
  m_connState = WOWC_CONNECTED;

  (void)addr;
}

WowConnection::WowConnection(WowConnectionResponse *response, void(__fastcall *func)()) {
  Init(response, func);
}

WowConnection::~WowConnection() {
  ASSERT(m_refCount == 0);

  if (s_network) {
    s_network->Remove(this);
    s_network->PlatformDestruct(this);
  }

  if (m_sock >= 0) {
    CloseSocket(m_sock);
    m_sock = -1;
  }

  FREEIFUSED(m_readBuffer);

  while (SENDNODE *sn = m_sendList.Head()) {
    m_sendList.UnlinkNode(sn);
    FreeSendNode(sn);
  }
}

int WowConnection::AddRef() {
  return SInterlockedIncrement(reinterpret_cast<long *>(&m_refCount));
}

int WowConnection::Release() {
  int ref = SInterlockedDecrement(reinterpret_cast<long *>(&m_refCount));

  if (ref <= 0) {
    if (s_network) {
      s_network->Delete(this);
    } else {
      DEL(this);
    }
  }

  return ref;
}

void WowConnection::Init(WowConnectionResponse *response, void(__fastcall *func)()) {
  m_refCount = 1;
  m_responseRef = 0;
  m_sendDepth = 0;
  m_sendDepthBytes = 0;
  m_oldsock = -1;
  m_wantWriteNotification = 0;
  SetState(WOWC_UNINITIALIZED);
  m_threadInit = func;
  m_response = response;
  m_connectionFreed = 0;
  m_needBytes = 0;

  memset(&m_stats, 0, sizeof(m_stats));
  m_stats.m_connectTime = OsGetTime();
  m_stats.m_lastDataReceived = OsGetTime();

  m_haveSizeBytes = 0;
  m_listenPort = 0;
  m_retryConnection = 0;
  m_connectAddress = 0;
  m_connectPort = 0;
  m_connectRetryInterval = -1;
  memset(&m_peer, 0, sizeof(m_peer));
  m_bufferAutoSendSize = 0;
  m_serviceFlags = 0;
  m_serviceCount = 0;
  m_readBuffer = 0;
  m_readBytes = 0;
  m_readBufferSize = 0;
  m_event = 0;
  SetState(WOWC_INITIALIZED);
  m_type = WOWC_TYPE_MESSAGES;
}

void WowConnection::Disconnect() {
  m_lock.Enter();

  if (m_sock >= 0 && m_connState == WOWC_CONNECTED) {
    SetState(WOWC_DISCONNECTING);
  }

  m_lock.Leave();
}

void WowConnection::DoDisconnect() {
  WowConnectionResponse *response;
  int                    sock;

  m_lock.Enter();

  if (m_sock >= 0) {
    CloseSocket(m_sock);
  }

  SetState(WOWC_DISCONNECTED);
  AddRef();
  AcquireResponseRef();
  response = m_response;
  sock = m_sock;
  m_lock.Leave();

  if (response && sock >= 0) {
    response->WCDisconnected(this, OsGetAsyncTimeMs(), &m_peer);
  }

  m_lock.Enter();
  m_sock = -1;
  ReleaseResponseRef();
  m_lock.Leave();
  Release();
}

WowConnection::SENDNODE *WowConnection::NewSendNode(void *data, int size, bool raw) {
  SENDNODE *sn = static_cast<SENDNODE *>(WDataStore::AllocBuffer(size + sizeof(SENDNODE) + 3));

  if (sn) {
    new (sn) SENDNODE(reinterpret_cast<unsigned char *>(sn + 1), size, data, raw);
  }

  return sn;
}

void WowConnection::FreeSendNode(SENDNODE *sn) {
  WDataStore::FreeBuffer(sn, sn->datasize + sizeof(SENDNODE) + 3);
}

WC_SEND_RESULT WowConnection::Send(CDataStore *msg) {
  unsigned int size = msg->Size();
  void        *data;

  msg->GetDataInSitu(data, size);
  SENDNODE      *sn = NewSendNode(data, size, false);
  WC_SEND_RESULT result = WC_SEND_ERROR;

  m_lock.Enter();

  if (m_connState == WOWC_CONNECTED) {
    ASSERT(m_sock >= 0);

    if (!m_sendList.IsEmpty()) {
      m_sendList.LinkNode(sn, LIST_LINK_BEFORE, 0);
      ++m_sendDepth;
      m_sendDepthBytes += sn->size;
      s_network->PlatformChangeState(this, m_connState);
      m_lock.Leave();
      return WC_SEND_QUEUED;
    }

    int sent = send(m_sock, reinterpret_cast<const char *>(sn->data), sn->size, 0);

    if (sent == sn->size) {
      FreeSendNode(sn);
      m_lock.Leave();
      return WC_SEND_SENT;
    }

    if (sent > 0) {
      sn->offset += sent;
      result = WC_SEND_QUEUED;
    } else if (WSAGetLastError() != WSAEWOULDBLOCK) {
      SetState(WOWC_DISCONNECTING);
      result = WC_SEND_ERROR;
    }

    m_sendList.LinkNode(sn, LIST_LINK_BEFORE, 0);
    ++m_sendDepth;
    m_sendDepthBytes += sn->size;

    if (m_sendDepth >= 100000U) {
      SetState(WOWC_DISCONNECTING);
      result = WC_SEND_ERROR;
    }

    s_network->PlatformChangeState(this, m_connState);
    m_lock.Leave();
    return result;
  }

  FreeSendNode(sn);
  m_lock.Leave();
  return WC_SEND_ERROR;
}

WC_SEND_RESULT WowConnection::SendRaw(unsigned char *data, int len) {
  SENDNODE      *sn = NewSendNode(data, len, true);
  WC_SEND_RESULT result = WC_SEND_ERROR;

  m_lock.Enter();

  if (m_connState == WOWC_CONNECTED) {
    ASSERT(m_sock >= 0);

    if (!m_sendList.IsEmpty()) {
      m_sendList.LinkNode(sn, LIST_LINK_BEFORE, 0);
      ++m_sendDepth;
      m_sendDepthBytes += sn->size;
      s_network->PlatformChangeState(this, m_connState);
      m_lock.Leave();
      return WC_SEND_QUEUED;
    }

    int sent = send(m_sock, reinterpret_cast<const char *>(sn->data), sn->size, 0);

    if (sent == sn->size) {
      FreeSendNode(sn);
      m_lock.Leave();
      return WC_SEND_SENT;
    }

    if (sent > 0) {
      sn->offset += sent;
      result = WC_SEND_QUEUED;
    } else if (WSAGetLastError() != WSAEWOULDBLOCK) {
      SetState(WOWC_DISCONNECTING);
      result = WC_SEND_ERROR;
    }

    m_sendList.LinkNode(sn, LIST_LINK_BEFORE, 0);
    ++m_sendDepth;
    m_sendDepthBytes += sn->size;

    if (m_sendDepth >= 100000U) {
      SetState(WOWC_DISCONNECTING);
      result = WC_SEND_ERROR;
    }

    s_network->PlatformChangeState(this, m_connState);
    m_lock.Leave();
    return result;
  }

  FreeSendNode(sn);
  m_lock.Leave();
  return WC_SEND_ERROR;
}

void WowConnection::CheckConnect() {
  int                    sockerr;
  int                    sockerrlen = sizeof(sockerr);
  WowConnectionResponse *response;
  int                    len;

  if (getsockopt(m_sock, SOL_SOCKET, SO_ERROR, reinterpret_cast<char *>(&sockerr), &sockerrlen) == 0) {
    if (sockerr) {
      CloseSocket(m_sock);
      m_sock = -1;
      SetState(WOWC_DISCONNECTED);
      AddRef();
      AcquireResponseRef();
      response = m_response;
      m_lock.Leave();

      if (response) {
        response->WCCantConnect(this, OsGetAsyncTimeMs(), &m_peer);
      }
    } else {
      SetState(WOWC_CONNECTED);
      AddRef();
      AcquireResponseRef();
      response = m_response;
      m_lock.Leave();

      len = sizeof(sockaddr_in);
      getpeername(m_sock, reinterpret_cast<sockaddr *>(&m_peer), &len);
      len = sizeof(sockaddr_in);
      getsockname(m_sock, reinterpret_cast<sockaddr *>(&m_peer.selfAddr), &len);
      response->WCConnected(this, 0, OsGetAsyncTimeMs(), &m_peer);
    }

    m_lock.Enter();
    ReleaseResponseRef();
    Release();
  }
}

void WowConnection::CheckAccept() {
  sockaddr_in            addr;
  WowConnectionResponse *response;
  unsigned long          on;
  int                    len;
  int                    i;

  for (i = 0; i < 1000; ++i) {
    len = sizeof(addr);
    int sock = accept(m_sock, reinterpret_cast<sockaddr *>(&addr), &len);

    if (sock < 0) {
      break;
    }

    on = 1;
    ioctlsocket(sock, FIONBIO, &on);
    WowConnection *conn = new (ALLOC(sizeof(WowConnection))) WowConnection(sock, &addr, m_response);

    AddRef();
    AcquireResponseRef();
    response = m_response;
    m_lock.Leave();

    if (response) {
      response->WCConnected(this, conn, OsGetAsyncTimeMs(), &conn->m_peer);
    }

    if (!s_network->m_connections.IsLinked(conn)) {
      s_network->Add(conn);
    }

    m_lock.Enter();
    ReleaseResponseRef();
    Release();
  }
}

void WowConnection::DoWrites() {
  int          w;
  int          sock;
  unsigned int sendWriteNotify;
  unsigned int disconnected = 0;

  AddRef();
  m_lock.Enter();

  if (m_connState == WOWC_CONNECTING) {
    CheckConnect();
  } else {
    while (m_sendList.Head() && !disconnected) {
      SENDNODE *sn = m_sendList.Head();
      int       writeLen = min(sn->size - sn->offset, 1024);
      ASSERT(writeLen > 0);

      w = send(m_sock, reinterpret_cast<const char *>(sn->data + sn->offset), writeLen, 0);

      if (w == writeLen) {
        if (writeLen == sn->size - sn->offset) {
          m_sendList.UnlinkNode(sn);
          --m_sendDepth;
          m_sendDepthBytes -= sn->size;
          ASSERT(m_sendDepthBytes >= 0);
          ASSERT(m_sendDepth >= 0);
          FreeSendNode(sn);
        } else {
          sn->offset += writeLen;
        }
      } else {
        ASSERT(w < writeLen);

        if (w > 0) {
          sn->offset += w;
          break;
        }

        if (w >= 0) {
          break;
        }

        if (WSAGetLastError() == WSAEWOULDBLOCK) {
          break;
        }

        disconnected = 1;
      }
    }
  }

  WowConnectionResponse *response = 0;
  sock = -1;
  sendWriteNotify = 0;

  if (disconnected) {
    AcquireResponseRef();
    response = m_response;
    CloseSocket(m_sock);
    sock = m_sock;
    SetState(WOWC_DISCONNECTED);
  } else if (m_wantWriteNotification && m_sendList.IsEmpty()) {
    sendWriteNotify = 1;
    m_wantWriteNotification = 0;
    AcquireResponseRef();
    response = m_response;
  }

  m_lock.Leave();

  if (disconnected) {
    if (response && sock >= 0) {
      response->WCDisconnected(this, OsGetAsyncTimeMs(), &m_peer);
    }

    m_lock.Enter();
    m_sock = -1;
    ReleaseResponseRef();
    m_lock.Leave();
    Release();
    return;
  }

  if (sendWriteNotify) {
    if (response) {
      response->WCWriteReady(this);
    }

    m_lock.Enter();
    ReleaseResponseRef();
    m_lock.Leave();
  }

  Release();
}

void WowConnection::DoMessageReads() {
  int bytesRead;

  if (!m_readBuffer) {
    m_readBuffer = static_cast<unsigned char *>(ALLOC(1024));
    m_readBufferSize = 1024;
    m_readBytes = 0;
  }

  while (1) {
    int sizeWanted = -1;
    int sizeBytesWanted = 2;

    if (m_readBytes >= 2) {
      if (m_readBuffer[0] & 0x80) {
        sizeBytesWanted = 3;

        if (m_readBytes >= 3) {
          sizeWanted = ((((m_readBuffer[0] & 0x7f) << 8) | m_readBuffer[1]) << 8) | m_readBuffer[2];
          sizeWanted += sizeBytesWanted;
        }
      } else {
        sizeWanted = ((m_readBuffer[0] & 0x7f) << 8) | m_readBuffer[1];
        sizeWanted += sizeBytesWanted;
      }
    }

    if (m_readBytes >= m_readBufferSize) {
      m_readBuffer = static_cast<unsigned char *>(SMemReAlloc(m_readBuffer, m_readBufferSize + 1024, __FILE__, __LINE__, 0));
      m_readBufferSize += 1024;
    }

    int sizeToRead;

    if (sizeWanted < 0) {
      sizeToRead = sizeBytesWanted;
      ASSERT(sizeToRead <= m_readBufferSize - m_readBytes);
    } else {
      sizeToRead = min(m_readBufferSize - m_readBytes, sizeWanted - m_readBytes);
      ASSERT(sizeToRead <= sizeWanted - m_readBytes);
    }

    ASSERT(sizeToRead >= 0);

    bytesRead = 0;

    if (sizeToRead > 0) {
      do {
        bytesRead = recv(m_sock, reinterpret_cast<char *>(m_readBuffer + m_readBytes), sizeToRead, 0);
      } while (bytesRead < 0 && WSAGetLastError() == WSAEINTR);

      if (bytesRead <= 0) {
        break;
      }
    }

    m_readBytes += bytesRead;

    if (sizeWanted >= 0 && m_readBytes >= sizeWanted) {
      CDataStore msg(m_readBuffer + sizeBytesWanted, sizeWanted - sizeBytesWanted);

      AcquireResponseRef();
      WowConnectionResponse *response = m_response;
      m_lock.Leave();

      if (response) {
        response->WCMessageReady(this, OsGetAsyncTimeMs(), &msg);
      }

      m_lock.Enter();
      m_readBytes = 0;
      ReleaseResponseRef();
    }

    if (bytesRead <= 0) {
      return;
    }
  }

  if (bytesRead < 0 && WSAGetLastError() == WSAEWOULDBLOCK) {
    return;
  }

  AcquireResponseRef();
  WowConnectionResponse *response = m_response;
  CloseSocket(m_sock);
  SetState(WOWC_DISCONNECTED);
  int sock = m_sock;
  m_lock.Leave();

  if (response && sock >= 0) {
    response->WCDisconnected(this, OsGetAsyncTimeMs(), &m_peer);
  }

  m_lock.Enter();
  m_sock = -1;
  ReleaseResponseRef();
}

void WowConnection::DoStreamReads() {
  unsigned char buf[4096];
  int           bytesRead;

  while (1) {
    do {
      bytesRead = recv(m_sock, reinterpret_cast<char *>(buf), sizeof(buf), 0);
    } while (bytesRead < 0 && WSAGetLastError() == WSAEINTR);

    if (bytesRead <= 0) {
      break;
    }

    AcquireResponseRef();
    WowConnectionResponse *response = m_response;
    m_lock.Leave();

    if (response) {
      response->WCDataReady(this, OsGetAsyncTimeMs(), buf, bytesRead);
    }

    m_lock.Enter();
    ReleaseResponseRef();
  }

  if (bytesRead < 0 && WSAGetLastError() == WSAEWOULDBLOCK) {
    return;
  }

  AcquireResponseRef();
  WowConnectionResponse *response = m_response;
  CloseSocket(m_sock);
  SetState(WOWC_DISCONNECTED);
  int sock = m_sock;
  m_lock.Leave();

  if (response && sock >= 0) {
    response->WCDisconnected(this, OsGetAsyncTimeMs(), &m_peer);
  }

  m_lock.Enter();
  m_sock = -1;
  ReleaseResponseRef();
}

void WowConnection::DoReads() {
  AddRef();
  m_lock.Enter();

  if (m_connState == WOWC_LISTENING) {
    CheckAccept();
    m_lock.Leave();
    Release();
    return;
  }

  if (m_connState == WOWC_CONNECTED) {
    if (m_type == WOWC_TYPE_MESSAGES) {
      DoMessageReads();
    } else if (m_type == WOWC_TYPE_STREAM) {
      DoStreamReads();
      m_lock.Leave();
      Release();
      return;
    }
  }

  m_lock.Leave();
  Release();
}

void WowConnection::DoExceptions() {
  AddRef();
  m_lock.Enter();

  if (m_connState == WOWC_CONNECTING) {
    CheckConnect();
  }

  m_lock.Leave();
  Release();
}

void WowConnection::StartConnect() {
  sockaddr_in   addr;
  unsigned long on;

  if (m_sock >= 0) {
    CloseSocket(m_sock);
  }

  m_sock = CreateSocket();

  if (m_sock >= 0) {
    on = 1;
    ioctlsocket(m_sock, FIONBIO, &on);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(m_connectPort);
    addr.sin_addr.s_addr = m_connectAddress;

    if (connect(m_sock, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) >= 0) {
      ASSERT(0);
      SetState(WOWC_ERROR);
      return;
    }

    if (WSAGetLastError() != WSAEWOULDBLOCK) {
      CloseSocket(m_sock);
      m_sock = -1;
      SetState(WOWC_ERROR);
      return;
    }
  }

  if (!netlink.IsLinked()) {
    s_network->Add(this);
  }

  SetState(WOWC_CONNECTING);
}

bool WowConnection::Connect(const char *address, int retryms) {
  char        name[256];
  int         port;
  const char *colon = SStrChr(address, ':');

  if (colon) {
    port = SStrToInt(colon + 1);
    SStrCopy(name, address, min(colon - address + 1, sizeof(name)));
  } else {
    port = 0;
    SStrCopy(name, address, sizeof(name));
  }

  Connect(name, port, retryms);
  return true;
}

bool WowConnection::Connect(unsigned long addr, unsigned short port, int retryms) {
  m_connectAddress = addr;
  m_connectPort = port;
  StartConnect();

  (void)retryms;
  return true;
}

bool WowConnection::Connect(const char *address, unsigned short port, int retryms) {
  hostent *host = gethostbyname(address);

  if (host) {
    unsigned char *addressBytes = reinterpret_cast<unsigned char *>(host->h_addr_list[0]);
    m_connectAddress = static_cast<unsigned long>(addressBytes[0]) | (static_cast<unsigned long>(addressBytes[1]) << 8) |
                       (static_cast<unsigned long>(addressBytes[2]) << 16) | (static_cast<unsigned long>(addressBytes[3]) << 24);
  } else {
    m_connectAddress = 0;
  }

  m_connectPort = port;
  StartConnect();

  (void)retryms;
  return true;
}

bool WowConnection::Reconnect() {
  StartConnect();
  return true;
}

bool WowConnection::Listen(unsigned short port) {
  sockaddr_in   addr;
  unsigned long on;

  if (m_sock >= 0) {
    return false;
  }

  m_sock = CreateSocket();
  if (m_sock < 0) {
    return false;
  }

  m_listenPort = port;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = INADDR_ANY;

  if (bind(m_sock, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0 || listen(m_sock, 10000) < 0) {
    CloseSocket(m_sock);
    m_sock = -1;
    return false;
  }

  on = 1;
  ioctlsocket(m_sock, FIONBIO, &on);
  s_network->Add(this);
  SetState(WOWC_LISTENING);
  return true;
}

void WowConnection::StopListening() {
}

char *WowConnection::GetStringAddress(char *buf, int size) {
  sockaddr_in  *self = reinterpret_cast<sockaddr_in *>(&m_peer.selfAddr);
  unsigned long addr = self->sin_addr.s_addr;

  SStrPrintf(
      buf, size, "%d.%d.%d.%d:%d", static_cast<unsigned char>(addr), static_cast<unsigned char>(addr >> 8), static_cast<unsigned char>(addr >> 16),
      static_cast<unsigned char>(addr >> 24), ntohs(self->sin_port)
  );
  return buf;
}

int __fastcall WowConnection::InitOsNet(
    bool(__fastcall *verifyAddr)(const NETADDR *),
    void(__fastcall *threadInit)(),
    int  numThreads,
    bool useEngine
) {
  WDataStore::StaticInitialize();
  m_verifyAddr = verifyAddr;
  s_destroyed = 0;
  s_network = new (ALLOC(sizeof(WowConnectionNet))) WowConnectionNet(numThreads, threadInit);
  s_network->PlatformInit(useEngine);
  s_network->Start();
  return 1;
}

void __fastcall WowConnection::DestroyOsNet() {
  ASSERT(!s_destroyed);
  s_destroyed = 1;
  s_network->Stop();
  s_network->PlatformDestroy();
  DEL(s_network);
  s_network = 0;
  WDataStore::StaticDestroy();
}

bool __fastcall WowConnection::IsDestroyed() {
  return s_destroyed != 0;
}

void __fastcall WowConnectionInitializer::Initialize() {
}

void __fastcall WowConnectionInitializer::Destroy() {
}

void WowConnection::SetResponse(WowConnectionResponse *response) {
  SCritSect *responseLock = &m_responseLock;

retry:
  responseLock->Enter();

  if (m_responseRef && m_responseRefThread != SGetCurrentThreadId()) {
    responseLock->Leave();
    OsSleep(50);
    goto retry;
  }

  m_response = response;
  responseLock->Leave();
}

void WowConnection::AcquireResponseRef() {
  m_responseLock.Enter();
  ASSERT(m_responseRef == 0 || GetState() == WOWC_LISTENING);
  ++m_responseRef;
  m_responseRefThread = SGetCurrentThreadId();
  m_responseLock.Leave();
}

void WowConnection::ReleaseResponseRef() {
  m_responseLock.Enter();
  ASSERT(m_responseRef > 0);
  --m_responseRef;
  m_responseLock.Leave();
}

void WowConnection::SetState(WOW_CONN_STATE state) {
  WOW_CONN_STATE oldState = m_connState;
  m_connState = state;
  s_network->PlatformChangeState(this, oldState);
}

void WowConnection::SetType(WOWC_TYPE type) {
  m_lock.Enter();
  m_type = type;
  m_lock.Leave();
}

void WowConnection::RequestWriteNotification() {
  m_lock.Enter();
  m_wantWriteNotification = 1;
  m_lock.Leave();
  s_network->PlatformChangeState(this, m_connState);
}

bool WowConnection::GetLocal(NETADDR &addr) {
  int size = sizeof(addr);

  return getsockname(m_sock, reinterpret_cast<sockaddr *>(&addr), &size) == 0;
}

unsigned long __fastcall WowConnection::GetAddr(NETADDR &addr) {
  return reinterpret_cast<sockaddr_in *>(&addr)->sin_addr.s_addr;
}

unsigned short __fastcall WowConnection::GetPort(NETADDR &addr) {
  return reinterpret_cast<sockaddr_in *>(&addr)->sin_port;
}

void __fastcall WowConnection::SetPort(NETADDR &addr, unsigned short port) {
  reinterpret_cast<sockaddr_in *>(&addr)->sin_port = htons(port);
}
