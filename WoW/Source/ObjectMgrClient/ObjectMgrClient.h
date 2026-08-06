#ifndef WOW_SOURCE_OBJECTMGRCLIENT_OBJECTMGRCLIENT_H
#define WOW_SOURCE_OBJECTMGRCLIENT_OBJECTMGRCLIENT_H

#include "Object/Object.h"

#include <stpl.h>

class CGObject_C;
class CGWorldFrame;
class ClientConnection;
class NameCache;
struct FADEOUTHASHOBJ;
struct ITEMEXPIRATION;
struct NAMEPLATEDESC;
struct PLAYERPORTRAIT;
struct UNITHASHOBJ;
struct UNITONESHOTEFFECTDESC;
struct C_OBJECTHASH;
struct CMirrorHandler;

template <class RECORD, class KEY, class HASHKEY>
class DBCache;

static C_OBJECTHASH *FindActiveObj(DWORDLONG guid);

class CHashKeyGUID {
  friend class TSHashObject<C_OBJECTHASH, CHashKeyGUID>;
  friend class TSHashObject<FADEOUTHASHOBJ, CHashKeyGUID>;
  friend class TSHashObject<ITEMEXPIRATION, CHashKeyGUID>;
  friend class TSHashObject<NAMEPLATEDESC, CHashKeyGUID>;
  friend class TSHashObject<PLAYERPORTRAIT, CHashKeyGUID>;
  friend class TSHashObject<UNITHASHOBJ, CHashKeyGUID>;
  friend class TSHashObject<UNITONESHOTEFFECTDESC, CHashKeyGUID>;
  friend class CGWorldFrame;
  friend C_OBJECTHASH *FindActiveObj(DWORDLONG guid);
  friend class DBCache<NameCache, DWORDLONG, CHashKeyGUID>;

 private:
  CHashKeyGUID(int guid);

 public:
  CHashKeyGUID(DWORDLONG guid) : m_guid(guid) {
  }

 private:
  CHashKeyGUID(const CHashKeyGUID &key) : m_guid(key.m_guid) {
  }

 public:
  CHashKeyGUID() : m_guid(0) {
  }

 private:
  DWORDLONG m_guid;

 public:
  CHashKeyGUID &operator=(const CHashKeyGUID &key) {
    m_guid = key.m_guid;
    return *this;
  }
  BYTE operator==(const CHashKeyGUID &key) const {
    return m_guid == key.m_guid;
  }
  DWORDLONG GetGUID() const {
    return m_guid;
  }
};

struct C_OBJECTHASH : public TSHashObject<C_OBJECTHASH, CHashKeyGUID> {
  C_OBJECTHASH(const C_OBJECTHASH &object);
  C_OBJECTHASH();

  UINT memHandle;
  UINT thisMemHandle;
  LISTDECL(CMirrorHandler, mirrorHandlers[634]);
  LINKDECLEX(C_OBJECTHASH, link);
  LINKDECLEX(C_OBJECTHASH, reenableLink);
};

enum HANDLER_PRIORITY {
  HANDLER_PRIORITY_NORMAL = 0,
  HANDLER_PRIORITY_HIGH = 1
};

NODEDECL(CMirrorHandler) {
  LINKDECLEX(CMirrorHandler, callLink);
  int (*handler)(DWORDLONG guid, UINT offset, UINT bytes, LPCVOID data, LPVOID param);
  LPVOID                             param;
  UINT                               blocksLeft;
  UINT                               offset;
  TSGrowableArray_<BYTE, 'OMGR', 71> previous;
  HANDLER_PRIORITY                   priority;
};

inline C_OBJECTHASH::C_OBJECTHASH() : memHandle(0) {
}

NODEDECL(OBJHANDLERREQUEST) {
  DWORDLONG guid;
  UINT      offset;
  UINT      bytes;
  int (*handler)(DWORDLONG guid, UINT offset, UINT bytes, LPCVOID data, LPVOID param);
  LPVOID           param;
  HANDLER_PRIORITY priority;
  BYTE             set;
};

