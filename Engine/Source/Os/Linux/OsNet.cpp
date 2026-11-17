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

  NetConnection() {
    m_refCount = 0;
    m_socket = -1;
    m_state = 0;
    m_thread = 0;
    m_eventProc = 0;
    m_user = 0;
    m_readBytes = 0;
    memset(&m_addr, 0, sizeof(m_addr));
    m_closeRequested = 0;
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

  void SetCloseRequested() {
    m_lock.Enter();
    m_closeRequested = 1;
    m_lock.Leave();
  }

  BOOL WantWrite() {
    m_lock.Enter();
    BOOL wantWrite = m_writeList.Head() != 0 || m_state == STATE_CONNECTING;
    m_lock.Leave();
    return wantWrite;
  }

  void Connect(DWORD nodeNumber, WORD port, NETEVENTPROC eventProc, LPVOID user, LPCVOID data, DWORD bytes);
  bool Send(LPCVOID data, DWORD bytes);
  void DoRead();
  void DoWrite();
};

class NetThread {
 public:
  LPVOID                           m_thread;
  UINT                             m_threadId;
  DWORD                            m_load;
  SCritSect                        m_lock;
  TSExplicitList<NetConnection, 0> m_connections;
  SCritSect                        m_pendingLock;
  TSExplicitList<NetConnection, 0> m_pendingConnections;
  BYTE                             m_stop;
  BYTE                             m_done;

  NetThread() {
    m_thread = 0;
    m_load = 0;
    m_stop = 0;
    m_done = 0;
  }

  UINT Run();
};

static NetThread s_ioThreads[4];

static NetThread *ChooseThread() {
  DWORD minLoad = 0xFFFFFFFF;
  int   index = 0;

  for (int i = 0; i < 4; ++i) {
    if (s_ioThreads[i].m_load < minLoad) {
      index = i;
      minLoad = s_ioThreads[i].m_load;
    }
  }

  return &s_ioThreads[index];
}

void NetConnection::Connect(DWORD nodeNumber, WORD port, NETEVENTPROC eventProc, LPVOID user, LPCVOID data, DWORD bytes) {
  sockaddr_in *peer = (sockaddr_in *)&m_addr.peerAddr;

  m_lock.Enter();
  m_eventProc = eventProc;
  m_user = user;

  if (m_socket >= 0) {
    Close();
  }

  m_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (m_socket < 0) {
    m_state = STATE_CONNECTING;
    m_closeRequested = 1;
    m_lock.Leave();
    return;
  }

  fcntl(m_socket, F_SETFL, O_NONBLOCK);
  peer->sin_family = AF_INET;
  peer->sin_port = htons(port);
  peer->sin_addr.s_addr = nodeNumber;
  m_state = STATE_CONNECTING;

  if (connect(m_socket, (sockaddr *)peer, sizeof(sockaddr_in)) >= 0) {
    m_state = STATE_CONNECTED;
  } else if (errno != EINPROGRESS) {
    shutdown(m_socket, SHUT_RDWR);
    close(m_socket);
    m_socket = -1;
    m_closeRequested = 1;
  }

  m_lock.Leave();
}

bool NetConnection::Send(LPCVOID data, DWORD bytes) {
  bool  sent = false;
  bool  queue = false;
  DWORD offset = 0;

  m_lock.Enter();

  if (m_writeList.Head() || m_state == STATE_CONNECTING) {
    queue = true;
  } else if (m_state == STATE_CONNECTED) {
    int w = write(m_socket, data, bytes);
    if (w == (int)bytes) {
      sent = true;
    } else {
      queue = true;
      if (w > 0) {
        offset = w;
      }
    }
  }

  if (queue) {
    WRITEBLOCK *block = m_writeList.NewNode(LIST_TAIL, 0, 0);
    block->m_size = bytes - offset;
    block->m_data = (LPBYTE)ALLOC(block->m_size);
    block->m_offset = 0;
    block->m_event = 0;
    memcpy(block->m_data, (const BYTE *)data + offset, block->m_size);
  }

  m_lock.Leave();
  return sent;
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
      SetCloseRequested();
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

  int bytes = read(m_socket, m_readBuffer + m_readBytes, sizeof(m_readBuffer) - m_readBytes);

  if (bytes <= 0) {
    m_lock.Leave();
    if (bytes < 0 && errno == EAGAIN) {
      return;
    }
    Close();
    return;
  }

  m_readBytes += bytes;
  m_lock.Leave();

  DWORD consumed = 0;
  m_eventProc((HNETCONN)this, &m_addr, NETNOTE_DATA, m_user, m_readBuffer, m_readBytes, &consumed);

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
    m_thread->m_lock.Enter();
    if (m_thread->m_connections.IsLinked(this)) {
      Release();
      m_thread->m_connections.UnlinkNode(this);
    }
    m_thread->m_lock.Leave();
  }
}

