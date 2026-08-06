#include "Os/W32/OsTcp.h"
#include <Base/Base.h>

#include <stdarg.h>

#include "Os/OsTime.h"
#include "Os/W32/Debugging.h"

namespace OsNet {

  static HASHKEY_NONE s_hashKey;
  static LPCSTR       OSNET_LOGFILE = "OsNetLog.txt";

  LPCSTR OSNETERR_INTERNAL = "Internal error";
  LPCSTR OSNETERR_WINSOCKSTARTUP = "WSAStartup failed";
  LPCSTR OSNETERR_WINSOCKVERSION = "WSAStartup wrong version";
  LPCSTR OSNETERR_SENDFAILED = "Send failed";
  LPCSTR OSNETERR_THREADFAILED = "CreateThread failed";
  LPCSTR OSNETERR_EVENTFAILED = "CreateEvent failed";
  LPCSTR OSNETERR_BINDFAILED = "Bind failed";
  LPCSTR OSNETERR_LISTENFAILED = "Listen failed";
  LPCSTR OSNETERR_SOCKETFAILED = "Socket failed";
  LPCSTR OSNETERR_SELECTFAILED = "Select failed";
  LPCSTR OSNETERR_ACCEPTFAILED = "Accept failed";
  LPCSTR OSNETERR_PORTFAILED = "CreateIoCompletionPort failed";
  LPCSTR OSNETERR_OVERLAPTYPE = "Unknown overlap type";
  LPCSTR OSNETERR_LISTENCLOSED = "Listen closed";
  LPCSTR OSNETERR_ACCEPTEXFAILED = "AcceptEx failed";

  CInitCritSect TCPNET::s_initLock;
  DWORD         TCPNET::s_initCount[5];
  TCPNET *volatile TCPNET::s_pnet;
  volatile int              TCPNET::s_baseShutdown;
  volatile int              TCPNET::s_pumpShutdown;
  volatile int              TCPNET::s_tcpShutdown;
  int                       TCPNET::s_preTerminateHostAddr;
  WSADATA                   TCPNET::s_wsaData;
  HMODULE                   TCPNET::s_mswsockModule;
  HMODULE                   TCPNET::s_ws2Module;
  LPFN_ACCEPTEX             TCPNET::s_AcceptEx;
  LPFN_GETACCEPTEXSOCKADDRS TCPNET::s_GetAcceptExSockaddrs;
  LPFN_WSASEND              TCPNET::s_WSASend;
  LPFN_WSARECV              TCPNET::s_WSARecv;
  HSLOG                     TCPNET::s_log;
  int                       TCPNET::s_qpcexists;
  float                     TCPNET::s_qpctoms;

  template <class T, int LINKOFFSET, int SLOTS>
  TSSlottedListEx<T, LINKOFFSET, SLOTS>::TSSlottedListEx() {
  }

  template <class T, int LINKOFFSET, int SLOTS>
  TSSlottedListEx<T, LINKOFFSET, SLOTS>::~TSSlottedListEx() {
  }

  template <class T, int LINKOFFSET, int SLOTS>
  TSExplicitList<T, LINKOFFSET> &TSSlottedListEx<T, LINKOFFSET, SLOTS>::UnlinkAll(TSExplicitList<T, LINKOFFSET> &list) {
    int slot;

    for (slot = 0; slot < SLOTS; ++slot) {
      m_locks[slot].Enter();
      while (m_lists[slot].Head()) {
        list.LinkNode(m_lists[slot].Head(), LIST_TAIL, 0);
        m_count.Dec();
      }
      m_locks[slot].Leave();
    }

    return list;
  }

  template <>
  void TSSlottedListEx<NETCONN, 8, 8>::Iterator::Advance() {
    m_curr = m_next;
    if (!m_curr) {
      long markSlot = m_slot;
      do {
        m_slottedList.m_locks[m_slot].Leave();
        m_slot = (static_cast<BYTE>(m_slot) + 1) & 7;
        m_slottedList.m_locks[m_slot].Enter();
        m_curr = m_slottedList.m_lists[m_slot].Head();
      } while (!m_curr && m_slot != markSlot);
    }

    if (m_curr) {
      m_next = m_slottedList.m_lists[m_slot].Next(m_curr);
    }
  }

  template <>
  void TSSlottedListEx<TCPLISTEN, 8, 1>::Iterator::Advance() {
    m_curr = m_next;
    if (!m_curr) {
      long markSlot = m_slot;
      do {
        m_slottedList.m_locks[m_slot].Leave();
        m_slot = (static_cast<BYTE>(m_slot) + 1) & (1 - 1);
        m_slottedList.m_locks[m_slot].Enter();
        m_curr = m_slottedList.m_lists[m_slot].Head();
      } while (!m_curr && m_slot != markSlot);
    }

    if (m_curr) {
      m_next = m_slottedList.m_lists[m_slot].Next(m_curr);
    }
  }

  template <>
  void TSSlottedListEx<NETCONNECT, 8, 1>::Iterator::Advance() {
    m_curr = m_next;
    if (!m_curr) {
      long markSlot = m_slot;
      do {
        m_slottedList.m_locks[m_slot].Leave();
        m_slot = (static_cast<BYTE>(m_slot) + 1) & (1 - 1);
        m_slottedList.m_locks[m_slot].Enter();
        m_curr = m_slottedList.m_lists[m_slot].Head();
      } while (!m_curr && m_slot != markSlot);
    }

    if (m_curr) {
      m_next = m_slottedList.m_lists[m_slot].Next(m_curr);
    }
  }

  OUTPUT::~OUTPUT() {
    if (m_dataBytes) {
      FREEIFUSED(m_data);
    }

    if (m_completionEvent) {
      m_completionEvent->Set();
    }
  }

  int NETSELSOCK::IsClosed() const {
    return m_sock == INVALID_SOCKET;
  }

  void NETCONN::IncIo() {
  }

  void NETCONN::DecIo() {
  }

  void NETCONN::CompleteWrite(NETOVERLAP *poverlap, DWORD bytes) {
  }

  void NETCONN::CompleteRead(NETOVERLAP *poverlap, DWORD bytes) {
  }

  void NETCONNFULL::SetNagle(int enable) {
  }

  int NETCONNFULL::SetWindow(DWORD size) {
    return 0;
  }

  void NETCONNFULL::SetRecvTimeout(DWORD timeoutMs) {
  }

  int FILECONN::IsClosed() const {
    return m_file == INVALID_HANDLE_VALUE;
  }

  NETCONNECT::~NETCONNECT() {
    if (m_data) {
      FREE(m_data);
    }
  }

  int LOOPCONNECT::IsClosed() const {
    return 0;
  }

  int FILECONNECT::IsClosed() const {
    return m_file == INVALID_HANDLE_VALUE;
  }

  TCPHOSTADDRINFO::~TCPHOSTADDRINFO() {
    FREEIFUSED(m_hostNameList);
    CloseHandle(m_thread);
  }

  void TCPHOSTADDRINFO::Complete() {
    m_hostAddrProc(m_addrs.Ptr(), m_addrs.Count(), m_user);
  }

  void NETSELECTSETS::Clear() {
    int selectSet;

    for (selectSet = SELECTSET_FIRST; selectSet < SELECT_MAX; ++selectSet) {
      FD_ZERO(&m_sets[selectSet]);
    }

    m_selsockTable.Clear();
  }

  void NETSELECTSETS::AddSelSock(NETSELSOCK *selsock) {
    NETSELSOCKPTR *selsockPtr = m_selsockTable.New(selsock->m_sock, s_hashKey, 0, 0);

    selsockPtr->ptr = selsock;
  }

  void NETSELECTSETS::AddToSet(NETSELSOCK *selsock, SELECTSET selectSet) {
    FD_SET(selsock->m_sock, &m_sets[selectSet]);
  }

  int NETSELECTSETS::Select(DWORD timeoutTotal, long selsockTotal) {
    timeval timeout;
    int     result;
    UINT    selectSet;

    timeout.tv_sec = 0;
    timeout.tv_usec = 1000 * timeoutTotal / ((static_cast<DWORD>(selsockTotal) >> 6) + 1);

    result = select(
        0, m_sets[SELECTSET_R].fd_count ? &m_sets[SELECTSET_R] : 0, m_sets[SELECTSET_W].fd_count ? &m_sets[SELECTSET_W] : 0,
        m_sets[SELECTSET_E].fd_count ? &m_sets[SELECTSET_E] : 0, timeoutTotal ? &timeout : 0
    );
    if (result == SOCKET_ERROR) {
      TCPNET::LogWrite(OSNETERR_SELECTFAILED);
      return 0;
    }

    if (result) {
      for (selectSet = SELECTSET_FIRST; selectSet < SELECT_MAX; ++selectSet) {
        fd_set *set = &m_sets[selectSet];

        while (set->fd_count) {
          NETSELSOCKPTR *selsockPtr = m_selsockTable.Ptr(set->fd_array[--set->fd_count], s_hashKey);

          selsockPtr->ptr->Selected(m_net, static_cast<SELECTSET>(selectSet));
        }
      }
    }

    return 1;
  }

  TCPACCEPT::TCPACCEPT(TCPLISTEN *listen) : m_listen(listen), m_sock(INVALID_SOCKET) {
    memset(&m_overlap.m_overlapped, 0, sizeof(m_overlap.m_overlapped));
    m_overlap.m_type = OVERLAPTYPE_ACCEPT;
    Init();
  }

  TCPACCEPT::~TCPACCEPT() {
    if (m_sock != INVALID_SOCKET) {
      closesocket(m_sock);
    }
  }

  void TCPACCEPT::Init() {
    DWORD size;

    m_lock.Enter();

    if (m_listen->m_enabled) {
      if (m_sock == INVALID_SOCKET) {
        m_sock = socket(AF_INET, SOCK_STREAM, 0);
        if (m_sock == INVALID_SOCKET) {
          TCPNET::LogWrite("%s 1", OSNETERR_SOCKETFAILED);
          m_lock.Leave();
          return;
        }

        if (!TCPNET::s_AcceptEx(m_listen->m_sock, m_sock, m_addr, 0, 0x20, 0x20, &size, &m_overlap.m_overlapped) &&
            WSAGetLastError() != ERROR_IO_PENDING)
        {
          TCPNET::LogWrite(OSNETERR_ACCEPTEXFAILED);
          m_lock.Leave();
          return;
        }
      }
    } else if (m_sock != INVALID_SOCKET) {
      closesocket(m_sock);
      m_sock = INVALID_SOCKET;
    }

    m_lock.Leave();
  }

  UINT TCPACCEPT::Complete(NETCONNADDR *connAddr, int makeSock) {
    int      selfSize;
    int      peerSize;
    NETADDR *pselfAddr;
    NETADDR *ppeerAddr;
    UINT     sock;

    m_lock.Enter();

    if (!m_listen->m_enabled) {
      closesocket(m_sock);
      m_sock = INVALID_SOCKET;
      m_lock.Leave();
      return INVALID_SOCKET;
    }

    sock = m_sock;
    if (makeSock) {
      TCPNET::s_GetAcceptExSockaddrs(
          m_addr, 0, 0x20, 0x20, reinterpret_cast<sockaddr **>(&pselfAddr), &selfSize, reinterpret_cast<sockaddr **>(&ppeerAddr), &peerSize
      );
      connAddr->peerAddr = *ppeerAddr;
      connAddr->selfAddr = *pselfAddr;
      setsockopt(sock, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, reinterpret_cast<LPCSTR>(&m_listen->m_sock), sizeof(m_listen->m_sock));
    } else {
      closesocket(sock);
      sock = INVALID_SOCKET;
    }

    m_sock = INVALID_SOCKET;
    Init();
    m_lock.Leave();
    return sock;
  }

  TCPLISTEN::TCPLISTEN(UINT sock, WORD port, NETEVENTPROC eventProc, LPVOID user, DWORD acceptCount)
      : NETSELSOCK(sock), m_portAddr(port), m_eventProc(eventProc), m_user(user), m_enabled(1) {
    while (acceptCount) {
      TCPACCEPT *accept = new (ALLOC(sizeof(TCPACCEPT))) TCPACCEPT(this);
      m_acceptList.LinkNode(accept, LIST_TAIL, 0);
      --acceptCount;
    }
  }

  TCPLISTEN::~TCPLISTEN() {
    m_acceptList.Clear();
    if (m_sock != INVALID_SOCKET) {
      closesocket(m_sock);
    }
  }

  int TCPLISTEN::Enable(int enable) {
    if (m_enabled == enable) {
      return 0;
    }

    m_enabled = enable;
    if (listen(m_sock, enable ? 250 : 0)) {
      TCPNET::LogWrite("%s 1", OSNETERR_LISTENFAILED);
      closesocket(m_sock);
      m_sock = INVALID_SOCKET;
      return 0;
    }

    ITERATELIST(TCPACCEPT, m_acceptList, accept) {
      accept->Init();
    }

    return 1;
  }

  void TCPLISTEN::Close() {
    if (m_sock != INVALID_SOCKET) {
      Enable(0);
      closesocket(m_sock);
      m_sock = INVALID_SOCKET;
    }
  }

  void TCPLISTEN::AddToSelectSets(NETSELECTSETS *selectSets) {
    selectSets->AddSelSock(this);
    selectSets->AddToSet(this, SELECTSET_R);
  }

  void TCPLISTEN::Selected(TCPNET *pnet, SELECTSET selectSet) {
    NETCONNADDR connAddr;
    int         addrSize;
    UINT        sock;

    if (selectSet != SELECTSET_R) {
      TCPNET::LogWrite("%s 1", OSNETERR_INTERNAL);
      return;
    }

    memset(&connAddr, 0, sizeof(connAddr));
    addrSize = sizeof(connAddr.peerAddr);
    sock = accept(m_sock, reinterpret_cast<sockaddr *>(&connAddr.peerAddr), &addrSize);
    if (sock == INVALID_SOCKET) {
      TCPNET::LogWrite(OSNETERR_ACCEPTFAILED);
      return;
    }

    if (!m_enabled) {
      closesocket(sock);
      return;
    }

    addrSize = sizeof(connAddr.selfAddr);
    getsockname(sock, reinterpret_cast<sockaddr *>(&connAddr.selfAddr), &addrSize);
    pnet->CompleteAccept(this, sock, &connAddr);
  }

  void NETCONNECT::NoteCantConnect(NETEVENTPROC eventProc, const NETCONNADDR *pconnAddr) {
    DWORD bytesProcessed = 0;

    if (eventProc) {
      eventProc(0, pconnAddr, NETNOTE_CANTCONNECT, m_user, 0, 0, &bytesProcessed);
    }
  }

  void LOOPCONNECT::AddToSelectSets(NETSELECTSETS *) {
    TCPNET::LogWrite("%s 2", OSNETERR_INTERNAL);
  }

  void LOOPCONNECT::Selected(TCPNET *, SELECTSET) {
    TCPNET::LogWrite("%s 3", OSNETERR_INTERNAL);
  }

  void LOOPCONNECT::Fail() {
    NETCONNADDR connAddr;

    memset(&connAddr, 0, sizeof(connAddr));
    NoteCantConnect(m_eventProcSrc, &connAddr);
  }

