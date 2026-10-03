#ifndef WOW_SOURCE_NET_NETINTERNAL_H
#define WOW_SOURCE_NET_NETINTERNAL_H

#include <stpl.h>

#include "Event/EvtApi.h"

class NetClient;

NODEDECL(NETEVENTQUEUENODE) {
  EVENTID m_eventId;
  DWORD   m_timeReceived;
  LPVOID  m_data;
  DWORD   m_dataSize;

  ~NETEVENTQUEUENODE() {
    FREEIFUSED(m_data);
  }
};

class NETEVENTQUEUE {
  friend class NetClient;

  NetClient *m_client;
  SCritSect  m_critsect;
  LISTDECL(NETEVENTQUEUENODE, m_eventQueue);

  NETEVENTQUEUE(const NETEVENTQUEUE &queue);

 public:
  NETEVENTQUEUE(NetClient *client);

 private:
  NETEVENTQUEUE &operator=(const NETEVENTQUEUE &queue);

 public:
  ~NETEVENTQUEUE();

  void AddEvent(EVENTID eventId, LPVOID conn, NetClient *client, LPCVOID data, DWORD bytes);
  void Poll();
};

#endif
