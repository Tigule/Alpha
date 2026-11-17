#include <Base/Base.h>

#include "Os/OsNet.h"
#include "Os/OsTime.h"

#include <storm.h>
#include <stpl.h>
#include <SThread.h>

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

class NetThread;

class NetConnection {
 public:
  enum {
    STATE_CONNECTING = 1,
    STATE_CONNECTED = 2,
    STATE_CLOSED = 3
  };

  struct WRITEBLOCK : public TSLinkedNode<WRITEBLOCK> {
    DWORD   m_size;
    LPBYTE  m_data;
    DWORD   m_offset;
    SEvent *m_event;

    ~WRITEBLOCK() {
      if (m_data) {
        FREE(m_data);
      }
      if (m_event) {
        m_event->Set();
      }
    }
  };

  TSLink<NetConnection>                      m_link;
  SCritSect                                  m_lock;
  int                                        m_refCount;
  NETCONNADDR                                m_addr;
  int                                        m_socket;
  NETEVENTPROC                               m_eventProc;
  LPVOID                                     m_user;
  int                                        m_state;
  TSList<WRITEBLOCK, TSGetLink<WRITEBLOCK> > m_writeList;
  BYTE                                       m_readBuffer[1460];
  DWORD                                      m_readBytes;
  NetThread                                 *m_thread;
  BYTE                                       m_closeRequested;

  NetConnection(int socket, int state, NETEVENTPROC eventProc, LPVOID user, const sockaddr_in &peer) {
    m_refCount = 0;
    m_socket = socket;
    m_eventProc = eventProc;
    m_user = user;
    m_state = state;
    m_readBytes = 0;
    m_thread = 0;
    m_closeRequested = 0;
    memset(&m_addr, 0, sizeof(m_addr));
    memcpy(&m_addr.peerAddr, &peer, sizeof(peer));
  }

  ~NetConnection();

  int AddRef() {
    m_lock.Enter();
    int refCount = ++m_refCount;
    m_lock.Leave();
    return refCount;
  }

  int Release() {
    m_lock.Enter();
    int refCount = --m_refCount;
    m_lock.Leave();
    if (!refCount) {
      delete this;
    }
    return refCount;
  }

  void Close() {
    m_lock.Enter();
    m_closeRequested = 0;
    if (m_socket >= 0) {
      shutdown(m_socket, SHUT_RDWR);
      close(m_socket);
    }
    m_socket = -1;
    int state = m_state;
    m_state = STATE_CLOSED;
    AddRef();
    NETEVENTPROC eventProc = m_eventProc;
    LPVOID       user = m_user;
    while (m_writeList.Head()) {
      m_writeList.DeleteNode(m_writeList.Head());
    }
    m_lock.Leave();

    if (eventProc) {
      DWORD consumed = 0;
      if (state == STATE_CONNECTING) {
        eventProc((HNETCONN)this, &m_addr, NETNOTE_CANTCONNECT, user, 0, 0, &consumed);
      } else if (state != STATE_CLOSED) {
        eventProc((HNETCONN)this, &m_addr, NETNOTE_DISCONNECT, user, 0, 0, &consumed);
      }
    }

    Release();
  }

  void DoRead();
  void DoWrite();
};

class NetListener {
 public:
  TSLink<NetListener> m_link;
  BYTE                m_disabled;
  int                 m_refCount;
  SCritSect           m_lock;
  WORD                m_port;
  NETEVENTPROC        m_eventProc;
  LPVOID              m_user;
  NetThread          *m_thread;
  int                 m_socket;

  ~NetListener();

  void AcceptConnections();
};

class NetThread {
 public:
  struct LISTENENABLE : public TSLinkedNode<LISTENENABLE> {
    WORD m_port;
    int  m_enable;
  };

  LPVOID                                         m_thread;
  UINT                                           m_threadId;
  SCritSect                                      m_lock;
  TSExplicitList<NetConnection, 0>               m_connections;
  TSExplicitList<NetListener, 0>                 m_listeners;
  SCritSect                                      m_pendingLock;
  TSExplicitList<NetConnection, 0>               m_pendingConnections;
  TSExplicitList<NetListener, 0>                 m_pendingListeners;
  TSList<LISTENENABLE, TSGetLink<LISTENENABLE> > m_pendingEnables;
  BYTE                                           m_stop;
  BYTE                                           m_done;

