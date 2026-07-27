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
  int          m_bytesReceived;
  int          m_bytesSent;
  int          m_messagesReceived;
  int          m_messagesSent;
  unsigned int m_connectTime;
  unsigned int m_lastDataReceived;
};

class WowConnection;

class WowConnectionResponse {
 public:
  virtual ~WowConnectionResponse() {
  }

  virtual void WCMessageReady(WowConnection *connection, unsigned long timeStamp, CDataStore *message) = 0;
  virtual void WCConnected(WowConnection *connection, WowConnection *established, unsigned long timeStamp, const NETCONNADDR *address) = 0;
  virtual void WCCantConnect(WowConnection *connection, unsigned long timeStamp, const NETCONNADDR *address) = 0;
  virtual void WCDisconnected(WowConnection *connection, unsigned long timeStamp, const NETCONNADDR *address) = 0;
  virtual void WCGlobalLock();
  virtual void WCGlobalUnlock();
  virtual void WCDataReady(WowConnection *conn, unsigned long timeStamp, unsigned char *data, int len);
  virtual void WCWriteReady(WowConnection *conn);
};

class WowConnection {
 public:
  struct SENDNODE : public TSLinkedNode<SENDNODE> {
    SENDNODE(const SENDNODE &node);
    SENDNODE(unsigned char *d, int s, void *p, unsigned char raw) : data(d) {
      if (!raw) {
        int headerSize;

        if (s <= 0x7fff) {
          data[0] = static_cast<unsigned char>(s >> 8);
          data[1] = static_cast<unsigned char>(s);
          headerSize = 2;
        } else {
          ASSERT(s <= 0x7fffffff);
          data[0] = static_cast<unsigned char>((s >> 16) | 0x80);
          data[1] = static_cast<unsigned char>(s >> 8);
          data[2] = static_cast<unsigned char>(s);
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

    unsigned char *data;
    unsigned int   size;
    unsigned int   offset;
    unsigned int   datasize;
  };

  ~WowConnection();

  WowConnection(WowConnectionResponse *response, void(*func)());
  WowConnection(int sock, sockaddr_in *addr, WowConnectionResponse *response);
  WowConnection(const WowConnection &connection);
  WowConnection &operator=(const WowConnection &connection);

  int            AddRef();
  int            Release();
  bool           Connect(unsigned long addr, unsigned short port, int retryms);
  bool           Connect(const char *address, unsigned short port, int retryms);
  bool           Connect(const char *address, int retryms);
  void           StartConnect();
  WOW_CONN_STATE GetState() {
    return m_connState;
  }
  void                             SetResponse(WowConnectionResponse *response);
  void                             SetType(WOWC_TYPE type);
  void                             AcquireResponseRef();
  void                             ReleaseResponseRef();
  void                             CheckConnect();
  void                             CheckAccept();
  void                             Disconnect();
  void                             DoWrites();
  void                             DoReads();
  void                             DoMessageReads();
  void                             DoStreamReads();
  void                             DoExceptions();
  void                             DoDisconnect();
  WC_SEND_RESULT                   Send(CDataStore *msg);
  WC_SEND_RESULT                   SendRaw(unsigned char *data, int len);
  void                             RequestWriteNotification();
  bool                             GetLocal(NETADDR &addr);
  static unsigned long GetAddr(NETADDR &addr);
  static unsigned short GetPort(NETADDR &addr);
  static void SetPort(NETADDR &addr, unsigned short port);
  bool                             Reconnect();
  bool                             Listen(unsigned short port);
  void                             StopListening();
  char                            *GetStringAddress(char *buf, int size);

  static int InitOsNet(bool(*verifyAddr)(const NETADDR *), void(*threadInit)(), int numThreads, bool useEngine);
  static void DestroyOsNet();
  static bool IsDestroyed();

 private:
  friend class WowConnectionNet;

  void      Init(WowConnectionResponse *response, void(*func)());
  int       CreateSocket();
  void      CloseSocket(int sock);
  SENDNODE *NewSendNode(void *data, int size, bool raw);
  void      FreeSendNode(SENDNODE *sn);
  void      SetState(WOW_CONN_STATE state);

  int                    m_refCount;
  int                    m_sock;
  int                    m_oldsock;
  unsigned char          m_connectionFreed;
  WOW_CONN_STATE         m_connState;
  WowConnectionResponse *m_response;
  unsigned int           m_needBytes;
  unsigned char         *m_readBuffer;
  int                    m_readBytes;
  int                    m_readBufferSize;
  SCritSect              m_outLock;
  WowConnectionStats     m_stats;
  unsigned int           m_haveSizeBytes;
  unsigned short         m_listenPort;
  void(*m_threadInit)();
  unsigned int   m_connectAddress;
  unsigned short m_connectPort;
  int            m_connectRetryInterval;
  unsigned int   m_retryConnection;
  NETCONNADDR    m_peer;
  unsigned int   m_bufferAutoSendSize;
  SCritSect      m_responseLock;
  int            m_responseRef;
  unsigned int   m_responseRefThread;
  static bool(*m_verifyAddr)(const NETADDR *);
  TSLink<WowConnection>                  netlink;
  TSList<SENDNODE, TSGetLink<SENDNODE> > m_sendList;
  int                                    m_sendDepth;
  int                                    m_sendDepthBytes;
  unsigned int                           m_serviceFlags;
  SCritSect                              m_lock;
  long                                   m_serviceCount;
  void                                  *m_event;
  WOWC_TYPE                              m_type;
  unsigned char                          m_wantWriteNotification;
};

#endif
