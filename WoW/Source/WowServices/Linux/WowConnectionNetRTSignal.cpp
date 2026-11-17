#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
#include <Base/Base.h>

#include "WowServices/WowConnectionNet.h"

NODEDECL(FREEFD) {
  int index;
};

static struct {
  SCritSect lock;
  pollfd    pfds[16384];
  int       maxIndex;
  LISTDECL(FREEFD, freeList);
} s_platform;

static int            s_workerPipe[2] = {-1, -1};
static pid_t          s_pid;
static BYTE           s_usingEngine;
static sigset_t       s_sigset;
static WowConnection *s_fdToConnection[32768];
static int            s_fdToIndex[32768];
static BYTE           s_fdCheck[32768];

void WowConnectionNet::PlatformInit(bool useEngine) {
  s_usingEngine = useEngine;
  if (useEngine) {
    OsNetInitialize(0, 7);
  }

  sigemptyset(&s_sigset);
  sigaddset(&s_sigset, SIGRTMIN);
  sigaddset(&s_sigset, SIGIO);
  sigprocmask(SIG_BLOCK, &s_sigset, 0);

  memset(s_fdToConnection, 0, sizeof(s_fdToConnection));

  for (int i = 1; i < sizeof(s_platform.pfds) / sizeof(s_platform.pfds[0]); ++i) {
    s_platform.pfds[i].fd = -1;
    FREEFD *fd = s_platform.freeList.NewNode(LIST_TAIL, 0, 0);
    fd->index = i;
  }
}

void WowConnectionNet::PlatformDestroy() {
  if (s_usingEngine) {
    OsNetDestroy(7);
  }
}

void WowConnectionNet::PlatformAdd(WowConnection *conn) {
  if (conn->m_oldsock >= 0 || conn->m_sock < 0) {
    return;
  }

  ASSERT(!s_fdToConnection[conn->m_sock]);
  s_fdToConnection[conn->m_sock] = conn;
  conn->m_oldsock = conn->m_sock;

  s_platform.lock.Enter();

  FREEFD *fd = s_platform.freeList.Head();
  ASSERT(fd);
  int i = fd->index;
  s_platform.freeList.DeleteNode(fd);

  s_platform.pfds[i].fd = conn->m_sock;
  s_platform.pfds[i].events = POLLIN | POLLPRI | POLLOUT | POLLERR;
  s_platform.pfds[i].revents = 0;
  s_fdToIndex[conn->m_sock] = i;

  if (i > s_platform.maxIndex) {
    s_platform.maxIndex = i;
  }

  ASSERT(s_pid != 0);
  f_owner_ex owner;
  owner.type = F_OWNER_TID;
  owner.pid = s_pid;
  fcntl(conn->m_sock, F_SETOWN_EX, &owner);
  fcntl(conn->m_sock, F_SETSIG, SIGRTMIN);
  fcntl(conn->m_sock, F_SETFL, fcntl(conn->m_sock, F_GETFL) | O_NONBLOCK | O_ASYNC);
  ASSERT(i < (sizeof(s_platform.pfds) / sizeof(s_platform.pfds[0])));

  s_fdCheck[conn->m_sock] = 1;
  s_platform.lock.Leave();
  PlatformWorkerReady();
}

void WowConnectionNet::PlatformRemove(WowConnection *conn) {
  if (conn->m_oldsock < 0) {
    return;
  }

  ASSERT(s_fdToConnection[conn->m_oldsock] == conn);
  SInterlockedExchangePointer((LPVOID *)&s_fdToConnection[conn->m_oldsock], 0);

  s_platform.lock.Enter();

  int     i = s_fdToIndex[conn->m_oldsock];
  FREEFD *fd = s_platform.freeList.NewNode(LIST_HEAD, 0, 0);
  fd->index = i;
  s_platform.pfds[i].fd = -1;
  s_platform.pfds[i].events = 0;

  if (i == s_platform.maxIndex) {
    while (i > 0 && s_platform.pfds[i].fd < 0) {
      --i;
    }
    s_platform.maxIndex = i;
  }

  s_fdCheck[conn->m_oldsock] = 0;
  s_platform.lock.Leave();

  conn->m_oldsock = -1;
  PlatformWorkerReady();
}

void WowConnectionNet::PlatformChangeState(WowConnection *conn, WOW_CONN_STATE oldState) {
  switch (conn->m_connState) {
    case WOWC_CONNECTING:
    case WOWC_LISTENING:
      PlatformRemove(conn);
      PlatformAdd(conn);
      return;

    case WOWC_CONNECTED:
    case WOWC_DISCONNECTING:
      break;

    default:
      PlatformRemove(conn);
      return;
  }

  s_platform.lock.Enter();
  if (conn->m_oldsock >= 0) {
    s_fdCheck[conn->m_oldsock] = 1;
  }
  s_platform.lock.Leave();
  PlatformWorkerReady();
}

void WowConnectionNet::PlatformDestruct(WowConnection *conn) {
}

void WowConnectionNet::PlatformWorkerReady() {
  char c = 1;
  write(s_workerPipe[1], &c, 1);
}

