#include <Base/Base.h>

#include "Os/OsNet.h"

#include <storm.h>
#include <stpl.h>

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#define OSNET_RECV_SIZE 0x1000

NODEDECL(NETCONN) {
  int          socket;
  int          connecting;
  int          closed;
  int          refCount;
  NETEVENTPROC eventProc;
  LPVOID       user;
  NETCONNADDR  addr;
};

static SCritSect s_netLock;
static LISTDECL(NETCONN, s_connList);
static int     s_initialized;
static int     s_pumpExit;
static SThread s_pumpThread;

static void AddrFromSockaddr(const sockaddr_in &source, NETADDR *dest) {
  memcpy(dest, &source, sizeof(sockaddr_in));
}

static void ConnFillAddr(NETCONN *conn) {
  sockaddr_in peer;
  sockaddr_in self;
  socklen_t   len;

  len = sizeof(peer);
  if (!getpeername(conn->socket, reinterpret_cast<sockaddr *>(&peer), &len)) {
    AddrFromSockaddr(peer, &conn->addr.peerAddr);
  }

  len = sizeof(self);
  if (!getsockname(conn->socket, reinterpret_cast<sockaddr *>(&self), &len)) {
    AddrFromSockaddr(self, &conn->addr.selfAddr);
  }
}

static void ConnClose(NETCONN *conn) {
  if (conn->socket >= 0) {
    close(conn->socket);
    conn->socket = -1;
  }

  conn->closed = 1;
}

static UINT APIENTRY NetPumpThread(LPVOID) {
  while (!s_pumpExit) {
    OsNetPump(100);
  }

  return 0;
}

int OsNetInitialize(DWORD hints, DWORD parts) {
  if (s_initialized) {
    return 1;
  }

  s_initialized = 1;
  s_pumpExit = 0;

  // the scheduler only pumps for a net server, so sockets get their own
  // thread here just as they do on win32
  SThread::Create(NetPumpThread, 0, s_pumpThread, const_cast<char *>("OsTcp_Pump"));

  return 1;
}

void OsNetDestroy(DWORD parts) {
  NETCONN *conn;

  s_pumpExit = 1;
  s_pumpThread.Wait(1000);

  s_netLock.Enter();

  while ((conn = s_connList.Head()) != 0) {
    ConnClose(conn);
    s_connList.DeleteNode(conn);
  }

  s_netLock.Leave();
  s_initialized = 0;
}

HNETCONN__ *OsNetConnCopyHandle(HNETCONN__ *conn) {
  NETCONN *netConn = reinterpret_cast<NETCONN *>(conn);

  if (netConn) {
    s_netLock.Enter();
    ++netConn->refCount;
    s_netLock.Leave();
  }

  return conn;
}

void OsNetConnFreeHandle(HNETCONN__ *conn) {
  NETCONN *netConn = reinterpret_cast<NETCONN *>(conn);

  if (!netConn) {
    return;
  }

  s_netLock.Enter();

  if (!--netConn->refCount) {
    ConnClose(netConn);
    s_connList.DeleteNode(netConn);
  }

  s_netLock.Leave();
}

DWORD OsNetGetHostAddr(LPCSTR hostName) {
  addrinfo  hints;
  addrinfo *results;
  DWORD     address = 0;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  if (getaddrinfo(hostName, 0, &hints, &results)) {
    return 0;
  }

  if (results) {
    address = reinterpret_cast<sockaddr_in *>(results->ai_addr)->sin_addr.s_addr;
  }

  freeaddrinfo(results);
  return address;
}

int OsNetGetHostAddrs(LPCSTR hostNameList, WORD defaultPort, NETHOSTADDRPROC hostAddrProc, LPVOID user) {
  LPCSTR cursor = hostNameList;
  DWORD  count = 0;

  while (*cursor) {
    char    name[0x100];
    char   *colon;
    WORD    port = defaultPort;
    DWORD   address;
    NETADDR addr;
    DWORD   chars = 0;

    while (*cursor && *cursor != ' ' && *cursor != ',' && *cursor != ';' && chars < sizeof(name) - 1) {
      name[chars++] = *cursor++;
    }

    name[chars] = 0;

    while (*cursor == ' ' || *cursor == ',' || *cursor == ';') {
      ++cursor;
    }

    if (!chars) {
      continue;
    }

    colon = SStrChr(name, ':');
    if (colon) {
      *colon = 0;
      port = static_cast<WORD>(SStrToInt(colon + 1));
    }

    address = OsNetGetHostAddr(name);
    if (!address) {
      continue;
    }

    OsNetAddrMake(address, port, &addr);
    hostAddrProc(&addr, count++, user);
  }

  return count != 0;
}

DWORD OsNetAddrGetAddress(const NETADDR *netAddr, WORD *port) {
  const sockaddr_in *addr = reinterpret_cast<const sockaddr_in *>(netAddr);

  if (port) {
    *port = ntohs(addr->sin_port);
  }

  return addr->sin_addr.s_addr;
}

