#ifndef ENGINE_SOURCE_OS_OSNET_H
#define ENGINE_SOURCE_OS_OSNET_H

#include <windows.h>

union NETADDR {
  unsigned char data[16];
  struct {
    unsigned __int64 pos;
    void            *operationId;
  } file;
};

struct NETCONNADDR {
  NETADDR peerAddr;
  NETADDR selfAddr;
};

struct HNETCONN__ {
  int unused;
};

enum NETNOTE {
  NETNOTE_CONNECT = 0,
  NETNOTE_DATA = 1,
  NETNOTE_DISCONNECT = 2,
  NETNOTE_CANTCONNECT = 3,
  NETNOTE_FILEWRITE = 4,
  NETNOTE_FILEREAD = 5
};

enum OS_SEND {
  OS_SEND_OK = 0,
  OS_SEND_SOCKET_CLOSED = 1,
  OS_SEND_TIMEOUT = 2,
  OS_SEND_ERROR = 3
};

enum NETADDRDIFF {
  NETADDR_DIFF_EQUAL = 0,
  NETADDR_DIFF_NODE = 1,
  NETADDR_DIFF_NETWORK = 2,
  NETADDR_DIFF_PORT = 3,
  NETADDR_DIFF_PROTOCOL = 4
};

typedef int(__stdcall *NETEVENTPROC)(HNETCONN__ *, const NETCONNADDR *, NETNOTE, void *, const void *, unsigned long, unsigned long *);
typedef void(__stdcall *NETHOSTADDRPROC)(const NETADDR *, unsigned long, void *);

int OsNetInitialize(unsigned long hints, unsigned long parts);
void OsNetDestroy(unsigned long parts);
void OsNetPump(unsigned long timeout);
HNETCONN__ *OsNetConnCopyHandle(HNETCONN__ *conn);
void OsNetConnFreeHandle(HNETCONN__ *conn);
void OsNetConnAddr(HNETCONN__ *conn, NETCONNADDR *connAddr);
void OsNetConnClose(HNETCONN__ *conn);
int OsNetConnIsClosed(HNETCONN__ *conn);
void OsNetConnSetEventProc(HNETCONN__ *conn, NETEVENTPROC eventProc);
void OsNetConnSetUser(HNETCONN__ *conn, void *user);
void OsNetConnSetEventProcAndUser(HNETCONN__ *conn, NETEVENTPROC eventProc, void *user);
void OsLoopConnect(NETEVENTPROC eventProcSrc, NETEVENTPROC eventProcDst, void *user, const void *data, unsigned long bytes);
void OsLoopConnSend(HNETCONN__ *conn, const void *data, unsigned long bytes);
int OsTcpListen(unsigned short port, NETEVENTPROC eventProc, void *user);
void OsTcpListenEnable(unsigned short port, int enable);
unsigned long OsNetGetHostAddr(const char *hostName);
int OsNetGetHostAddrs(const char *hostNameList, unsigned short defaultPort, NETHOSTADDRPROC hostAddrProc, void *user);
void
OsTcpConnect(unsigned long nodeNumber, unsigned short port, NETEVENTPROC eventProc, void *user, const void *data, unsigned long bytes);
void OsTcpConnSend(HNETCONN__ *conn, const void *data, unsigned long bytes);
OS_SEND OsTcpConnSendSync(HNETCONN__ *conn, const void *data, unsigned long bytes, unsigned long *bytesSent, unsigned long timeout);
void OsTcpConnSetNagle(HNETCONN__ *conn, int enable);
int OsTcpConnSetWindow(HNETCONN__ *conn, unsigned long size);
void OsTcpConnSetRecvTimeout(HNETCONN__ *conn, unsigned long timeoutMs);
unsigned long OsTcpAddrLoop();
void OsUdpConnect(const NETADDR *addr, unsigned short portMin, unsigned short portMax, NETEVENTPROC eventProc, void *user);
void OsUdpConnSendTo(HNETCONN__ *conn, const void *data, unsigned long bytes, unsigned long addrCount, const NETADDR *addrArray);
void OsFileConnCreate(const char *fileName, NETEVENTPROC eventProc, void *user, int readOnly);
int OsFileConnRead(HNETCONN__ *conn, unsigned __int64 pos, void *buffer, unsigned long bytes, void *operationId);
int OsFileConnWrite(HNETCONN__ *conn, unsigned __int64 pos, const void *data, unsigned long bytes, void *operationId);
void OsFileConnClose(HNETCONN__ *conn);
void OsNetAddrMake(unsigned long nodeNumber, unsigned short port, NETADDR *netAddr);
void OsNetAddrMakeBroadcast(unsigned short port, NETADDR *netAddr);
void OsNetAddrMakeFromStr(const char *addrStr, unsigned short port, NETADDR *netAddr);
int OsNetAddrLoopback(const NETADDR *netAddr1, const NETADDR *netAddr2);
int OsNetAddrCompare(const NETADDR *netAddr1, const NETADDR *netAddr2, NETADDRDIFF *difflevel);
unsigned long OsNetAddrGetAddress(const NETADDR *netAddr, unsigned short *port);
const char *OsNetAddrToStr(const NETADDR *netAddr, char *string, unsigned long length);
unsigned long OsNetAddrToHostOrder(unsigned long nodeNumber);

#endif
