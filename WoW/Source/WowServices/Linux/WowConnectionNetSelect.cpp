#define FD_SETSIZE 1024
#include <winsock2.h>
#include <Base/Base.h>

#include <stdlib.h>

#include "WowServices/WowConnectionNet.h"

static int     s_workerPipe[2] = {-1, -1};
static BYTE    s_usingEngine;
static WSADATA s_wsaData;

static void WinsockInit() {
  WSAStartup(0x0202, &s_wsaData);
}

static void WinsockDestroy() {
}

void WowConnectionNet::PlatformInit(bool useEngine) {
  s_usingEngine = useEngine;
  if (useEngine) {
    OsNetInitialize(0, 7);
  } else {
    WinsockInit();
  }
}

void WowConnectionNet::PlatformDestroy() {
  if (s_usingEngine) {
    OsNetDestroy(7);
  } else {
    WinsockDestroy();
  }
}

void WowConnectionNet::PlatformAdd(WowConnection *conn) {
  PlatformWorkerReady();
}

void WowConnectionNet::PlatformRemove(WowConnection *conn) {
  PlatformWorkerReady();
}

void WowConnectionNet::PlatformChangeState(WowConnection *conn, WOW_CONN_STATE oldState) {
  PlatformWorkerReady();
}

void WowConnectionNet::PlatformDestruct(WowConnection *conn) {
}

void WowConnectionNet::PlatformWorkerReady() {
  char c = 1;
  send(s_workerPipe[1], &c, 1, 0);
}

static void MakeSocketPipe(int *const pipes) {
  sockaddr_in addr;
  sockaddr_in incoming;
  int         len;
  DWORD       on;
  int         listener = socket(AF_INET, SOCK_STREAM, 0);

  if (listener < 0) {
    return;
  }

  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr("127.0.0.1");

  WORD port = static_cast<WORD>(rand() % 10000 + 40000);
  int  b;

  while (1) {
    addr.sin_port = htons(port);
    b = bind(listener, reinterpret_cast<const sockaddr *>(&addr), sizeof(addr));
    if (b >= 0) {
      break;
    }
    ++port;
    if (port >= 65000) {
      ASSERT(b >= 0);
      break;
    }
  }

  if (listen(listener, 1) < 0) {
    ASSERT(0);
  }

  pipes[1] = socket(AF_INET, SOCK_STREAM, 0);
  if (connect(pipes[1], reinterpret_cast<const sockaddr *>(&addr), sizeof(addr)) < 0) {
    ASSERT(0);
  }

  on = 1;
  ioctlsocket(pipes[1], FIONBIO, &on);

  len = sizeof(incoming);
  pipes[0] = accept(listener, reinterpret_cast<sockaddr *>(&incoming), &len);
  ASSERT(pipes[0] >= 0);

  on = 1;
  ioctlsocket(pipes[0], FIONBIO, &on);
  closesocket(listener);
}

void WowConnectionNet::PlatformRun() {
  MakeSocketPipe(s_workerPipe);

  TSGrowableArray<WowConnection *> conns;

  while (!m_stop) {
    fd_set       wfds;
    fd_set       rfds;
    fd_set       efds;
    timeval      tv;
    char         c;
    register int maxSocket = s_workerPipe[0];
    int          disconnectingCount = 0;
    int          numConns = 0;

    tv.tv_sec = 30;
    tv.tv_usec = 0;
    FD_ZERO(&rfds);
    FD_ZERO(&wfds);
    FD_ZERO(&efds);
    FD_SET(s_workerPipe[0], &rfds);

    m_connectionsLock.Enter();

    ITERATELIST(WowConnection, m_connections, conn) {
      if (conn->m_serviceCount) {
        continue;
      }

      switch (conn->m_connState) {
        case WOWC_CONNECTED:
          FD_SET(conn->m_sock, &rfds);
          FD_SET(conn->m_sock, &efds);
          if (conn->m_sendDepth > 0 || conn->m_wantWriteNotification) {
            FD_SET(conn->m_sock, &wfds);
          }
          if (maxSocket <= conn->m_sock) {
            maxSocket = conn->m_sock;
          }
          break;

        case WOWC_CONNECTING:
          FD_SET(conn->m_sock, &efds);
          FD_SET(conn->m_sock, &wfds);
          if (maxSocket <= conn->m_sock) {
            maxSocket = conn->m_sock;
          }
          break;

        case WOWC_LISTENING:
          FD_SET(conn->m_sock, &rfds);
          if (maxSocket <= conn->m_sock) {
            maxSocket = conn->m_sock;
          }
          break;

        case WOWC_DISCONNECTING:
          ++disconnectingCount;
          break;

        default:
          continue;
      }

      if (numConns >= static_cast<int>(conns.Count())) {
        *conns.New() = conn;
        ++numConns;
      } else {
        conns[numConns++] = conn;
      }

      conn->AddRef();
    }

    m_connectionsLock.Leave();

    if (disconnectingCount > 0) {
      tv.tv_sec = 0;
      tv.tv_usec = 0;
    }

    select(maxSocket + 1, &rfds, &wfds, &efds, &tv);

    if (FD_ISSET(s_workerPipe[0], &rfds)) {
      while (recv(s_workerPipe[0], &c, 1, 0) > 0) {
      }
    }

    for (int i = 0; i < numConns; ++i) {
      WowConnection *conn = conns[i];
      UINT           flags = 0;

      if (conn->m_sock >= 0) {
        if (FD_ISSET(conn->m_sock, &wfds)) {
          flags |= 1;
        }
        if (FD_ISSET(conn->m_sock, &rfds)) {
          flags |= 2;
        }
        if (FD_ISSET(conn->m_sock, &efds)) {
          flags |= 4;
        }
      }

      if (conn->m_connState == WOWC_DISCONNECTING) {
        flags |= 8;
      }

      if (flags) {
        SignalWorker(conn, flags);
      }

      conn->Release();
    }
  }
}