  void LOOPCONNECT::Complete(TCPNET *pnet) {
    if (m_eventProcDst) {
      pnet->LoopCompleteConnect(this);
    } else {
      Fail();
    }
  }

  void TCPCONNECT::Fail() {
    NETCONNADDR  connAddr;
    sockaddr_in *peerAddr;

    memset(&connAddr, 0, sizeof(connAddr));
    peerAddr = reinterpret_cast<sockaddr_in *>(&connAddr.peerAddr);
    peerAddr->sin_family = AF_INET;
    peerAddr->sin_port = htons(static_cast<WORD>(m_portAddr));
    peerAddr->sin_addr.s_addr = m_nodeNumber;
    NoteCantConnect(m_eventProc, &connAddr);

    if (m_sock != INVALID_SOCKET) {
      closesocket(m_sock);
      m_sock = INVALID_SOCKET;
    }
  }

  void TCPCONNECT::Complete(TCPNET *pnet) {
    pnet->TcpCompleteConnect(this);
  }

  void TCPCONNECT::AddToSelectSets(NETSELECTSETS *selectSets) {
    selectSets->AddSelSock(this);
    selectSets->AddToSet(this, SELECTSET_W);
    selectSets->AddToSet(this, SELECTSET_E);
  }

  void TCPCONNECT::Selected(TCPNET *pnet, SELECTSET selectSet) {
    if (selectSet == SELECTSET_W) {
      Complete(pnet);
    } else if (selectSet == SELECTSET_E) {
      Fail();
    } else {
      TCPNET::LogWrite("%s 4", OSNETERR_INTERNAL);
    }
  }

  void FILECONNECT::Selected(TCPNET *, SELECTSET) {
    TCPNET::LogWrite("%s 5", OSNETERR_INTERNAL);
  }

  void FILECONNECT::AddToSelectSets(NETSELECTSETS *) {
    TCPNET::LogWrite("%s 6", OSNETERR_INTERNAL);
  }

  void FILECONNECT::Fail() {
    NETCONNADDR connAddr;
    memset(&connAddr, 0, sizeof(connAddr));
    NoteCantConnect(m_eventProc, &connAddr);
  }

  void FILECONNECT::Complete(TCPNET *pnet) {
    pnet->FileCompleteConnect(this);
  }

  void TCPNET::MakeConnAddr(UINT sock, DWORD port, NETCONNADDR *connAddr) {
    int peerSize = sizeof(connAddr->selfAddr);

    getsockname(sock, reinterpret_cast<sockaddr *>(&connAddr->selfAddr), &peerSize);
    reinterpret_cast<sockaddr_in *>(&connAddr->selfAddr)->sin_port = htons(static_cast<WORD>(port));
    *reinterpret_cast<DWORD *>(&connAddr->selfAddr.data[8]) = 0;
    *reinterpret_cast<DWORD *>(&connAddr->selfAddr.data[12]) = 0;

    peerSize = sizeof(connAddr->peerAddr);
    getpeername(sock, reinterpret_cast<sockaddr *>(&connAddr->peerAddr), &peerSize);
    reinterpret_cast<sockaddr_in *>(&connAddr->peerAddr)->sin_port = htons(static_cast<WORD>(port));
    *reinterpret_cast<DWORD *>(&connAddr->peerAddr.data[8]) = 0;
    *reinterpret_cast<DWORD *>(&connAddr->peerAddr.data[12]) = 0;
  }

  UINT TCPNET::CreateListenSocket(WORD port) {
    sockaddr_in addr;
    int         mode = 1;
    UINT        sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock == INVALID_SOCKET) {
      LogWrite("%s 2", OSNETERR_SOCKETFAILED);
      return INVALID_SOCKET;
    }

    ioctlsocket(sock, FIONBIO, reinterpret_cast<DWORD *>(&mode));
    memset(&addr, 0, sizeof(addr));
    addr.sin_port = htons(port);
    addr.sin_family = AF_INET;

    if (bind(sock, reinterpret_cast<const sockaddr *>(&addr), sizeof(addr))) {
      LogWrite("%s 1", OSNETERR_BINDFAILED);
      closesocket(sock);
      return INVALID_SOCKET;
    }

    if (listen(sock, 250)) {
      LogWrite("%s 2", OSNETERR_LISTENFAILED);
      closesocket(sock);
      return INVALID_SOCKET;
    }

