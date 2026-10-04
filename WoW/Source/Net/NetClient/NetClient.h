#ifndef WOW_SOURCE_NET_NETCLIENT_NETCLIENT_H
#define WOW_SOURCE_NET_NETCLIENT_NETCLIENT_H

#include <stddef.h>
#include <storm.h>

#include "WowServices/WDataStore.h"
#include "WowServices/WowConnection.h"
#include "WowSvcs/WowSvcsClient/ClientServices.h"

class CDataStore;
class ClntObjMgr;
class NetClientRedirect;
class NETEVENTQUEUE;
class WowConnection;
struct NETCONNADDR;

void ClientNetGetRealms(LPCSTR serverAddress, void (*fcn)(CDataStore *, LPVOID), LPVOID userData);

enum NETSTATE {
  NS_UNINITIALIZED = 0,
  NS_INITIALIZING = 1,
  NS_INITIALIZED = 2,
  NS_REDIRECT_CONNECTING = 3,
  NS_GETTING_REALMS = 4,
  NS_CONNECTING = 5,
  NS_CONNECTED = 6,
  NS_DISCONNECTING = 7
};

class NetClient : public WowConnectionResponse {
 public:
  NetClient();
  NetClient(const NetClient &client);
  virtual ~NetClient();

  NetClient &operator=(const NetClient &client);

  virtual BOOL Initialize();
  virtual void Destroy();
  virtual BOOL DelayedDelete();

  void Connect(LPCSTR hostName);
  void Disconnect();
  void Send(CDataStore *msg);
  void SetMessageHandler(NETMESSAGE msgId, BOOL (*handler)(LPVOID, NETMESSAGE, DWORD, CDataStore *), LPVOID param);
  void ClearMessageHandler(NETMESSAGE msgId);

  NETSTATE GetState() {
    return m_netState;
  }

  void HandleIdle();

  virtual BOOL HandleData(DWORD timeReceived, LPVOID data, int size);
  virtual BOOL HandleConnect();
  virtual BOOL HandleDisconnect();
  virtual BOOL HandleCantConnect();

  void AddRef() {
    long newval = SInterlockedIncrement((long *)&m_refCount);
    ASSERT(newval > 0);
  }

  void DelRef() {
    long newval = SInterlockedDecrement((long *)&m_refCount);
    ASSERT(newval >= 0);
    if (!newval && m_deleteMe) {
      DEL(this);
    }
  }

  void SetDelete() {
    m_deleteMe = 1;
  }

  BYTE GetDelete() {
    return m_deleteMe;
  }

  void GetNetStats(float &bandwidthIn, float &bandwidthOut, DWORD &latency);

  void SetObjMgr(ClntObjMgr *objMgr) {
    m_objMgr = objMgr;
  }

  void PollEventQueue();
  UINT GetAddr();

 private:
  friend class NetClientRedirect;

  int  Connect(LPCSTR hostName, WORD port);

  static int s_clientCount;

  NetClient **m_redirectHandle;
  int         m_redirectBytesRead;
  char        m_redirectHostPort[0x401];
  NETSTATE    m_netState;
  BOOL (*m_handlers[NUM_MSG_TYPES])(LPVOID, NETMESSAGE, DWORD, CDataStore *);
  LPVOID             m_handlerParams[NUM_MSG_TYPES];
  NETEVENTQUEUE     *m_netEventQueue;
  WowConnection     *m_serverConnection;
  int                m_refCount;
  BYTE               m_deleteMe;
  DWORD              m_pingSent;
  DWORD              m_pingSequence;
  DWORD              m_latency[16];
  DWORD              m_latencyStart;
  DWORD              m_latencyEnd;
  DWORD              m_bytesSent;
  DWORD              m_bytesReceived;
  DWORD              m_connectedTimestamp;
  SCritSect          m_pingLock;
  ClntObjMgr        *m_objMgr;
  ClntObjMgr        *m_saveObjMgr;
  NetClientRedirect *m_redirect;

  void PushObjMgr();
  void PopObjMgr();
  void CancelRedirect();
  void ProcessMessage(DWORD timeStamp, CDataStore *msg);
  static int __stdcall ClientRedirectEventHandler(
      HNETCONN__        *conn,
      const NETCONNADDR *connAddr,
      NETNOTE            note,
      LPVOID             user,
      LPCVOID            data,
      DWORD              bytes,
      DWORD             *bytesProcessed
  );

  virtual void WCMessageReady(WowConnection *conn, DWORD timeStamp, CDataStore *msg);
  virtual void WCConnected(WowConnection *conn, WowConnection *inbound, DWORD timeStamp, const NETCONNADDR *addr);
  virtual void WCDisconnected(WowConnection *conn, DWORD timeStamp, const NETCONNADDR *addr);
  virtual void WCCantConnect(WowConnection *conn, DWORD timeStamp, const NETCONNADDR *addr);

  void PongHandler(CDataStore *msg);
  void DisplayNetworkStats();
  void Ping();
};

#endif
