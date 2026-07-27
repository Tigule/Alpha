#include "NetInternal.h"

#include "Os/OsTime.h"

#include <string.h>

#include "NetClient/NetClient.h"

void NETEVENTQUEUE::Poll() {
  m_critsect.Enter();

  unsigned char deleted = 0;
  m_client->AddRef();

  ITERATELIST(NETEVENTQUEUENODE, m_eventQueue, event) {
    if (!m_client->GetDelete()) {
      switch (event->m_eventId) {
        case EVENT_ID_NET_DATA:
          m_client->HandleData(event->m_timeReceived, event->m_data, event->m_dataSize);
          break;

        case EVENT_ID_NET_CONNECT:
          m_client->HandleConnect();
          break;

        case EVENT_ID_NET_DISCONNECT:
          m_client->HandleDisconnect();
          break;

        case EVENT_ID_NET_CANTCONNECT:
          m_client->HandleCantConnect();
          break;

        case EVENT_ID_NET_DESTROY:
          m_client->SetDelete();
          deleted = 1;
          break;
      }
    }

    m_client->DelRef();
  }

  if (!deleted) {
    m_client->HandleIdle();
  }

  m_client->DelRef();

  m_eventQueue.Clear();

  m_critsect.Leave();
}

NETEVENTQUEUE::NETEVENTQUEUE(NetClient *client) {
  m_client = client;
}

NETEVENTQUEUE::~NETEVENTQUEUE() {
  m_critsect.Enter();

  m_eventQueue.Clear();

  m_critsect.Leave();
}

void NETEVENTQUEUE::AddEvent(EVENTID eventId, void *conn, NetClient *client, const void *data, unsigned long bytes) {
  m_critsect.Enter();

  NETEVENTQUEUENODE *event = m_eventQueue.NewNode(LIST_TAIL, 0, 0);
  event->m_eventId = eventId;

  if (bytes) {
    event->m_data = ALLOC(bytes);
    memcpy(event->m_data, data, bytes);
    event->m_dataSize = bytes;
  } else {
    event->m_data = 0;
    event->m_dataSize = 0;
  }

  event->m_timeReceived = OsGetAsyncTimeMs();
  client->AddRef();

  m_critsect.Leave();
}
