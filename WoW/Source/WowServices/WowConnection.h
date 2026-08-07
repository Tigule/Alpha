#ifndef WOW_SOURCE_WOWSERVICES_WOWCONNECTION_H
#define WOW_SOURCE_WOWSERVICES_WOWCONNECTION_H

#include "Os/OsNet.h"

#include <stpl.h>

#ifdef SetPort
#undef SetPort
#endif

class CDataStore;
struct sockaddr_in;

enum WOW_CONN_STATE {
  WOWC_UNINITIALIZED = 0,
  WOWC_INITIALIZED = 1,
  WOWC_CONNECTING = 2,
  WOWC_LISTENING = 3,
  WOWC_ACCEPTED = 4,
  WOWC_CONNECTED = 5,
  WOWC_DISCONNECTED = 6,
  WOWC_DISCONNECTING = 7,
  WOWC_ERROR = 8
};

enum WOWC_TYPE {
  WOWC_TYPE_MESSAGES = 0,
  WOWC_TYPE_STREAM = 1
};

enum WC_SEND_RESULT {
  WC_SEND_SENT = 0,
  WC_SEND_QUEUED = 1,
  WC_SEND_ERROR = 2
};

struct WowConnectionStats {
  int   m_bytesReceived;
  int   m_bytesSent;
  int   m_messagesReceived;
  int   m_messagesSent;
  DWORD m_connectTime;
  DWORD m_lastDataReceived;
};

class WowConnection;

class WowConnectionResponse {
 public:
  virtual ~WowConnectionResponse() {
  }

  virtual void WCMessageReady(WowConnection *connection, DWORD timeStamp, CDataStore *message) = 0;
  virtual void WCConnected(WowConnection *connection, WowConnection *established, DWORD timeStamp, const NETCONNADDR *address) = 0;
  virtual void WCCantConnect(WowConnection *connection, DWORD timeStamp, const NETCONNADDR *address) = 0;
  virtual void WCDisconnected(WowConnection *connection, DWORD timeStamp, const NETCONNADDR *address) = 0;
  virtual void WCGlobalLock();
  virtual void WCGlobalUnlock();
  virtual void WCDataReady(WowConnection *conn, DWORD timeStamp, BYTE *data, int len);
  virtual void WCWriteReady(WowConnection *conn);
};

class WowConnection {
 public:
  NODEDECL(SENDNODE) {
    SENDNODE(const SENDNODE &node);
    SENDNODE(BYTE * d, int s, LPVOID p, BYTE raw) : data(d) {
      if (!raw) {
        int headerSize;

        if (s <= 0x7fff) {
          data[0] = static_cast<BYTE>(s >> 8);
          data[1] = static_cast<BYTE>(s);
          headerSize = 2;
        } else {
          ASSERT(s <= 0x7fffffff);
          data[0] = static_cast<BYTE>((s >> 16) | 0x80);
          data[1] = static_cast<BYTE>(s >> 8);
          data[2] = static_cast<BYTE>(s);
          headerSize = 3;
        }

        memcpy(data + headerSize, p, s);
        datasize = s;
        size = s + headerSize;
        offset = 0;
      } else {
        memcpy(data, p, s);
        size = s;
        offset = 0;
        datasize = s;
      }
    }
    ~SENDNODE();

    BYTE *data;
    UINT  size;
    UINT  offset;
    UINT  datasize;
  };
  typedef SENDNODE       *PSENDNODE;
  typedef const SENDNODE *PCSENDNODE;

  ~WowConnection();

  WowConnection(WowConnectionResponse *response, void (*func)());
  WowConnection(int sock, sockaddr_in *addr, WowConnectionResponse *response);
  WowConnection(int connection, const NETCONNADDR *address, WowConnectionResponse *response, void (*func)());
  WowConnection(const WowConnection &connection);
  WowConnection &operator=(const WowConnection &connection);