  NetThread() {
    m_thread = 0;
    m_stop = 0;
    m_done = 0;
  }

  void AddConnection(NetConnection *conn) {
    m_pendingLock.Enter();
    conn->AddRef();
    m_pendingConnections.LinkNode(conn, LIST_TAIL, 0);
    conn->m_thread = this;
    m_pendingLock.Leave();
  }

  void RemoveConnection(NetConnection *conn) {
    m_lock.Enter();
    if (m_connections.IsLinked(conn)) {
      m_connections.UnlinkNode(conn);
    }
    m_lock.Leave();
  }

  UINT Run();
};

static NetThread s_ioThreads[4];

LPCSTR OsNetAddrToStr(const NETADDR *netAddr, char *string, DWORD length) {
  const sockaddr_in *addr = (const sockaddr_in *)netAddr;
  DWORD              address = addr->sin_addr.s_addr;

  SStrPrintf(
      string, length, "%d.%d.%d.%d:%u", address & 0xFF, (address >> 8) & 0xFF, (address >> 16) & 0xFF, (address >> 24) & 0xFF, ntohs(addr->sin_port)
  );
  return string;
}

DWORD OsNetAddrGetAddress(const NETADDR *netAddr, WORD *port) {
  const sockaddr_in *addr = (const sockaddr_in *)netAddr;

  if (port) {
    *port = ntohs(addr->sin_port);
  }

  return addr->sin_addr.s_addr;
}

void OsNetAddrMake(DWORD nodeNumber, WORD port, NETADDR *netAddr) {
  sockaddr_in *addr = (sockaddr_in *)netAddr;

  addr->sin_family = AF_INET;
  addr->sin_port = htons(port);
  addr->sin_addr.s_addr = nodeNumber;
  memset(addr->sin_zero, 0, sizeof(addr->sin_zero));
}

void OsNetAddrMakeFromStr(LPCSTR addrStr, WORD port, NETADDR *netAddr) {
  char   tempStr[32];
  LPCSTR colon = SStrChr(addrStr, ':');

  if (colon) {
    SStrCopy(tempStr, addrStr, colon - addrStr + 1 > sizeof(tempStr) ? sizeof(tempStr) : colon - addrStr + 1);
    addrStr = tempStr;
    port = SStrToInt(colon + 1);
  }

  OsNetAddrMake(inet_addr(addrStr), port, netAddr);
}

void OsNetPump(DWORD timeout) {
}

void OsNetDestroy(DWORD parts) {
  int running;

  for (int i = 0; i < 4; ++i) {
    __atomic_store_n(&s_ioThreads[i].m_stop, 1, __ATOMIC_RELEASE);
  }

  do {
    OsSleep(100);
    running = 0;
    for (int i = 0; i < 4; ++i) {
      if (!__atomic_load_n(&s_ioThreads[i].m_done, __ATOMIC_ACQUIRE)) {
        ++running;
      }
    }
  } while (running);
}

UINT APIENTRY IOThreadProc(LPVOID param);

BOOL OsNetInitialize(DWORD hints, DWORD parts) {
  signal(SIGPIPE, SIG_IGN);

  for (int i = 0; i < 4; ++i) {
    char name[64];

    snprintf(name, sizeof(name), "Net Thread %d", i);
    __atomic_store_n(&s_ioThreads[i].m_stop, 0, __ATOMIC_RELEASE);
    s_ioThreads[i].m_thread = SCreateThread(IOThreadProc, &s_ioThreads[i], &s_ioThreads[i].m_threadId, 0, name);
  }

  return 1;
}

HNETCONN OsNetConnCopyHandle(HNETCONN conn) {
  if (conn) {
    ((NetConnection *)conn)->AddRef();
  }

  return conn;
}

void OsNetConnFreeHandle(HNETCONN conn) {
  if (conn) {
    ((NetConnection *)conn)->Release();
  }
}

