#ifndef ENGINE_SOURCE_OS_W32_OSTCP_H
#define ENGINE_SOURCE_OS_W32_OSTCP_H

#ifndef INCL_WINSOCK_API_TYPEDEFS
#define INCL_WINSOCK_API_TYPEDEFS 1
#endif
#include <winsock2.h>
#include <mswsock.h>

#include "Os/OsNet.h"

#include <stpl.h>

#include <W32/ISThread.h>

namespace OsNet {

  class NETSELECTSETS;
  struct NETSELSOCKPTR;
  struct TCPNET;
  class TCPCONN;
  class IOTCPCONN;
  class SLTCPCONN;
  struct TCPACCEPT;
  struct TCPLISTEN;
  struct NETCONNECT;
  struct LOOPCONNECT;
  struct TCPCONNECT;
  struct FILECONNECT;
  struct TCPHOSTADDRINFO;
  struct INPUT;
  class FILECONN;
  class IOFILECONN;
  class SLFILECONN;

  enum SELECTSET {
    SELECTSET_FIRST = 0,
    SELECTSET_R = 0,
    SELECTSET_W = 1,
    SELECTSET_E = 2,
    SELECT_MAX = 3
  };

  enum OVERLAPTYPE {
    OVERLAPTYPE_READ = 0,
    OVERLAPTYPE_WRITE = 1,
    OVERLAPTYPE_ACCEPT = 2
  };

  enum OUTPUTSTATE {
    OUTPUTSTATE_WAITING = 0,
    OUTPUTSTATE_WRITING = 1,
    OUTPUTSTATE_COMPLETED = 2,
    _UNIQUE_SYMBOL_OUTPUTSTATE_345 = -1 // todo: `_UNIQUE_SYMBOL_NAME_LINE = -1` macro?
  };

  enum CONNLIST {
    CONNLIST_LOOP_CONNECTED = 0,
    CONNLIST_TCP_CONNECTED = 1,
    CONNLIST_UDP_CONNECTED = 2,
    CONNLIST_FILE_CONNECTED = 3,
    CONNLISTS = 4,
    CONNLIST_NONE = 4,
    _UNIQUE_SYMBOL_CONNLIST_448 = -1 // todo: `_UNIQUE_SYMBOL_NAME_LINE = -1` macro?
  };

  class LOCKEDLONG {
   public:
    LOCKEDLONG(long value = 0) : m_value(value) {
    }

    operator long() const {
      return m_value;
    }

    long Inc() {
      return InterlockedIncrement(const_cast<long *>(&m_value));
    }

    long Dec() {
      return InterlockedDecrement(const_cast<long *>(&m_value));
    }

   private:
    volatile long m_value;
  };

  class CEventLock {
   public:
    CEventLock() : m_event(CreateEventA(0, FALSE, TRUE, 0)) {
    }

    ~CEventLock() {
      if (m_event) {
        CloseHandle(m_event);
      }
    }

    int Enter() {
      return WaitForSingleObject(m_event, INFINITE);
    }

    void Leave() {
      SetEvent(m_event);
    }

   private:
    LPVOID m_event;
  };

  struct NETOVERLAP {
    void Init(OVERLAPTYPE type) {
      memset(&m_overlapped, 0, sizeof(m_overlapped));
      m_type = type;
    }

    OVERLAPPED  m_overlapped;
    OVERLAPTYPE m_type;
  };

  NODEDECL(OUTPUT) {
    OUTPUT() {
    }
    OUTPUT(const OUTPUT &);

    NETOVERLAP  m_overlap;
    OUTPUTSTATE m_state;
    union {
      struct {
        LPVOID m_operationId;
      } m_file;
      struct {
        DWORD m_time;
      } m_sock;
    };
    DWORD   m_bytes;
    DWORD   m_dataBytes;
    BYTE   *m_data;
    SEvent *m_completionEvent;

    ~OUTPUT();
  };

  NODEDECL(INPUT) {
    INPUT() {
    }
    INPUT(const INPUT &);

    NETOVERLAP m_overlap;
    union {
      struct {
        LPVOID m_operationId;
      } m_file;
      struct {
      } m_sock;
    };
    DWORD m_bytes;
    BYTE *m_buffer;
  };