void OsTcpConnect(DWORD nodeNumber, WORD port, NETEVENTPROC eventProc, LPVOID user, LPCVOID data, DWORD bytes) {
  NETCONN    *conn;
  sockaddr_in target;
  int         handle;
  int         on = 1;

  handle = socket(AF_INET, SOCK_STREAM, 0);
  if (handle < 0) {
    return;
  }

  fcntl(handle, F_SETFL, fcntl(handle, F_GETFL, 0) | O_NONBLOCK);
  setsockopt(handle, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on));

  memset(&target, 0, sizeof(target));
  target.sin_len = sizeof(target);
  target.sin_family = AF_INET;
  target.sin_port = htons(port);
  target.sin_addr.s_addr = nodeNumber;

  s_netLock.Enter();

  conn = s_connList.NewNode(LIST_TAIL, 0, 0);
  conn->socket = handle;
  conn->connecting = 1;
  conn->closed = 0;
  conn->refCount = 1;
  conn->eventProc = eventProc;
  conn->user = user;

  memset(&conn->addr, 0, sizeof(conn->addr));
  AddrFromSockaddr(target, &conn->addr.peerAddr);

  s_netLock.Leave();

  if (connect(handle, reinterpret_cast<sockaddr *>(&target), sizeof(target)) && errno != EINPROGRESS) {
    ConnClose(conn);

    if (eventProc) {
      DWORD consumed = 0;
      eventProc(reinterpret_cast<HNETCONN__ *>(conn), &conn->addr, NETNOTE_CANTCONNECT, user, 0, 0, &consumed);
    }

    return;
  }

  if (data && bytes) {
    OsTcpConnSend(reinterpret_cast<HNETCONN__ *>(conn), data, bytes);
  }
}

void OsTcpConnSend(HNETCONN__ *conn, LPCVOID data, DWORD bytes) {
  NETCONN *netConn = reinterpret_cast<NETCONN *>(conn);

  if (!netConn || netConn->socket < 0) {
    return;
  }

  send(netConn->socket, data, bytes, 0);
}

void OsNetPump(DWORD timeout) {
  fd_set         readSet;
  fd_set         writeSet;
  struct timeval wait;
  NETCONN       *conn;
  int            highest = -1;

  if (!s_initialized) {
    return;
  }

  FD_ZERO(&readSet);
  FD_ZERO(&writeSet);

  s_netLock.Enter();

  ITERATELIST(NETCONN, s_connList, cursor) {
    if (cursor->socket < 0) {
      continue;
    }

    FD_SET(cursor->socket, &readSet);

    if (cursor->connecting) {
      FD_SET(cursor->socket, &writeSet);
    }

    if (cursor->socket > highest) {
      highest = cursor->socket;
    }
  }

  s_netLock.Leave();

  wait.tv_sec = timeout / 1000;
  wait.tv_usec = timeout % 1000 * 1000;

  if (highest < 0) {
    // nothing to watch yet, but the caller still asked to wait
    select(0, 0, 0, 0, &wait);
    return;
  }

  if (select(highest + 1, &readSet, &writeSet, 0, &wait) <= 0) {
    return;
  }

  s_netLock.Enter();
  conn = s_connList.Head();
  s_netLock.Leave();

  while (conn) {
    NETCONN *next;
    DWORD    consumed = 0;

    s_netLock.Enter();
    next = s_connList.Next(conn);
    s_netLock.Leave();

    if (conn->socket >= 0 && conn->connecting && FD_ISSET(conn->socket, &writeSet)) {
      int       error = 0;
      socklen_t len = sizeof(error);

      getsockopt(conn->socket, SOL_SOCKET, SO_ERROR, &error, &len);
      conn->connecting = 0;

      if (error) {
        ConnClose(conn);

        if (conn->eventProc) {
          conn->eventProc(reinterpret_cast<HNETCONN__ *>(conn), &conn->addr, NETNOTE_CANTCONNECT, conn->user, 0, 0, &consumed);
        }

        conn = next;
        continue;
      }

      ConnFillAddr(conn);

      if (conn->eventProc) {
        conn->eventProc(reinterpret_cast<HNETCONN__ *>(conn), &conn->addr, NETNOTE_CONNECT, conn->user, 0, 0, &consumed);
      }
    }

    if (conn->socket >= 0 && FD_ISSET(conn->socket, &readSet)) {
      BYTE    buffer[OSNET_RECV_SIZE];
      ssize_t received = recv(conn->socket, buffer, sizeof(buffer), 0);

      if (received > 0) {
        if (conn->eventProc) {
          conn->eventProc(
              reinterpret_cast<HNETCONN__ *>(conn), &conn->addr, NETNOTE_DATA, conn->user, buffer, static_cast<DWORD>(received), &consumed
          );
        }
      } else if (!received || errno != EWOULDBLOCK) {
        ConnClose(conn);

        if (conn->eventProc) {
          conn->eventProc(reinterpret_cast<HNETCONN__ *>(conn), &conn->addr, NETNOTE_DISCONNECT, conn->user, 0, 0, &consumed);
        }
      }
    }

    conn = next;
  }
}

void OsNetAddrMake(DWORD nodeNumber, WORD port, NETADDR *netAddr) {
  sockaddr_in addr;

  memset(&addr, 0, sizeof(addr));
  addr.sin_len = sizeof(addr);
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = nodeNumber;

  AddrFromSockaddr(addr, netAddr);
}