  int            AddRef();
  int            Release();
  bool           Connect(DWORD addr, WORD port, int retryms);
  bool           Connect(LPCSTR address, WORD port, int retryms);
  bool           Connect(LPCSTR address, int retryms);
  void           StartConnect();
  WOW_CONN_STATE GetState() {
    return m_connState;
  }
  void                   SetResponse(WowConnectionResponse *response);
  WowConnectionResponse *GetResponse();
  DWORD                  Connection();
  void                   AddIncomingData(LPCVOID data, DWORD bytes, DWORD timeStamp, DWORD *consumed);
  void                   SetType(WOWC_TYPE type);
  void                   AcquireResponseRef();
  void                   ReleaseResponseRef();
  void                   CheckConnect();
  void                   CheckAccept();
  void                   Disconnect();
  void                   DoWrites();
  void                   DoReads();
  void                   DoMessageReads();
  void                   DoStreamReads();
  void                   DoExceptions();
  void                   DoDisconnect();
  WC_SEND_RESULT         Send(CDataStore *msg);
  WC_SEND_RESULT         Send(CDataStore *msg, CDataStore *reply);
  WC_SEND_RESULT         SendRaw(BYTE *data, int len);
  void                   RequestWriteNotification();
  void                   Idle();
  void                   GetPeer(NETADDR &address);
  void                   GetPeer(NETCONNADDR &address);
  bool                   GetLocal(NETADDR &addr);
  static DWORD           GetAddr(NETADDR &addr);
  static WORD            GetPort(NETADDR &addr);
  static void            SetPort(NETADDR &addr, WORD port);
  bool                   Reconnect();
  bool                   Listen(WORD port);
  void                   StopListening();
  char                  *GetStringAddress(char *buf, int size);
  DWORD                  GetConnectAddress();
  WORD                   GetConnectPort();
  void                   SetAutoSendSize(DWORD bytes);
  WORD                   GetListenPort();
  WOWC_TYPE              GetType();
  bool                   WantsWriteNotification();

  static int  InitOsNet(bool (*verifyAddr)(const NETADDR *), void (*threadInit)(), int numThreads, bool useEngine);
  static void DestroyOsNet();
  static bool IsDestroyed();

 private:
  friend class WowConnectionNet;

  void      Init(WowConnectionResponse *response, void (*func)());
  int       CreateSocket();
  void      CloseSocket(int sock);
  SENDNODE *NewSendNode(LPVOID data, int size, bool raw);
  void      FreeSendNode(SENDNODE *sn);
  void      DoSends();
  void      SetState(WOW_CONN_STATE state);

  int                    m_refCount;
  int                    m_sock;
  int                    m_oldsock;
  BYTE                   m_connectionFreed;
  WOW_CONN_STATE         m_connState;
  WowConnectionResponse *m_response;
  DWORD                  m_needBytes;
  BYTE                  *m_readBuffer;
  int                    m_readBytes;
  int                    m_readBufferSize;
  SCritSect              m_outLock;
  WowConnectionStats     m_stats;
  DWORD                  m_haveSizeBytes;
  WORD                   m_listenPort;
  void (*m_threadInit)();
  DWORD       m_connectAddress;
  WORD        m_connectPort;
  int         m_connectRetryInterval;
  DWORD       m_retryConnection;
  NETCONNADDR m_peer;
  DWORD       m_bufferAutoSendSize;
  SCritSect   m_responseLock;
  int         m_responseRef;
  DWORD       m_responseRefThread;
  static bool (*m_verifyAddr)(const NETADDR *);
  LINKDECLEX(WowConnection, netlink);
  LISTDECL(SENDNODE, m_sendList);
  int       m_sendDepth;
  int       m_sendDepthBytes;
  UINT      m_serviceFlags;
  SCritSect m_lock;
  long      m_serviceCount;
  LPVOID    m_event;
  WOWC_TYPE m_type;
  BOOL      m_wantWriteNotification;
};

#endif
