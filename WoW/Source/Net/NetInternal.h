#ifndef WOW_SOURCE_NET_NETINTERNAL_H
#define WOW_SOURCE_NET_NETINTERNAL_H

#include <stpl.h>

#include "Event/EvtApi.h"

class NetClient;

struct NETEVENTQUEUENODE : public TSLinkedNode<NETEVENTQUEUENODE> {
  ~NETEVENTQUEUENODE() {
    FREEIFUSED(m_data);
  }

  EVENTID       m_eventId;
  unsigned long m_timeReceived;
  void         *m_data;
  unsigned long m_dataSize;
};

class NETEVENTQUEUE {
  friend class NetClient;

 private:
  NETEVENTQUEUE(const NETEVENTQUEUE &queue);
  NETEVENTQUEUE &operator=(const NETEVENTQUEUE &queue);

 public:
  NETEVENTQUEUE(NetClient *client);
  ~NETEVENTQUEUE();

  void AddEvent(EVENTID eventId, void *conn, NetClient *client, const void *data, unsigned long bytes);
  void Poll();

 private:
  NetClient                                               *m_client;
  SCritSect                                                m_critsect;
  TSList<NETEVENTQUEUENODE, TSGetLink<NETEVENTQUEUENODE> > m_eventQueue;
};

#endif