    return sock;
  }

  LPVOID TCPNET::IoCompletionPresent(DWORD *pumpThreadCount) {
    SYSTEM_INFO si;
    LPVOID      port = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);

    if (port) {
      memset(&si, 0, sizeof(si));
      GetSystemInfo(&si);
      if (!si.dwNumberOfProcessors) {
        si.dwNumberOfProcessors = 1;
      }
      *pumpThreadCount = 2 * si.dwNumberOfProcessors;
    }

    return port;
  }

  void TCPNET::IncludeDependantParts(DWORD *parts) {
    *parts |= 1;
    if (*parts & 2) {
      *parts |= 8;
    }
    if (*parts & 6) {
      *parts |= 16;
    }
  }

  int TCPNET::Initialize(DWORD hints, DWORD parts) {
    OSVERSIONINFOA versionInfo;
    LARGE_INTEGER  freq;

    IncludeDependantParts(&parts);
    s_initLock.Enter();
    BYTE  selectedParts = static_cast<BYTE>(parts);
    DWORD initializedParts = 0;
    int   result = 1;

    if (selectedParts & 1) {
      initializedParts = result;
      if (s_initCount[0]++ == 0) {
        if (!SLogCreate(OSNET_LOGFILE, 0, &s_log)) {
          s_log = 0;
        }

        s_qpcexists = QueryPerformanceFrequency(&freq);
        if (s_qpcexists) {
          s_qpctoms = 1000.0f / freq.QuadPart;
        } else {
          s_qpctoms = 0.0f;
        }

        versionInfo.dwOSVersionInfoSize = sizeof(versionInfo);
        if (GetVersionExA(&versionInfo)) {
          s_preTerminateHostAddr = 1;
          if (versionInfo.dwMajorVersion >= 5) {
            s_preTerminateHostAddr = 0;
          }
        } else {
          s_preTerminateHostAddr = 0;
        }

        ASSERT(!s_pnet);
        s_pnet = NEW(TCPNET);
        if (!s_pnet->BaseInitialize(hints)) {
          goto failed;
        }
      }
    }

    if (selectedParts & 8) {
      initializedParts |= 8;
      if (s_initCount[2]++ == 0 && !s_pnet->WinsockInitialize(hints)) {
        goto failed;
      }
    }

    if (selectedParts & 16) {
      initializedParts |= 16;
      if (s_initCount[3]++ == 0 && !s_pnet->IoInitialize(hints)) {
        goto failed;
      }
    }

    if (selectedParts & 4) {
      initializedParts |= 4;
      ++s_initCount[4];
    }

    if (selectedParts & 2) {
      initializedParts |= 2;
      if (s_initCount[1]++ == 0 && !s_pnet->TcpInitialize(hints)) {
        goto failed;
      }
    }

    goto done;

  failed:
    result = 0;
    Destroy(initializedParts);

  done:
    s_initLock.Leave();
    return result;
  }

  void TCPNET::Destroy(DWORD parts) {
    IncludeDependantParts(&parts);
    s_initLock.Enter();

    if (s_pnet) {
      BYTE selectedParts = static_cast<BYTE>(parts);

      if ((selectedParts & 2) && --s_initCount[1] == 0) {
        s_pnet->TcpDestroy();
      }

      if (selectedParts & 4) {
        --s_initCount[4];
      }

      if ((selectedParts & 16) && --s_initCount[3] == 0) {
        s_pnet->IoDestroy();
      }

      if ((selectedParts & 8) && --s_initCount[2] == 0) {
        s_pnet->WinsockDestroy();
      }

      if ((selectedParts & 1) && --s_initCount[0] == 0) {
        s_pnet->BaseDestroy();
        s_pnet->DecRef();
        s_pnet = 0;

        if (s_log) {
          SLogClose(s_log);
          s_log = 0;
        }
      }
    }

    s_initLock.Leave();
  }

  void __cdecl TCPNET::LogWrite(LPCSTR format, ...) {
    char    line[1024];
    va_list arglist;

    va_start(arglist, format);
    DWORD chars = SStrPrintf(line, sizeof(line), "t[0x%08x] se[%u] we[%u] ", GetCurrentThreadId(), WSAGetLastError(), GetLastError());
    SStrVPrintf(&line[chars], sizeof(line) - chars, format, arglist);
    va_end(arglist);

    OsOutputDebugString("OsNet: (INFO): %s\n", line);
    if (s_log) {
      SLogWrite(s_log, line);
      SLogFlush(s_log);
    }
  }

  void __cdecl TCPNET::LogDump(LPCSTR header, LPCVOID data, DWORD bytes) {
    if (s_log) {
      SLogWrite(s_log, "%s", header);
      SLogDump(s_log, data, bytes);
      SLogFlush(s_log);
    }
  }

  void TCPNET::IoPump(DWORD timeout) {
    LARGE_INTEGER currtime;
    LARGE_INTEGER starttime;
    NETOVERLAP   *poverlap;
    DWORD         bytes;
    NETCONN      *pconn;
    long          elapsed = 0;

    starttime.QuadPart = 0;
    if (timeout != INFINITE) {
      if (s_qpcexists) {
        QueryPerformanceCounter(&starttime);
      } else {
        starttime.LowPart = GetTickCount();
      }
    }

    BOOL completed =
        GetQueuedCompletionStatus(m_port, &bytes, reinterpret_cast<DWORD *>(&pconn), reinterpret_cast<OVERLAPPED **>(&poverlap), timeout);

    while (!s_pumpShutdown) {
      if (timeout != INFINITE && !completed && !poverlap) {
        return;
      }

      if (pconn) {
        if (completed && poverlap) {
          switch (poverlap->m_type) {
            case OVERLAPTYPE_READ:
              pconn->CompleteRead(poverlap, bytes);
              break;

            case OVERLAPTYPE_WRITE:
              pconn->CompleteWrite(poverlap, bytes);
              break;

            default:
              LogWrite("%s 1", OSNETERR_OVERLAPTYPE);
              break;
          }
        }
        pconn->DecIo();
      } else if (poverlap) {
        if (poverlap->m_type == OVERLAPTYPE_ACCEPT) {
          CompleteAcceptEx(CONTAINING_RECORD(poverlap, TCPACCEPT, m_overlap), completed);
        } else {
          LogWrite("%s 2", OSNETERR_OVERLAPTYPE);
        }
      }

      if (timeout != INFINITE) {
        if (s_qpcexists) {
          QueryPerformanceCounter(&currtime);
          elapsed = static_cast<long>((currtime.QuadPart - starttime.QuadPart) * s_qpctoms);
          if (elapsed < 0) {
            return;
          }
        } else {
          elapsed = GetTickCount() - starttime.LowPart;
        }

        if (elapsed >= timeout) {
          return;
        }
      }

      completed =
          GetQueuedCompletionStatus(m_port, &bytes, reinterpret_cast<DWORD *>(&pconn), reinterpret_cast<OVERLAPPED **>(&poverlap), timeout - elapsed);
    }
  }

  void TCPNET::Pump(DWORD timeout) {
    if (m_pumpThreads.Count()) {
      Sleep(timeout);
    } else if (m_port) {
      IoPump(timeout);
    } else {
      LogWrite("%s 7", OSNETERR_INTERNAL);
      Sleep(timeout);
    }
  }

  UINT __stdcall TCPNET::IoPumpThread(LPVOID lpnet) {
    TCPNET *net = static_cast<TCPNET *>(lpnet);

    net->IoPump(INFINITE);
    net->WakePumpThread();
    net->DecRef();
    return 0;
  }

  UINT __stdcall TCPNET::SlPumpThread(LPVOID lpnet) {
    TCPNET                                    *net = static_cast<TCPNET *>(lpnet);
    NETSELECTSETS                              selectSets(net);
    TSSlottedListEx<NETCONN, 8, 8>::Iterator   connIt(net->m_connList[CONNLIST_TCP_CONNECTED]);
    TSSlottedListEx<TCPLISTEN, 8, 1>::Iterator listenIt(net->m_listenList);

    while (!s_pumpShutdown) {
      DWORD listenCount;
      long  selsockCount = 0;

      if (s_tcpShutdown) {
        break;
      }

      selectSets.Clear();
      if (!net->m_listenThread) {
        TCPLISTEN *listen = listenIt.CycleInit();

        while (listen && selsockCount < 8) {
          if (listen->IsClosed()) {
            DEL(listen);
            listen = listenIt.SkipDeletedAndCycleNext();
          } else {
            if (listen->m_enabled) {
              listen->AddToSelectSets(&selectSets);
              ++selsockCount;
            }
            listen = listenIt.CycleNext();
          }
        }
        listenIt.CycleDone();
      }
      listenCount = selsockCount;

      {
        TSExplicitList<NETCONN, 8> disconnectList;
        NETCONN                   *conn = connIt.CycleInit();

        while (conn && selsockCount < 64) {
          if (conn->IsClosed()) {
            net->LinkConn(conn, CONNLIST_NONE);
            disconnectList.LinkNode(conn, LIST_TAIL, 0);
            conn = connIt.SkipDeletedAndCycleNext();
          } else {
            conn->AddToSelectSets(&selectSets);
            ++selsockCount;
            conn = connIt.CycleNext();
          }
        }
        connIt.CycleDone();

        {
          long selsockTotal = net->m_connList[CONNLIST_TCP_CONNECTED].Count() + listenCount;

          while ((conn = disconnectList.Head()) != 0) {
            disconnectList.UnlinkNode(conn);
            conn->Disconnect(1);
          }

          if (selsockCount) {
            selectSets.Select(100, selsockTotal);
          } else {
            Sleep(100);
          }
        }
      }
    }

    net->DecRef();
    return 0;
  }

  UINT __stdcall TCPNET::ListenThread(LPVOID lpnet) {
    TCPNET                                    *net = static_cast<TCPNET *>(lpnet);
    NETSELECTSETS                              selectSets(net);
    TSSlottedListEx<TCPLISTEN, 8, 1>::Iterator listenIt(net->m_listenList);

    while (!s_tcpShutdown) {
      DWORD      selsockCount = 0;
      TCPLISTEN *listen;

      selectSets.Clear();
      listen = listenIt.CycleInit();
      while (listen && selsockCount < 64) {
        if (listen->IsClosed()) {
          DEL(listen);
          listen = listenIt.SkipDeletedAndCycleNext();
        } else {
          if (listen->m_enabled) {
            listen->AddToSelectSets(&selectSets);
            ++selsockCount;
          }
          listen = listenIt.CycleNext();
        }
      }
      listenIt.CycleDone();

      if (selsockCount) {
        selectSets.Select(1000, selsockCount);
      } else {
        Sleep(1000);
      }
    }

    net->DecRef();
    return 0;
  }

  TCPNET::TCPNET()
      : m_refCount(1),
        m_pumpThreadCount(0),
        m_udpPumpThread(0),
        m_udpPumpEvent(0),
        m_listenThread(0),
        m_baseThread(0),
        m_baseEvent(0),
        m_baseTcpShutdown(0),
        m_baseTcpShutdownEvent(0),
        m_port(0),
        m_hostAddrInfoCount(0),
        m_hostAddrInfoId(0) {
    LISTEXSETLINK(LOOPCONN, m_loopDisconnectList, m_linkNet);
  }

  int TCPNET::BaseInitialize(DWORD hints) {
    UINT id;

    m_baseEvent = CreateEventA(0, FALSE, FALSE, 0);
    if (!m_baseEvent) {
      return 0;
    }

    m_baseTcpShutdownEvent = CreateEventA(0, FALSE, FALSE, 0);
    if (!m_baseTcpShutdownEvent) {
      return 0;
    }

    IncRef();
    m_baseThread = SCreateThread(BaseThread, this, &id, 0, const_cast<char *>("OsTcp_Base"));
    if (!m_baseThread) {
      DecRef();
      LogWrite("%s 1", OSNETERR_THREADFAILED);
      return 0;
    }

    return 1;
  }

  int TCPNET::WinsockInitialize(DWORD hints) {
    if (WSAStartup(MAKEWORD(2, 2), &s_wsaData)) {
      LogWrite(OSNETERR_WINSOCKSTARTUP);
      return 0;
    }

    BYTE major = LOBYTE(s_wsaData.wVersion);
    BYTE minor = HIBYTE(s_wsaData.wVersion);
    if (major < 1 || (major == 1 && minor < 1)) {
      LogWrite(OSNETERR_WINSOCKVERSION);
      return 0;
    }

    s_mswsockModule = LoadLibraryA("mswsock.dll");
    if (s_mswsockModule) {
      s_AcceptEx = reinterpret_cast<LPFN_ACCEPTEX>(GetProcAddress(s_mswsockModule, "AcceptEx"));
      s_GetAcceptExSockaddrs = reinterpret_cast<LPFN_GETACCEPTEXSOCKADDRS>(GetProcAddress(s_mswsockModule, "GetAcceptExSockaddrs"));
    }

    if (major >= 2) {
      s_ws2Module = LoadLibraryA("ws2_32.dll");
      if (s_ws2Module) {
        s_WSASend = reinterpret_cast<LPFN_WSASEND>(GetProcAddress(s_ws2Module, "WSASend"));
        s_WSARecv = reinterpret_cast<LPFN_WSARECV>(GetProcAddress(s_ws2Module, "WSARecv"));
      }
    }

    return 1;
  }

  int TCPNET::IoInitialize(DWORD hints) {
    DWORD pumpThreadCount = 1;

    if (!(hints & 0x40000000)) {
      m_port = IoCompletionPresent(&pumpThreadCount);
    }

    if (static_cast<long>(hints) < 0 && !m_port) {
      return 0;
    }

    if (m_port && (hints & 1)) {
      pumpThreadCount = 0;
    }
    m_pumpThreadCount = pumpThreadCount;

    if (m_port && !PumpThreadsInitialize()) {
      return 0;
    }

    return 1;
  }

  int TCPNET::TcpInitialize(DWORD hints) {
    UINT id;

    if (!m_port && !PumpThreadsInitialize()) {
      return 0;
    }

    if (hints & 2) {
      IncRef();
      m_listenThread = SCreateThread(ListenThread, this, &id, 0, const_cast<char *>("OsTcp_Listen"));
      if (!m_listenThread) {
        DecRef();
        LogWrite("%s 2", OSNETERR_THREADFAILED);
        return 0;
      }
      SetThreadPriority(m_listenThread, THREAD_PRIORITY_BELOW_NORMAL);
    }

    m_udpPumpEvent = CreateEventA(0, FALSE, FALSE, 0);
    if (!m_udpPumpEvent) {
      return 0;
    }

    IncRef();
    m_udpPumpThread = SCreateThread(UdpPumpThread, this, &id, 0, const_cast<char *>("OsTcp_UdpPump"));
    if (!m_udpPumpThread) {
      DecRef();
      LogWrite("%s 3", OSNETERR_THREADFAILED);
      return 0;
    }

    return 1;
  }

  TCPNET::~TCPNET() {
    int list;

    for (list = 0; list < CONNLISTS; ++list) {
      NETCONNSIMPLELIST connSimpleList;
      NETCONN          *conn;

      m_connList[list].UnlinkAll(connSimpleList);
      for (conn = connSimpleList.Head(); conn;) {
        NETCONN *next = connSimpleList.Next(conn);

        conn->DecRef();
        conn = next;
      }
    }

    for (list = 0; list < CONNLISTS; ++list) {
      m_connectList[list].Clear();
    }

    m_hostAddrInfoList.UnlinkAll();
  }

  void TCPNET::IncRef() {
    m_refCount.Inc();
  }

  void TCPNET::DecRef() {
    if (!m_refCount.Dec()) {
      DEL(this);
    }
  }

  void TCPNET::BaseDestroy() {
    s_baseShutdown = 1;
    SetEvent(m_baseEvent);
    WaitForSingleObject(m_baseThread, INFINITE);
    CloseHandle(m_baseThread);
    m_baseThread = 0;

    m_connList[CONNLIST_LOOP_CONNECTED].Clear();
    m_loopInputRecycleList.Clear();

    CloseHandle(m_baseTcpShutdownEvent);
    m_baseTcpShutdownEvent = 0;
    CloseHandle(m_baseEvent);
    m_baseEvent = 0;
    s_baseShutdown = 0;
  }

  void TCPNET::WinsockDestroy() {
    if (s_ws2Module) {
      s_WSASend = 0;
      s_WSARecv = 0;
      FreeLibrary(s_ws2Module);
      s_ws2Module = 0;
    }

    if (s_mswsockModule) {
      s_AcceptEx = 0;
      s_GetAcceptExSockaddrs = 0;
      FreeLibrary(s_mswsockModule);
      s_mswsockModule = 0;
    }

    WSACleanup();

    if (!s_preTerminateHostAddr) {
      s_pnet->m_hostAddrInfoLock.Enter();

      while (TCPHOSTADDRINFO *info = s_pnet->m_hostAddrInfoList.Head()) {
        TerminateThread(info->m_thread, 0);
        FREEIFUSED(info->m_hostNameList);
        CloseHandle(info->m_thread);
        DEL(info);
        s_pnet->DecRef();
      }

      s_pnet->m_hostAddrInfoLock.Leave();
    }
  }

  void TCPNET::WakePumpThread() {
    if (m_port && m_pumpThreads.Count()) {
      PostQueuedCompletionStatus(m_port, 0, 0, 0);
    }
  }

  int TCPNET::PumpThreadsInitialize() {
    UINT   id;
    LPVOID thread;
    DWORD  i;

    for (i = 0; i < m_pumpThreadCount; ++i) {
      IncRef();

      thread = SCreateThread(m_port ? IoPumpThread : SlPumpThread, this, &id, 0, 0);
      if (!thread) {
        DecRef();
        LogWrite("%s 4", OSNETERR_THREADFAILED);
        return 0;
      }

      *m_pumpThreads.New() = thread;
    }

    return 1;
  }

  void TCPNET::PumpThreadsDestroy() {
    UINT i;

    if (m_pumpThreads.Count()) {
      WaitForMultipleObjects(m_pumpThreads.Count(), m_pumpThreads.Ptr(), TRUE, INFINITE);

      for (i = 0; i < m_pumpThreads.Count(); ++i) {
        CloseHandle(m_pumpThreads[i]);
      }

      m_pumpThreads.SetCount(0);
    }
  }

  void TCPNET::IoDestroy() {
    s_pumpShutdown = 1;
    WakePumpThread();

    if (m_port) {
      PumpThreadsDestroy();
      CloseHandle(m_port);
      m_port = 0;
    }

    m_listenList.Clear();
    m_pumpThreadCount = 0;
    s_pumpShutdown = 0;
  }

  void TCPNET::TcpDestroy() {
    m_baseTcpShutdown = 1;
    SetEvent(m_baseEvent);
    WaitForSingleObject(m_baseTcpShutdownEvent, INFINITE);

    s_tcpShutdown = 1;
    SetEvent(m_udpPumpEvent);

    if (m_listenThread) {
      SetThreadPriority(m_listenThread, THREAD_PRIORITY_NORMAL);
      WaitForSingleObject(m_listenThread, INFINITE);
      m_listenThread = 0;
    }

    if (m_udpPumpThread) {
      WaitForSingleObject(m_udpPumpThread, INFINITE);
      m_udpPumpThread = 0;
    }

    if (!m_port) {
      PumpThreadsDestroy();
    }

    CloseHandle(m_udpPumpEvent);
    m_udpPumpEvent = 0;

    TSSlottedListEx<TCPLISTEN, 8, 1>::Iterator listenIt(m_listenList);
    TCPLISTEN                                 *listen = listenIt.CycleInit();
    while (listen) {
      listen->Close();
      listen = listenIt.CycleNext();
    }
    listenIt.CycleDone();

    s_tcpShutdown = 0;
  }

  void TCPNET::TcpMakeConn(UINT sock, NETEVENTPROC eventProc, LPVOID user, const NETCONNADDR *pconnAddr, LPCVOID data, DWORD bytes) {
    if (m_port) {
      new (ALLOC(sizeof(IOTCPCONN))) IOTCPCONN(this, m_port, sock, eventProc, user, pconnAddr, data, bytes);
    } else {
      new (ALLOC(sizeof(SLTCPCONN))) SLTCPCONN(this, sock, eventProc, user, pconnAddr, data, bytes);
    }
  }

  void TCPNET::UdpMakeConn(UINT sock, NETEVENTPROC eventProc, LPVOID user, const NETCONNADDR *pconnAddr) {
    new (ALLOC(sizeof(UDPCONN))) UDPCONN(this, sock, eventProc, user, pconnAddr);
  }

  void TCPNET::FileMakeConn(LPVOID file, NETEVENTPROC eventProc, LPVOID user, const NETCONNADDR *pconnAddr) {
    if (m_port) {
      new (ALLOC(sizeof(IOFILECONN))) IOFILECONN(this, m_port, file, eventProc, user, pconnAddr);
    } else {
      new (ALLOC(sizeof(SLFILECONN))) SLFILECONN(this, file, eventProc, user, pconnAddr);
    }
  }

  void TCPNET::LoopMakeConn(NETEVENTPROC eventProcSrc, NETEVENTPROC eventProcDst, LPVOID user, LPCVOID data, DWORD bytes) {
    NETCONNADDR connAddr;
    LOOPCONN   *connDst;
    LOOPCONN   *connSrc;

    ASSERT(eventProcSrc);
    ASSERT(eventProcDst);

    memset(&connAddr, 0, sizeof(connAddr));
    connDst = new (ALLOC(sizeof(LOOPCONN))) LOOPCONN(this, eventProcDst, 0, &connAddr);
    connSrc = new (ALLOC(sizeof(LOOPCONN))) LOOPCONN(this, eventProcSrc, user, &connAddr);

    m_loopLock.Enter();
    connSrc->IncRef();
    connDst->m_loopConn = connSrc;
    connDst->IncRef();
    connSrc->m_loopConn = connDst;
    if (data && bytes) {
      connDst->EnqueueInput(data, bytes);
    }
    m_loopLock.Leave();

    connDst->Connect();
    connSrc->Connect();
  }

  void TCPNET::CompleteAcceptEx(TCPACCEPT *paccept, int makeConn) {
    NETCONNADDR connAddr;
    UINT        sock = paccept->Complete(&connAddr, makeConn);

    if (sock != INVALID_SOCKET) {
      TcpMakeConn(sock, paccept->m_listen->m_eventProc, paccept->m_listen->m_user, &connAddr, 0, 0);
    }
  }

  void TCPNET::CompleteAccept(TCPLISTEN *plisten, UINT sock, const NETCONNADDR *pconnAddr) {
    TcpMakeConn(sock, plisten->m_eventProc, plisten->m_user, pconnAddr, 0, 0);
  }

  void TCPNET::LoopCompleteConnect(LOOPCONNECT *pconnect) {
    LoopMakeConn(pconnect->m_eventProcSrc, pconnect->m_eventProcDst, pconnect->m_user, pconnect->m_data, pconnect->m_bytes);
  }

  void TCPNET::TcpCompleteConnect(TCPCONNECT *pconnect) {
    NETCONNADDR connAddr;

    MakeConnAddr(pconnect->m_sock, pconnect->m_portAddr, &connAddr);
    TcpMakeConn(pconnect->m_sock, pconnect->m_eventProc, pconnect->m_user, &connAddr, pconnect->m_data, pconnect->m_bytes);
    pconnect->m_sock = INVALID_SOCKET;
  }

  void TCPNET::FileCompleteConnect(FILECONNECT *pconnect) {
    NETCONNADDR   connAddr;
    LARGE_INTEGER fileSize;

    memset(&connAddr, 0, sizeof(connAddr));
    fileSize.LowPart = GetFileSize(static_cast<HANDLE>(pconnect->m_file), reinterpret_cast<DWORD *>(&fileSize.HighPart));
    connAddr.selfAddr.file.pos = fileSize.QuadPart;
    FileMakeConn(pconnect->m_file, pconnect->m_eventProc, pconnect->m_user, &connAddr);
    pconnect->m_file = INVALID_HANDLE_VALUE;
  }

  int TCPNET::TcpListen(WORD port, NETEVENTPROC eventProc, LPVOID user) {
    if (!eventProc) {
      return 0;
    }

    TSSlottedListEx<TCPLISTEN, 8, 1>::Iterator listenIt(m_listenList);
    DWORD                                      acceptCount = 0;
    TCPLISTEN                                 *listen = listenIt.CycleInit();
    while (listen) {
      if (!listen->IsClosed() && listen->m_portAddr == port) {
        listen->m_eventProc = eventProc;
        listen->m_user = user;
        listen->Enable(1);
        listenIt.CycleDone();
        return 1;
      }
      listen = listenIt.CycleNext();
    }
    listenIt.CycleDone();

    UINT sock = CreateListenSocket(port);
    if (sock == INVALID_SOCKET) {
      return 0;
    }

    if (!m_listenThread && m_port) {
      if (!CreateIoCompletionPort(reinterpret_cast<HANDLE>(sock), m_port, 0, 0)) {
        LogWrite("%s 1", OSNETERR_PORTFAILED);
        closesocket(sock);
        return 0;
      }
      acceptCount = 16;
    }

    listen = new (ALLOC(sizeof(TCPLISTEN))) TCPLISTEN(sock, port, eventProc, user, acceptCount);
    m_listenList.Link(listen);
    return 1;
  }

  void TCPNET::TcpListenEnable(WORD port, int enable) {
    TSSlottedListEx<TCPLISTEN, 8, 1>::Iterator listenIt(m_listenList);
    TCPLISTEN                                 *listen = listenIt.CycleInit();

    while (listen) {
      if (listen->m_portAddr == port) {
        if (!listen->IsClosed()) {
          listen->Enable(enable);
        } else {
          LogWrite(OSNETERR_LISTENCLOSED);
        }
        break;
      }
      listen = listenIt.CycleNext();
    }
    listenIt.CycleDone();
  }

  void TCPNET::LoopConnectInit(LOOPCONNECT *pconnect) {
    if (!pconnect->m_eventProcSrc) {
      DEL(pconnect);
      return;
    }

    m_connectList[CONNECTLIST_LOOP_CONNECTED].Link(pconnect);
    SetEvent(m_baseEvent);
  }

  void TCPNET::TcpConnectInit(TCPCONNECT *pconnect) {
    if (!pconnect->m_eventProc) {
      TSSlottedListEx<TCPLISTEN, 8, 1>::Iterator listenIt(m_listenList);
      TCPLISTEN                                 *listen = listenIt.CycleInit();

      while (listen) {
        if (!listen->IsClosed() && listen->m_portAddr == pconnect->m_portAddr) {
          pconnect->m_eventProc = listen->m_eventProc;
          break;
        }
        listen = listenIt.CycleNext();
      }
      listenIt.CycleDone();
    }

    if (!pconnect->m_eventProc) {
      DEL(pconnect);
      return;
    }

    CONNECTLIST connectList = CONNECTLIST_TCP_CONNECTED;

    pconnect->m_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (pconnect->m_sock == INVALID_SOCKET) {
      LogWrite("%s 3", OSNETERR_SOCKETFAILED);
    } else {
      sockaddr_in addr;
      int         mode = 1;

      ioctlsocket(pconnect->m_sock, FIONBIO, reinterpret_cast<DWORD *>(&mode));
      memset(&addr, 0, sizeof(addr));
      addr.sin_family = AF_INET;
      addr.sin_port = htons(static_cast<WORD>(pconnect->m_portAddr));
      addr.sin_addr.s_addr = pconnect->m_nodeNumber;
      if (::connect(pconnect->m_sock, reinterpret_cast<const sockaddr *>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        if (WSAGetLastError() == WSAEWOULDBLOCK) {
          connectList = CONNECTLIST_TCP_CONNECTING;
        } else {
          closesocket(pconnect->m_sock);
          pconnect->m_sock = INVALID_SOCKET;
        }
      }
    }

    m_connectList[connectList].Link(pconnect);
    SetEvent(m_baseEvent);
  }

  void TCPNET::FileConnectInit(FILECONNECT *pconnect) {
    if (!pconnect->m_eventProc) {
      if (pconnect->m_file != INVALID_HANDLE_VALUE) {
        CloseHandle(static_cast<HANDLE>(pconnect->m_file));
        pconnect->m_file = INVALID_HANDLE_VALUE;
      }
      DEL(pconnect);
      return;
    }

    m_connectList[CONNECTLIST_FILE_CONNECTED].Link(pconnect);
    SetEvent(m_baseEvent);
  }

  UINT __stdcall TCPNET::BaseThread(LPVOID lpnet) {
    TCPNET                  *net = static_cast<TCPNET *>(lpnet);
    NETSELECTSETS            selectSets(net);
    NETCONNECTLIST::Iterator connectIt(net->m_connectList[CONNECTLIST_TCP_CONNECTING]);

    for (;;) {
      long connectCount = 0;
      int  list;

      for (list = 0; list < CONNECTLISTS; ++list) {
        connectCount += net->m_connectList[list].Count();
      }

      if (!connectCount) {
        if (s_baseShutdown) {
          break;
        }
        WaitForSingleObject(net->m_baseEvent, INFINITE);
      }
      if (s_baseShutdown) {
        break;
      }

      if (net->m_baseTcpShutdown) {
        net->m_baseTcpShutdown = 0;

        {
          LISTDECLEX(NETCONNECT, m_link, connectFailList);
          NETCONNECT *connect;

          net->m_connectList[CONNECTLIST_TCP_CONNECTING].UnlinkAll(connectFailList);
          net->m_connectList[CONNECTLIST_TCP_CONNECTED].UnlinkAll(connectFailList);
          connectIt.Reset();

          connect = connectFailList.Head();
          while (connect) {
            NETCONNECT *next = connectFailList.Next(connect);

            if (!connect->IsClosed()) {
              connect->Fail();
            }
            DEL(connect);
            connect = next;
          }
        }

        {
          LISTDECL(TCPHOSTADDRINFO, hostAddrInfoFailList);
          TCPHOSTADDRINFO *info;

          net->m_hostAddrInfoLock.Enter();
          while ((info = net->m_hostAddrInfoList.Head()) != 0) {
            if (s_preTerminateHostAddr) {
              TerminateThread(info->m_thread, 0);
            }
            info->m_infoId = 0;
            hostAddrInfoFailList.LinkNode(info, LIST_TAIL, 0);
            net->m_hostAddrInfoCount.Dec();
          }
          net->m_hostAddrInfoLock.Leave();

          info = hostAddrInfoFailList.Head();
          while (info) {
            TCPHOSTADDRINFO *next = hostAddrInfoFailList.Next(info);

            info->m_hostAddrProc(0, 0, info->m_user);
            if (s_preTerminateHostAddr) {
              DEL(info);
              net->DecRef();
            }
            info = next;
          }

          if (!s_preTerminateHostAddr) {
            s_pnet->m_hostAddrInfoLock.Enter();
            s_pnet->m_hostAddrInfoList.Combine(&hostAddrInfoFailList, LIST_TAIL, 0);
            s_pnet->m_hostAddrInfoLock.Leave();
          }

          SetEvent(net->m_baseTcpShutdownEvent);
        }
      }

      {
        LISTDECLEX(LOOPCONN::INPUT, m_linkNet, loopInputList);
        LISTEXDYN(LOOPCONN) loopDisconnectList;
        LISTEXSETLINK(LOOPCONN, loopDisconnectList, m_linkNet);
        LOOPCONN::INPUT *pinput;
        LOOPCONN        *conn;

        net->m_loopLock.Enter();
        loopInputList.Combine(&net->m_loopInputList, LIST_TAIL, 0);
        loopDisconnectList.Combine(&net->m_loopDisconnectList, LIST_TAIL, 0);

        for (pinput = loopInputList.Head(); pinput; pinput = loopInputList.Next(pinput)) {
          pinput->m_link.Unlink();
        }
        net->m_loopLock.Leave();

        for (pinput = loopInputList.Head(); pinput; pinput = loopInputList.Next(pinput)) {
          pinput->m_conn->CompleteInput(pinput);
        }

        net->m_loopLock.Enter();
        pinput = loopInputList.Head();
        while (pinput) {
          LOOPCONN::INPUT *next = loopInputList.Next(pinput);

          net->LoopFreeInput(pinput);
          pinput = next;
        }
        net->m_loopLock.Leave();

        while ((conn = loopDisconnectList.Head()) != 0) {
          loopDisconnectList.UnlinkNode(conn);
          conn->Disconnect(1);
        }
      }

      selectSets.Clear();
      {
        LISTDECLEX(NETCONNECT, m_link, connectCompleteList);
        long        selsockCount = 0;
        long        selsockTotal;
        NETCONNECT *connect = connectIt.CycleInit();

        while (connect && selsockCount < 64) {
          if (connect->IsClosed()) {
            connectIt.Unlink();
            DEL(connect);
            connect = connectIt.SkipDeletedAndCycleNext();
          } else {
            connect->AddToSelectSets(&selectSets);
            ++selsockCount;
            connect = connectIt.CycleNext();
          }
        }
        connectIt.CycleDone();

        net->m_connectList[CONNECTLIST_LOOP_CONNECTED].UnlinkAll(connectCompleteList);
        net->m_connectList[CONNECTLIST_TCP_CONNECTED].UnlinkAll(connectCompleteList);
        net->m_connectList[CONNECTLIST_FILE_CONNECTED].UnlinkAll(connectCompleteList);
        selsockTotal = net->m_connectList[CONNECTLIST_TCP_CONNECTING].Count();

        connect = connectCompleteList.Head();
        while (connect) {
          NETCONNECT *next = connectCompleteList.Next(connect);

          if (connect->IsClosed()) {
            connect->Fail();
          } else {
            connect->Complete(net);
          }
          DEL(connect);
          connect = next;
        }

        if (selsockCount) {
          selectSets.Select(100, selsockTotal);
        }
      }

      if (net->m_hostAddrInfoCount > 0) {
        LISTDECL(TCPHOSTADDRINFO, hostAddrInfoReadyList);
        TCPHOSTADDRINFO *info;

        net->m_hostAddrInfoLock.Enter();
        while ((info = net->m_hostAddrInfoList.Head()) != 0) {
          if (info->m_ready) {
            hostAddrInfoReadyList.LinkNode(info, LIST_TAIL, 0);
            net->m_hostAddrInfoCount.Dec();
          }
        }
        net->m_hostAddrInfoLock.Leave();

        while ((info = hostAddrInfoReadyList.Head()) != 0) {
          info->Complete();
          DEL(info);
          net->DecRef();
        }
      }
    }

    {
      LISTDECLEX(NETCONNECT, m_link, connectFailList);
      NETCONNECT *connect;

      net->m_connectList[CONNECTLIST_LOOP_CONNECTED].UnlinkAll(connectFailList);
      connect = connectFailList.Head();
      while (connect) {
        NETCONNECT *next = connectFailList.Next(connect);

        if (!connect->IsClosed()) {
          connect->Fail();
        }
        DEL(connect);
        connect = next;
      }
    }

    net->DecRef();
    return 0;
  }

  UINT __stdcall TCPNET::GetHostAddrsThread(LPVOID lpparam) {
    char    hostName[1024];
    int     hostAddrInfoFound;
    TCPNET *pnet;
    NETADDR netAddr;
    WORD    defaultPort;
    char  **hostAddr;
    DWORD   infoId;

    TCPHOSTADDRTHREAD *param = static_cast<TCPHOSTADDRTHREAD *>(lpparam);
    pnet = param->m_net;
    infoId = param->m_infoId;
    defaultPort = param->m_defaultPort;
    SetEvent(param->m_event);

    for (;;) {
      hostName[0] = 0;
      hostAddrInfoFound = 0;

      TCPHOSTADDRINFO *info;
      if (!pnet->m_hostAddrInfoLock.Enter()) {
        info = pnet->LockedFindHostAddrInfo(infoId);
        if (info) {
          if (info->m_hostNameCurr) {
            char *nextHostName = SStrChr(info->m_hostNameCurr, ';');
            if (nextHostName) {
              *nextHostName++ = 0;
            }
            SStrCopy(hostName, info->m_hostNameCurr, sizeof(hostName));
            info->m_hostNameCurr = nextHostName;
          }
          hostAddrInfoFound = 1;
        }
      }
      pnet->m_hostAddrInfoLock.Leave();

      if (!hostAddrInfoFound) {
        return 0;
      }

      if (!hostName[0]) {
        hostAddrInfoFound = 0;
        if (!pnet->m_hostAddrInfoLock.Enter()) {
          info = pnet->LockedFindHostAddrInfo(infoId);
          if (info) {
            info->m_ready = 1;
            hostAddrInfoFound = 1;
          }
        }
        pnet->m_hostAddrInfoLock.Leave();
        if (hostAddrInfoFound) {
          SetEvent(pnet->m_baseEvent);
        }
        return 0;
      }

      if (hostName[0] >= '0' && hostName[0] <= '9') {
        OsNetAddrMakeFromStr(hostName, defaultPort, &netAddr);
        hostAddrInfoFound = 0;
        if (!pnet->m_hostAddrInfoLock.Enter()) {
          info = pnet->LockedFindHostAddrInfo(infoId);
          if (info) {
            *info->m_addrs.New() = netAddr;
            hostAddrInfoFound = 1;
          }
        }
        pnet->m_hostAddrInfoLock.Leave();
        if (!hostAddrInfoFound) {
          return 0;
        }
      } else {
        hostent *host = gethostbyname(hostName);
        if (host && host->h_addrtype == AF_INET && host->h_length == 4) {
          hostAddrInfoFound = 0;
          if (!pnet->m_hostAddrInfoLock.Enter()) {
            info = pnet->LockedFindHostAddrInfo(infoId);
            if (info) {
              hostAddr = host->h_addr_list;
              if (*hostAddr) {
                hostAddrInfoFound = 1;
                do {
                  OsNetAddrMake(*reinterpret_cast<DWORD *>(*hostAddr), defaultPort, &netAddr);
                  *info->m_addrs.New() = netAddr;
                } while (*++hostAddr);
              }
            }
          }
          pnet->m_hostAddrInfoLock.Leave();
          if (!hostAddrInfoFound) {
            return 0;
          }
        }
      }
    }
  }

  void TCPNET::LoopConnect(NETEVENTPROC eventProcSrc, NETEVENTPROC eventProcDst, LPVOID user, LPCVOID data, DWORD bytes) {
    LOOPCONNECT *connect = NEW(LOOPCONNECT);

    connect->m_eventProcSrc = eventProcSrc;
    connect->m_eventProcDst = eventProcDst;
    connect->m_user = user;
    if (data && bytes) {
      connect->m_data = ALLOC(bytes);
      memcpy(connect->m_data, data, bytes);
      connect->m_bytes = bytes;
      LoopConnectInit(connect);
    } else {
      connect->m_data = 0;
      connect->m_bytes = 0;
      LoopConnectInit(connect);
    }
  }

  void TCPNET::TcpConnect(DWORD nodeNumber, WORD port, NETEVENTPROC eventProc, LPVOID user, LPCVOID data, DWORD bytes) {
    if (!nodeNumber) {
      NETEVENTPROC                               eventProcDst = 0;
      TSSlottedListEx<TCPLISTEN, 8, 1>::Iterator listenIt(m_listenList);
      TCPLISTEN                                 *listen = listenIt.CycleInit();

      while (listen) {
        if (!listen->IsClosed() && listen->m_portAddr == port) {
          eventProcDst = listen->m_eventProc;
          break;
        }
        listen = listenIt.CycleNext();
      }
      listenIt.CycleDone();

      LoopConnect(eventProc, eventProcDst, user, data, bytes);
      return;
    }

    TCPCONNECT *connect = NEW(TCPCONNECT);
    connect->m_portAddr = port;
    connect->m_nodeNumber = nodeNumber;
    connect->m_eventProc = eventProc;
    connect->m_user = user;
    if (data && bytes) {
      connect->m_data = ALLOC(bytes);
      memcpy(connect->m_data, data, bytes);
      connect->m_bytes = bytes;
    } else {
      connect->m_data = 0;
      connect->m_bytes = 0;
    }
    connect->m_sock = INVALID_SOCKET;
    TcpConnectInit(connect);
  }

  UINT __stdcall TCPNET::UdpPumpThread(LPVOID lpnet) {
    TCPNET                                  *net = static_cast<TCPNET *>(lpnet);
    NETSELECTSETS                            selectSets(net);
    TSSlottedListEx<NETCONN, 8, 8>::Iterator connIt(net->m_connList[CONNLIST_UDP_CONNECTED]);

    for (;;) {
      while (!net->m_connList[CONNLIST_UDP_CONNECTED].Count()) {
        if (s_tcpShutdown) {
          break;
        }
        WaitForSingleObject(net->m_udpPumpEvent, INFINITE);
      }
      if (s_tcpShutdown) {
        break;
      }

      selectSets.Clear();
      {
        TSExplicitList<NETCONN, 8> disconnectList;
        long                       selsockCount = 0;
        NETCONN                   *conn = connIt.CycleInit();

        while (conn && selsockCount < 64) {
          if (conn->IsClosed()) {
            net->LinkConn(conn, CONNLIST_NONE);
            disconnectList.LinkNode(conn, LIST_TAIL, 0);
            conn = connIt.SkipDeletedAndCycleNext();
          } else {
            conn->AddToSelectSets(&selectSets);
            ++selsockCount;
            conn = connIt.CycleNext();
          }
        }
        connIt.CycleDone();

        {
          long selsockTotal = net->m_connList[CONNLIST_UDP_CONNECTED].Count();

          while ((conn = disconnectList.Head()) != 0) {
            conn->Disconnect(1);
          }

          if (selsockCount) {
            selectSets.Select(100, selsockTotal);
          } else {
            Sleep(100);
          }
        }
      }
    }

    net->DecRef();
    return 0;
  }

  void TCPNET::UdpConnect(const NETADDR *addr, WORD portMin, WORD portMax, NETEVENTPROC eventProc, LPVOID user) {
    int  enabled;
    int  mode;
    UINT sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (sock != INVALID_SOCKET) {
      mode = 1;
      ioctlsocket(sock, FIONBIO, reinterpret_cast<DWORD *>(&mode));
      enabled = 1;
      setsockopt(sock, SOL_SOCKET, SO_BROADCAST, reinterpret_cast<LPCSTR>(&enabled), sizeof(enabled));

      NETCONNADDR connAddr;
      memset(&connAddr, 0, sizeof(connAddr));
      while (portMin <= portMax) {
        reinterpret_cast<sockaddr_in *>(&connAddr.selfAddr)->sin_family = AF_INET;
        reinterpret_cast<sockaddr_in *>(&connAddr.selfAddr)->sin_port = htons(portMin);
        reinterpret_cast<sockaddr_in *>(&connAddr.selfAddr)->sin_addr.s_addr =
            addr ? reinterpret_cast<const sockaddr_in *>(addr)->sin_addr.s_addr : INADDR_ANY;
        if (!bind(sock, reinterpret_cast<const sockaddr *>(&connAddr.selfAddr), sizeof(connAddr.selfAddr))) {
          UdpMakeConn(sock, eventProc, user, &connAddr);
          SetEvent(m_udpPumpEvent);
          return;
        }
        if (portMin++ == portMax) {
          break;
        }
      }
      LogWrite("%s 2", OSNETERR_BINDFAILED);
    } else {
      LogWrite("%s 4", OSNETERR_SOCKETFAILED);
    }

    if (eventProc) {
      NETCONNADDR connAddr;
      DWORD       bytesProcessed;

      memset(&connAddr, 0, sizeof(connAddr));
      reinterpret_cast<sockaddr_in *>(&connAddr.selfAddr)->sin_family = AF_INET;
      reinterpret_cast<sockaddr_in *>(&connAddr.selfAddr)->sin_addr.s_addr =
          addr ? reinterpret_cast<const sockaddr_in *>(addr)->sin_addr.s_addr : INADDR_ANY;
      eventProc(0, &connAddr, NETNOTE_CANTCONNECT, user, 0, 0, &bytesProcessed);
    }
  }

  void TCPNET::FileConnCreate(LPCSTR fileName, NETEVENTPROC eventProc, LPVOID user, int readOnly) {
    LPVOID file = CreateFileA(
        fileName, readOnly ? GENERIC_READ : GENERIC_READ | GENERIC_WRITE, readOnly ? FILE_SHARE_READ | FILE_SHARE_WRITE : FILE_SHARE_READ, 0,
        CREATE_ALWAYS, FILE_FLAG_RANDOM_ACCESS | (m_port ? FILE_FLAG_OVERLAPPED : 0), 0
    );
    FILECONNECT *connect = NEW(FILECONNECT);

    connect->m_eventProc = eventProc;
    connect->m_user = user;
    connect->m_data = 0;
    connect->m_bytes = 0;
    connect->m_file = file;
    FileConnectInit(connect);
  }

  TCPHOSTADDRINFO *TCPNET::LockedFindHostAddrInfo(DWORD infoId) {
    TCPHOSTADDRINFO *info = m_hostAddrInfoList.Head();

    while (info && info->m_infoId != infoId) {
      info = m_hostAddrInfoList.Next(info);
    }
    return info;
  }

  int TCPNET::GetHostAddrs(LPCSTR hostNameList, WORD defaultPort, NETHOSTADDRPROC hostAddrProc, LPVOID user) {
    TCPHOSTADDRTHREAD hostAddrThreadParam;
    UINT              id;

    FATALASSERT(hostNameList);
    FATALASSERT(hostAddrProc);

    TCPHOSTADDRINFO *info = NEW(TCPHOSTADDRINFO);
    info->m_hostNameList = static_cast<char *>(ALLOC(SStrLen(hostNameList) + 1));
    SStrCopy(info->m_hostNameList, hostNameList, 0x7FFFFFFF);
    info->m_hostNameCurr = info->m_hostNameList;
    info->m_hostAddrProc = hostAddrProc;
    info->m_user = user;
    info->m_ready = 0;

    hostAddrThreadParam.m_net = this;
    hostAddrThreadParam.m_event = CreateEventA(0, FALSE, FALSE, 0);
    hostAddrThreadParam.m_defaultPort = defaultPort;

    m_hostAddrInfoLock.Enter();
    do {
      ++m_hostAddrInfoId;
    } while (!m_hostAddrInfoId || LockedFindHostAddrInfo(m_hostAddrInfoId));
    info->m_infoId = m_hostAddrInfoId;
    hostAddrThreadParam.m_infoId = m_hostAddrInfoId;

    IncRef();
    LPVOID thread = SCreateThread(GetHostAddrsThread, &hostAddrThreadParam, &id, 0, const_cast<char *>("OsTcp_DNS"));
    if (thread) {
      info->m_thread = thread;
      m_hostAddrInfoList.LinkNode(info, LIST_TAIL, 0);
      m_hostAddrInfoLock.Leave();
      m_hostAddrInfoCount.Inc();
      WaitForSingleObject(hostAddrThreadParam.m_event, INFINITE);
      if (hostAddrThreadParam.m_event) {
        CloseHandle(hostAddrThreadParam.m_event);
      }
      return 1;
    }

    DecRef();
    DEL(info);
    LogWrite("%s 5", OSNETERR_THREADFAILED);
    m_hostAddrInfoLock.Leave();
    if (hostAddrThreadParam.m_event) {
      CloseHandle(hostAddrThreadParam.m_event);
    }
    return 0;
  }

  void TCPNET::LinkConn(NETCONN *pconn, CONNLIST tolist) {
    UINT toslot = 0;

    pconn->m_lock.Enter();
    if (pconn->m_list != CONNLIST_NONE) {
      m_connList[pconn->m_list].Unlink(pconn, pconn->m_listSlot);
    }
    if (tolist != CONNLIST_NONE) {
      toslot = m_connList[tolist].Link(pconn);
    }
    pconn->m_list = tolist;
    pconn->m_listSlot = toslot;
    pconn->m_lock.Leave();
  }

  LOOPCONN::INPUT *TCPNET::LoopAllocInput(DWORD bytes) {
    LOOPCONN::INPUT *pinput = 0;

    if (bytes <= sizeof(reinterpret_cast<LOOPCONN *>(0)->m_data)) {
      pinput = m_loopInputRecycleList.Head();
      if (pinput) {
        m_loopInputRecycleList.UnlinkNode(pinput);
      } else {
        bytes = sizeof(reinterpret_cast<LOOPCONN *>(0)->m_data);
      }
    }

    if (!pinput) {
      pinput = new (ALLOC(sizeof(LOOPCONN::INPUT) + bytes - sizeof(pinput->m_data))) LOOPCONN::INPUT;
      pinput->m_dataBytes = bytes;
    }

    return pinput;
  }

  void TCPNET::LoopFreeInput(LOOPCONN::INPUT *pinput) {
    if (pinput->m_dataBytes == sizeof(reinterpret_cast<LOOPCONN *>(0)->m_data)) {
      m_loopInputRecycleList.LinkNode(pinput, LIST_HEAD, 0);
    } else {
      DEL(pinput);
    }
  }

  NETCONN::NETCONN(TCPNET *net, UINT sock, NETEVENTPROC eventProc, LPVOID user, const NETCONNADDR *pconnAddr)
      : NETSELSOCK(sock),
        m_list(CONNLIST_NONE),
        m_listSlot(0),
        m_refCount(1),
        m_eventProcUserLock(0),
        m_eventProc(eventProc),
        m_user(user),
        m_net(net) {
    m_time = GetTickCount();
    m_connAddr = *pconnAddr;
  }

  void NETCONN::Disconnect(int notify) {
    Close();
    m_net->LinkConn(this, CONNLIST_NONE);
    if (notify) {
      NoteDisconnect();
    }
    DecRef();
  }

  void NETCONN::GetEventProcAndUser(NETEVENTPROC &eventProc, LPVOID &user) {
    int eventProcUserLock;

    do {
      do {
        eventProcUserLock = m_eventProcUserLock;
      } while (eventProcUserLock & 1);

      eventProc = m_eventProc;
      user = m_user;
    } while (eventProcUserLock != m_eventProcUserLock);
  }

  void NETCONN::SetEventProcAndUser(NETEVENTPROC eventProc, LPVOID user) {
    long eventProcUserLock;

    do {
      eventProcUserLock = m_eventProcUserLock & ~1;
    } while (SInterlockedCompareExchange(&m_eventProcUserLock, eventProcUserLock | 1, eventProcUserLock) != eventProcUserLock);

    m_eventProc = eventProc;
    m_user = user;
    ++*reinterpret_cast<volatile int *>(&m_eventProcUserLock);
  }

  void NETCONN::SetEventProc(NETEVENTPROC eventProc) {
    long eventProcUserLock;

    do {
      eventProcUserLock = m_eventProcUserLock & ~1;
    } while (SInterlockedCompareExchange(&m_eventProcUserLock, eventProcUserLock | 1, eventProcUserLock) != eventProcUserLock);

    ++*reinterpret_cast<volatile int *>(&m_eventProcUserLock);
    m_eventProc = eventProc;
  }

  void NETCONN::SetUser(LPVOID user) {
    long eventProcUserLock;

    do {
      eventProcUserLock = m_eventProcUserLock & ~1;
    } while (SInterlockedCompareExchange(&m_eventProcUserLock, eventProcUserLock | 1, eventProcUserLock) != eventProcUserLock);

    ++*reinterpret_cast<volatile int *>(&m_eventProcUserLock);
    m_user = user;
  }

  int NETCONN::NoteCantConnect() {
    DWORD        bytesProcessed;
    LPVOID       user;
    NETEVENTPROC eventProc;

    GetEventProcAndUser(eventProc, user);
    if (eventProc) {
      return eventProc(0, &m_connAddr, NETNOTE_CANTCONNECT, user, 0, 0, &bytesProcessed);
    }
    return 0;
  }

  int NETCONN::NoteConnect() {
    DWORD        bytesProcessed;
    LPVOID       user;
    NETEVENTPROC eventProc;

    GetEventProcAndUser(eventProc, user);
    if (eventProc) {
      return eventProc(reinterpret_cast<HNETCONN__ *>(this), &m_connAddr, NETNOTE_CONNECT, user, 0, 0, &bytesProcessed);
    }
    return 0;
  }

  int NETCONN::NoteDisconnect() {
    DWORD        bytesProcessed;
    LPVOID       user;
    NETEVENTPROC eventProc;

    GetEventProcAndUser(eventProc, user);
    if (eventProc) {
      return eventProc(reinterpret_cast<HNETCONN__ *>(this), &m_connAddr, NETNOTE_DISCONNECT, user, 0, 0, &bytesProcessed);
    }
    return 0;
  }

  int NETCONN::NoteData(LPVOID data, DWORD bytes, DWORD *bytesProcessed, const NETCONNADDR *connAddr) {
    LPVOID       user;
    NETEVENTPROC eventProc;

    GetEventProcAndUser(eventProc, user);
    if (eventProc) {
      if (!connAddr) {
        connAddr = &m_connAddr;
      }
      return eventProc(reinterpret_cast<HNETCONN__ *>(this), connAddr, NETNOTE_DATA, user, data, bytes, bytesProcessed);
    }
    return 0;
  }

  int NETCONN::NoteFileOperation(LPVOID data, DWORD bytes, DWORD offset, DWORD offsetHigh, LPVOID operationId, NETNOTE note) {
    NETCONNADDR  connAddr;
    DWORD        bytesProcessed;
    NETEVENTPROC eventProc;
    LPVOID       user;

    memset(&connAddr, 0, sizeof(connAddr));
    connAddr.selfAddr.file.pos = (static_cast<DWORDLONG>(offsetHigh) << 32) | offset;
    connAddr.selfAddr.file.operationId = operationId;
    bytesProcessed = 0;
    GetEventProcAndUser(eventProc, user);

    if (eventProc) {
      return eventProc(reinterpret_cast<HNETCONN__ *>(this), &connAddr, note, user, data, bytes, &bytesProcessed);
    }
    return 0;
  }

  void NETCONN::IncRef() {
    m_refCount.Inc();
  }

  void NETCONN::DecRef() {
    if (!m_refCount.Dec()) {
      DEL(this);
    }
  }

  void NETCONN::ConnAddr(NETCONNADDR *connAddr) {
    if (m_sock != INVALID_SOCKET) {
      *connAddr = m_connAddr;
    } else {
      memset(connAddr, 0, sizeof(*connAddr));
    }
  }

  void NETCONN::Close() {
    m_lock.Enter();
    CloseAndUnlock();
  }

  LOOPCONN::LOOPCONN(TCPNET *net, NETEVENTPROC eventProc, LPVOID user, const NETCONNADDR *pconnAddr)
      : NETCONNFULL(net, INVALID_SOCKET, eventProc, user, pconnAddr), m_loopConn(0), m_bytes(0) {
  }

  LOOPCONN::~LOOPCONN() {
    ASSERT(!m_inputList.Head());
    ASSERT(!m_linkNet.IsLinked());
    m_net->LinkConn(this, CONNLIST_NONE);
  }

  void LOOPCONN::AddToSelectSets(NETSELECTSETS *selectSets) {
    TCPNET::LogWrite("%s 8", OSNETERR_INTERNAL);
  }

  void LOOPCONN::Selected(TCPNET *pnet, SELECTSET selectSet) {
    TCPNET::LogWrite("%s 9", OSNETERR_INTERNAL);
  }

  void LOOPCONN::Connect() {
    LOOPCONN *loopConn;

    if (!NoteConnect()) {
      Disconnect(1);
      return;
    }

    m_net->m_loopLock.Enter();
    m_net->LinkConn(this, CONNLIST_LOOP_CONNECTED);
    loopConn = m_loopConn;
    if (!loopConn) {
      m_net->m_loopDisconnectList.LinkNode(this, LIST_TAIL, 0);
    }
    m_net->m_loopLock.Leave();

    if (!loopConn || m_inputList.Head()) {
      SetEvent(m_net->m_baseEvent);
    }
  }

  void LOOPCONN::CompleteInput(INPUT *pinput) {
    BYTE *inputData;
    DWORD inputBytes;

    if (!m_loopConn) {
      return;
    }

    inputData = pinput->m_data;
    inputBytes = pinput->m_bytes;

    while (inputBytes && m_bytes) {
      DWORD bytes = sizeof(m_data) - m_bytes;
      DWORD bytesProcessed = 0;

      if (bytes > inputBytes) {
        bytes = inputBytes;
      }

      memcpy(&m_data[m_bytes], inputData, bytes);
      m_bytes += bytes;
      inputData += bytes;
      inputBytes -= bytes;

      if (!NoteData(m_data, m_bytes, &bytesProcessed, 0)) {
        Close();
        return;
      }

      if (bytesProcessed >= m_bytes) {
        m_bytes = 0;
      } else if (bytesProcessed) {
        memmove(m_data, &m_data[bytesProcessed], m_bytes - bytesProcessed);
        m_bytes -= bytesProcessed;
      } else if (m_bytes >= sizeof(m_data)) {
        Close();
        return;
      }
    }

    if (!inputBytes) {
      return;
    }

    ASSERT(!m_bytes);

    {
      DWORD bytesProcessed = 0;

      if (!NoteData(inputData, inputBytes, &bytesProcessed, 0)) {
        Close();
        return;
      }

      if (bytesProcessed < inputBytes) {
        DWORD bytes = inputBytes - bytesProcessed;

        if (bytes >= sizeof(m_data)) {
          Close();
          return;
        }

        memcpy(m_data, &inputData[bytesProcessed], bytes);
        m_bytes = bytes;
      }
    }
  }

  void LOOPCONN::EnqueueInput(LPCVOID data, DWORD bytes) {
    INPUT *pinput = m_inputList.Tail();

    if (pinput && pinput->m_bytes < pinput->m_dataBytes) {
      DWORD copyBytes = pinput->m_dataBytes - pinput->m_bytes;

      if (copyBytes > bytes) {
        copyBytes = bytes;
      }

      memcpy(&pinput->m_data[pinput->m_bytes], data, copyBytes);
      pinput->m_bytes += copyBytes;
      data = static_cast<const BYTE *>(data) + copyBytes;
      bytes -= copyBytes;
    }

    if (bytes) {
      pinput = m_net->LoopAllocInput(bytes);
      pinput->m_bytes = bytes;
      pinput->m_conn = this;
      memcpy(pinput->m_data, data, bytes);
      m_inputList.LinkNode(pinput, LIST_TAIL, 0);
      m_net->m_loopInputList.LinkNode(pinput, LIST_TAIL, 0);
    }
  }

  void LOOPCONN::Send(LPCVOID data, DWORD bytes) {
    LOOPCONN *loopConn;

    if (!bytes) {
      return;
    }

    m_net->m_loopLock.Enter();
    loopConn = m_loopConn;
    if (loopConn) {
      loopConn->EnqueueInput(data, bytes);
    }
    m_net->m_loopLock.Leave();

    if (loopConn) {
      SetEvent(m_net->m_baseEvent);
    }
  }

  OS_SEND LOOPCONN::SendSync(LPCVOID data, DWORD bytes, DWORD *bytesSent, DWORD) {
    Send(data, bytes);
    *bytesSent = bytes;
    return OS_SEND_OK;
  }

  void LOOPCONN::CloseAndUnlock() {
    LOOPCONN *loopConn = m_loopConn;

    if (!loopConn) {
      m_net->m_loopLock.Leave();
      return;
    }

    ASSERT(loopConn->m_loopConn == this);
    loopConn->m_loopConn = 0;

    while (INPUT *pinput = loopConn->m_inputList.Head()) {
      DEL(pinput);
    }
    if (loopConn->ConnList() == CONNLIST_LOOP_CONNECTED) {
      m_net->m_loopDisconnectList.LinkNode(loopConn, LIST_TAIL, 0);
    }

    m_loopConn = 0;
    while (INPUT *pinput = m_inputList.Head()) {
      DEL(pinput);
    }
    if (ConnList() == CONNLIST_LOOP_CONNECTED) {
      m_net->m_loopDisconnectList.LinkNode(this, LIST_TAIL, 0);
    }

    m_net->m_loopLock.Leave();
    SetEvent(m_net->m_baseEvent);
    loopConn->DecRef();
    DecRef();
  }

  void LOOPCONN::Close() {
    m_net->m_loopLock.Enter();
    CloseAndUnlock();
  }

  int LOOPCONN::IsClosed() const {
    LOOPCONN *loopConn;

    m_net->m_loopLock.Enter();
    loopConn = m_loopConn;
    m_net->m_loopLock.Leave();
    return !loopConn;
  }

  UDPCONN::UDPCONN(TCPNET *net, UINT sock, NETEVENTPROC eventProc, LPVOID user, const NETCONNADDR *pconnAddr)
      : NETCONNLESS(net, sock, eventProc, user, pconnAddr) {
    if (NoteConnect()) {
      m_net->LinkConn(this, CONNLIST_UDP_CONNECTED);
    } else {
      Disconnect(1);
    }
  }

  UDPCONN::~UDPCONN() {
    m_net->LinkConn(this, CONNLIST_NONE);
  }

  void UDPCONN::SendTo(LPCVOID data, DWORD bytes, DWORD addrCount, const NETADDR *addrArray) {
    FATALASSERT(addrArray);

    while (addrCount) {
      --addrCount;
      sendto(
          m_sock, reinterpret_cast<LPCSTR>(data), bytes, 0, reinterpret_cast<const sockaddr *>(&addrArray[addrCount]), sizeof(addrArray[addrCount])
      );
    }
  }

  void UDPCONN::CloseAndUnlock() {
    if (m_sock != INVALID_SOCKET) {
      closesocket(m_sock);
      m_sock = INVALID_SOCKET;
    }
    m_lock.Leave();
  }

  void UDPCONN::AddToSelectSets(NETSELECTSETS *selectSets) {
    selectSets->AddSelSock(this);
    selectSets->AddToSet(this, SELECTSET_R);
  }

  void UDPCONN::Selected(TCPNET *pnet, SELECTSET selectSet) {
    BYTE        data[1460];
    NETCONNADDR connAddr;
    DWORD       bytesProcessed;
    int         addrSize;

    if (selectSet != SELECTSET_R) {
      TCPNET::LogWrite("%s 10", OSNETERR_INTERNAL);
      return;
    }

    memset(&connAddr, 0, sizeof(connAddr));
    addrSize = sizeof(connAddr.peerAddr);
    addrSize = recvfrom(m_sock, reinterpret_cast<char *>(data), sizeof(data), 0, reinterpret_cast<sockaddr *>(&connAddr.peerAddr), &addrSize);
    if (addrSize > 0) {
      bytesProcessed = 0;
      NoteData(data, addrSize, &bytesProcessed, &connAddr);
    }
  }

  TCPCONN::TCPCONN(TCPNET *net, UINT sock, NETEVENTPROC eventProc, LPVOID user, const NETCONNADDR *pconnAddr)
      : NETCONNFULL(net, sock, eventProc, user, pconnAddr), m_bytes(0) {
    int mode = 1;
    int flag = 1;

    setsockopt(m_sock, SOL_SOCKET, SO_KEEPALIVE, reinterpret_cast<LPCSTR>(&flag), sizeof(flag));

    ioctlsocket(m_sock, FIONBIO, reinterpret_cast<DWORD *>(&mode));
  }

  TCPCONN::~TCPCONN() {
    m_outputList.Clear();
    m_net->LinkConn(this, CONNLIST_NONE);
  }

  void TCPCONN::CompleteWrite(NETOVERLAP *poverlap, DWORD bytes) {
    OUTPUT *poutput;

    m_lock.Enter();
    poutput = CONTAINING_RECORD(poverlap, OUTPUT, m_overlap);
    poutput->m_state = OUTPUTSTATE_COMPLETED;

    while ((poutput = m_outputList.Head()) != 0) {
      if (poutput->m_state != OUTPUTSTATE_COMPLETED) {
        StartWriteAndLeaveLock(poutput);
        return;
      }

      DEL(poutput);
    }

    m_lock.Leave();
  }

  void TCPCONN::CompleteRead(NETOVERLAP *poverlap, DWORD bytes) {
    DWORD bytesProcessed;

    if (!bytes) {
      return;
    }

    m_bytes += bytes;
    bytesProcessed = 0;
    if (NoteData(m_data, m_bytes, &bytesProcessed, 0)) {
      if (bytesProcessed >= m_bytes) {
        m_bytes = 0;
        goto startRead;
      } else if (bytesProcessed) {
        memmove(m_data, &m_data[bytesProcessed], m_bytes - bytesProcessed);
        m_bytes -= bytesProcessed;
        StartRead();
        return;
      }

      if (m_bytes >= sizeof(m_data)) {
        Close();
        return;
      }

    startRead:
      StartRead();
      return;
    }

    Close();
  }

  OUTPUT *TCPCONN::LockedEnqueue(LPCVOID data, DWORD bytes) {
    OUTPUT *poutput;
    DWORD   time;

    time = GetTickCount();
    poutput = m_outputList.Head();

    if (poutput && time - m_time > 600000 && time - poutput->m_sock.m_time > 120000) {
      Close();
      return 0;
    }

    poutput = m_outputList.Tail();
    if (poutput && poutput->m_state == OUTPUTSTATE_WAITING && poutput->m_bytes < poutput->m_dataBytes) {
      DWORD copyBytes = poutput->m_dataBytes - poutput->m_bytes;

      if (copyBytes > bytes) {
        copyBytes = bytes;
      }

      memcpy(&poutput->m_data[poutput->m_bytes], data, copyBytes);
      poutput->m_bytes += copyBytes;
      data = static_cast<const BYTE *>(data) + copyBytes;
      bytes -= copyBytes;
    }

    if (bytes) {
      poutput = NEW(OUTPUT);
      poutput->m_overlap.Init(OVERLAPTYPE_WRITE);
      poutput->m_completionEvent = 0;
      poutput->m_sock.m_time = time;
      poutput->m_state = OUTPUTSTATE_WAITING;
      poutput->m_bytes = bytes;
      poutput->m_dataBytes = bytes > sizeof(m_data) ? bytes : sizeof(m_data);
      poutput->m_data = static_cast<BYTE *>(ALLOC(poutput->m_dataBytes));
      memcpy(poutput->m_data, data, bytes);
      m_outputList.LinkNode(poutput, LIST_TAIL, 0);
    }

    return poutput;
  }

  void TCPCONN::Send(LPCVOID data, DWORD bytes) {
    int     oldOutput;
    int     sent;
    int     error;
    OUTPUT *poutput;

    m_lock.Enter();
    if (m_sock == INVALID_SOCKET) {
      m_lock.Leave();
      return;
    }

    oldOutput = m_outputList.Head() != 0;
    if (!oldOutput) {
      sent = send(m_sock, static_cast<LPCSTR>(data), bytes, 0);

      if (sent == SOCKET_ERROR) {
        error = WSAGetLastError();

        if (error != WSAEWOULDBLOCK) {
          if (error != WSAECONNABORTED && error != WSAECONNRESET) {
            TCPNET::LogWrite("%s 1", OSNETERR_SENDFAILED);
          }
          Close();
          goto done;
        }
      } else if (sent >= bytes) {
        goto done;
      } else {
        data = static_cast<const BYTE *>(data) + sent;
        bytes -= sent;
      }
    }

    poutput = LockedEnqueue(data, bytes);
    if (!oldOutput && poutput) {
      StartWriteAndLeaveLock(poutput);
      return;
    }

  done:
    m_lock.Leave();
  }

  OS_SEND TCPCONN::SendSync(LPCVOID data, DWORD bytes, DWORD *bytesSent, DWORD timeout) {
    fd_set  write_fds;
    DWORD   startTime;
    timeval tv;
    OUTPUT *poutput;
    SEvent  event(0, 0);

    *bytesSent = 0;
    m_lock.Enter();
    if (m_sock == INVALID_SOCKET) {
      m_lock.Leave();
      return OS_SEND_SOCKET_CLOSED;
    }

    startTime = OsGetAsyncTimeMs();
    poutput = m_outputList.Tail();
    while (poutput) {
      poutput->m_completionEvent = &event;
      m_lock.Leave();
      if (event.Wait(timeout)) {
        OUTPUT *output;

        m_lock.Enter();
        output = m_outputList.Tail();
        ASSERT(output == 0 || output == poutput);
        if (output) {
          ASSERT(output->m_completionEvent == &event);
          output->m_completionEvent = 0;
        }
        m_lock.Leave();
        return OS_SEND_TIMEOUT;
      }

      timeout += startTime - OsGetAsyncTimeMs();
      m_lock.Enter();
      poutput = m_outputList.Tail();
      if (poutput && static_cast<long>(timeout) <= 0) {
        m_lock.Leave();
        return OS_SEND_TIMEOUT;
      }
    }

    for (;;) {
      int sent = send(m_sock, static_cast<LPCSTR>(data), bytes, 0);

      if (sent != SOCKET_ERROR) {
        *bytesSent += sent;
        if (sent >= bytes) {
          m_lock.Leave();
          return OS_SEND_OK;
        }

        data = static_cast<const BYTE *>(data) + sent;
        bytes -= sent;
        continue;
      }

      int error = WSAGetLastError();
      if (error != WSAEWOULDBLOCK) {
        if (error != WSAECONNABORTED && error != WSAECONNRESET) {
          TCPNET::LogWrite("%s 1", OSNETERR_SENDFAILED);
        }
        Close();
        m_lock.Leave();
        return OS_SEND_ERROR;
      }

      FD_ZERO(&write_fds);
      FD_SET(m_sock, &write_fds);
      timeout += startTime - OsGetAsyncTimeMs();
      tv.tv_sec = static_cast<long>(timeout) / 1000;
      tv.tv_usec = 1000 * (static_cast<long>(timeout) % 1000);

      m_lock.Leave();
      select(m_sock + 1, 0, &write_fds, 0, &tv);
      m_lock.Enter();

      timeout += startTime - OsGetAsyncTimeMs();
      if (!FD_ISSET(m_sock, &write_fds) && static_cast<long>(timeout) < 0) {
        m_lock.Leave();
        return OS_SEND_TIMEOUT;
      }

      ASSERT(!m_outputList.Tail());
    }
  }

  void TCPCONN::CloseAndUnlock() {
    if (m_sock != INVALID_SOCKET) {
      closesocket(m_sock);
      m_sock = INVALID_SOCKET;
    }
    m_lock.Leave();
  }

  void TCPCONN::SetNagle(int enable) {
    int nodelay = !enable;

    setsockopt(m_sock, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<LPCSTR>(&nodelay), sizeof(nodelay));
  }

  int TCPCONN::SetWindow(DWORD size) {
    int isize = size;

    if (setsockopt(m_sock, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<LPCSTR>(&isize), sizeof(isize))) {
      return 0;
    }

    isize = size;
    return setsockopt(m_sock, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<LPCSTR>(&isize), sizeof(isize)) == 0;
  }

  void TCPCONN::SetRecvTimeout(DWORD timeoutMs) {
    int itimeoutMs = timeoutMs;

    setsockopt(m_sock, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<LPCSTR>(&itimeoutMs), sizeof(itimeoutMs));
  }

  IOTCPCONN::IOTCPCONN(
      TCPNET            *net,
      LPVOID             port,
      UINT               sock,
      NETEVENTPROC       eventProc,
      LPVOID             user,
      const NETCONNADDR *pconnAddr,
      LPCVOID            data,
      DWORD              bytes
  )
      : TCPCONN(net, sock, eventProc, user, pconnAddr), m_ioCount(1) {
    if (!CreateIoCompletionPort(reinterpret_cast<HANDLE>(m_sock), static_cast<HANDLE>(port), reinterpret_cast<DWORD>(this), 0)) {
      TCPNET::LogWrite("%s 2", OSNETERR_PORTFAILED);
      Disconnect(0);
      return;
    }

    if (data && bytes) {
      Send(data, bytes);
    }

    if (NoteConnect()) {
      m_net->LinkConn(this, CONNLIST_TCP_CONNECTED);
      StartRead();
    }

    DecIo();
  }

  void IOTCPCONN::IncIo() {
    m_ioCount.Inc();
  }

  void IOTCPCONN::DecIo() {
    m_lock.Enter();
    if (!m_ioCount.Dec()) {
      CloseAndUnlock();
      Disconnect(1);
      return;
    }
    m_lock.Leave();
  }

  void IOTCPCONN::StartWriteAndLeaveLock(OUTPUT *poutput) {
    DWORD bytes = 0;

    poutput->m_state = OUTPUTSTATE_WRITING;
    IncIo();
    m_lock.Leave();

    if (TCPNET::s_WSASend) {
      WSABUF wsabuf;

      wsabuf.len = poutput->m_bytes;
      wsabuf.buf = reinterpret_cast<char *>(poutput->m_data);
      if (TCPNET::s_WSASend(m_sock, &wsabuf, 1, &bytes, 0, &poutput->m_overlap.m_overlapped, 0) != SOCKET_ERROR ||
          WSAGetLastError() == ERROR_IO_PENDING)
      {
        return;
      }
    } else if (
        WriteFile(reinterpret_cast<HANDLE>(m_sock), poutput->m_data, poutput->m_bytes, 0, &poutput->m_overlap.m_overlapped) ||
        GetLastError() == ERROR_IO_PENDING
    )
    {
      return;
    }

    Close();
    DecIo();
  }

  void IOTCPCONN::StartRead() {
    DWORD bytes = 0;

    IncIo();
    m_readOverlap.Init(OVERLAPTYPE_READ);

    if (TCPNET::s_WSARecv) {
      WSABUF wsabuf;
      DWORD  flags = 0;

      wsabuf.buf = reinterpret_cast<char *>(&m_data[m_bytes]);
      wsabuf.len = sizeof(m_data) - m_bytes;
      if (TCPNET::s_WSARecv(m_sock, &wsabuf, 1, &bytes, &flags, &m_readOverlap.m_overlapped, 0) != SOCKET_ERROR ||
          WSAGetLastError() == ERROR_IO_PENDING)
      {
        return;
      }
    } else if (
        ReadFile(reinterpret_cast<HANDLE>(m_sock), &m_data[m_bytes], sizeof(m_data) - m_bytes, 0, &m_readOverlap.m_overlapped) ||
        GetLastError() == ERROR_IO_PENDING
    )
    {
      return;
    }

    Close();
    DecIo();
  }

  void IOTCPCONN::AddToSelectSets(NETSELECTSETS *selectSets) {
    TCPNET::LogWrite("%s 11", OSNETERR_INTERNAL);
  }

  void IOTCPCONN::Selected(TCPNET *pnet, SELECTSET selectSet) {
    TCPNET::LogWrite("%s 12", OSNETERR_INTERNAL);
  }

  SLTCPCONN::SLTCPCONN(TCPNET *net, UINT sock, NETEVENTPROC eventProc, LPVOID user, const NETCONNADDR *pconnAddr, LPCVOID data, DWORD bytes)
      : TCPCONN(net, sock, eventProc, user, pconnAddr) {
    if (data && bytes) {
      Send(data, bytes);
    }

    if (NoteConnect()) {
      m_net->LinkConn(this, CONNLIST_TCP_CONNECTED);
    } else {
      Disconnect(1);
    }
  }

  void SLTCPCONN::StartWriteAndLeaveLock(OUTPUT *poutput) {
    m_lock.Leave();
  }

  void SLTCPCONN::StartRead() {
  }

  void SLTCPCONN::ContinueWrite() {
    m_lock.Enter();
    if (m_sock == INVALID_SOCKET) {
      m_lock.Leave();
      return;
    }

    while (OUTPUT *poutput = m_outputList.Head()) {
      poutput->m_state = OUTPUTSTATE_WRITING;
      int sent = send(m_sock, reinterpret_cast<LPCSTR>(poutput->m_data), poutput->m_bytes, 0);

      if (sent != SOCKET_ERROR && sent >= poutput->m_bytes) {
        CompleteWrite(&poutput->m_overlap, poutput->m_bytes);
        continue;
      }

      if (sent != SOCKET_ERROR) {
        memmove(poutput->m_data, &poutput->m_data[sent], poutput->m_bytes - sent);
        poutput->m_bytes -= sent;
        m_lock.Leave();
        return;
      }

      int error = WSAGetLastError();
      if (error != WSAEWOULDBLOCK) {
        if (error != WSAECONNABORTED && error != WSAECONNRESET) {
          TCPNET::LogWrite("%s 2", OSNETERR_SENDFAILED);
        }
        Close();
      }
      break;
    }

    m_lock.Leave();
  }

  void SLTCPCONN::ContinueRead() {
    DWORD bytes = sizeof(m_data) - m_bytes;

    if (bytes >= sizeof(m_data)) {
      bytes = sizeof(m_data);
    }

    int read = recv(m_sock, reinterpret_cast<char *>(&m_data[m_bytes]), bytes, 0);
    if (read != SOCKET_ERROR) {
      if (!read) {
        Close();
        return;
      }

      CompleteRead(0, read);
      return;
    }

    if (WSAGetLastError() == WSAEWOULDBLOCK) {
      return;
    }

    Close();
  }

  void SLTCPCONN::AddToSelectSets(NETSELECTSETS *selectSets) {
    selectSets->AddSelSock(this);
    selectSets->AddToSet(this, SELECTSET_R);
    if (m_outputList.Head()) {
      selectSets->AddToSet(this, SELECTSET_W);
    }
  }

  void SLTCPCONN::Selected(TCPNET *pnet, SELECTSET selectSet) {
    if (selectSet == SELECTSET_R) {
      ContinueRead();
    } else if (selectSet == SELECTSET_W) {
      ContinueWrite();
    } else {
      TCPNET::LogWrite("%s 13", OSNETERR_INTERNAL);
    }
  }

  FILECONN::FILECONN(TCPNET *net, LPVOID file, NETEVENTPROC eventProc, LPVOID user, const NETCONNADDR *pconnAddr)
      : NETCONN(net, INVALID_SOCKET, eventProc, user, pconnAddr), m_ioCount(1), m_file(file) {
  }

  FILECONN::~FILECONN() {
    m_outputList.Clear();
    m_inputList.Clear();
    m_net->LinkConn(this, CONNLIST_NONE);
  }

  void FILECONN::Selected(TCPNET *, SELECTSET) {
    TCPNET::LogWrite("%s 14", OSNETERR_INTERNAL);
  }

  void FILECONN::AddToSelectSets(NETSELECTSETS *) {
    TCPNET::LogWrite("%s 15", OSNETERR_INTERNAL);
  }

  void FILECONN::IncIo() {
    m_ioCount.Inc();
  }

  void FILECONN::DecIo() {
    m_lock.Enter();
    if (m_ioCount.Dec()) {
      m_lock.Leave();
    } else {
      CloseAndUnlock();
      Disconnect(1);
    }
  }

  void FILECONN::CompleteWrite(NETOVERLAP *poverlap, DWORD bytes) {
    OUTPUT *poutput;

    m_lock.Enter();
    poutput = CONTAINING_RECORD(poverlap, OUTPUT, m_overlap);
    poutput->m_state = OUTPUTSTATE_COMPLETED;
    poutput->m_bytes = bytes;

    while ((poutput = m_outputList.Head()) != 0) {
      if (poutput->m_state != OUTPUTSTATE_COMPLETED) {
        if (poutput->m_state == OUTPUTSTATE_WAITING) {
          StartWriteAndLeaveLock(poutput);
          return;
        }
        break;
      }

      m_outputList.UnlinkNode(poutput);
      int connected = m_file != INVALID_HANDLE_VALUE;
      m_lock.Leave();
      if (connected) {
        connected = NoteFileOperation(
            poutput->m_data, poutput->m_dataBytes, poutput->m_overlap.m_overlapped.Offset, poutput->m_overlap.m_overlapped.OffsetHigh,
            poutput->m_file.m_operationId, NETNOTE_FILEWRITE
        );
      }
      DEL(poutput);
      if (!connected) {
        Close();
        return;
      }
      m_lock.Enter();
    }

    m_lock.Leave();
  }

  void FILECONN::CompleteRead(NETOVERLAP *poverlap, DWORD bytes) {
    INPUT *pinput = CONTAINING_RECORD(poverlap, INPUT, m_overlap);

    m_lock.Enter();
    m_inputList.UnlinkNode(pinput);
    m_lock.Leave();

    int connected = NoteFileOperation(
        pinput->m_buffer, bytes, pinput->m_overlap.m_overlapped.Offset, pinput->m_overlap.m_overlapped.OffsetHigh, pinput->m_file.m_operationId,
        NETNOTE_FILEREAD
    );
    DEL(pinput);
    if (!connected) {
      Close();
    }
  }

  OUTPUT *FILECONN::LockedEnqueue(DWORDLONG pos, LPCVOID data, DWORD bytes, LPVOID operationId) {
    OUTPUT *poutput = NEW(OUTPUT);

    poutput->m_overlap.Init(OVERLAPTYPE_WRITE);
    poutput->m_overlap.m_overlapped.Offset = static_cast<DWORD>(pos);
    poutput->m_overlap.m_overlapped.OffsetHigh = static_cast<DWORD>(pos >> 32);
    poutput->m_file.m_operationId = operationId;
    poutput->m_state = OUTPUTSTATE_WAITING;
    poutput->m_bytes = bytes;
    poutput->m_dataBytes = 0;
    poutput->m_data = static_cast<BYTE *>(const_cast<LPVOID>(data));
    poutput->m_completionEvent = 0;
    m_outputList.LinkNode(poutput, LIST_TAIL, 0);
    return poutput;
  }

  int FILECONN::Write(DWORDLONG pos, LPCVOID data, DWORD bytes, LPVOID operationId) {
    m_lock.Enter();
    if (m_file == INVALID_HANDLE_VALUE) {
      m_lock.Leave();
      return 0;
    }

    OUTPUT *poutput = LockedEnqueue(pos, data, bytes, operationId);
    StartWriteAndLeaveLock(poutput);
    return 1;
  }

  int FILECONN::Read(DWORDLONG pos, LPVOID buffer, DWORD bytes, LPVOID operationId) {
    INPUT *pinput = NEW(INPUT);

    pinput->m_overlap.Init(OVERLAPTYPE_READ);
    pinput->m_overlap.m_overlapped.Offset = static_cast<DWORD>(pos);
    pinput->m_overlap.m_overlapped.OffsetHigh = static_cast<DWORD>(pos >> 32);
    pinput->m_file.m_operationId = operationId;
    pinput->m_bytes = bytes;
    pinput->m_buffer = static_cast<BYTE *>(buffer);

    m_lock.Enter();
    if (m_file == INVALID_HANDLE_VALUE) {
      m_lock.Leave();
      DEL(pinput);
      return 0;
    }

    m_inputList.LinkNode(pinput, LIST_TAIL, 0);
    IncIo();
    m_lock.Leave();
    StartRead(pinput);
    return 1;
  }

  IOFILECONN::IOFILECONN(TCPNET *net, LPVOID port, LPVOID file, NETEVENTPROC eventProc, LPVOID user, const NETCONNADDR *pconnAddr)
      : FILECONN(net, file, eventProc, user, pconnAddr) {
    if (!CreateIoCompletionPort(static_cast<HANDLE>(m_file), static_cast<HANDLE>(port), reinterpret_cast<DWORD>(this), 0)) {
      TCPNET::LogWrite("%s 3", OSNETERR_PORTFAILED);
      NoteCantConnect();
      CloseHandle(static_cast<HANDLE>(m_file));
      m_file = INVALID_HANDLE_VALUE;
      Disconnect(0);
      return;
    }

    if (NoteConnect()) {
      m_net->LinkConn(this, CONNLIST_FILE_CONNECTED);
    } else {
      DecIo();
    }
  }

  void IOFILECONN::StartWriteAndLeaveLock(OUTPUT *poutput) {
    poutput->m_state = OUTPUTSTATE_WRITING;
    IncIo();
    m_lock.Leave();
    if (!WriteFile(static_cast<HANDLE>(m_file), poutput->m_data, poutput->m_bytes, 0, &poutput->m_overlap.m_overlapped) &&
        GetLastError() != ERROR_IO_PENDING)
    {
      PostQueuedCompletionStatus(m_net->m_port, 0, reinterpret_cast<DWORD>(this), &poutput->m_overlap.m_overlapped);
    }
  }

  void IOFILECONN::StartRead(INPUT *pinput) {
    if (!ReadFile(static_cast<HANDLE>(m_file), pinput->m_buffer, pinput->m_bytes, 0, &pinput->m_overlap.m_overlapped) &&
        GetLastError() != ERROR_IO_PENDING)
    {
      PostQueuedCompletionStatus(m_net->m_port, 0, reinterpret_cast<DWORD>(this), &pinput->m_overlap.m_overlapped);
    }
  }

  void IOFILECONN::CloseAndUnlock() {
    if (m_file == INVALID_HANDLE_VALUE) {
      m_lock.Leave();
      return;
    }

    CloseHandle(static_cast<HANDLE>(m_file));
    m_file = INVALID_HANDLE_VALUE;
    m_lock.Leave();
    PostQueuedCompletionStatus(m_net->m_port, 0, reinterpret_cast<DWORD>(this), 0);
  }

  UINT __stdcall SLFILECONN::Thread(LPVOID lpfileConn) {
    SLFILECONN *fileConn = static_cast<SLFILECONN *>(lpfileConn);

    for (;;) {
      WaitForSingleObject(fileConn->m_event, INFINITE);

      for (;;) {
        fileConn->m_lock.Enter();
        OUTPUT *poutput = fileConn->m_outputList.Head();
        if (!poutput) {
          break;
        }
        LPVOID file = fileConn->m_file;
        fileConn->m_lock.Leave();

        DWORD bytes = 0;
        if (file != INVALID_HANDLE_VALUE) {
          long offsetHigh = poutput->m_overlap.m_overlapped.OffsetHigh;
          SetFilePointer(static_cast<HANDLE>(file), poutput->m_overlap.m_overlapped.Offset, &offsetHigh, FILE_BEGIN);
          WriteFile(static_cast<HANDLE>(file), poutput->m_data, poutput->m_bytes, &bytes, 0);
        }
        fileConn->CompleteWrite(&poutput->m_overlap, bytes);
        fileConn->DecIo();
      }

      for (;;) {
        INPUT *pinput = fileConn->m_inputList.Head();
        if (!pinput) {
          break;
        }
        LPVOID file = fileConn->m_file;
        fileConn->m_lock.Leave();

        DWORD bytes = 0;
        if (file != INVALID_HANDLE_VALUE) {
          long offsetHigh = pinput->m_overlap.m_overlapped.OffsetHigh;
          SetFilePointer(static_cast<HANDLE>(file), pinput->m_overlap.m_overlapped.Offset, &offsetHigh, FILE_BEGIN);
          ReadFile(static_cast<HANDLE>(file), pinput->m_buffer, pinput->m_bytes, &bytes, 0);
        }
        fileConn->CompleteRead(&pinput->m_overlap, bytes);
        fileConn->DecIo();
        fileConn->m_lock.Enter();
      }

      if (fileConn->m_file == INVALID_HANDLE_VALUE) {
        break;
      }
      fileConn->m_lock.Leave();
    }

    fileConn->m_lock.Leave();
    fileConn->DecIo();
    return 0;
  }

  SLFILECONN::SLFILECONN(TCPNET *net, LPVOID file, NETEVENTPROC eventProc, LPVOID user, const NETCONNADDR *pconnAddr)
      : FILECONN(net, file, eventProc, user, pconnAddr), m_event(CreateEventA(0, FALSE, FALSE, 0)) {
    UINT id;

    if (m_event) {
      m_thread = SCreateThread(Thread, this, &id, 0, 0);
    }

    if (m_event && m_thread) {
      if (NoteConnect()) {
        m_net->LinkConn(this, CONNLIST_FILE_CONNECTED);
      } else {
        DecIo();
      }
    } else {
      TCPNET::LogWrite("%s 6", OSNETERR_THREADFAILED);
      NoteCantConnect();
      Disconnect(0);
    }
  }

  SLFILECONN::~SLFILECONN() {
    if (m_thread) {
      CloseHandle(m_thread);
      m_thread = 0;
    }
    if (m_event) {
      CloseHandle(m_event);
      m_event = 0;
    }
  }

  void SLFILECONN::StartWriteAndLeaveLock(OUTPUT *poutput) {
    poutput->m_state = OUTPUTSTATE_WRITING;
    IncIo();
    m_lock.Leave();
    SetEvent(m_event);
  }

  void SLFILECONN::StartRead(INPUT *pinput) {
    SetEvent(m_event);
  }

  void SLFILECONN::CloseAndUnlock() {
    if (m_file != INVALID_HANDLE_VALUE) {
      CloseHandle(static_cast<HANDLE>(m_file));
      m_file = INVALID_HANDLE_VALUE;
      SetEvent(m_event);
    }
    m_lock.Leave();
  }

}  // namespace OsNet

