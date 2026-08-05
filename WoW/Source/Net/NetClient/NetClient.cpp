#include <WowConst.h>

#include "NetClient.h"

#include <ctype.h>
#include <new>
#include <stdlib.h>
#include <string.h>

#include "Base/Base.h"
#include "Base/CDataStore.h"
#include "Console/ConsoleClient.h"
#include "Glue/CGlueMgr.h"
#include "Net/NetInternal.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "Os/OsTime.h"

void WowConnectionResponse::WCGlobalLock() {
}

void WowConnectionResponse::WCGlobalUnlock() {
}

void WowConnectionResponse::WCDataReady(WowConnection *conn, unsigned long timeStamp, unsigned char *data, int len) {
}

void WowConnectionResponse::WCWriteReady(WowConnection *conn) {
}

NODEDECL(NETCLIENTNODE) {
  NETCLIENTNODE() {
  }

  NetClient *client;
};

struct CLIENT_NETSTATS {
  unsigned long bytesSent;
  unsigned long messagesSent;
  unsigned long sendTimestamp;
  unsigned long bytesReceived;
  unsigned long messagesReceived;
  unsigned long receivTimestamp;
  unsigned long logTimestamp;
};

struct CLIENTNETGETREALMSDATA {
  void(*fcn)(CDataStore *, void *);
  void      *userData;
  CDataStore data;
};

class NetClientRedirect : public WowConnectionResponse {
 public:
  NetClientRedirect(NetClient *owner) {
    m_owner = owner;
    m_conn = 0;
  }
  NetClientRedirect(const NetClientRedirect &redirect);
  virtual ~NetClientRedirect() {
    if (m_conn) {
      m_conn->SetResponse(0);
      m_conn->Release();
      m_conn = 0;
    }
  }

  void Connect(const char *host, unsigned short port) {
    if (m_conn) {
      m_conn->SetResponse(0);
      m_conn->Release();
    }

    m_conn = new (ALLOC(sizeof(WowConnection))) WowConnection(this, 0);
    m_conn->SetType(WOWC_TYPE_STREAM);
    m_conn->Connect(host, port, -1);
  }

  virtual void WCMessageReady(WowConnection *, unsigned long, CDataStore *) {
    ASSERT(0);
  }

  virtual void WCConnected(WowConnection *, WowConnection *, unsigned long, const NETCONNADDR *) {
  }

  virtual void WCDisconnected(WowConnection *conn, unsigned long __formal, const NETCONNADDR *addr) {
    if (m_owner) {
      m_owner->m_redirectHostPort[m_owner->m_redirectBytesRead] = 0;

      char *hostPort = m_owner->m_redirectHostPort;
      char *port = SStrChr(hostPort, ':');
      if (port) {
        *port++ = 0;
        m_owner->m_netState = NS_INITIALIZED;
        m_owner->Connect(m_owner->m_redirectHostPort, static_cast<unsigned short>(atoi(port)));
      } else {
        m_owner->m_netState = NS_CONNECTING;
        m_owner->m_netEventQueue->AddEvent(EVENT_ID_NET_CANTCONNECT, conn, m_owner, 0, 0);
      }
    }
  }

  virtual void WCCantConnect(WowConnection *conn, unsigned long, const NETCONNADDR *) {
    if (m_owner) {
      m_owner->m_netState = NS_CONNECTING;
      m_owner->m_netEventQueue->AddEvent(EVENT_ID_NET_CANTCONNECT, conn, m_owner, 0, 0);
    }
  }

  virtual void WCDataReady(WowConnection *conn, unsigned long __formal, unsigned char *data, int bytes) {
    if (m_owner) {
      int bytesRead = m_owner->m_redirectBytesRead;
      if (bytesRead + bytes > 1024) {
        m_conn->Disconnect();
        return;
      }

      memcpy(&m_owner->m_redirectHostPort[bytesRead], data, bytes);
      m_owner->m_redirectBytesRead += bytes;
    }
  }

 private:
  NetClient     *m_owner;
  WowConnection *m_conn;
};

static LISTDECL(NETCLIENTNODE, s_clientList);
static CLIENT_NETSTATS                                  s_stats;
static HPROPCONTEXT                                     s_propContext;

static void LogStats() {
  char message[128];

  SStrPrintf(
      message, sizeof(message), "Client net stats: %Lu bytes sent, %Lu bytes received, %Lu msec elapsed", s_stats.bytesSent, s_stats.bytesReceived,
      OsGetAsyncTimeMs() - s_stats.logTimestamp
  );
  ConsoleWrite(message, DEFAULT_COLOR);
}

int NetClient::s_clientCount;