  struct NETSELSOCK {
    NETSELSOCK() : m_sock(INVALID_SOCKET) {
    }
    NETSELSOCK(const NETSELSOCK &);
    NETSELSOCK(UINT sock) : m_sock(sock) {
    }

   private:
    virtual void Selected(TCPNET *net, SELECTSET selectSet) = 0;

   public:
    virtual BOOL IsClosed() const;
    virtual void AddToSelectSets(NETSELECTSETS *selectSets) = 0;

    UINT m_sock;

    friend class NETSELECTSETS;
  };

  struct NETSELSOCKPTR : public TSHashObject<NETSELSOCKPTR, HASHKEY_NONE> {
    NETSELSOCK *ptr;
  };

  class NETSELECTSETS {
   public:
    NETSELECTSETS(TCPNET *net) : m_net(net) {
    }

    void Clear();
    void AddSelSock(NETSELSOCK *selsock);
    void AddToSet(NETSELSOCK *selsock, SELECTSET selectSet);
    BOOL Select(DWORD timeoutTotal, long selsockTotal);

   private:
    TCPNET                                          *m_net;
    fd_set                                           m_sets[SELECT_MAX];
    TSHashTableReuse<NETSELSOCKPTR, HASHKEY_NONE, 1> m_selsockTable;
  };

  class NETCONN : public NETSELSOCK {
   public:
    NETCONN(TCPNET *net, UINT sock, NETEVENTPROC eventProc, LPVOID user, const NETCONNADDR *pconnAddr);

   protected:
    virtual void CloseAndUnlock() = 0;

   public:
    virtual ~NETCONN() {
    }
    virtual void IncIo();
    virtual void DecIo();
    virtual void CompleteWrite(NETOVERLAP *poverlap, DWORD bytes);
    virtual void CompleteRead(NETOVERLAP *poverlap, DWORD bytes);
    virtual void Close();

    void IncRef();
    void DecRef();
    void GetEventProcAndUser(NETEVENTPROC &eventProc, LPVOID &user);
    void SetEventProcAndUser(NETEVENTPROC eventProc, LPVOID user);
    void SetEventProc(NETEVENTPROC eventProc);
    void SetUser(LPVOID user);
    int  NoteCantConnect();
    int  NoteConnect();
    int  NoteDisconnect();
    int  NoteData(LPVOID data, DWORD bytes, DWORD *bytesProcessed, const NETCONNADDR *connAddr);
    int  NoteFileOperation(LPVOID data, DWORD bytes, DWORD offset, DWORD offsetHigh, LPVOID operationId, NETNOTE note);
    void ConnAddr(NETCONNADDR *connAddr);

   private:
    LINKDECLEX(NETCONN, m_link);
    BYTE         m_list;
    BYTE         m_listSlot;
    WORD         m_reserved;
    LOCKEDLONG   m_refCount;
    NETCONNADDR  m_connAddr;
    long         m_eventProcUserLock;
    NETEVENTPROC m_eventProc;
    LPVOID       m_user;

   protected:
    CCritSect m_lock;
    DWORD     m_time;
    TCPNET   *m_net;

    CONNLIST ConnList() const {
      return static_cast<CONNLIST>(m_list);
    }
    void Disconnect(int notify);

    friend struct TCPNET;
  };

  class NETCONNFULL : public NETCONN {
   public:
    NETCONNFULL(TCPNET *net, UINT sock, NETEVENTPROC eventProc, LPVOID user, const NETCONNADDR *pconnAddr)
        : NETCONN(net, sock, eventProc, user, pconnAddr) {
    }

    virtual void    Send(LPCVOID data, DWORD bytes) = 0;
    virtual OS_SEND SendSync(LPCVOID data, DWORD bytes, DWORD *bytesSent, DWORD timeout) = 0;
    virtual void    SetNagle(int enable);
    virtual BOOL    SetWindow(DWORD size);
    virtual void    SetRecvTimeout(DWORD timeoutMs);
  };

