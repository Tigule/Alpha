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

int __fastcall           OsNetInitialize(unsigned long hints, unsigned long parts);
void __fastcall          OsNetDestroy(unsigned long parts);
void __fastcall          OsNetPump(unsigned long timeout);
HNETCONN__ *__fastcall   OsNetConnCopyHandle(HNETCONN__ *conn);
void __fastcall          OsNetConnFreeHandle(HNETCONN__ *conn);
void __fastcall          OsNetConnAddr(HNETCONN__ *conn, NETCONNADDR *connAddr);
void __fastcall          OsNetConnClose(HNETCONN__ *conn);
int __fastcall           OsNetConnIsClosed(HNETCONN__ *conn);
void __fastcall          OsNetConnSetEventProc(HNETCONN__ *conn, NETEVENTPROC eventProc);
void __fastcall          OsNetConnSetUser(HNETCONN__ *conn, void *user);
void __fastcall          OsNetConnSetEventProcAndUser(HNETCONN__ *conn, NETEVENTPROC eventProc, void *user);
void __fastcall          OsLoopConnect(NETEVENTPROC eventProcSrc, NETEVENTPROC eventProcDst, void *user, const void *data, unsigned long bytes);
void __fastcall          OsLoopConnSend(HNETCONN__ *conn, const void *data, unsigned long bytes);
int __fastcall           OsTcpListen(unsigned short port, NETEVENTPROC eventProc, void *user);
void __fastcall          OsTcpListenEnable(unsigned short port, int enable);
unsigned long __fastcall OsNetGetHostAddr(const char *hostName);
int __fastcall           OsNetGetHostAddrs(const char *hostNameList, unsigned short defaultPort, NETHOSTADDRPROC hostAddrProc, void *user);
void __fastcall
OsTcpConnect(unsigned long nodeNumber, unsigned short port, NETEVENTPROC eventProc, void *user, const void *data, unsigned long bytes);
void __fastcall          OsTcpConnSend(HNETCONN__ *conn, const void *data, unsigned long bytes);
OS_SEND __fastcall       OsTcpConnSendSync(HNETCONN__ *conn, const void *data, unsigned long bytes, unsigned long *bytesSent, unsigned long timeout);
void __fastcall          OsTcpConnSetNagle(HNETCONN__ *conn, int enable);
int __fastcall           OsTcpConnSetWindow(HNETCONN__ *conn, unsigned long size);
void __fastcall          OsTcpConnSetRecvTimeout(HNETCONN__ *conn, unsigned long timeoutMs);
unsigned long __fastcall OsTcpAddrLoop();
void __fastcall          OsUdpConnect(const NETADDR *addr, unsigned short portMin, unsigned short portMax, NETEVENTPROC eventProc, void *user);
void __fastcall          OsUdpConnSendTo(HNETCONN__ *conn, const void *data, unsigned long bytes, unsigned long addrCount, const NETADDR *addrArray);
void __fastcall          OsFileConnCreate(const char *fileName, NETEVENTPROC eventProc, void *user, int readOnly);
int __fastcall           OsFileConnRead(HNETCONN__ *conn, unsigned __int64 pos, void *buffer, unsigned long bytes, void *operationId);
int __fastcall           OsFileConnWrite(HNETCONN__ *conn, unsigned __int64 pos, const void *data, unsigned long bytes, void *operationId);
void __fastcall          OsFileConnClose(HNETCONN__ *conn);
void __fastcall          OsNetAddrMake(unsigned long nodeNumber, unsigned short port, NETADDR *netAddr);
void __fastcall          OsNetAddrMakeBroadcast(unsigned short port, NETADDR *netAddr);
void __fastcall          OsNetAddrMakeFromStr(const char *addrStr, unsigned short port, NETADDR *netAddr);
int __fastcall           OsNetAddrLoopback(const NETADDR *netAddr1, const NETADDR *netAddr2);
int __fastcall           OsNetAddrCompare(const NETADDR *netAddr1, const NETADDR *netAddr2, NETADDRDIFF *difflevel);
unsigned long __fastcall OsNetAddrGetAddress(const NETADDR *netAddr, unsigned short *port);
const char *__fastcall   OsNetAddrToStr(const NETADDR *netAddr, char *string, unsigned long length);
unsigned long __fastcall OsNetAddrToHostOrder(unsigned long nodeNumber);

#endif