NetClient::NetClient() {
  m_netState = NS_UNINITIALIZED;
  m_netEventQueue = 0;
  m_serverConnection = 0;
  m_refCount = 0;
  m_deleteMe = 0;
  m_pingSent = 0;
  m_pingSequence = 0;
  m_bytesSent = 0;
  m_bytesReceived = 0;
  m_connectedTimestamp = 0;
  m_objMgr = 0;
  m_saveObjMgr = 0;
  m_redirect = 0;

  NETCLIENTNODE *node = s_clientList.NewNode(LIST_TAIL, 0, 0);
  node->client = this;
}

NetClient::~NetClient() {
  Destroy();

  ITERATELIST(NETCLIENTNODE, s_clientList, node) {
    if (node->client == this) {
      ITERATE_DELETEANDBREAK
    }
  }
}

static void InitializePropContext() {
  if (PropGetSelectedContext() != s_propContext) {
    PropSelectContext(s_propContext);
  }
}

int NetClient::Initialize() {
  ASSERT(m_netState == NS_UNINITIALIZED);

  if (!s_clientCount) {
    s_propContext = PropGetSelectedContext();
    if (!WowConnection::InitOsNet(0, InitializePropContext, 1, true)) {
      return 0;
    }
  }

  ++s_clientCount;

  m_netEventQueue = new (ALLOC(sizeof(NETEVENTQUEUE))) NETEVENTQUEUE(this);
  ASSERT(m_netEventQueue);

  memset(m_handlers, 0, sizeof(m_handlers));
  memset(m_handlerParams, 0, sizeof(m_handlerParams));

  m_serverConnection = new (ALLOC(sizeof(WowConnection))) WowConnection(this, 0);
  ASSERT(m_serverConnection);

  m_netState = NS_INITIALIZED;
  s_stats.logTimestamp = OsGetAsyncTimeMs();
  return 1;
}

void NetClient::Destroy() {
  if (m_netState != NS_UNINITIALIZED) {
    Disconnect();

    m_serverConnection->SetResponse(0);
    m_serverConnection->Release();
    m_serverConnection = 0;

    memset(m_handlers, 0, sizeof(m_handlers));
    memset(m_handlerParams, 0, sizeof(m_handlerParams));

    if (m_netEventQueue) {
      m_netEventQueue->~NETEVENTQUEUE();
      SMemFree(m_netEventQueue, "delete", -1, 0);
    }
    m_netEventQueue = 0;

    DELIFUSED(m_redirect);

    --s_clientCount;
    if (!s_clientCount) {
      OsSleep(1);
      WowConnection::DestroyOsNet();
    }

    m_netState = NS_UNINITIALIZED;
    if (!s_clientCount) {
      LogStats();
    }
  }
}

int NetClient::DelayedDelete() {
  if (m_netState != NS_UNINITIALIZED) {
    Disconnect();
    m_netEventQueue->AddEvent(EVENT_ID_NET_DESTROY, 0, this, 0, 0);
    return 1;
  }

  return 0;
}

void NetClient::CancelRedirect() {
  if (m_netState == NS_REDIRECT_CONNECTING) {
    *m_redirectHandle = 0;
    m_netState = NS_INITIALIZED;
  }
}

void NetClient::Connect(const char *hostName) {
  char hostport[1024];

  ASSERT(hostName);

  CancelRedirect();
  ASSERT(m_netState == NS_INITIALIZED);

  m_redirectBytesRead = 0;
  unsigned short port = 9090;
  SStrCopy(hostport, hostName, sizeof(hostport));

  char *portString = SStrChr(hostport, ':');
  if (portString) {
    *portString = 0;
    port = static_cast<unsigned short>(atoi(portString + 1));
  }

  m_netState = NS_REDIRECT_CONNECTING;

  if (!m_redirect) {
    m_redirect = new (ALLOC(sizeof(NetClientRedirect))) NetClientRedirect(this);
  }

  m_redirect->Connect(hostport, port);
}

int NetClient::Connect(const char *hostName, unsigned short port) {
  ASSERT(m_netState == NS_INITIALIZED);

  m_netState = NS_CONNECTING;
  m_serverConnection->Connect(hostName, port, -1);
  return 1;
}

void NetClient::Disconnect() {
  CancelRedirect();

  if (m_netState == NS_CONNECTED) {
    m_netState = NS_DISCONNECTING;
    m_serverConnection->Disconnect();
  }
}

void NetClient::Send(CDataStore *msg) {
  if (m_netState == NS_CONNECTED) {
    unsigned long bytes = msg->Size() - msg->Tell();

    if (bytes) {
      m_serverConnection->Send(msg);
      s_stats.bytesSent += bytes;
      m_bytesSent += bytes;
    }
  }
}