int OsNetInitialize(DWORD hints, DWORD parts) {
  return OsNet::TCPNET::Initialize(hints, parts);
}

void OsNetDestroy(DWORD parts) {
  OsNet::TCPNET::Destroy(parts);
}

void OsNetPump(DWORD timeout) {
  OsNet::TCPNET *net = OsNet::TCPNET::Net();
  if (net) {
    net->Pump(timeout);
  } else {
    Sleep(timeout);
  }
}

HNETCONN__ *OsNetConnCopyHandle(HNETCONN__ *conn) {
  FATALASSERT(conn);
  reinterpret_cast<OsNet::NETCONN *>(conn)->IncRef();
  return conn;
}

void OsNetConnFreeHandle(HNETCONN__ *conn) {
  FATALASSERT(conn);
  reinterpret_cast<OsNet::NETCONN *>(conn)->DecRef();
}

void OsNetConnAddr(HNETCONN__ *conn, NETCONNADDR *connAddr) {
  FATALASSERT(conn);
  reinterpret_cast<OsNet::NETCONN *>(conn)->ConnAddr(connAddr);
}

void OsNetConnClose(HNETCONN__ *conn) {
  FATALASSERT(conn);
  reinterpret_cast<OsNet::NETCONN *>(conn)->Close();
}