DWORD OsNetGetHostAddr(LPCSTR hostName) {
  char     name[256];
  hostent *host;
  DWORD    result = 0;

  name[0] = 0;
  if (!hostName) {
    gethostname(name, sizeof(name));
    hostName = name;
  }

  host = gethostbyname(hostName);
  if (host && host->h_addrtype == AF_INET && host->h_addr_list[0]) {
    memcpy(&result, host->h_addr_list[0], sizeof(result));
  }

  return result;
}

BOOL OsNetGetHostAddrs(LPCSTR hostNameList, WORD defaultPort, NETHOSTADDRPROC hostAddrProc, LPVOID user) {
  char                     hostNames[1024];
  TSGrowableArray<NETADDR> addrs;

  SStrCopy(hostNames, hostNameList, sizeof(hostNames));

  for (char *hostName = hostNames; hostName && *hostName;) {
    char *nextHostName = SStrChr(hostName, ';');
    if (nextHostName) {
      *nextHostName++ = 0;
    }

    if (hostName[0] >= '0' && hostName[0] <= '9') {
      OsNetAddrMakeFromStr(hostName, defaultPort, addrs.New());
    } else {
      hostent *host = gethostbyname(hostName);
      if (host && host->h_addrtype == AF_INET && host->h_length == 4) {
        for (char **hostAddr = host->h_addr_list; *hostAddr; ++hostAddr) {
          DWORD nodeNumber;
          memcpy(&nodeNumber, *hostAddr, sizeof(nodeNumber));
          OsNetAddrMake(nodeNumber, defaultPort, addrs.New());
        }
      }
    }

    hostName = nextHostName;
  }

  hostAddrProc(addrs.Ptr(), addrs.Count(), user);
  return 1;
}

void OsTcpConnSend(HNETCONN conn, LPCVOID data, DWORD bytes) {
  NetConnection *netConn = (NetConnection *)conn;

  if (!netConn || !data || !bytes) {
    return;
  }

  netConn->m_lock.Enter();

  if (netConn->m_state != NetConnection::STATE_CLOSED) {
    NetConnection::WRITEBLOCK *block = netConn->m_writeList.NewNode(LIST_TAIL, 0, 0);
    block->m_size = bytes;
    block->m_data = (LPBYTE)ALLOC(bytes);
    block->m_offset = 0;
    block->m_event = 0;
    memcpy(block->m_data, data, bytes);
  }

  netConn->m_lock.Leave();
}

void OsTcpConnect(DWORD nodeNumber, WORD port, NETEVENTPROC eventProc, LPVOID user, LPCVOID data, DWORD bytes) {
  sockaddr_in addr;
  int         s;

  OsNetAddrMake(nodeNumber, port, (NETADDR *)&addr);

  s = socket(AF_INET, SOCK_STREAM, 0);
  if (s >= 0) {
    fcntl(s, F_SETFL, O_NONBLOCK);
  }

  NetConnection *conn = new NetConnection(s, NetConnection::STATE_CONNECTING, eventProc, user, addr);
  conn->AddRef();

  if (data && bytes) {
    OsTcpConnSend((HNETCONN)conn, data, bytes);
  }

  if (s < 0 || (connect(s, (sockaddr *)&addr, sizeof(addr)) && errno != EINPROGRESS)) {
    conn->m_closeRequested = 1;
  } else {
    socklen_t length = sizeof(sockaddr_in);
    getsockname(s, (sockaddr *)&conn->m_addr.selfAddr, &length);
  }

  s_ioThreads[(UINT)s % 4].AddConnection(conn);
  conn->Release();
}

