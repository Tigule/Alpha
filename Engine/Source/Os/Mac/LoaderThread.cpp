#include <Base/Base.h>

#include <storm.h>
#include <SThread.h>

#include "Os/Mac/GlobalAUGraph.h"
#include "Os/Mac/LoaderThread.h"
#include "Os/OsTime.h"

struct LoaderThread {
  SThread                m_thread;
  SEvent                 m_event;
  StreamPlayer *volatile m_head;
  volatile bool          m_running;

  static UINT WorkThread(LPVOID param);
};

static LoaderThread s_loaderThread;

void LoaderThreadInitialize() {
  s_loaderThread.m_running = true;
  BOOL success = SThread::Create(LoaderThread::WorkThread, &s_loaderThread, s_loaderThread.m_thread, 0);
  ASSERT(success);
}

void LoaderThreadCleanUp() {
  s_loaderThread.m_running = false;
  s_loaderThread.m_event.Set();
  s_loaderThread.m_thread.Wait(0xFFFFFFFF);
}

StreamPlayer::StreamPlayer()
    : m_state(e_inactive) {
  do {
    m_next = s_loaderThread.m_head;
  } while (SInterlockedCompareExchangePointer((LPVOID *)&s_loaderThread.m_head, this, m_next) != m_next);
}

StreamPlayer::~StreamPlayer() {
  ASSERTVALUE(m_state != e_active, m_state);
}

void StreamPlayer::SetStreamState(StreamState state) {
  SInterlockedExchange((LONG *)&m_state, state);
  if (state == e_active) {
    s_loaderThread.m_event.Set();
  }
}

UINT LoaderThread::WorkThread(LPVOID param) {
  LoaderThread *loader = (LoaderThread *)param;

  do {
    DWORD deadline = OsGetAsyncTimeMsPrecise() + 100;

    StreamPlayer *prev = 0;
    StreamPlayer *player = loader->m_head;
    while (player) {
      StreamPlayer *next = player->m_next;
      if (player->m_state == StreamPlayer::e_dead) {
        if (prev || SInterlockedCompareExchangePointer((LPVOID *)&loader->m_head, next, player) != player) {
          if (!prev) {
            prev = loader->m_head;
            while (prev->m_next != player) {
              prev = prev->m_next;
            }
          }
          prev->m_next = next;
        }
        delete player;
      } else {
        if (player->m_state == StreamPlayer::e_active) {
          player->ReadAhead();
        }
        prev = player;
      }
      player = next;
    }

    LONG remaining = (LONG)(deadline - OsGetAsyncTimeMsPrecise());
    if (remaining >= 0) {
      loader->m_event.Wait(remaining);
    }
  } while (loader->m_running);

  return 0;
}