  class NETCONNLESS : public NETCONN {
   public:
    NETCONNLESS(TCPNET *net, UINT sock, NETEVENTPROC eventProc, LPVOID user, const NETCONNADDR *pconnAddr)
        : NETCONN(net, sock, eventProc, user, pconnAddr) {
    }

    virtual void SendTo(LPCVOID data, DWORD bytes, DWORD addrCount, const NETADDR *addrArray) = 0;
  };

  class TCPCONN : public NETCONNFULL {
   public:
    TCPCONN(TCPNET *net, UINT sock, NETEVENTPROC eventProc, LPVOID user, const NETCONNADDR *pconnAddr);
    virtual ~TCPCONN();
    virtual void    Send(LPCVOID data, DWORD bytes);
    virtual OS_SEND SendSync(LPCVOID data, DWORD bytes, DWORD *bytesSent, DWORD timeout);
    virtual void    SetNagle(int enable);
    virtual BOOL    SetWindow(DWORD size);
    virtual void    SetRecvTimeout(DWORD timeoutMs);

   protected:
    virtual void CompleteWrite(NETOVERLAP *poverlap, DWORD bytes);
    virtual void CompleteRead(NETOVERLAP *poverlap, DWORD bytes);
    virtual void StartWriteAndLeaveLock(OUTPUT *poutput) = 0;
    virtual void StartRead() = 0;
    virtual void CloseAndUnlock();

    LISTDECL(OUTPUT, m_outputList);
    DWORD m_bytes;
    BYTE  m_data[1460];

   private:
    OUTPUT *LockedEnqueue(LPCVOID data, DWORD bytes);
  };

  class IOTCPCONN : public TCPCONN {
   public:
    IOTCPCONN(TCPNET *net, LPVOID port, UINT sock, NETEVENTPROC eventProc, LPVOID user, const NETCONNADDR *pconnAddr, LPCVOID data, DWORD bytes);

   private:
    virtual void IncIo();
    virtual void DecIo();
    virtual void StartWriteAndLeaveLock(OUTPUT *poutput);
    virtual void StartRead();
    virtual void AddToSelectSets(NETSELECTSETS *selectSets);
    virtual void Selected(TCPNET *pnet, SELECTSET selectSet);

    LOCKEDLONG m_ioCount;
    NETOVERLAP m_readOverlap;
  };

  class SLTCPCONN : public TCPCONN {
   public:
    SLTCPCONN(TCPNET *net, UINT sock, NETEVENTPROC eventProc, LPVOID user, const NETCONNADDR *pconnAddr, LPCVOID data, DWORD bytes);

   private:
    virtual void StartWriteAndLeaveLock(OUTPUT *poutput);
    virtual void StartRead();
    void         ContinueWrite();
    void         ContinueRead();
    virtual void AddToSelectSets(NETSELECTSETS *selectSets);
    virtual void Selected(TCPNET *pnet, SELECTSET selectSet);
  };

  class FILECONN : public NETCONN {
   public:
    FILECONN(TCPNET *net, LPVOID file, NETEVENTPROC eventProc, LPVOID user, const NETCONNADDR *pconnAddr);
    virtual ~FILECONN();
    virtual BOOL IsClosed() const;
    virtual void AddToSelectSets(NETSELECTSETS *);
    BOOL         Write(DWORDLONG pos, LPCVOID data, DWORD bytes, LPVOID operationId);
    BOOL         Read(DWORDLONG pos, LPVOID buffer, DWORD bytes, LPVOID operationId);

   protected:
    virtual void IncIo();
    virtual void DecIo();
    virtual void CompleteWrite(NETOVERLAP *poverlap, DWORD bytes);
    virtual void CompleteRead(NETOVERLAP *poverlap, DWORD bytes);
    virtual void StartWriteAndLeaveLock(OUTPUT *poutput) = 0;
    virtual void StartRead(INPUT *pinput) = 0;

   private:
    virtual void Selected(TCPNET *pnet, SELECTSET selectSet);
    LOCKEDLONG   m_ioCount;

   protected:
    LPVOID m_file;
    LISTDECL(OUTPUT, m_outputList);
    LISTDECL(INPUT, m_inputList);