UINT NetThread::Run() {
  while (!__atomic_load_n(&m_stop, __ATOMIC_ACQUIRE)) {
    fd_set         readSet;
    fd_set         writeSet;
    UINT           maxSocket = 0;
    struct timeval timeout;
    NetConnection *conn;
    NetConnection *next;

    FD_ZERO(&readSet);
    FD_ZERO(&writeSet);

    m_lock.Enter();
    m_pendingLock.Enter();

    while ((conn = m_pendingConnections.Head()) != 0) {
      m_pendingConnections.UnlinkNode(conn);
      m_connections.LinkNode(conn, LIST_TAIL, 0);
    }

    m_pendingLock.Leave();

    for (conn = m_connections.Head(); conn; conn = next) {
      next = m_connections.Next(conn);

      if (conn->m_closeRequested) {
        conn->Close();
      } else if (conn->m_state == NetConnection::STATE_CLOSED && conn->m_refCount == 1) {
        m_connections.UnlinkNode(conn);
        conn->Release();
      } else if (conn->m_state == NetConnection::STATE_CONNECTED) {
        FD_SET(conn->m_socket, &readSet);
        if ((UINT)conn->m_socket > maxSocket) {
          maxSocket = conn->m_socket;
        }
        if (conn->WantWrite()) {
          FD_SET(conn->m_socket, &writeSet);
        }
      } else if (conn->m_state == NetConnection::STATE_CONNECTING) {
        FD_SET(conn->m_socket, &writeSet);
        if ((UINT)conn->m_socket > maxSocket) {
          maxSocket = conn->m_socket;
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

    m_lock.Leave();
  }

  __atomic_store_n(&m_done, 1, __ATOMIC_RELEASE);
  return 0;
}

static UINT APIENTRY IOThreadProc(LPVOID param) {
  return ((NetThread *)param)->Run();
}

BOOL OsNetInitialize(DWORD hints, DWORD parts) {
  for (int i = 0; i < 4; ++i) {
    char name[64];

    snprintf(name, sizeof(name), "Net Thread %d", i);
    __atomic_store_n(&s_ioThreads[i].m_stop, 0, __ATOMIC_RELEASE);
    s_ioThreads[i].m_thread = SCreateThread(IOThreadProc, &s_ioThreads[i], &s_ioThreads[i].m_threadId, 0, name);
  }

  return TRUE;
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

void OsNetPump(DWORD timeout) {
}

HNETCONN OsNetConnCopyHandle(HNETCONN conn) {
  ((NetConnection *)conn)->AddRef();
  return conn;
}

void OsNetConnFreeHandle(HNETCONN conn) {
  ((NetConnection *)conn)->Release();
}

void OsTcpConnect(DWORD nodeNumber, WORD port, NETEVENTPROC eventProc, LPVOID user, LPCVOID data, DWORD bytes) {
  NetThread     *thread = ChooseThread();
  NetConnection *conn = new NetConnection;

  conn->Connect(nodeNumber, port, eventProc, user, data, bytes);

  thread->m_pendingLock.Enter();
  conn->AddRef();
  thread->m_pendingConnections.LinkNode(conn, LIST_TAIL, 0);
  conn->m_thread = thread;
  thread->m_pendingLock.Leave();
}

void OsTcpConnSend(HNETCONN conn, LPCVOID data, DWORD bytes) {
  ((NetConnection *)conn)->Send(data, bytes);
}

void OsNetAddrMake(DWORD nodeNumber, WORD port, NETADDR *netAddr) {
  sockaddr_in *addr = (sockaddr_in *)netAddr;

  addr->sin_family = AF_INET;
  addr->sin_port = htons(port);
  addr->sin_addr.s_addr = nodeNumber;
  memset(addr->sin_zero, 0, sizeof(addr->sin_zero));
}

DWORD OsNetAddrGetAddress(const NETADDR *netAddr, WORD *port) {
  const sockaddr_in *addr = (const sockaddr_in *)netAddr;

  if (port) {
    *port = ntohs(addr->sin_port);
  }

  return addr->sin_addr.s_addr;
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
  if (host) {
    memcpy(&result, host->h_addr_list[0], sizeof(result));
  }

  return result;
}

BOOL OsNetGetHostAddrs(LPCSTR hostNameList, WORD defaultPort, NETHOSTADDRPROC hostAddrProc, LPVOID user) {
  hostent                 *host = gethostbyname(hostNameList);
  TSGrowableArray<NETADDR> addrs;

  if (!host || host->h_addrtype != AF_INET || host->h_length != 4) {
    hostAddrProc(0, 0, user);
    return TRUE;
  }

  for (char **hostAddr = host->h_addr_list; *hostAddr; ++hostAddr) {
    DWORD nodeNumber;

    memcpy(&nodeNumber, *hostAddr, sizeof(nodeNumber));
    OsNetAddrMake(nodeNumber, defaultPort, addrs.New());
  }

  hostAddrProc(addrs.Ptr(), addrs.Count(), user);
  return TRUE;
}