void NetListener::AcceptConnections() {
  for (;;) {
    sockaddr_in addr;
    socklen_t   length = sizeof(addr);

    m_lock.Enter();

    int s = accept(m_socket, (sockaddr *)&addr, &length);
    if (s < 0) {
      m_lock.Leave();
      return;
    }

    if (m_disabled) {
      close(s);
      m_lock.Leave();
      continue;
    }

    fcntl(s, F_SETFL, O_NONBLOCK);

    NetConnection *conn = new NetConnection(s, NetConnection::STATE_CONNECTED, m_eventProc, m_user, addr);
    conn->AddRef();
    m_thread->AddConnection(conn);

    NETEVENTPROC eventProc = m_eventProc;
    LPVOID       user = m_user;
    m_lock.Leave();

    if (eventProc) {
      DWORD consumed = 0;
      eventProc((HNETCONN)conn, &conn->m_addr, NETNOTE_CONNECT, user, 0, 0, &consumed);
    }

    conn->Release();
  }
}

NetListener::~NetListener() {
  if (m_thread) {
    m_thread->m_lock.Enter();
    if (m_thread->m_listeners.IsLinked(this)) {
      m_thread->m_listeners.UnlinkNode(this);
    }
    m_thread->m_lock.Leave();
  }
}

void NetConnection::DoWrite() {
  m_lock.Enter();

  if (m_state == STATE_CONNECTING) {
    int       error;
    socklen_t length = sizeof(error);

    if (getsockopt(m_socket, SOL_SOCKET, SO_ERROR, &error, &length)) {
      m_lock.Leave();
      return;
    }

    NETEVENTPROC eventProc = m_eventProc;
    LPVOID       user = m_user;
    DWORD        consumed = 0;

    if (error) {
      if (m_socket >= 0) {
        shutdown(m_socket, SHUT_RDWR);
        close(m_socket);
      }
      m_socket = -1;
      m_state = STATE_CLOSED;
      m_lock.Leave();
      eventProc((HNETCONN)this, &m_addr, NETNOTE_CANTCONNECT, user, 0, 0, &consumed);
      return;
    }

    m_state = STATE_CONNECTED;
    m_lock.Leave();
    eventProc((HNETCONN)this, &m_addr, NETNOTE_CONNECT, user, 0, 0, &consumed);
    m_lock.Enter();
  }

  WRITEBLOCK *block = m_writeList.Head();
  while (block) {
    int writeLen = block->m_size - block->m_offset;
    ASSERT(writeLen > 0);

    int w = write(m_socket, block->m_data + block->m_offset, writeLen);
    if (w == writeLen) {
      block = m_writeList.DeleteNode(block);
      continue;
    }

    if (w > 0) {
      block->m_offset += w;
      if (block->m_offset < block->m_size) {
        break;
      }
      ASSERT(block->m_offset == block->m_size);
      block = m_writeList.DeleteNode(block);
      continue;
    }

    if (w < 0 && errno != EAGAIN && errno != EINTR) {
      m_lock.Enter();
      m_closeRequested = 1;
      m_lock.Leave();
      break;
    }

    ASSERT(w != 0);
    break;
  }

  m_lock.Leave();
}

void NetConnection::DoRead() {
  m_lock.Enter();

  if (m_socket < 0 || m_state != STATE_CONNECTED) {
    m_lock.Leave();
    return;
  }

  if (m_readBytes >= sizeof(m_readBuffer)) {
    Close();
    m_lock.Leave();
    return;
  }

  int bytes;
  do {
    bytes = read(m_socket, m_readBuffer + m_readBytes, sizeof(m_readBuffer) - m_readBytes);
  } while (bytes < 0 && errno == EINTR);

  if (bytes <= 0) {
    m_lock.Leave();
    if (bytes < 0 && errno == EAGAIN) {
      return;
    }
    Close();
    return;
  }

  m_readBytes += bytes;
  NETEVENTPROC eventProc = m_eventProc;
  LPVOID       user = m_user;
  m_lock.Leave();

  DWORD consumed = 0;
  eventProc((HNETCONN)this, &m_addr, NETNOTE_DATA, user, m_readBuffer, m_readBytes, &consumed);

  m_lock.Enter();
  if (consumed >= m_readBytes) {
    m_readBytes = 0;
  } else {
    memmove(m_readBuffer, m_readBuffer + consumed, m_readBytes - consumed);
    m_readBytes -= consumed;
  }
  m_lock.Leave();
}