   private:
    OUTPUT *LockedEnqueue(DWORDLONG pos, LPCVOID data, DWORD bytes, LPVOID operationId);
  };

  class IOFILECONN : public FILECONN {
   public:
    IOFILECONN(TCPNET *net, LPVOID port, LPVOID file, NETEVENTPROC eventProc, LPVOID user, const NETCONNADDR *pconnAddr);

   private:
    virtual void StartWriteAndLeaveLock(OUTPUT *poutput);
    virtual void StartRead(INPUT *pinput);

   protected:
    virtual void CloseAndUnlock();
  };

  class SLFILECONN : public FILECONN {
   public:
    SLFILECONN(TCPNET *net, LPVOID file, NETEVENTPROC eventProc, LPVOID user, const NETCONNADDR *pconnAddr);
    virtual ~SLFILECONN();

   private:
    static UINT __stdcall Thread(LPVOID lpfileConn);
    virtual void          StartWriteAndLeaveLock(OUTPUT *poutput);
    virtual void          StartRead(INPUT *pinput);

    LPVOID m_thread;
    LPVOID m_event;

   protected:
    virtual void CloseAndUnlock();
  };

  struct NETCONNECT : public NETSELSOCK {
    ~NETCONNECT();

    virtual void Fail() = 0;
    virtual void Complete(TCPNET *net) = 0;

    void NoteCantConnect(NETEVENTPROC eventProc, const NETCONNADDR *pconnAddr);

    LINKDECLEX(NETCONNECT, m_link);
    LPVOID m_user;
    LPVOID m_data;
    DWORD  m_bytes;
  };

  struct LOOPCONNECT : public NETCONNECT {
    virtual BOOL IsClosed() const;
    virtual void AddToSelectSets(NETSELECTSETS *);
    virtual void Fail();
    virtual void Complete(TCPNET *pnet);

    NETEVENTPROC m_eventProcSrc;
    NETEVENTPROC m_eventProcDst;

   private:
    virtual void Selected(TCPNET *pnet, SELECTSET selectSet);
  };

  struct TCPCONNECT : public NETCONNECT {
    virtual void AddToSelectSets(NETSELECTSETS *selectSets);
    virtual void Fail();
    virtual void Complete(TCPNET *pnet);

    DWORD        m_nodeNumber;
    DWORD        m_portAddr;
    NETEVENTPROC m_eventProc;

   private:
    virtual void Selected(TCPNET *pnet, SELECTSET selectSet);
  };

  struct FILECONNECT : public NETCONNECT {
    virtual BOOL IsClosed() const;
    virtual void AddToSelectSets(NETSELECTSETS *);
    virtual void Fail();
    virtual void Complete(TCPNET *pnet);

    LPVOID       m_file;
    NETEVENTPROC m_eventProc;

   private:
    virtual void Selected(TCPNET *net, SELECTSET selectSet);
  };

  class LOOPCONN : public NETCONNFULL {
   public:
    struct INPUT {
      LINKDECLEX(INPUT, m_link);
      LINKDECLEX(INPUT, m_linkNet);
      LOOPCONN *m_conn;
      DWORD     m_bytes;
      DWORD     m_dataBytes;
      BYTE      m_data[4];
    };

    typedef INPUT       *PINPUT;
    typedef const INPUT *PCINPUT;

    LOOPCONN(TCPNET *net, NETEVENTPROC eventProc, LPVOID user, const NETCONNADDR *pconnAddr);
    virtual ~LOOPCONN();
    virtual void    Send(LPCVOID data, DWORD bytes);
    virtual OS_SEND SendSync(LPCVOID data, DWORD bytes, DWORD *bytesSent, DWORD timeout);
    virtual void    Close();
    virtual BOOL    IsClosed() const;

   private:
    LOOPCONN *m_loopConn;
    LINKDECLEX(LOOPCONN, m_linkNet);
    LISTDECLEX(INPUT, m_link, m_inputList);
    DWORD m_bytes;
    BYTE  m_data[1460];

   protected:
    virtual void CloseAndUnlock();