void NetClient::SetMessageHandler(NETMESSAGE msgId, int(*handler)(void *, NETMESSAGE, unsigned long, CDataStore *), void *param) {
  ASSERT(msgId < NUM_MSG_TYPES);
  ASSERT(handler);
  ASSERT(m_handlers[msgId] == 0);

  m_handlers[msgId] = handler;
  m_handlerParams[msgId] = param;
}

void NetClient::ClearMessageHandler(NETMESSAGE msgId) {
  ASSERT(msgId < NUM_MSG_TYPES);

  m_handlers[msgId] = 0;
  m_handlerParams[msgId] = 0;
}

void NetClient::ProcessMessage(unsigned long timeStamp, CDataStore *msg) {
  ++s_stats.messagesReceived;

  unsigned long dwid;
  msg->Get(dwid);
  NETMESSAGE id = static_cast<NETMESSAGE>(dwid);

  if (id >= NUM_MSG_TYPES) {
    SErrDisplayErrorFmt(
        STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1,
        isprint((dwid >> 24) & 0xFF) && isprint((dwid >> 16) & 0xFF) && isprint((dwid >> 8) & 0xFF) && isprint(dwid & 0xFF)
            ? "\"%s\", %s = %ld (0x%08X, '%c%c%c%c')"
            : "\"%s\", %s = %ld (0x%08X)",
        "id < NUM_MSG_TYPES", "id", id, id, (dwid >> 24) & 0xFF, (dwid >> 16) & 0xFF, (dwid >> 8) & 0xFF, dwid & 0xFF
    );
  }

  if (m_handlers[id]) {
    m_handlers[id](m_handlerParams[id], id, timeStamp, msg);

    if (!msg->IsRead() || !msg->IsValid()) {
      ConsolePrintf("Message ID %d %s read", id, msg->IsRead() ? "over" : "under");
    }
  } else {
    msg->Reset();
  }
}

void NetClient::WCMessageReady(WowConnection *conn, unsigned long timeStamp, CDataStore *msg) {
  unsigned long bytes = msg->Size();
  void         *data;

  msg->GetDataInSitu(data, bytes);
  SInterlockedExchangeAdd((long *)&m_bytesReceived, msg->Size());

  msg->Seek(0);

  int msgID;
  msg->Get(msgID);
  if (msgID == SMSG_PONG) {
    PongHandler(msg);
  } else {
    msg->Seek(msg->Size());
    m_netEventQueue->AddEvent(EVENT_ID_NET_DATA, conn, this, data, bytes);
  }
}

void NetClient::WCConnected(WowConnection *conn, WowConnection *inbound, unsigned long timeStamp, const NETCONNADDR *addr) {
  m_pingLock.Enter();

  m_connectedTimestamp = timeStamp;
  m_bytesReceived = 0;
  m_bytesSent = 0;
  m_latencyStart = 0;
  m_latencyEnd = 0;
  m_pingSent = OsGetAsyncTimeMsPrecise();

  m_pingLock.Leave();

  m_netEventQueue->AddEvent(EVENT_ID_NET_CONNECT, conn, this, 0, 0);
}

void NetClient::WCDisconnected(WowConnection *conn, unsigned long timeStamp, const NETCONNADDR *addr) {
  DisplayNetworkStats();

  if (m_netEventQueue) {
    m_netEventQueue->AddEvent(EVENT_ID_NET_DISCONNECT, conn, this, 0, 0);
  }
}

void NetClient::WCCantConnect(WowConnection *conn, unsigned long timeStamp, const NETCONNADDR *addr) {
  m_netEventQueue->AddEvent(EVENT_ID_NET_CANTCONNECT, conn, this, 0, 0);
}

int NetClient::HandleData(unsigned long timeReceived, void *data, int size) {
  PushObjMgr();

  ASSERT(m_netState == NS_CONNECTED || m_netState == NS_DISCONNECTING);

  s_stats.bytesReceived += size + 2;
  if (m_netState == NS_CONNECTED) {
    CDataStore theMessage(static_cast<unsigned char *>(data), size);
    ProcessMessage(timeReceived, &theMessage);
  }

  PopObjMgr();
  return 1;
}

int NetClient::HandleConnect() {
  PushObjMgr();

  ASSERT(m_netState == NS_CONNECTING);
  m_netState = NS_CONNECTED;

  PopObjMgr();
  return 1;
}

int NetClient::HandleDisconnect() {
  PushObjMgr();

  ASSERT(m_netState == NS_CONNECTED || m_netState == NS_DISCONNECTING);
  m_netState = NS_INITIALIZED;
  CGlueMgr::NetDisconnectHandler(this, 0);

  PopObjMgr();
  return 1;
}

int NetClient::HandleCantConnect() {
  PushObjMgr();

  ASSERT(m_netState == NS_CONNECTING);
  m_netState = NS_INITIALIZED;

  PopObjMgr();
  return 1;
}

