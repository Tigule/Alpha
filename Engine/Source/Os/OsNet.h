#ifndef ENGINE_SOURCE_OS_OSNET_H
#define ENGINE_SOURCE_OS_OSNET_H

#include <windows.h>

union NETADDR {
  BYTE data[16];
  struct {
    DWORDLONG pos;
    LPVOID    operationId;
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

typedef int(__stdcall *NETEVENTPROC)(HNETCONN__ *, const NETCONNADDR *, NETNOTE, LPVOID, LPCVOID, DWORD, DWORD *);
typedef void(__stdcall *NETHOSTADDRPROC)(const NETADDR *, DWORD, LPVOID);

int         OsNetInitialize(DWORD hints, DWORD parts);
void        OsNetDestroy(DWORD parts);
void        OsNetPump(DWORD timeout);
HNETCONN__ *OsNetConnCopyHandle(HNETCONN__ *conn);
void        OsNetConnFreeHandle(HNETCONN__ *conn);
void        OsNetConnAddr(HNETCONN__ *conn, NETCONNADDR *connAddr);
void        OsNetConnClose(HNETCONN__ *conn);
int         OsNetConnIsClosed(HNETCONN__ *conn);
void        OsNetConnSetEventProc(HNETCONN__ *conn, NETEVENTPROC eventProc);
void        OsNetConnSetUser(HNETCONN__ *conn, LPVOID user);
void        OsNetConnSetEventProcAndUser(HNETCONN__ *conn, NETEVENTPROC eventProc, LPVOID user);
void        OsLoopConnect(NETEVENTPROC eventProcSrc, NETEVENTPROC eventProcDst, LPVOID user, LPCVOID data, DWORD bytes);
void        OsLoopConnSend(HNETCONN__ *conn, LPCVOID data, DWORD bytes);
int         OsTcpListen(WORD port, NETEVENTPROC eventProc, LPVOID user);
void        OsTcpListenEnable(WORD port, int enable);
DWORD       OsNetGetHostAddr(LPCSTR hostName);
int         OsNetGetHostAddrs(LPCSTR hostNameList, WORD defaultPort, NETHOSTADDRPROC hostAddrProc, LPVOID user);
void        OsTcpConnect(DWORD nodeNumber, WORD port, NETEVENTPROC eventProc, LPVOID user, LPCVOID data, DWORD bytes);
void        OsTcpConnSend(HNETCONN__ *conn, LPCVOID data, DWORD bytes);
OS_SEND     OsTcpConnSendSync(HNETCONN__ *conn, LPCVOID data, DWORD bytes, DWORD *bytesSent, DWORD timeout);
void        OsTcpConnSetNagle(HNETCONN__ *conn, int enable);
int         OsTcpConnSetWindow(HNETCONN__ *conn, DWORD size);
void        OsTcpConnSetRecvTimeout(HNETCONN__ *conn, DWORD timeoutMs);
DWORD       OsTcpAddrLoop();
void        OsUdpConnect(const NETADDR *addr, WORD portMin, WORD portMax, NETEVENTPROC eventProc, LPVOID user);
void        OsUdpConnSendTo(HNETCONN__ *conn, LPCVOID data, DWORD bytes, DWORD addrCount, const NETADDR *addrArray);
void        OsFileConnCreate(LPCSTR fileName, NETEVENTPROC eventProc, LPVOID user, int readOnly);
int         OsFileConnRead(HNETCONN__ *conn, DWORDLONG pos, LPVOID buffer, DWORD bytes, LPVOID operationId);
int         OsFileConnWrite(HNETCONN__ *conn, DWORDLONG pos, LPCVOID data, DWORD bytes, LPVOID operationId);
void        OsFileConnClose(HNETCONN__ *conn);
void        OsNetAddrMake(DWORD nodeNumber, WORD port, NETADDR *netAddr);
void        OsNetAddrMakeBroadcast(WORD port, NETADDR *netAddr);
void        OsNetAddrMakeFromStr(LPCSTR addrStr, WORD port, NETADDR *netAddr);
int         OsNetAddrLoopback(const NETADDR *netAddr1, const NETADDR *netAddr2);
int         OsNetAddrCompare(const NETADDR *netAddr1, const NETADDR *netAddr2, NETADDRDIFF *difflevel);
DWORD       OsNetAddrGetAddress(const NETADDR *netAddr, WORD *port);
LPCSTR      OsNetAddrToStr(const NETADDR *netAddr, char *string, DWORD length);
DWORD       OsNetAddrToHostOrder(DWORD nodeNumber);

#endif