   private:
    virtual void AddToSelectSets(NETSELECTSETS *selectSets);
    virtual void Selected(TCPNET *pnet, SELECTSET selectSet);
    void         Connect();
    void         CompleteInput(INPUT *pinput);
    void         EnqueueInput(LPCVOID data, DWORD bytes);

    friend struct TCPNET;
  };

  class UDPCONN : public NETCONNLESS {
   public:
    UDPCONN(TCPNET *net, UINT sock, NETEVENTPROC eventProc, LPVOID user, const NETCONNADDR *pconnAddr);
    virtual ~UDPCONN();
    virtual void SendTo(LPCVOID data, DWORD bytes, DWORD addrCount, const NETADDR *addrArray);

   protected:
    virtual void CloseAndUnlock();

   private:
    virtual void AddToSelectSets(NETSELECTSETS *selectSets);
    virtual void Selected(TCPNET *pnet, SELECTSET selectSet);
  };

  template <class T, int LINKOFFSET, int SLOTS>
  class TSSlottedListEx {
   private:
    TSSlottedListEx(const TSSlottedListEx &);

   public:
    enum {
      MAXSLOTS = 256
    };

    class Iterator {
     public:
      void Reset() {
        m_curr = 0;
        m_next = 0;
        m_mark = 0;
        m_slot = SLOTS - 1;
      }

      T *CycleInit() {
        m_slottedList.m_locks[m_slot].Enter();
        if (!m_curr) {
          Advance();
        }
        m_mark = m_curr;
        return m_curr;
      }

      void CycleDone() {
        m_slottedList.m_locks[m_slot].Leave();
      }

      T *CycleNext() {
        Advance();
        return m_curr != m_mark ? m_curr : 0;
      }

      T *SkipDeletedAndCycleNext() {
        if (m_curr == m_mark) {
          m_mark = 0;
        }

        Advance();
        if (!m_mark) {
          m_mark = m_curr;
        }
        return m_curr;
      }

      void Unlink() {
        ASSERT(m_curr);
        m_slottedList.m_lists[m_slot].UnlinkNode(m_curr);
        m_slottedList.m_count.Dec();
      }

     private:
      Iterator(TSSlottedListEx<T, LINKOFFSET, SLOTS> &slottedList) : m_slottedList(slottedList), m_curr(0), m_next(0), m_mark(0), m_slot(SLOTS - 1) {
      }
      Iterator(const Iterator &);
      Iterator &operator=(const Iterator &);

      void Advance();

      TSSlottedListEx<T, LINKOFFSET, SLOTS> &m_slottedList;
      T                                     *m_curr;
      T                                     *m_next;
      T                                     *m_mark;
      long                                   m_slot;

      friend struct TCPNET;
    };

    TSSlottedListEx();
    virtual ~TSSlottedListEx();

    static long Slots() {
      return SLOTS;
    }

    void Clear() {
      int slot;

      for (slot = 0; slot < SLOTS; ++slot) {
        m_locks[slot].Enter();
        while (m_lists[slot].Head()) {
          m_lists[slot].DeleteNode(m_lists[slot].Head());
          m_count.Dec();
        }
        m_locks[slot].Leave();
      }
    }

    long Count() {
      return m_count;
    }

    BYTE Link(T *ptr) {
      BYTE slot;

      m_linkSlot.Inc();
      slot = static_cast<BYTE>(static_cast<long>(m_linkSlot) & (SLOTS - 1));
      m_locks[slot].Enter();
      m_lists[slot].LinkNode(ptr, LIST_TAIL, 0);
      m_locks[slot].Leave();
      m_count.Inc();
      return slot;
    }

    void Unlink(T *ptr, BYTE slot) {
      ASSERT((LONG)slot >= 0 && (LONG)slot < SLOTS);

      m_locks[slot].Enter();
      m_lists[slot].UnlinkNode(ptr);
      m_locks[slot].Leave();
      m_count.Dec();
    }

    TSExplicitList<T, LINKOFFSET> &UnlinkAll(TSExplicitList<T, LINKOFFSET> &list);

   private:
    TSSlottedListEx &operator=(const TSSlottedListEx &);

