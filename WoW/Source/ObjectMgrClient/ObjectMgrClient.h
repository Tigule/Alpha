#ifndef WOW_SOURCE_OBJECTMGRCLIENT_OBJECTMGRCLIENT_H
#define WOW_SOURCE_OBJECTMGRCLIENT_OBJECTMGRCLIENT_H

#include "Object/Object.h"

#include <stpl.h>

class CGObject_C;
class ClientConnection;
class NameCache;
struct C_OBJECTHASH;
struct CMirrorHandler;

template <class RECORD, class KEY, class HASHKEY>
class DBCache;

static C_OBJECTHASH *__fastcall FindActiveObj(unsigned __int64 guid);

class CHashKeyGUID {
  friend class TSHashObject<C_OBJECTHASH, CHashKeyGUID>;
  friend C_OBJECTHASH *__fastcall FindActiveObj(unsigned __int64 guid);
  friend class DBCache<NameCache, unsigned __int64, CHashKeyGUID>;

 public:
  CHashKeyGUID(unsigned __int64 guid) : m_guid(guid) {
  }
  CHashKeyGUID(const CHashKeyGUID &key) : m_guid(key.m_guid) {
  }

  CHashKeyGUID() : m_guid(0) {
  }

 private:
  CHashKeyGUID(int guid);

  unsigned __int64 m_guid;

 public:
  CHashKeyGUID &operator=(const CHashKeyGUID &key) {
    m_guid = key.m_guid;
    return *this;
  }
  unsigned char operator==(const CHashKeyGUID &key) {
    return m_guid == key.m_guid;
  }
  unsigned __int64 GetGUID() {
    return m_guid;
  }
};

struct C_OBJECTHASH : public TSHashObject<C_OBJECTHASH, CHashKeyGUID> {
  C_OBJECTHASH(const C_OBJECTHASH &object);
  C_OBJECTHASH();
  ~C_OBJECTHASH();

  C_OBJECTHASH &operator=(const C_OBJECTHASH &object);

  unsigned int                                       memHandle;
  unsigned int                                       thisMemHandle;
  TSList<CMirrorHandler, TSGetLink<CMirrorHandler> > mirrorHandlers[634];
  TSLink<C_OBJECTHASH>                               link;
  TSLink<C_OBJECTHASH>                               reenableLink;
};

enum HANDLER_PRIORITY {
  HANDLER_PRIORITY_NORMAL = 0,
  HANDLER_PRIORITY_HIGH = 1
};

struct CMirrorHandler : public TSLinkedNode<CMirrorHandler> {
  TSLink<CMirrorHandler> callLink;
  int(__fastcall *handler)(unsigned __int64 guid, unsigned int offset, unsigned int bytes, const void *data, void *param);
  void                                       *param;
  unsigned int                                blocksLeft;
  unsigned int                                offset;
  TSGrowableArray_<unsigned char, 'OMGR', 71> previous;
  HANDLER_PRIORITY                            priority;

  CMirrorHandler() {
  }
  ~CMirrorHandler();
};

inline C_OBJECTHASH::C_OBJECTHASH() : memHandle(0) {
}

inline C_OBJECTHASH::~C_OBJECTHASH() {
}

struct OBJHANDLERREQUEST : public TSLinkedNode<OBJHANDLERREQUEST> {
  OBJHANDLERREQUEST(const OBJHANDLERREQUEST &request);
  OBJHANDLERREQUEST() {
  }
  ~OBJHANDLERREQUEST() {
  }

  OBJHANDLERREQUEST &operator=(const OBJHANDLERREQUEST &request);

  unsigned __int64 guid;
  unsigned int     offset;
  unsigned int     bytes;
  int(__fastcall *handler)(unsigned __int64 guid, unsigned int offset, unsigned int bytes, const void *data, void *param);
  void            *param;
  HANDLER_PRIORITY priority;
  unsigned char    set;
};

enum PLAYER_TYPE {
  PLAYER_NORMAL = 0,
  PLAYER_BOT = 1
};

class ClntObjMgr {
 public:
  ClntObjMgr(const ClntObjMgr &mgr);
  ClntObjMgr(PLAYER_TYPE type, void *clientPtr)
      : m_callingMirrorHandlers(0),
        m_allowGuidDeref(1),
        m_activePlayer(0),
        m_type(type),
        m_mapID(0),
        m_net(0),
        m_movement(0),
        m_clientPtr(clientPtr) {
  }
  ~ClntObjMgr() {
  }

