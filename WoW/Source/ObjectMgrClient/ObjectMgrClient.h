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
class ClntObjMgr;

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
  DWORDLONG m_guid;

  CHashKeyGUID(int guid);

 public:
  CHashKeyGUID() : m_guid(0) {
  }

 private:
  CHashKeyGUID(const CHashKeyGUID &key) : m_guid(key.m_guid) {
  }

 public:
  CHashKeyGUID(DWORDLONG guid) : m_guid(guid) {
  }
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

enum HANDLER_PRIORITY {
  HANDLER_PRIORITY_NORMAL = 0,
  HANDLER_PRIORITY_HIGH = 1
};

enum PLAYER_TYPE {
  PLAYER_NORMAL = 0,
  PLAYER_BOT = 1
};

ClntObjMgr       *ClntObjMgrGetCurrent();
ClntObjMgr       *ClntObjMgrCreate(PLAYER_TYPE type, LPVOID clientPtr);
void              ClntObjMgrSetCurrent(ClntObjMgr *mgr);
BOOL              ClntObjMgrIsValid(int forWriting);
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
BOOL              ClntObjMgrEnumVisibleObjects(BOOL (*handler)(DWORDLONG object, LPVOID param), LPVOID param);
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
    BOOL (*handler)(DWORDLONG, UINT, UINT, LPCVOID, LPVOID),
    LPVOID           param,
    HANDLER_PRIORITY priority
);
void ClntObjMgrUnsetObjMirrorHandler(DWORDLONG guid, UINT offset, BOOL (*handler)(DWORDLONG, UINT, UINT, LPCVOID, LPVOID), LPVOID param);
void ClntObjMgrSetTypeMirrorHandler(
    OBJECT_TYPE hierType,
    UINT        offset,
    UINT        bytes,
    BOOL (*handler)(DWORDLONG, UINT, UINT, LPCVOID, LPVOID),
    LPVOID           param,
    HANDLER_PRIORITY priority
);
void ClntObjMgrUnsetTypeMirrorHandler(OBJECT_TYPE hierType, UINT offset, BOOL (*handler)(DWORDLONG, UINT, UINT, LPCVOID, LPVOID));

#endif