enum PLAYER_TYPE {
  PLAYER_NORMAL = 0,
  PLAYER_BOT = 1
};

class ClntObjMgr {
 public:
  ClntObjMgr(const ClntObjMgr &mgr);
  ClntObjMgr(PLAYER_TYPE type, LPVOID clientPtr)
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

  TSHashTable<C_OBJECTHASH, CHashKeyGUID> m_objects;
  TSHashTable<C_OBJECTHASH, CHashKeyGUID> m_lazyCleanupObjects;
  LISTDECLEX(C_OBJECTHASH, link, m_lazyCleanupFifo);
  LISTDECLEX(C_OBJECTHASH, link, m_freeObjects);
  LISTDECLEX(C_OBJECTHASH, link, m_visibleObjects);
  LISTDECLEX(C_OBJECTHASH, reenableLink, m_reenabledObjects);
  int m_callingMirrorHandlers;
  LISTDECL(OBJHANDLERREQUEST, m_pendingObjHandlerRequests);
  int               m_allowGuidDeref;
  DWORDLONG         m_legalGuidDeref;
  DWORDLONG         m_activePlayer;
  PLAYER_TYPE       m_type;
  UINT              m_mapID;
  ClientConnection *m_net;
  LPVOID            m_movement;
  LPVOID            m_clientPtr;
};

ClntObjMgr       *ClntObjMgrGetCurrent();
ClntObjMgr       *ClntObjMgrCreate(PLAYER_TYPE type, LPVOID clientPtr);
void              ClntObjMgrSetCurrent(ClntObjMgr *mgr);
int               ClntObjMgrIsValid(int forWriting);
void              ClntObjMgrInitializeShared();
void              ClntObjMgrInitialize();
void              ClntObjMgrDestroy();
DWORDLONG         ClntObjMgrGetActivePlayer();
void              ClntObjMgrSetActivePlayer(DWORDLONG guid);
PLAYER_TYPE       ClntObjMgrGetPlayerType();
LPVOID            ClntObjMgrGetMovementGlobals();
void              ClntObjMgrSetMovementGlobals(LPVOID ptr);
CGObject_C       *ClntObjMgrObjectPtr(DWORDLONG guid, LPCSTR fileName, UINT lineNumber);
UINT              ClntObjMgrGetMapID();
void              ClntObjMgrSetMapID(UINT mapID);
int               ClntObjMgrEnumVisibleObjects(int (*handler)(DWORDLONG object, LPVOID param), LPVOID param);
void              ClntObjMgrObjectInRange(DWORDLONG guid);
void              ClntObjMgrHideObject(DWORDLONG guid);
void              ClntObjMgrObjectOutOfRange(DWORDLONG guid, int shutdown);
void              ClntObjMgrFreeObject(DWORDLONG guid);
void              ClntObjMgrSetNet(ClientConnection *net);
ClientConnection *ClntObjMgrGetNet();
LPVOID            ClntObjMgrGetClientPtr();
void              ClntObjMgrDestruct(ClntObjMgr *mgr);
void              ClntObjMgrDestroyShared();
void              ClntObjMgrSetObjMirrorHandler(
    DWORDLONG guid,
    UINT      offset,
    UINT      bytes,
    int (*handler)(DWORDLONG, UINT, UINT, LPCVOID, LPVOID),
    LPVOID           param,
    HANDLER_PRIORITY priority
);
void ClntObjMgrUnsetObjMirrorHandler(DWORDLONG guid, UINT offset, int (*handler)(DWORDLONG, UINT, UINT, LPCVOID, LPVOID), LPVOID param);
void ClntObjMgrSetTypeMirrorHandler(
    OBJECT_TYPE hierType,
    UINT        offset,
    UINT        bytes,
    int (*handler)(DWORDLONG, UINT, UINT, LPCVOID, LPVOID),
    LPVOID           param,
    HANDLER_PRIORITY priority
);
void ClntObjMgrUnsetTypeMirrorHandler(OBJECT_TYPE hierType, UINT offset, int (*handler)(DWORDLONG, UINT, UINT, LPCVOID, LPVOID));

#endif