  ClntObjMgr &operator=(const ClntObjMgr &mgr);

  TSHashTable<C_OBJECTHASH, CHashKeyGUID>                  m_objects;
  TSHashTable<C_OBJECTHASH, CHashKeyGUID>                  m_lazyCleanupObjects;
  TSExplicitList<C_OBJECTHASH, 7648>                       m_lazyCleanupFifo;
  TSExplicitList<C_OBJECTHASH, 7648>                       m_freeObjects;
  TSExplicitList<C_OBJECTHASH, 7648>                       m_visibleObjects;
  TSExplicitList<C_OBJECTHASH, 7656>                       m_reenabledObjects;
  int                                                      m_callingMirrorHandlers;
  TSList<OBJHANDLERREQUEST, TSGetLink<OBJHANDLERREQUEST> > m_pendingObjHandlerRequests;
  int                                                      m_allowGuidDeref;
  unsigned __int64                                         m_legalGuidDeref;
  unsigned __int64                                         m_activePlayer;
  PLAYER_TYPE                                              m_type;
  unsigned int                                             m_mapID;
  ClientConnection                                        *m_net;
  void                                                    *m_movement;
  void                                                    *m_clientPtr;
};

ClntObjMgr *__fastcall       ClntObjMgrGetCurrent();
ClntObjMgr *__fastcall       ClntObjMgrCreate(PLAYER_TYPE type, void *clientPtr);
void __fastcall              ClntObjMgrSetCurrent(ClntObjMgr *mgr);
int __fastcall               ClntObjMgrIsValid(int forWriting);
void __fastcall              ClntObjMgrInitializeShared();
void __fastcall              ClntObjMgrInitialize();
void __fastcall              ClntObjMgrDestroy();
unsigned __int64 __fastcall  ClntObjMgrGetActivePlayer();
void __fastcall              ClntObjMgrSetActivePlayer(unsigned __int64 guid);
PLAYER_TYPE __fastcall       ClntObjMgrGetPlayerType();
void *__fastcall             ClntObjMgrGetMovementGlobals();
void __fastcall              ClntObjMgrSetMovementGlobals(void *ptr);
CGObject_C *__fastcall       ClntObjMgrObjectPtr(unsigned __int64 guid, const char *fileName, unsigned int lineNumber);
unsigned int __fastcall      ClntObjMgrGetMapID();
void __fastcall              ClntObjMgrSetMapID(unsigned int mapID);
int __fastcall               ClntObjMgrEnumVisibleObjects(int(__fastcall *handler)(unsigned __int64 object, void *param), void *param);
void __fastcall              ClntObjMgrObjectInRange(unsigned __int64 guid);
void __fastcall              ClntObjMgrHideObject(unsigned __int64 guid);
void __fastcall              ClntObjMgrObjectOutOfRange(unsigned __int64 guid, int shutdown);
void __fastcall              ClntObjMgrFreeObject(unsigned __int64 guid);
void __fastcall              ClntObjMgrSetNet(ClientConnection *net);
ClientConnection *__fastcall ClntObjMgrGetNet();
void *__fastcall             ClntObjMgrGetClientPtr();
void __fastcall              ClntObjMgrDestruct(ClntObjMgr *mgr);
void __fastcall              ClntObjMgrDestroyShared();
void __fastcall              ClntObjMgrSetObjMirrorHandler(
    unsigned __int64 guid,
    unsigned int     offset,
    unsigned int     bytes,
    int(__fastcall *handler)(unsigned __int64, unsigned int, unsigned int, const void *, void *),
    void            *param,
    HANDLER_PRIORITY priority
);
void __fastcall ClntObjMgrUnsetObjMirrorHandler(
    unsigned __int64 guid,
    unsigned int     offset,
    int(__fastcall *handler)(unsigned __int64, unsigned int, unsigned int, const void *, void *),
    void *param
);
void __fastcall ClntObjMgrSetTypeMirrorHandler(
    OBJECT_TYPE hierType,
    unsigned int offset,
    unsigned int bytes,
    int(__fastcall *handler)(unsigned __int64, unsigned int, unsigned int, const void *, void *),
    void *param,
    HANDLER_PRIORITY priority
);
void __fastcall ClntObjMgrUnsetTypeMirrorHandler(
    OBJECT_TYPE hierType,
    unsigned int offset,
    int(__fastcall *handler)(unsigned __int64, unsigned int, unsigned int, const void *, void *)
);

#endif
