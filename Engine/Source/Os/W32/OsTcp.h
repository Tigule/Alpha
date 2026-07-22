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
    OUTPUTSTATE_COMPLETED = 2
  };

  enum CONNLIST {
    CONNLIST_LOOP_CONNECTED = 0,
    CONNLIST_TCP_CONNECTED = 1,
    CONNLIST_UDP_CONNECTED = 2,
    CONNLIST_FILE_CONNECTED = 3,
    CONNLISTS = 4,
    CONNLIST_NONE = 4
  };

  class LOCKEDLONG {
   public:
    LOCKEDLONG(long value = 0) : m_value(value) {
    }

    operator long() {
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

  struct CEventLock {
    CEventLock() : m_event(CreateEventA(0, FALSE, TRUE, 0)) {
    }

    ~CEventLock() {
      if (m_event) {
        CloseHandle(m_event);
      }
    }

    void Lock() {
      WaitForSingleObject(m_event, INFINITE);
    }

    void Unlock() {
      SetEvent(m_event);
    }

    void *m_event;
  };

  struct NETOVERLAP {
    void Init(OVERLAPTYPE type) {
      memset(&m_overlapped, 0, sizeof(m_overlapped));
      m_type = type;
    }

    OVERLAPPED  m_overlapped;
    OVERLAPTYPE m_type;
  };

  struct OUTPUT : public TSLinkedNode<OUTPUT> {
    NETOVERLAP  m_overlap;
    OUTPUTSTATE m_state;
    union {
      struct {
        void *m_operationId;
      } m_file;
      struct {
        unsigned long m_time;
      } m_sock;
    };
    unsigned long  m_bytes;
    unsigned long  m_dataBytes;
    unsigned char *m_data;
    SEvent        *m_completionEvent;

    ~OUTPUT();
  };

  struct INPUT : public TSLinkedNode<INPUT> {
    NETOVERLAP    m_overlap;
    void         *m_operationId;
    unsigned long m_bytes;
    void         *m_data;
  };

  struct NETSELSOCK {
    NETSELSOCK(unsigned int sock = INVALID_SOCKET) : m_sock(sock) {
    }

    virtual void Selected(TCPNET *net, SELECTSET selectSet) = 0;
    virtual int  IsClosed() const;
    virtual void AddToSelectSets(NETSELECTSETS *selectSets) = 0;

    unsigned int m_sock;
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
    int  Select(unsigned long timeoutTotal, long selsockTotal);

   private:
    TCPNET                                          *m_net;
    fd_set                                           m_sets[SELECT_MAX];
    TSHashTableReuse<NETSELSOCKPTR, HASHKEY_NONE, 1> m_selsockTable;
  };

  class NETCONN : public NETSELSOCK {
   public:
    NETCONN(TCPNET *net, unsigned int sock, NETEVENTPROC eventProc, void *user, const NETCONNADDR *pconnAddr);
    virtual void CloseAndUnlock() = 0;
    virtual ~NETCONN() {
    }
    virtual void IncIo();
    virtual void DecIo();
    virtual void CompleteWrite(NETOVERLAP *poverlap, unsigned long bytes);
    virtual void CompleteRead(NETOVERLAP *poverlap, unsigned long bytes);
    virtual void Close();

    void IncRef();
    void DecRef();
    void GetEventProcAndUser(NETEVENTPROC &eventProc, void *&user);
    void SetEventProcAndUser(NETEVENTPROC eventProc, void *user);
    void SetEventProc(NETEVENTPROC eventProc);
    void SetUser(void *user);
    int  NoteCantConnect();
    int  NoteConnect();
    int  NoteDisconnect();
    int  NoteData(void *data, unsigned long bytes, unsigned long *bytesProcessed, const NETCONNADDR *connAddr);
    int  NoteFileOperation(void *data, unsigned long bytes, unsigned long offset, unsigned long offsetHigh, void *operationId, NETNOTE note);
    void ConnAddr(NETCONNADDR *connAddr);

    TSLink<NETCONN> m_link;
    unsigned char   m_list;
    unsigned char   m_listSlot;
    unsigned short  m_reserved;
    LOCKEDLONG      m_refCount;
    NETCONNADDR     m_connAddr;
    int             m_eventProcUserLock;
    NETEVENTPROC    m_eventProc;
    void           *m_user;
    CCritSect       m_lock;
    unsigned long   m_time;
    TCPNET         *m_net;

   protected:
    void Disconnect(int notify);

    friend struct TCPNET;
  };

  class NETCONNFULL : public NETCONN {
   public:
    NETCONNFULL(TCPNET *net, unsigned int sock, NETEVENTPROC eventProc, void *user, const NETCONNADDR *pconnAddr)
        : NETCONN(net, sock, eventProc, user, pconnAddr) {
    }

    virtual ~NETCONNFULL() {
    }
    virtual void    Send(const void *data, unsigned long bytes) = 0;
    virtual OS_SEND SendSync(const void *data, unsigned long bytes, unsigned long *bytesSent, unsigned long timeout) = 0;
    virtual void    SetNagle(int enable);
    virtual int     SetWindow(unsigned long size);
    virtual void    SetRecvTimeout(unsigned long timeoutMs);
  };

  class NETCONNLESS : public NETCONN {
   public:
    NETCONNLESS(TCPNET *net, unsigned int sock, NETEVENTPROC eventProc, void *user, const NETCONNADDR *pconnAddr)
        : NETCONN(net, sock, eventProc, user, pconnAddr) {
    }

    virtual ~NETCONNLESS() {
    }
    virtual void SendTo(const void *data, unsigned long bytes, unsigned long addrCount, const NETADDR *addrArray) = 0;
  };

  class TCPCONN : public NETCONNFULL {
   public:
    TCPCONN(TCPNET *net, unsigned int sock, NETEVENTPROC eventProc, void *user, const NETCONNADDR *pconnAddr);
    virtual ~TCPCONN();
    virtual void    Send(const void *data, unsigned long bytes);
    virtual OS_SEND SendSync(const void *data, unsigned long bytes, unsigned long *bytesSent, unsigned long timeout);
    virtual void    SetNagle(int enable);
    virtual int     SetWindow(unsigned long size);
    virtual void    SetRecvTimeout(unsigned long timeoutMs);

   protected:
    virtual void CompleteWrite(NETOVERLAP *poverlap, unsigned long bytes);
    virtual void CompleteRead(NETOVERLAP *poverlap, unsigned long bytes);
    virtual void StartWriteAndLeaveLock(OUTPUT *poutput) = 0;
    virtual void StartRead() = 0;
    virtual void CloseAndUnlock();

    TSList<OUTPUT, TSGetLink<OUTPUT> > m_outputList;
    unsigned long                      m_bytes;
    unsigned char                      m_data[1460];

   private:
    OUTPUT *LockedEnqueue(const void *data, unsigned long bytes);
  };

  class IOTCPCONN : public TCPCONN {
   public:
    IOTCPCONN(
        TCPNET            *net,
        void              *port,
        unsigned int       sock,
        NETEVENTPROC       eventProc,
        void              *user,
        const NETCONNADDR *pconnAddr,
        const void        *data,
        unsigned long      bytes
    );

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
    SLTCPCONN(
        TCPNET            *net,
        unsigned int       sock,
        NETEVENTPROC       eventProc,
        void              *user,
        const NETCONNADDR *pconnAddr,
        const void        *data,
        unsigned long      bytes
    );

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
    FILECONN(TCPNET *net, void *file, NETEVENTPROC eventProc, void *user, const NETCONNADDR *pconnAddr);
    virtual ~FILECONN();
    virtual int  IsClosed() const;
    virtual void AddToSelectSets(NETSELECTSETS *__formal);
    int          Write(unsigned __int64 pos, const void *data, unsigned long bytes, void *operationId);
    int          Read(unsigned __int64 pos, void *buffer, unsigned long bytes, void *operationId);

   protected:
    virtual void IncIo();
    virtual void DecIo();
    virtual void CompleteWrite(NETOVERLAP *poverlap, unsigned long bytes);
    virtual void CompleteRead(NETOVERLAP *poverlap, unsigned long bytes);
    virtual void StartWriteAndLeaveLock(OUTPUT *poutput) = 0;
    virtual void StartRead(INPUT *pinput) = 0;

   private:
    virtual void Selected(TCPNET *pnet, SELECTSET selectSet);
    LOCKEDLONG   m_ioCount;

   protected:
    void                              *m_file;
    TSList<OUTPUT, TSGetLink<OUTPUT> > m_outputList;
    TSList<INPUT, TSGetLink<INPUT> >   m_inputList;

   private:
    OUTPUT *LockedEnqueue(unsigned __int64 pos, const void *data, unsigned long bytes, void *operationId);
  };

  class IOFILECONN : public FILECONN {
   public:
    IOFILECONN(TCPNET *net, void *port, void *file, NETEVENTPROC eventProc, void *user, const NETCONNADDR *pconnAddr);

   private:
    virtual void StartWriteAndLeaveLock(OUTPUT *poutput);
    virtual void StartRead(INPUT *pinput);

   protected:
    virtual void CloseAndUnlock();
  };

  class SLFILECONN : public FILECONN {
   public:
    SLFILECONN(TCPNET *net, void *file, NETEVENTPROC eventProc, void *user, const NETCONNADDR *pconnAddr);
    virtual ~SLFILECONN();

   private:
    static unsigned int __stdcall Thread(void *lpfileConn);
    virtual void                  StartWriteAndLeaveLock(OUTPUT *poutput);
    virtual void                  StartRead(INPUT *pinput);

    void *m_thread;
    void *m_event;

   protected:
    virtual void CloseAndUnlock();
  };

  struct NETCONNECT : public NETSELSOCK {
    ~NETCONNECT();

    virtual void Fail() = 0;
    virtual void Complete(TCPNET *net) = 0;

    void NoteCantConnect(NETEVENTPROC eventProc, const NETCONNADDR *pconnAddr);

    TSLink<NETCONNECT> m_link;
    void              *m_user;
    void              *m_data;
    unsigned long      m_bytes;
  };

  struct LOOPCONNECT : public NETCONNECT {
    virtual int  IsClosed() const;
    virtual void AddToSelectSets(NETSELECTSETS *__formal);
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

    unsigned long m_nodeNumber;
    unsigned long m_portAddr;
    NETEVENTPROC  m_eventProc;

   private:
    virtual void Selected(TCPNET *pnet, SELECTSET selectSet);
  };

  struct FILECONNECT : public NETCONNECT {
    virtual int  IsClosed() const;
    virtual void AddToSelectSets(NETSELECTSETS *__formal);
    virtual void Fail();
    virtual void Complete(TCPNET *pnet);

    void        *m_file;
    NETEVENTPROC m_eventProc;

   private:
    virtual void Selected(TCPNET *net, SELECTSET selectSet);
  };

  class LOOPCONN : public NETCONNFULL {
   public:
    struct INPUT {
      TSLink<INPUT> m_link;
      TSLink<INPUT> m_linkNet;
      LOOPCONN     *m_conn;
      unsigned long m_bytes;
      unsigned long m_dataBytes;
      unsigned char m_data[4];

      ~INPUT();
    };

    LOOPCONN(TCPNET *net, NETEVENTPROC eventProc, void *user, const NETCONNADDR *pconnAddr);
    virtual ~LOOPCONN();
    virtual void    Send(const void *data, unsigned long bytes);
    virtual OS_SEND SendSync(const void *data, unsigned long bytes, unsigned long *bytesSent, unsigned long timeout);
    virtual void    Close();
    virtual int     IsClosed() const;

    LOOPCONN                *m_loopConn;
    TSLink<LOOPCONN>         m_linkNet;
    TSExplicitList<INPUT, 0> m_inputList;
    unsigned long            m_bytes;
    unsigned char            m_data[1460];

   protected:
    virtual void CloseAndUnlock();

   private:
    virtual void AddToSelectSets(NETSELECTSETS *selectSets);
    virtual void Selected(TCPNET *pnet, SELECTSET selectSet);
    void         Connect();
    void         CompleteInput(INPUT *pinput);
    void         EnqueueInput(const void *data, unsigned long bytes);

    friend struct TCPNET;
  };

  class UDPCONN : public NETCONNLESS {
   public:
    UDPCONN(TCPNET *net, unsigned int sock, NETEVENTPROC eventProc, void *user, const NETCONNADDR *pconnAddr);
    virtual ~UDPCONN();
    virtual void SendTo(const void *data, unsigned long bytes, unsigned long addrCount, const NETADDR *addrArray);

   protected:
    virtual void CloseAndUnlock();

   private:
    virtual void AddToSelectSets(NETSELECTSETS *selectSets);
    virtual void Selected(TCPNET *pnet, SELECTSET selectSet);
  };

  template <class T, int LINKOFFSET, int SLOTS>
  class TSSlottedListEx {
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

    long Slots() {
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

    unsigned char Link(T *ptr) {
      unsigned char slot;

      m_linkSlot.Inc();
      slot = static_cast<unsigned char>(static_cast<long>(m_linkSlot) & (SLOTS - 1));
      m_locks[slot].Enter();
      m_lists[slot].LinkNode(ptr, LIST_TAIL, 0);
      m_locks[slot].Leave();
      m_count.Inc();
      return slot;
    }

    void Unlink(T *ptr, unsigned char slot) {
      ASSERT((LONG)slot >= 0 && (LONG)slot < SLOTS);

      m_locks[slot].Enter();
      m_lists[slot].UnlinkNode(ptr);
      m_locks[slot].Leave();
      m_count.Dec();
    }

    TSExplicitList<T, LINKOFFSET> &UnlinkAll(TSExplicitList<T, LINKOFFSET> &list);

   private:
    TSExplicitList<T, LINKOFFSET> m_lists[SLOTS];
    CCritSect                     m_locks[SLOTS];
    LOCKEDLONG                    m_linkSlot;
    LOCKEDLONG                    m_count;

    friend class Iterator;
    friend struct TCPNET;
  };

  struct TCPACCEPT : public TSLinkedNode<TCPACCEPT> {
    TCPACCEPT(TCPLISTEN *listen);
    ~TCPACCEPT();

    void         Init();
    unsigned int Complete(NETCONNADDR *connAddr, int makeSock);

    NETOVERLAP    m_overlap;
    CCritSect     m_lock;
    unsigned char m_addr[64];
    TCPLISTEN    *m_listen;
    unsigned int  m_sock;
  };

  struct TCPLISTEN : public NETSELSOCK {
    TCPLISTEN(unsigned int sock, unsigned short port, NETEVENTPROC eventProc, void *user, unsigned long acceptCount);
    ~TCPLISTEN();

    int          Enable(int enable);
    void         Close();
    virtual void AddToSelectSets(NETSELECTSETS *selectSets);

    TSLink<TCPLISTEN>                        m_link;
    unsigned int                             m_portAddr;
    NETEVENTPROC                             m_eventProc;
    void                                    *m_user;
    int                                      m_enabled;
    TSList<TCPACCEPT, TSGetLink<TCPACCEPT> > m_acceptList;

   private:
    virtual void Selected(TCPNET *pnet, SELECTSET selectSet);
  };

  struct TCPHOSTADDRINFO : public TSLinkedNode<TCPHOSTADDRINFO> {
    ~TCPHOSTADDRINFO();
    void Complete();

    char                    *m_hostNameList;
    char                    *m_hostNameCurr;
    unsigned long            m_infoId;
    void                    *m_thread;
    NETHOSTADDRPROC          m_hostAddrProc;
    void                    *m_user;
    int                      m_ready;
    TSGrowableArray<NETADDR> m_addrs;
  };

  struct TCPHOSTADDRTHREAD {
    TCPNET        *m_net;
    unsigned long  m_infoId;
    void          *m_event;
    unsigned short m_defaultPort;
  };

  struct TCPNET {
   public:
    TCPNET();
    ~TCPNET();

    static int __fastcall  Initialize(unsigned long hints, unsigned long parts);
    static void __fastcall Destroy(unsigned long parts);

    void Pump(unsigned long timeout);
    int  TcpListen(unsigned short port, NETEVENTPROC eventProc, void *user);
    void TcpListenEnable(unsigned short port, int enable);
    void TcpConnect(unsigned long nodeNumber, unsigned short port, NETEVENTPROC eventProc, void *user, const void *data, unsigned long bytes);
    void UdpConnect(const NETADDR *addr, unsigned short portMin, unsigned short portMax, NETEVENTPROC eventProc, void *user);
    void FileConnCreate(const char *fileName, NETEVENTPROC eventProc, void *user, int readOnly);
    int  GetHostAddrs(const char *hostNameList, unsigned short defaultPort, NETHOSTADDRPROC hostAddrProc, void *user);
    TCPHOSTADDRINFO *LockedFindHostAddrInfo(unsigned long infoId);
    void             LoopConnect(NETEVENTPROC eventProcSrc, NETEVENTPROC eventProcDst, void *user, const void *data, unsigned long bytes);
    void             LoopCompleteConnect(LOOPCONNECT *pconnect);
    void             TcpCompleteConnect(TCPCONNECT *pconnect);
    void             FileCompleteConnect(FILECONNECT *pconnect);
    void             LinkConn(NETCONN *pconn, CONNLIST tolist);
    LOOPCONN::INPUT *LoopAllocInput(unsigned long bytes);
    void             LoopFreeInput(LOOPCONN::INPUT *pinput);

    static void __cdecl LogWrite(const char *format, ...);
    static void __cdecl LogDump(const char *header, const void *data, unsigned long bytes);

    static LPFN_ACCEPTEX             s_AcceptEx;
    static LPFN_GETACCEPTEXSOCKADDRS s_GetAcceptExSockaddrs;
    static LPFN_WSASEND              s_WSASend;
    static LPFN_WSARECV              s_WSARecv;
    static HSLOG                     s_log;
    static int                       s_qpcexists;
    static float                     s_qpctoms;

    void CompleteAcceptEx(TCPACCEPT *paccept, int makeConn);
    void CompleteAccept(TCPLISTEN *plisten, unsigned int sock, const NETCONNADDR *pconnAddr);

   private:
    static void __fastcall         MakeConnAddr(unsigned int sock, unsigned long port, NETCONNADDR *connAddr);
    static unsigned int __fastcall CreateListenSocket(unsigned short port);
    static void *__fastcall        IoCompletionPresent(unsigned long *pumpThreadCount);
    static void __fastcall         IncludeDependantParts(unsigned long *parts);

    int  BaseInitialize(unsigned long hints);
    void BaseDestroy();
    int  WinsockInitialize(unsigned long hints);
    void WinsockDestroy();
    int  IoInitialize(unsigned long hints);
    void IoDestroy();
    int  TcpInitialize(unsigned long hints);
    void TcpDestroy();
    void IncRef();
    void DecRef();
    void WakePumpThread();
    int  PumpThreadsInitialize();
    void PumpThreadsDestroy();

    static unsigned int __stdcall IoPumpThread(void *lpnet);
    static unsigned int __stdcall SlPumpThread(void *lpnet);
    static unsigned int __stdcall BaseThread(void *lpnet);
    static unsigned int __stdcall GetHostAddrsThread(void *lpparam);
    static unsigned int __stdcall ListenThread(void *lpnet);
    static unsigned int __stdcall UdpPumpThread(void *lpnet);

    void IoPump(unsigned long timeout);
    void TcpMakeConn(unsigned int sock, NETEVENTPROC eventProc, void *user, const NETCONNADDR *pconnAddr, const void *data, unsigned long bytes);
    void UdpMakeConn(unsigned int sock, NETEVENTPROC eventProc, void *user, const NETCONNADDR *pconnAddr);
    void FileMakeConn(void *file, NETEVENTPROC eventProc, void *user, const NETCONNADDR *pconnAddr);
    void LoopMakeConn(NETEVENTPROC eventProcSrc, NETEVENTPROC eventProcDst, void *user, const void *data, unsigned long bytes);
    void LoopConnectInit(LOOPCONNECT *pconnect);
    void TcpConnectInit(TCPCONNECT *pconnect);
    void FileConnectInit(FILECONNECT *pconnect);

    LOCKEDLONG                                           m_refCount;
    unsigned int                                         m_pumpThreadCount;
    TSGrowableArray<void *>                              m_pumpThreads;
    void                                                *m_udpPumpThread;
    void                                                *m_udpPumpEvent;
    CCritSect                                            m_loopLock;
    TSExplicitList<LOOPCONN::INPUT, 8>                   m_loopInputRecycleList;
    TSExplicitList<LOOPCONN::INPUT, 8>                   m_loopInputList;
    TSExplicitList<LOOPCONN, 108>                        m_loopDisconnectList;
    TSSlottedListEx<NETCONN, 8, 8>                       m_connList[CONNLISTS];
    void                                                *m_listenThread;
    TSSlottedListEx<TCPLISTEN, 8, 1>                     m_listenList;
    void                                                *m_baseThread;
    void                                                *m_baseEvent;
    int                                                  m_baseTcpShutdown;
    void                                                *m_baseTcpShutdownEvent;
    TSSlottedListEx<NETCONNECT, 8, 1>                    m_connectList[CONNLISTS];
    void                                                *m_port;
    LOCKEDLONG                                           m_hostAddrInfoCount;
    CEventLock                                           m_hostAddrInfoLock;
    unsigned long                                        m_hostAddrInfoId;
    TSList<TCPHOSTADDRINFO, TSGetLink<TCPHOSTADDRINFO> > m_hostAddrInfoList;

    static CInitCritSect s_initLock;
    static unsigned long s_initCount[5];
    static TCPNET *volatile s_pnet;
    static volatile int s_baseShutdown;
    static volatile int s_pumpShutdown;
    static volatile int s_tcpShutdown;
    static int          s_preTerminateHostAddr;
    static WSADATA      s_wsaData;
    static HMODULE      s_mswsockModule;
    static HMODULE      s_ws2Module;

    friend void __fastcall ::OsNetPump(unsigned long timeout);
    friend int __fastcall ::OsTcpListen(unsigned short port, NETEVENTPROC eventProc, void *user);
    friend void __fastcall ::OsTcpListenEnable(unsigned short port, int enable);
    friend void __fastcall ::OsTcpConnect(
        unsigned long  nodeNumber,
        unsigned short port,
        NETEVENTPROC   eventProc,
        void          *user,
        const void    *data,
        unsigned long  bytes
    );
    friend void __fastcall ::OsUdpConnect(const NETADDR *addr, unsigned short portMin, unsigned short portMax, NETEVENTPROC eventProc, void *user);
    friend void __fastcall ::OsFileConnCreate(const char *fileName, NETEVENTPROC eventProc, void *user, int readOnly);
    friend int __fastcall ::OsNetGetHostAddrs(const char *hostNameList, unsigned short defaultPort, NETHOSTADDRPROC hostAddrProc, void *user);
    friend struct LOOPCONN;
    friend class IOFILECONN;
    friend class SLFILECONN;
  };

  extern const char *OSNETERR_INTERNAL;
  extern const char *OSNETERR_WINSOCKSTARTUP;
  extern const char *OSNETERR_WINSOCKVERSION;
  extern const char *OSNETERR_SENDFAILED;
  extern const char *OSNETERR_THREADFAILED;
  extern const char *OSNETERR_EVENTFAILED;
  extern const char *OSNETERR_BINDFAILED;
  extern const char *OSNETERR_LISTENFAILED;
  extern const char *OSNETERR_SOCKETFAILED;
  extern const char *OSNETERR_SELECTFAILED;
  extern const char *OSNETERR_ACCEPTFAILED;
  extern const char *OSNETERR_PORTFAILED;
  extern const char *OSNETERR_OVERLAPTYPE;
  extern const char *OSNETERR_LISTENCLOSED;
  extern const char *OSNETERR_ACCEPTEXFAILED;

}  // namespace OsNet

#endif
