#ifndef WOW_SOURCE_WOWSERVICES_WOWCONNECTIONNET_H
#define WOW_SOURCE_WOWSERVICES_WOWCONNECTIONNET_H

#include "WowConnection.h"

class WowConnectionNet {
 public:
  struct Worker {
    Worker() : event(0, 0) {
    }

    WowConnectionNet *owner;
    SThread           thread;
    int               id;
    WowConnection    *serviceConn;
    SEvent            event;
    unsigned char     quit;
    SCritSect         lock;
  };

  WowConnectionNet(int numThreads, void(__fastcall *threadinit)());
  ~WowConnectionNet();

  void Start();
  void Stop();
  void RunWorker(int id);
  void Run();
  void Service(WowConnection *conn);
  void SignalWorker(WowConnection *conn, unsigned int flags);
  void Delete(WowConnection *conn);

  void PlatformInit(bool useEngine);
  void PlatformDestroy();
  void PlatformAdd(WowConnection *conn);
  void PlatformRemove(WowConnection *conn);
  void PlatformChangeState(WowConnection *conn, WOW_CONN_STATE oldState);
  void PlatformDestruct(WowConnection *conn);
  void PlatformRun();
  void PlatformWorkerReady();

 protected:
  void Add(WowConnection *conn);
  void Remove(WowConnection *conn);

 private:
  friend class WowConnection;

  SThread                            m_thread;
  SEvent                             m_stopEvent;
  unsigned char                      m_stop;
  int                                m_numWorkers;
  Worker                             m_workers[8];
  TSExplicitList<WowConnection, 188> m_connections;
  SCritSect                          m_connectionsLock;
  SSemaphore                         m_workerSem;
  void(__fastcall *m_threadinit)();
  void *m_connectionsChangedEvent;
};

#endif