    TSExplicitList<T, LINKOFFSET> m_lists[SLOTS];
    CCritSect                     m_locks[SLOTS];
    LOCKEDLONG                    m_linkSlot;
    LOCKEDLONG                    m_count;

    friend class Iterator;
    friend struct TCPNET;
  };

#define SLOTTEDLISTEX(structname, linkname, slots) TSSlottedListEx<structname, (int)&(((structname *)0)->linkname), slots>

  NODEDECL(TCPACCEPT) {
    TCPACCEPT(TCPLISTEN * listen);
    ~TCPACCEPT();

    void Init();
    UINT Complete(NETCONNADDR * connAddr, int makeSock);

    NETOVERLAP m_overlap;
    CCritSect  m_lock;
    BYTE       m_addr[64];
    TCPLISTEN *m_listen;
    UINT       m_sock;
  };

  struct TCPLISTEN : public NETSELSOCK {
    TCPLISTEN(UINT sock, WORD port, NETEVENTPROC eventProc, LPVOID user, DWORD acceptCount);
    ~TCPLISTEN();

    BOOL         Enable(int enable);
    void         Close();
    virtual void AddToSelectSets(NETSELECTSETS *selectSets);

    LINKDECLEX(TCPLISTEN, m_link);
    DWORD        m_portAddr;
    NETEVENTPROC m_eventProc;
    LPVOID       m_user;
    int          m_enabled;
    LISTDECL(TCPACCEPT, m_acceptList);

   private:
    virtual void Selected(TCPNET *pnet, SELECTSET selectSet);
  };

  NODEDECL(TCPHOSTADDRINFO) {
    ~TCPHOSTADDRINFO();
    void Fail() {
      m_hostAddrProc(0, 0, m_user);
    }
    void Complete();

    char                    *m_hostNameList;
    char                    *m_hostNameCurr;
    DWORD                    m_infoId;
    LPVOID                   m_thread;
    NETHOSTADDRPROC          m_hostAddrProc;
    LPVOID                   m_user;
    int                      m_ready;
    TSGrowableArray<NETADDR> m_addrs;
  };

  struct TCPHOSTADDRTHREAD {
    TCPHOSTADDRTHREAD() {
    }

    ~TCPHOSTADDRTHREAD() {
    }

    TCPNET *m_net;
    DWORD   m_infoId;
    LPVOID  m_event;
    WORD    m_defaultPort;
  };

  struct TCPNET {
   public:
    ~TCPNET();

    static BOOL    Initialize(DWORD hints, DWORD parts);
    static void    Destroy(DWORD parts);
    static TCPNET *Net() {
      return s_pnet;
    }

    void             Pump(DWORD timeout);
    BOOL             TcpListen(WORD port, NETEVENTPROC eventProc, LPVOID user);
    void             TcpListenEnable(WORD port, int enable);
    void             TcpConnect(DWORD nodeNumber, WORD port, NETEVENTPROC eventProc, LPVOID user, LPCVOID data, DWORD bytes);
    void             UdpConnect(const NETADDR *addr, WORD portMin, WORD portMax, NETEVENTPROC eventProc, LPVOID user);
    void             FileConnCreate(LPCSTR fileName, NETEVENTPROC eventProc, LPVOID user, int readOnly);
    BOOL             GetHostAddrs(LPCSTR hostNameList, WORD defaultPort, NETHOSTADDRPROC hostAddrProc, LPVOID user);
    TCPHOSTADDRINFO *LockedFindHostAddrInfo(DWORD infoId);
    void             LoopConnect(NETEVENTPROC eventProcSrc, NETEVENTPROC eventProcDst, LPVOID user, LPCVOID data, DWORD bytes);
    void             LoopCompleteConnect(LOOPCONNECT *pconnect);
    void             TcpCompleteConnect(TCPCONNECT *pconnect);
    void             FileCompleteConnect(FILECONNECT *pconnect);
    void             LinkConn(NETCONN *pconn, CONNLIST tolist);
    LOOPCONN::INPUT *LoopAllocInput(DWORD bytes);
    void             LoopFreeInput(LOOPCONN::INPUT *pinput);

    static void __cdecl LogWrite(LPCSTR format, ...);
    static void __cdecl LogDump(LPCSTR header, LPCVOID data, DWORD bytes);