static void MakeSocketPipe(int pipes[2]) {
  sockaddr_in addr;
  sockaddr_in incoming;
  socklen_t   len;
  int         listener = socket(AF_INET, SOCK_STREAM, 0);

  if (listener < 0) {
    return;
  }

  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr("127.0.0.1");

  WORD port = (WORD)(rand() % 10000 + 40000);
  int  b;

  while (1) {
    addr.sin_port = htons(port);
    b = bind(listener, (const sockaddr *)&addr, sizeof(addr));
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
  if (connect(pipes[1], (const sockaddr *)&addr, sizeof(addr)) < 0) {
    ASSERT(0);
  }

  fcntl(pipes[1], F_SETFL, O_NONBLOCK);

  len = sizeof(incoming);
  pipes[0] = accept(listener, (sockaddr *)&incoming, &len);
  ASSERT(pipes[0] >= 0);

  f_owner_ex owner;
  owner.type = F_OWNER_TID;
  owner.pid = s_pid;
  fcntl(pipes[0], F_SETOWN_EX, &owner);
  fcntl(pipes[0], F_SETFL, O_NONBLOCK | O_ASYNC);
  fcntl(pipes[0], F_SETSIG, SIGRTMIN);
  close(listener);
}

void WowConnectionNet::PlatformRun() {
  s_pid = gettid();
  MakeSocketPipe(s_workerPipe);
  s_platform.pfds[0].fd = s_workerPipe[0];
  s_platform.pfds[0].events = POLLIN | POLLPRI;

  while (!m_stop) {
    timespec  timeout;
    siginfo_t info;
    int       signum;

    s_platform.pfds[0].revents = 0;
    timeout.tv_sec = 5;
    timeout.tv_nsec = 0;

    while ((signum = sigtimedwait(&s_sigset, &info, &timeout)) != -1) {
      BYTE overflow = 0;
      int  i;

      timeout.tv_sec = 0;
      timeout.tv_nsec = 0;

      if (signum == SIGIO) {
        m_connectionsLock.Enter();
        s_platform.lock.Enter();
        signal(SIGRTMIN, SIG_IGN);
        signal(SIGRTMIN, SIG_DFL);
        for (i = 1; i < s_platform.maxIndex + 1; ++i) {
          if (s_platform.pfds[i].fd >= 0) {
            s_fdCheck[s_platform.pfds[i].fd] = 1;
          }
        }
        overflow = 1;
      } else if (signum == SIGRTMIN) {
        ASSERT(signum == SIGRTMIN);

        if (info.si_fd != s_workerPipe[0]) {
          UINT flags = 0;

          m_connectionsLock.Enter();
          s_platform.lock.Enter();

          WowConnection *conn = s_fdToConnection[info.si_fd];
          if (conn) {
            if (info.si_band & POLLOUT) {
              flags = 1;
            }
            if (info.si_band & (POLLIN | POLLPRI | POLLERR | POLLHUP)) {
              flags |= 2;
            }
            if (info.si_band & POLLERR) {
              flags |= 4;
            }
            if (flags) {
              s_fdCheck[info.si_fd] = 1;
              if (conn->m_serviceCount) {
                flags = 0;
              } else {
                conn->AddRef();
              }
            }
          }

          s_platform.lock.Leave();
          m_connectionsLock.Leave();

          if (flags) {
            SignalWorker(conn, flags);
            conn->Release();
          }
          continue;
        }

        char c;
        while (read(s_workerPipe[0], &c, 1) > 0) {
        }

        m_connectionsLock.Enter();
        s_platform.lock.Enter();
      } else {
        continue;
      }

      for (i = 1; i < s_platform.maxIndex + 1; ++i) {
        int fd = s_platform.pfds[i].fd;

        if (fd < 0 || !s_fdCheck[fd] || !s_fdToConnection[fd]) {
          continue;
        }

        WowConnection *conn = s_fdToConnection[fd];
        switch (conn->m_connState) {
          case WOWC_CONNECTING:
            s_platform.pfds[i].events = POLLOUT;
            break;
          case WOWC_LISTENING:
            s_platform.pfds[i].events = POLLIN | POLLPRI;
            break;
          case WOWC_CONNECTED:
            s_platform.pfds[i].events = POLLIN | POLLPRI | POLLERR | POLLHUP;
            if (conn->m_sendList.Head() || conn->m_wantWriteNotification) {
              s_platform.pfds[i].events |= POLLOUT;
            }
            break;
          default:
            s_platform.pfds[i].events = 0;
            break;
        }
      }

      int count = poll(s_platform.pfds, s_platform.maxIndex + 1, 0);
      if (overflow && count > 0) {
        printf("poll: %d\n", count);
      }

      for (i = 1; i < s_platform.maxIndex + 1; ++i) {
        int fd = s_platform.pfds[i].fd;

        if (fd < 0 || !s_fdCheck[fd]) {
          continue;
        }

        WowConnection *conn = s_fdToConnection[fd];
        if (!conn || conn->m_serviceCount) {
          continue;
        }

        UINT  flags = 0;
        short revents = s_platform.pfds[i].revents;

        if (conn->m_sock >= 0) {
          if (revents & POLLOUT) {
            if (overflow) {
              printf("Poll write ready %d\n", fd);
            }
            flags = 1;
          }
          if (revents & (POLLIN | POLLPRI | POLLERR | POLLHUP)) {
            if (overflow) {
              printf("Poll read ready %d\n", fd);
            }
            flags |= 2;
          }
          if (revents & POLLERR) {
            if (overflow) {
              printf("Poll error ready %d\n", fd);
            }
            flags |= 4;
          }
        }

        if (conn->m_connState == WOWC_DISCONNECTING) {
          flags |= 8;
        }

        if (!flags) {
          s_fdCheck[fd] = 0;
          continue;
        }

        conn->AddRef();
        s_platform.lock.Leave();
        m_connectionsLock.Leave();
        SignalWorker(conn, flags);
        conn->Release();
        m_connectionsLock.Enter();
        s_platform.lock.Enter();
      }

      s_platform.lock.Leave();
      m_connectionsLock.Leave();
    }
  }
}