int OsNetConnIsClosed(HNETCONN__ *conn) {
  if (!conn) {
    return 1;
  }
  return reinterpret_cast<OsNet::NETCONN *>(conn)->IsClosed();
}

void OsNetConnSetEventProc(HNETCONN__ *conn, NETEVENTPROC eventProc) {
  FATALASSERT(conn);
  reinterpret_cast<OsNet::NETCONN *>(conn)->SetEventProc(eventProc);
}

void OsNetConnSetUser(HNETCONN__ *conn, LPVOID user) {
  FATALASSERT(conn);
  reinterpret_cast<OsNet::NETCONN *>(conn)->SetUser(user);
}

void OsNetConnSetEventProcAndUser(HNETCONN__ *conn, NETEVENTPROC eventProc, LPVOID user) {
  FATALASSERT(conn);
  reinterpret_cast<OsNet::NETCONN *>(conn)->SetEventProcAndUser(eventProc, user);
}

void OsLoopConnect(NETEVENTPROC eventProcSrc, NETEVENTPROC eventProcDst, LPVOID user, LPCVOID data, DWORD bytes) {
  if (OsNet::TCPNET::Net()) {
    OsNet::TCPNET::Net()->LoopConnect(eventProcSrc, eventProcDst, user, data, bytes);
  }
}