    static LPFN_ACCEPTEX             s_AcceptEx;
    static LPFN_GETACCEPTEXSOCKADDRS s_GetAcceptExSockaddrs;
    static LPFN_WSASEND              s_WSASend;
    static LPFN_WSARECV              s_WSARecv;
    static HSLOG                     s_log;
    static int                       s_qpcexists;
    static float                     s_qpctoms;

    void CompleteAcceptEx(TCPACCEPT *paccept, int makeConn);
    void CompleteAccept(TCPLISTEN *plisten, UINT sock, const NETCONNADDR *pconnAddr);

   private:
    TCPNET();
    TCPNET(const TCPNET &);
    TCPNET &operator=(const TCPNET &);

    static void   MakeConnAddr(UINT sock, DWORD port, NETCONNADDR *connAddr);
    static UINT   CreateListenSocket(WORD port);
    static LPVOID IoCompletionPresent(DWORD *pumpThreadCount);
    static void   IncludeDependantParts(DWORD *parts);

    BOOL BaseInitialize(DWORD hints);
    void BaseDestroy();
    BOOL WinsockInitialize(DWORD hints);
    void WinsockDestroy();
    BOOL IoInitialize(DWORD hints);
    void IoDestroy();
    BOOL TcpInitialize(DWORD hints);
    void TcpDestroy();
    void IncRef();
    void DecRef();
    void WakePumpThread();

   public:
    int PostIo(DWORD bytes, DWORD key, OVERLAPPED *overlap) {
      return PostQueuedCompletionStatus(m_port, bytes, key, overlap);
    }
    void BaseWakeThread() {
      SetEvent(m_baseEvent);
    }
    void LoopLock() {
      m_loopLock.Enter();
    }
    void LoopUnlock() {
      m_loopLock.Leave();
    }
    void LoopLinkInput(LOOPCONN::INPUT *input) {
      m_loopInputList.LinkNode(input, LIST_TAIL, 0);
    }
    void LoopLinkDisconnectConn(LOOPCONN *conn) {
      m_loopDisconnectList.LinkNode(conn, LIST_TAIL, 0);
    }

   private:
    BOOL PumpThreadsInitialize();
    void PumpThreadsDestroy();

    static UINT __stdcall IoPumpThread(LPVOID lpnet);
    static UINT __stdcall SlPumpThread(LPVOID lpnet);
    static UINT __stdcall BaseThread(LPVOID lpnet);
    static UINT __stdcall GetHostAddrsThread(LPVOID lpparam);
    static UINT __stdcall ListenThread(LPVOID lpnet);
    static UINT __stdcall UdpPumpThread(LPVOID lpnet);

    void IoPump(DWORD timeout);
    void TcpMakeConn(UINT sock, NETEVENTPROC eventProc, LPVOID user, const NETCONNADDR *pconnAddr, LPCVOID data, DWORD bytes);
    void UdpMakeConn(UINT sock, NETEVENTPROC eventProc, LPVOID user, const NETCONNADDR *pconnAddr);
    void FileMakeConn(LPVOID file, NETEVENTPROC eventProc, LPVOID user, const NETCONNADDR *pconnAddr);
    void LoopMakeConn(NETEVENTPROC eventProcSrc, NETEVENTPROC eventProcDst, LPVOID user, LPCVOID data, DWORD bytes);
    void LoopConnectInit(LOOPCONNECT *pconnect);
    void TcpConnectInit(TCPCONNECT *pconnect);
    void FileConnectInit(FILECONNECT *pconnect);

    enum CONNECTLIST {
      CONNECTLIST_LOOP_CONNECTED = 0,
      CONNECTLIST_TCP_CONNECTED = 1,
      CONNECTLIST_TCP_CONNECTING = 2,
      CONNECTLIST_FILE_CONNECTED = 3,
      CONNECTLISTS = 4
    };

