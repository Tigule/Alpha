#include <Base/Base.h>

#include "WowConnectionNet.h"

#include "Console/ConsoleClient.h"

WowConnectionNet::WowConnectionNet(int numThreads, void (*threadinit)()) : m_stopEvent(0, 0), m_workerSem(0, numThreads) {
  m_numWorkers = numThreads;
  m_threadinit = threadinit;
  m_stop = 0;
}

void WowConnectionNet::Add(WowConnection *conn) {
  m_connectionsLock.Enter();
  ASSERT(!m_connections.IsLinked(conn));
  m_connections.LinkNode(conn, LIST_LINK_BEFORE, 0);
  PlatformAdd(conn);
  m_connectionsLock.Leave();
}

WowConnectionNet::~WowConnectionNet() {
}

void WowConnectionNet::Remove(WowConnection *conn) {
  m_connectionsLock.Enter();

  if (m_connections.IsLinked(conn)) {
    m_connections.UnlinkNode(conn);
  }

  PlatformRemove(conn);
  m_connectionsLock.Leave();
}

static UINT __stdcall WorkerProc(LPVOID param) {
  WowConnectionNet::Worker *worker = static_cast<WowConnectionNet::Worker *>(param);
  worker->owner->RunWorker(worker->id);
  return 0;
}

static UINT __stdcall MainProc(LPVOID param) {
  static_cast<WowConnectionNet *>(param)->Run();
  return 0;
}

void WowConnectionNet::Start() {
  char name[32];
  int  id;

  for (id = 0; id < m_numWorkers; ++id) {
    Worker *worker = &m_workers[id];
    worker->id = id;
    worker->serviceConn = 0;
    worker->quit = 0;
    worker->owner = this;
    SStrPrintf(name, sizeof(name), "Net Thread %d", id);
    SThread::Create(WorkerProc, worker, worker->thread, name);
  }

  for (; id < 8; ++id) {
    Worker *worker = &m_workers[id];
    worker->id = 0;
    worker->serviceConn = 0;
    worker->quit = 1;
    worker->owner = this;
  }

  SStrPrintf(name, sizeof(name), "Network");
  SThread::Create(MainProc, this, m_thread, name);
}

void WowConnectionNet::Stop() {
  int id;

  m_stop = 1;
  PlatformWorkerReady();
  if (m_stopEvent.Wait(10000)) {
    ConsolePrintf("Main network thread did not exit normally");
  }

  for (id = 0; id < m_numWorkers; ++id) {
    Worker *worker = &m_workers[id];
    worker->quit = 1;
    worker->event.Set();
    if (worker->thread.Wait(10000)) {
      ConsolePrintf("Network thread %d did not exit normally", id);
    }
  }
}

void WowConnectionNet::Service(WowConnection *conn) {
  if (conn->m_serviceFlags & 1) {
    conn->DoWrites();
  }
  if (conn->m_serviceFlags & 2) {
    conn->DoReads();
  }
  if (conn->m_serviceFlags & 4) {
    conn->DoExceptions();
  }
  if (conn->m_serviceFlags & 8) {
    conn->DoDisconnect();
  }

  conn->m_serviceFlags = 0;
}

void WowConnectionNet::RunWorker(int id) {
  if (m_threadinit) {
    m_threadinit();
  }

  m_workerSem.Signal(1);

  while (1) {
    while (m_workers[id].event.Wait(1000)) {
    }

    if (m_workers[id].quit) {
      return;
    }

    ASSERT(m_workers[id].serviceConn);
    Service(m_workers[id].serviceConn);
    ASSERT(m_workers[id].serviceConn->m_serviceCount == 1);

    m_workers[id].lock.Enter();
    SInterlockedDecrement(&m_workers[id].serviceConn->m_serviceCount);
    m_workers[id].serviceConn->Release();
    m_workers[id].serviceConn = 0;
    m_workers[id].lock.Leave();

    m_workerSem.Signal(1);
    PlatformWorkerReady();
  }
}

void WowConnectionNet::SignalWorker(WowConnection *conn, UINT flags) {
  if (m_workerSem.Wait(500)) {
    ConsolePrintf("Worker wait timed out");
    return;
  }

  conn->AddRef();
  conn->m_serviceFlags = flags;
  ASSERT(conn->m_serviceCount == 0);
  SInterlockedIncrement(&conn->m_serviceCount);

  int i;
  for (i = 0; i < m_numWorkers; ++i) {
    m_workers[i].lock.Enter();

    if (!m_workers[i].serviceConn) {
      m_workers[i].serviceConn = conn;
      m_workers[i].lock.Leave();
      m_workers[i].event.Set();
      break;
    }

    ASSERT(m_workers[i].serviceConn != conn);
    m_workers[i].lock.Leave();
  }

  ASSERT(i < m_numWorkers);
}

void WowConnectionNet::Run() {
  PlatformRun();
  m_stopEvent.Set();
}

void WowConnectionNet::Delete(WowConnection *conn) {
  m_connectionsLock.Enter();

  if (!conn->m_refCount) {
    DEL(conn);
  }

  m_connectionsLock.Leave();
}