void OsLoopConnSend(HNETCONN__ *conn, LPCVOID data, DWORD bytes) {
  FATALASSERT(conn);
  reinterpret_cast<OsNet::LOOPCONN *>(conn)->Send(data, bytes);
}

int OsTcpListen(WORD port, NETEVENTPROC eventProc, LPVOID user) {
  if (OsNet::TCPNET::Net()) {
    return OsNet::TCPNET::Net()->TcpListen(port, eventProc, user);
  }
  return 0;
}

void OsTcpListenEnable(WORD port, int enable) {
  if (OsNet::TCPNET::Net()) {
    OsNet::TCPNET::Net()->TcpListenEnable(port, enable);
  }
}

void OsTcpConnect(DWORD nodeNumber, WORD port, NETEVENTPROC eventProc, LPVOID user, LPCVOID data, DWORD bytes) {
  if (OsNet::TCPNET::Net()) {
    OsNet::TCPNET::Net()->TcpConnect(nodeNumber, port, eventProc, user, data, bytes);
  }
}

void OsTcpConnSend(HNETCONN__ *conn, LPCVOID data, DWORD bytes) {
  FATALASSERT(conn);
  reinterpret_cast<OsNet::NETCONNFULL *>(conn)->Send(data, bytes);
}

OS_SEND OsTcpConnSendSync(HNETCONN__ *conn, LPCVOID data, DWORD bytes, DWORD *bytesSent, DWORD timeout) {
  FATALASSERT(conn);
  return reinterpret_cast<OsNet::NETCONNFULL *>(conn)->SendSync(data, bytes, bytesSent, timeout);
}