    typedef LISTEX(LOOPCONN, m_linkNet)          LISTLOOPCONN;
    typedef LISTEX(LOOPCONN::INPUT, m_linkNet)   LISTLOOPCONNINPUT;
    typedef SLOTTEDLISTEX(NETCONNECT, m_link, 1) NETCONNECTLIST;
    typedef LISTEX(NETCONNECT, m_link)           NETCONNECTSIMPLELIST;
    typedef SLOTTEDLISTEX(NETCONN, m_link, 8)    NETCONNLIST;
    typedef LISTEX(NETCONN, m_link)              NETCONNSIMPLELIST;
    typedef SLOTTEDLISTEX(TCPLISTEN, m_link, 1)  TCPLISTENLIST;

    LOCKEDLONG              m_refCount;
    DWORD                   m_pumpThreadCount;
    TSGrowableArray<LPVOID> m_pumpThreads;
    LPVOID                  m_udpPumpThread;
    LPVOID                  m_udpPumpEvent;
    CCritSect               m_loopLock;
    LISTLOOPCONNINPUT       m_loopInputRecycleList;
    LISTLOOPCONNINPUT       m_loopInputList;
    LISTLOOPCONN            m_loopDisconnectList;
    NETCONNLIST    m_connList[CONNLISTS];
    LPVOID         m_listenThread;
    TCPLISTENLIST  m_listenList;
    LPVOID         m_baseThread;
    LPVOID         m_baseEvent;
    int            m_baseTcpShutdown;
    LPVOID         m_baseTcpShutdownEvent;
    NETCONNECTLIST m_connectList[CONNECTLISTS];
    LPVOID         m_port;
    LOCKEDLONG     m_hostAddrInfoCount;
    CEventLock     m_hostAddrInfoLock;
    DWORD          m_hostAddrInfoId;
    LISTDECL(TCPHOSTADDRINFO, m_hostAddrInfoList);

    static CInitCritSect s_initLock;
    static DWORD         s_initCount[5];
    static TCPNET *volatile s_pnet;
    static volatile int s_baseShutdown;
    static volatile int s_pumpShutdown;
    static volatile int s_tcpShutdown;
    static int          s_preTerminateHostAddr;
    static WSADATA      s_wsaData;
    static HMODULE      s_mswsockModule;
    static HMODULE      s_ws2Module;

    friend void ::OsNetPump(DWORD timeout);
    friend int ::OsTcpListen(WORD port, NETEVENTPROC eventProc, LPVOID user);
    friend void ::OsTcpListenEnable(WORD port, int enable);
    friend void ::OsTcpConnect(DWORD nodeNumber, WORD port, NETEVENTPROC eventProc, LPVOID user, LPCVOID data, DWORD bytes);
    friend void ::OsUdpConnect(const NETADDR *addr, WORD portMin, WORD portMax, NETEVENTPROC eventProc, LPVOID user);
    friend void ::OsFileConnCreate(LPCSTR fileName, NETEVENTPROC eventProc, LPVOID user, int readOnly);
    friend int ::OsNetGetHostAddrs(LPCSTR hostNameList, WORD defaultPort, NETHOSTADDRPROC hostAddrProc, LPVOID user);
    friend struct LOOPCONN;
    friend class IOFILECONN;
    friend class SLFILECONN;
  };

  extern LPCSTR OSNETERR_INTERNAL;
  extern LPCSTR OSNETERR_WINSOCKSTARTUP;
  extern LPCSTR OSNETERR_WINSOCKVERSION;
  extern LPCSTR OSNETERR_SENDFAILED;
  extern LPCSTR OSNETERR_THREADFAILED;
  extern LPCSTR OSNETERR_EVENTFAILED;
  extern LPCSTR OSNETERR_BINDFAILED;
  extern LPCSTR OSNETERR_LISTENFAILED;
  extern LPCSTR OSNETERR_SOCKETFAILED;
  extern LPCSTR OSNETERR_SELECTFAILED;
  extern LPCSTR OSNETERR_ACCEPTFAILED;
  extern LPCSTR OSNETERR_PORTFAILED;
  extern LPCSTR OSNETERR_OVERLAPTYPE;
  extern LPCSTR OSNETERR_LISTENCLOSED;
  extern LPCSTR OSNETERR_ACCEPTEXFAILED;

}  // namespace OsNet

#endif
