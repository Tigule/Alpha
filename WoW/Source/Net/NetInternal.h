#ifndef WOW_SOURCE_NET_NETINTERNAL_H
#define WOW_SOURCE_NET_NETINTERNAL_H

#include <stpl.h>

#include "Event/EvtApi.h"

class NetClient;

NODEDECL(NETEVENTQUEUENODE) {
  ~NETEVENTQUEUENODE() {
    FREEIFUSED(m_data);
  }

  EVENTID m_eventId;
  DWORD   m_timeReceived;
  LPVOID  m_data;
  DWORD   m_dataSize;
};

class NETEVENTQUEUE {
  friend class NetClient;

 private:
  NETEVENTQUEUE(const NETEVENTQUEUE &queue);
  NETEVENTQUEUE &operator=(const NETEVENTQUEUE &queue);

 public:
  NETEVENTQUEUE(NetClient *client);
  ~NETEVENTQUEUE();

  void AddEvent(EVENTID eventId, LPVOID conn, NetClient *client, LPCVOID data, DWORD bytes);
  void Poll();

 private:
  NetClient *m_client;
  SCritSect  m_critsect;
  LISTDECL(NETEVENTQUEUENODE, m_eventQueue);
};

#endif