void OsTcpConnSetNagle(HNETCONN__ *conn, int enable) {
  FATALASSERT(conn);
  reinterpret_cast<OsNet::NETCONNFULL *>(conn)->SetNagle(enable);
}

int OsTcpConnSetWindow(HNETCONN__ *conn, DWORD size) {
  FATALASSERT(conn);
  return reinterpret_cast<OsNet::NETCONNFULL *>(conn)->SetWindow(size);
}

void OsTcpConnSetRecvTimeout(HNETCONN__ *conn, DWORD timeoutMs) {
  FATALASSERT(conn);
  reinterpret_cast<OsNet::NETCONNFULL *>(conn)->SetRecvTimeout(timeoutMs);
}

DWORD OsTcpAddrLoop() {
  return 0;
}

void OsUdpConnect(const NETADDR *addr, WORD portMin, WORD portMax, NETEVENTPROC eventProc, LPVOID user) {
  if (OsNet::TCPNET::Net()) {
    OsNet::TCPNET::Net()->UdpConnect(addr, portMin, portMax, eventProc, user);
  }
}

void OsUdpConnSendTo(HNETCONN__ *conn, LPCVOID data, DWORD bytes, DWORD addrCount, const NETADDR *addrArray) {
  FATALASSERT(conn);
  reinterpret_cast<OsNet::NETCONNLESS *>(conn)->SendTo(data, bytes, addrCount, addrArray);
}