NetConnection::~NetConnection() {
  if (m_thread) {
    m_thread->RemoveConnection(this);
  }

  while (m_writeList.Head()) {
    m_writeList.DeleteNode(m_writeList.Head());
  }
}

UINT NetThread::Run() {
  while (!__atomic_load_n(&m_stop, __ATOMIC_ACQUIRE)) {
    fd_set         readSet;
    fd_set         writeSet;
    int            maxSocket = 0;
    struct timeval timeout;

    FD_ZERO(&readSet);
    FD_ZERO(&writeSet);

    m_lock.Enter();
    m_pendingLock.Enter();

    NetListener *listener;
    while ((listener = m_pendingListeners.Head()) != 0) {
      m_pendingListeners.UnlinkNode(listener);
      m_listeners.LinkNode(listener, LIST_TAIL, 0);
    }

    NetConnection *conn;
    while ((conn = m_pendingConnections.Head()) != 0) {
      m_pendingConnections.UnlinkNode(conn);
      m_connections.LinkNode(conn, LIST_TAIL, 0);
    }

    LISTENENABLE *request = m_pendingEnables.Head();
    while (request) {
      for (listener = m_listeners.Head(); listener; listener = m_listeners.Next(listener)) {
        if (listener->m_port == request->m_port) {
          listener->m_disabled = request->m_enable ? 0 : 1;
        }
      }
      request = m_pendingEnables.DeleteNode(request);
    }

    m_pendingLock.Leave();

    NetConnection *next;
    for (conn = m_connections.Head(); conn; conn = next) {
      next = m_connections.Next(conn);

      if (conn->m_closeRequested) {
        conn->Close();
      } else if (conn->m_state == NetConnection::STATE_CLOSED && conn->m_refCount == 1) {
        m_connections.UnlinkNode(conn);
        conn->Release();
      } else if (conn->m_state == NetConnection::STATE_CONNECTED) {
        FD_SET(conn->m_socket, &readSet);
        if (conn->m_socket > maxSocket) {
          maxSocket = conn->m_socket;
        }

        conn->m_lock.Enter();
        BOOL wantWrite = conn->m_writeList.Head() != 0;
        conn->m_lock.Leave();

        if (wantWrite) {
          FD_SET(conn->m_socket, &writeSet);
        }
      } else if (conn->m_state == NetConnection::STATE_CONNECTING) {
        FD_SET(conn->m_socket, &writeSet);
        if (conn->m_socket > maxSocket) {
          maxSocket = conn->m_socket;
        }
      }
    }

    for (listener = m_listeners.Head(); listener; listener = m_listeners.Next(listener)) {
      if (listener->m_socket >= 0) {
        FD_SET(listener->m_socket, &readSet);
        if (listener->m_socket > maxSocket) {
          maxSocket = listener->m_socket;
        }
      }
    }

    m_lock.Leave();

    timeout.tv_sec = 2;
    timeout.tv_usec = 0;
    select(maxSocket + 1, &readSet, &writeSet, 0, &timeout);

    m_lock.Enter();

    for (conn = m_connections.Head(); conn; conn = next) {
      next = m_connections.Next(conn);

      if (conn->m_state == NetConnection::STATE_CONNECTED) {
        if (FD_ISSET(conn->m_socket, &readSet)) {
          conn->DoRead();
        }
        if (conn->m_socket >= 0 && FD_ISSET(conn->m_socket, &writeSet)) {
          conn->DoWrite();
        }
      } else if (conn->m_state == NetConnection::STATE_CONNECTING) {
        if (FD_ISSET(conn->m_socket, &writeSet)) {
          conn->DoWrite();
        }
      }
    }

    for (listener = m_listeners.Head(); listener; listener = m_listeners.Next(listener)) {
      if (listener->m_socket >= 0 && FD_ISSET(listener->m_socket, &readSet)) {
        listener->AcceptConnections();
      }
    }

    m_lock.Leave();
  }

  __atomic_store_n(&m_done, 1, __ATOMIC_RELEASE);
  return 0;
}

UINT APIENTRY IOThreadProc(LPVOID param) {
  return ((NetThread *)param)->Run();
}