static int __stdcall GetRealmsEventHandler(
    HNETCONN__        *conn,
    const NETCONNADDR *connAddr,
    NETNOTE            note,
    void              *user,
    const void        *data,
    unsigned long      bytes,
    unsigned long     *bytesProcessed
) {
  if (bytesProcessed) {
    *bytesProcessed = bytes;
  }

  CLIENTNETGETREALMSDATA &connectionData = *static_cast<CLIENTNETGETREALMSDATA *>(user);

  switch (note) {
    case NETNOTE_CONNECT:
      return 1;

    case NETNOTE_DATA:
      connectionData.data.PutData(data, bytes);
      return 1;

    case NETNOTE_DISCONNECT:
      connectionData.data.Finalize();
      connectionData.fcn(&connectionData.data, connectionData.userData);
      DEL(&connectionData);
      return 1;

    case NETNOTE_CANTCONNECT:
      connectionData.fcn(0, connectionData.userData);
      DEL(&connectionData);
      return 1;
  }

  return 0;
}

void ClientNetGetRealms(const char *serverAddress, void(*fcn)(CDataStore *, void *), void *userData) {
  ASSERT(serverAddress);
  ASSERT(fcn);

  CLIENTNETGETREALMSDATA *connectionData = NEW(CLIENTNETGETREALMSDATA);
  ASSERT(connectionData);

  connectionData->fcn = fcn;
  connectionData->userData = userData;

  unsigned long address = OsNetGetHostAddr(serverAddress);
  OsTcpConnect(address, 9100, GetRealmsEventHandler, connectionData, 0, 0);
}

void NetClient::PongHandler(CDataStore *msg) {
  unsigned long sequence;

  m_pingLock.Enter();
  msg->Get(sequence);

  if (sequence != m_pingSequence) {
    ConsolePrintf("Received pong with old sequence");
  } else {
    m_latency[m_latencyEnd] = OsGetAsyncTimeMsPrecise() - m_pingSent;

    ++m_latencyEnd;
    if (m_latencyEnd >= 16) {
      m_latencyEnd = 0;
    }

    if (m_latencyEnd == m_latencyStart) {
      ++m_latencyStart;
      if (m_latencyStart >= 16) {
        m_latencyStart = 0;
      }
    }
  }

  m_pingLock.Leave();
}

void NetClient::Ping() {
  CDataStore msg;

  m_pingLock.Enter();
  m_pingSent = OsGetAsyncTimeMsPrecise();
  msg.Put(CMSG_PING);
  msg.Put(++m_pingSequence);
  m_pingLock.Leave();

  msg.Finalize();
  Send(&msg);
}

void NetClient::DisplayNetworkStats() {
  float         bandwidthIn;
  float         bandwidthOut;
  unsigned long latency;

  m_pingLock.Enter();
  OsGetAsyncTimeMs();
  GetNetStats(bandwidthIn, bandwidthOut, latency);
  m_pingLock.Leave();
}

void NetClient::GetNetStats(float &bandwidthIn, float &bandwidthOut, unsigned long &latency) {
  m_pingLock.Enter();

  float connectedSeconds = (OsGetAsyncTimeMs() - m_connectedTimestamp) * 0.001f;
  bandwidthIn = m_bytesReceived * 0.0009765625f / connectedSeconds;
  bandwidthOut = m_bytesSent * 0.0009765625f / connectedSeconds;

  unsigned long count = 0;
  unsigned long total = 0;
  unsigned long current = m_latencyStart;

  while (current != m_latencyEnd) {
    if (current >= 16) {
      current = 0;
      if (!m_latencyEnd) {
        break;
      }
    }

    total += m_latency[current];
    ++count;
    ++current;
  }

  latency = count ? total / count : 0;

  m_pingLock.Leave();
}

void NetClient::HandleIdle() {
  if ((long)(OsGetAsyncTimeMsPrecise() - m_pingSent - 30000) >= 0) {
    Ping();
  }
}

void NetClient::PushObjMgr() {
  if (m_objMgr) {
    ASSERT(!m_saveObjMgr);

    m_saveObjMgr = ClntObjMgrGetCurrent();
    ClntObjMgrSetCurrent(m_objMgr);
  }
}

void NetClient::PopObjMgr() {
  if (m_objMgr) {
    ClntObjMgrSetCurrent(m_saveObjMgr);
  }
  m_saveObjMgr = 0;
}

void NetClient::PollEventQueue() {
  m_netEventQueue->Poll();
}

unsigned int NetClient::GetAddr() {
  NETADDR addr;

  if (!m_serverConnection) {
    return 0;
  }

  m_serverConnection->GetLocal(addr);
  return WowConnection::GetAddr(addr);
}