void OsNetAddrMake(DWORD nodeNumber, WORD port, NETADDR *netAddr) {
  sockaddr_in *addr = reinterpret_cast<sockaddr_in *>(netAddr);

  addr->sin_family = AF_INET;
  addr->sin_port = htons(port);
  addr->sin_addr.s_addr = nodeNumber;
  memset(addr->sin_zero, 0, sizeof(addr->sin_zero));
}

void OsNetAddrMakeBroadcast(WORD port, NETADDR *netAddr) {
  sockaddr_in *addr = reinterpret_cast<sockaddr_in *>(netAddr);

  addr->sin_family = AF_INET;
  addr->sin_port = htons(port);
  addr->sin_addr.s_addr = INADDR_BROADCAST;
  memset(addr->sin_zero, 0, sizeof(addr->sin_zero));
}

void OsNetAddrMakeFromStr(LPCSTR addrStr, WORD port, NETADDR *netAddr) {
  char   tempStr[32];
  LPCSTR colon = SStrChr(addrStr, ':');

  if (colon) {
    SStrCopy(tempStr, addrStr, colon - addrStr + 1 > sizeof(tempStr) ? sizeof(tempStr) : colon - addrStr + 1);
    addrStr = tempStr;
    port = static_cast<WORD>(SStrToUnsigned(colon + 1));
  }

  reinterpret_cast<sockaddr_in *>(netAddr)->sin_family = AF_INET;
  reinterpret_cast<sockaddr_in *>(netAddr)->sin_port = htons(port);
  reinterpret_cast<sockaddr_in *>(netAddr)->sin_addr.s_addr = inet_addr(addrStr);
  memset(reinterpret_cast<sockaddr_in *>(netAddr)->sin_zero, 0, sizeof(reinterpret_cast<sockaddr_in *>(netAddr)->sin_zero));
}

int OsNetAddrLoopback(const NETADDR *netAddr1, const NETADDR *netAddr2) {
  const sockaddr_in *addr1 = reinterpret_cast<const sockaddr_in *>(netAddr1);
  const sockaddr_in *addr2 = reinterpret_cast<const sockaddr_in *>(netAddr2);

  return addr1->sin_addr.s_addr == addr2->sin_addr.s_addr || addr1->sin_addr.s_addr == INADDR_LOOPBACK || addr2->sin_addr.s_addr == INADDR_LOOPBACK;
}

int OsNetAddrCompare(const NETADDR *netAddr1, const NETADDR *netAddr2, NETADDRDIFF *difflevel) {
  const WORD *addr1 = reinterpret_cast<const WORD *>(netAddr1);
  const WORD *addr2 = reinterpret_cast<const WORD *>(netAddr2);
  NETADDRDIFF diff;

  if (addr1[0] != addr2[0]) {
    diff = NETADDR_DIFF_PROTOCOL;
  } else if (addr1[1] != addr2[1]) {
    diff = NETADDR_DIFF_PORT;
  } else if (addr1[2] != addr2[2]) {
    diff = NETADDR_DIFF_NETWORK;
  } else if (addr1[3] != addr2[3]) {
    diff = NETADDR_DIFF_NODE;
  } else {
    diff = NETADDR_DIFF_EQUAL;
  }

  if (difflevel) {
    *difflevel = diff;
  }
  return diff == NETADDR_DIFF_EQUAL;
}

DWORD OsNetAddrGetAddress(const NETADDR *netAddr, WORD *port) {
  const sockaddr_in *addr = reinterpret_cast<const sockaddr_in *>(netAddr);

  if (port) {
    *port = ntohs(addr->sin_port);
  }
  return addr->sin_addr.s_addr;
}

LPCSTR OsNetAddrToStr(const NETADDR *netAddr, char *string, DWORD length) {
  SStrPrintf(
      string, length, "%s:%u", inet_ntoa(reinterpret_cast<const sockaddr_in *>(netAddr)->sin_addr),
      ntohs(reinterpret_cast<const sockaddr_in *>(netAddr)->sin_port)
  );
  return string;
}

DWORD OsNetAddrToHostOrder(DWORD nodeNumber) {
  return ntohl(nodeNumber);
}

DWORD OsNetGetHostAddr(LPCSTR hostName) {
  char     name[256];
  hostent *host;
  BYTE    *addr;

  name[0] = 0;
  if (!hostName) {
    gethostname(name, sizeof(name));
    hostName = name;
  }

  host = gethostbyname(hostName);
  if (!host) {
    return 0;
  }

  addr = reinterpret_cast<BYTE *>(host->h_addr_list[0]);
  return addr[0] | (static_cast<DWORD>(addr[1]) << 8) | (static_cast<DWORD>(addr[2]) << 16) | (static_cast<DWORD>(addr[3]) << 24);
}

int OsNetGetHostAddrs(LPCSTR hostNameList, WORD defaultPort, NETHOSTADDRPROC hostAddrProc, LPVOID user) {
  if (OsNet::TCPNET::Net()) {
    return OsNet::TCPNET::Net()->GetHostAddrs(hostNameList, defaultPort, hostAddrProc, user);
  }
  return 0;
}

void OsFileConnCreate(LPCSTR fileName, NETEVENTPROC eventProc, LPVOID user, int readOnly) {
  if (OsNet::TCPNET::Net()) {
    OsNet::TCPNET::Net()->FileConnCreate(fileName, eventProc, user, readOnly);
  }
}

int OsFileConnRead(HNETCONN__ *conn, DWORDLONG pos, LPVOID buffer, DWORD bytes, LPVOID operationId) {
  FATALASSERT(conn);
  return reinterpret_cast<OsNet::FILECONN *>(conn)->Read(pos, buffer, bytes, operationId);
}

int OsFileConnWrite(HNETCONN__ *conn, DWORDLONG pos, LPCVOID data, DWORD bytes, LPVOID operationId) {
  FATALASSERT(conn);
  return reinterpret_cast<OsNet::FILECONN *>(conn)->Write(pos, data, bytes, operationId);
}

void OsFileConnClose(HNETCONN__ *conn) {
  FATALASSERT(conn);
  reinterpret_cast<OsNet::FILECONN *>(conn)->Close();
}
