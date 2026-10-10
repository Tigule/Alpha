#include <Base/Base.h>
#include <Gx/Gx.h>
#include <MapDefs.h>
#include "WorldClient/World.h"
#include <WowConst.h>

#include "ObjectMgrClient.h"

#include <Base/Activity.h>
#include <Base/CDataStore.h>
#include <Services/SysMessage.h>
#include <malloc.h>
#include <new>
#include <string.h>
#include <storm.h>

#include "Console/ConsoleClient.h"
#include "Console/ConsoleCommand.h"
#include "Object/Object.h"
#include "Object/ObjectClient/Bag_C.h"
#include "Object/ObjectClient/Container_C.h"
#include "Object/ObjectClient/Corpse_C.h"
#include "Object/ObjectClient/DynamicObject_C.h"
#include "Object/ObjectClient/GameObject_C.h"
#include "Object/ObjectClient/Item_C.h"
#include "Object/ObjectClient/Object_C.h"
#include "Object/ObjectClient/Player_C.h"
#include "Object/ObjectClient/Unit_C.h"
#include "SoundInterface/SoundInterface.h"
#include "UIUtil/Camera.h"
#include "UIUtil/InputControl.h"
#include "Ui/GameUI.h"
#include "Object/mirror.h"
#include "ObjectAlloc/ObjectAllocTemplate.h"
#include "WowServices/WDataStore.h"
#include "WowSvcs/WowSvcsClient/ClientConnection.h"
#include "WowSvcs/WowSvcsClient/ClientServices.h"
#include "Ui/PartyFrame.h"

struct C_OBJECTHASH : public TSHashObject<C_OBJECTHASH, CHashKeyGUID> {
  C_OBJECTHASH();

  UINT memHandle;
  UINT thisMemHandle;
  LISTDECL(CMirrorHandler, mirrorHandlers[634]);
  LINKDECLEX(C_OBJECTHASH, link);
  LINKDECLEX(C_OBJECTHASH, reenableLink);
};

NODEDECL(CMirrorHandler) {
  LINKDECLEX(CMirrorHandler, callLink);
  BOOL (*handler)(DWORDLONG guid, UINT offset, UINT bytes, LPCVOID data, LPVOID param);
  LPVOID                             param;
  UINT                               blocksLeft;
  UINT                               offset;
  TSGrowableArray_<BYTE, 'OMGR', __LINE__> previous;
  HANDLER_PRIORITY                   priority;
};

inline C_OBJECTHASH::C_OBJECTHASH() : memHandle(0) {
}

NODEDECL(OBJHANDLERREQUEST) {
  DWORDLONG guid;
  UINT      offset;
  UINT      bytes;
  BOOL (*handler)(DWORDLONG guid, UINT offset, UINT bytes, LPCVOID data, LPVOID param);
  LPVOID           param;
  HANDLER_PRIORITY priority;
  BYTE             set;
};

class ClntObjMgr {
 public:
  ClntObjMgr(PLAYER_TYPE type, LPVOID clientPtr) {
    m_type = type;
    m_callingMirrorHandlers = 0;
    m_mapID = 0;
    m_activePlayer = 0;
    m_net = 0;
    m_movement = 0;
    m_allowGuidDeref = 1;
    m_clientPtr = clientPtr;
  }

  ClntObjMgr(const ClntObjMgr &mgr);
  ~ClntObjMgr() {
  }

  TSHashTable<C_OBJECTHASH, CHashKeyGUID> m_objects;
  TSHashTable<C_OBJECTHASH, CHashKeyGUID> m_lazyCleanupObjects;
  LISTDECLEX(C_OBJECTHASH, link, m_lazyCleanupFifo);
  LISTDECLEX(C_OBJECTHASH, link, m_freeObjects);
  LISTDECLEX(C_OBJECTHASH, link, m_visibleObjects);
  LISTDECLEX(C_OBJECTHASH, reenableLink, m_reenabledObjects);
  int                                     m_callingMirrorHandlers;
  LISTDECL(OBJHANDLERREQUEST, m_pendingObjHandlerRequests);
  int                                     m_allowGuidDeref;
  DWORDLONG                               m_legalGuidDeref;
  DWORDLONG                               m_activePlayer;
  PLAYER_TYPE                             m_type;
  UINT                                    m_mapID;
  ClientConnection                       *m_net;
  LPVOID                                  m_movement;
  LPVOID                                  m_clientPtr;
};

extern "C" int __stdcall zlib_uncompress(BYTE *dest, DWORD *destLen, const BYTE *source, DWORD sourceLen);

enum {
  UPDATE_PARTIAL = 0,
  UPDATE_MOVEMENT = 1,
  UPDATE_FULL = 2,
  UPDATE_OUT_OF_RANGE = 3,
  UPDATE_IN_RANGE = 4
};

static ClntObjMgr *s_curMgr;
static UINT        s_hashMemBlock;
static const UINT  s_objTotalSize[8] = {
    sizeof(CGObject_C) + CGObject::TotalFields() * sizeof(DWORD),
    sizeof(CGItem_C) + CGItem::TotalFields() * sizeof(DWORD),
    sizeof(CGContainer_C) + CGContainer::TotalFields() * sizeof(DWORD),
    sizeof(CGUnit_C) + CGUnit::TotalFields() * sizeof(DWORD),
    sizeof(CGPlayer_C) + CGPlayer::TotalFields() * sizeof(DWORD),
    sizeof(CGGameObject_C) + CGGameObject::TotalFields() * sizeof(DWORD),
    sizeof(CGDynamicObject_C) + CGDynamicObject::TotalFields() * sizeof(DWORD),
    sizeof(CGCorpse_C) + CGCorpse::TotalFields() * sizeof(DWORD),
};
static LPCSTR s_objNames[8] = {
    "CGObject_C", "CGItem_C", "CGContainer_C", "CGUnit_C", "CGPlayer_C", "CGGameObject_C", "CGDynamicObject_C", "CGCorpse_C",
};
static int s_heapSizes[8] = {
    0, 0x100, 0x20, 0x40, 0x40, 0x40, 0x20, 0x20,
};
static const UINT s_objMirrorBlocks[8] = {
    CGObject::TotalFields(),        CGItem::TotalFields(),       CGContainer::TotalFields(),     CGUnit::TotalFields(),
    CGPlayer::TotalFields(),        CGGameObject::TotalFields(), CGDynamicObject::TotalFields(), CGCorpse::TotalFields(),
};
static BYTE s_heapsAllocated;
static UINT s_objHeapId[7];
int         s_localPlayerUpdates;
static LISTDECL(CMirrorHandler, s_mirrorHandlers[8][634]);

static BOOL          MirrorHandlerRemoveQueued(DWORDLONG guid, CMirrorHandler *mirror);
static void          ProcessObjHandlersQueue();
static C_OBJECTHASH *AllocNewObj();
static void                 SkipCreateObject(CDataStore *msg);

static BOOL IsMaskBitSet(const UINT *changeMask, UINT dwordNum) {
  return changeMask[dwordNum >> 5] & (1 << (dwordNum & 0x1F));
}

static void FillInPartialObjectData(C_OBJECTHASH *foundObj, CDataStore *msg, bool forFullUpdate, bool zeroZeroBits);

static void FillInObjectData(C_OBJECTHASH *objhash, CDataStore *msg, CClientObjCreate *init, OBJECT_TYPE_ID objTypeID) {
  FATALASSERT(msg);
  CGObject_C *obj = (CGObject_C *)ObjectPtr(objhash->memHandle);
  FATALASSERT(obj);
  obj->SetTypeID(objTypeID);
  init->Get(msg);
  FillInPartialObjectData(objhash, msg, 1, 1);
}

static UINT GetDataBaseOffset(OBJECT_TYPE_ID section) {
  switch (section) {
    case ID_PLAYER:
      return 736;
    case ID_CONTAINER:
      return 144;
    case ID_ITEM:
    case ID_UNIT:
    case ID_GAMEOBJECT:
    case ID_DYNAMICOBJECT:
    case ID_CORPSE:
      return 24;
    case ID_OBJECT:
      return 0;
  }
  return 0;
}

static void CallBlockMirrorHandlersIfChanged(LISTPTREX(CMirrorHandler) handlerList, DWORDLONG guid, CGObject_C *obj, OBJECT_TYPE_ID objectTypeId) {
  s_curMgr->m_callingMirrorHandlers = 1;

  ITERATELISTPTR(CMirrorHandler, handlerList, mirror) {
    mirror->blocksLeft = 1;
    const BYTE *data = obj->GetData(0) + mirror->offset;
    if (memcmp(data, mirror->previous.Ptr(), mirror->previous.Count()) && !MirrorHandlerRemoveQueued(guid, mirror)) {
      FATALASSERT(mirror->handler);
      mirror->handler(
          guid, mirror->offset - GetDataBaseOffset(objectTypeId), mirror->previous.Count(), mirror->previous.Ptr(),
          mirror->param
      );
    }
  }

  s_curMgr->m_callingMirrorHandlers = 0;
  ProcessObjHandlersQueue();
}

static void CallBlockMirrorHandlers(LISTPTREX(CMirrorHandler) handlerList, DWORDLONG guid, CGObject_C *obj, OBJECT_TYPE_ID objectTypeId) {
  s_curMgr->m_callingMirrorHandlers = 1;

  ITERATELISTPTR(CMirrorHandler, handlerList, mirror) {
    if (!MirrorHandlerRemoveQueued(guid, mirror)) {
      FATALASSERT(mirror->handler);
      const BYTE *data = obj->GetData(0) + mirror->offset;
      if (mirror->previous.Count() >= sizeof(DWORD) || memcmp(mirror->previous.Ptr(), data, mirror->previous.Count())) {
        mirror->handler(
            guid, mirror->offset - GetDataBaseOffset(objectTypeId), mirror->previous.Count(), mirror->previous.Ptr(),
            mirror->param
        );
      }
      mirror->blocksLeft = 1;
    }
  }

  s_curMgr->m_callingMirrorHandlers = 0;
  ProcessObjHandlersQueue();
}

static void SavePreviousValue(LISTPTR(CMirrorHandler) handlerList, CGObject_C *obj) {
  FATALASSERT(obj);
  ITERATELISTPTR(CMirrorHandler, handlerList, mirrorHandler) {
    const BYTE *data = obj->GetData(0) + mirrorHandler->offset;
    memcpy(mirrorHandler->previous.Ptr(), data, mirrorHandler->previous.Count());
  }
}

static C_OBJECTHASH *FindActiveObj(DWORDLONG guid) {
  return s_curMgr->m_objects.Ptr(guid, CHashKeyGUID(guid));
}

static BOOL SetObjectBlock(CGObject_C *obj, UINT i, DWORD data) {
  FATALASSERT(obj);
  return obj->SetBlock(i, data);
}

static UINT IncTypeId(CGObject_C *obj, UINT currTypeId) {
  switch (obj->GetType()) {
    case HIER_TYPE_UNIT:
    case HIER_TYPE_PLAYER:
      switch (currTypeId) {
        case ID_OBJECT:
          return ID_UNIT;
        case ID_UNIT:
          return ID_PLAYER;
      }
      break;
    case HIER_TYPE_ITEM:
    case HIER_TYPE_CONTAINER:
      switch (currTypeId) {
        case ID_OBJECT:
          return ID_ITEM;
        case ID_ITEM:
          return ID_CONTAINER;
      }
      break;
    case HIER_TYPE_GAMEOBJECT:
      if (currTypeId == ID_OBJECT) {
        return ID_GAMEOBJECT;
      }
      break;
    case HIER_TYPE_DYNAMICOBJECT:
      if (currTypeId == ID_OBJECT) {
        return ID_DYNAMICOBJECT;
      }
      break;
    case HIER_TYPE_CORPSE:
      if (currTypeId == ID_OBJECT) {
        return ID_CORPSE;
      }
      break;
  }
  return ID_AIGROUP;
}

static void MirrorHandlerAdvanceBlock(LISTPTREX(CMirrorHandler) handlerList) {
  SAFEITERATELISTPTR(CMirrorHandler, handlerList, handler) {
    if (!--handler->blocksLeft) {
      handlerList->UnlinkNode(handler);
    }
  }
}

static BOOL GetMirrorHandler(LISTPTR(CMirrorHandler) mirrorHandlers, LISTPTREX(CMirrorHandler) handlerList) {
  UINT offDword;

  FATALASSERT(handlerList);
  if (!mirrorHandlers || mirrorHandlers->IsEmpty()) {
    return 0;
  }

  ITERATELISTPTR(CMirrorHandler, mirrorHandlers, handler) {
    offDword = handler->offset & 3;
    handlerList->LinkNode(handler, handler->priority == HANDLER_PRIORITY_HIGH ? LIST_HEAD : LIST_TAIL, 0);
    handler->blocksLeft = (handler->previous.Count() + offDword + 3) >> 2;
  }

  return 1;
}

static UINT GetNumDwordBlocks(OBJECT_TYPE objType) {
  switch (objType) {
    case HIER_TYPE_OBJECT:
      return 6;
    case HIER_TYPE_ITEM:
    case HIER_TYPE_CORPSE:
      return 36;
    case HIER_TYPE_CONTAINER:
      return 78;
    case HIER_TYPE_UNIT:
      return 184;
    case HIER_TYPE_PLAYER:
      return 634;
    case HIER_TYPE_GAMEOBJECT:
      return 20;
    case HIER_TYPE_DYNAMICOBJECT:
      return 16;
    default:
      FATALASSERT(0);
      return 0;
  }
}

static void SkipPartialObjectUpdate(CDataStore *msg) {
  UINT changeMasks[20];
  BYTE updateMaskBlocks;

  msg->Get(updateMaskBlocks);
  FATALASSERT(updateMaskBlocks <= (((((( (((sizeof(CGObjectData) + sizeof(CGItemData) + sizeof(CGContainerData)) > (sizeof(CGObjectData) + sizeof(CGUnitData) + sizeof(CGPlayerData))) ? (sizeof(CGObjectData) + sizeof(CGItemData) + sizeof(CGContainerData)) : (sizeof(CGObjectData) + sizeof(CGUnitData) + sizeof(CGPlayerData))) ) / sizeof(DWORD))+7)/8)+sizeof(uint)-1)/sizeof(uint)));

  for (int i = 0; i < updateMaskBlocks; ++i) {
    msg->Get(changeMasks[i]);
  }

  for (i = 0; i < updateMaskBlocks * (sizeof(uint) * 8); ++i) {
    if (IsMaskBitSet(changeMasks, i)) {
      DWORD junk;
      msg->Get(junk);
    }
  }
}

static void PartialUpdateFromFullUpdate(DWORD eventTime, C_OBJECTHASH *foundObj, CDataStore *msg) {
  FATALASSERT(foundObj);
  FATALASSERT(msg);
  SysMsgAdd("Updating unit data", SYSMSG_INFO, 0x20);
  CGObject_C *obj = (CGObject_C *)ObjectPtr(foundObj->memHandle);
  FATALASSERT(obj);

  if (obj->IsA(TYPE_UNIT)) {
    CClientObjCreate createData;
    createData.Get(msg);
    ((CGUnit_C *)obj)->SetClientInitData(eventTime, createData, obj->GetGUID() == ClntObjMgrGetActivePlayer());
  } else {
    CClientObjCreate::Skip(msg);
  }
  FillInPartialObjectData(foundObj, msg, 0, 1);
}

static void FillInPartialObjectData(C_OBJECTHASH *foundObj, CDataStore *msg, bool forFullUpdate, bool zeroZeroBits) {
  UINT changeMasks[20];
  BYTE updateMaskBlocks;

  FATALASSERT(foundObj);
  FATALASSERT(msg);
  CGObject_C *obj = (CGObject_C *)ObjectPtr(foundObj->memHandle);
  FATALASSERT(obj);

  msg->Get(updateMaskBlocks);
  FATALASSERT(updateMaskBlocks <= (((((( (((sizeof(CGObjectData) + sizeof(CGItemData) + sizeof(CGContainerData)) > (sizeof(CGObjectData) + sizeof(CGUnitData) + sizeof(CGPlayerData))) ? (sizeof(CGObjectData) + sizeof(CGItemData) + sizeof(CGContainerData)) : (sizeof(CGObjectData) + sizeof(CGUnitData) + sizeof(CGPlayerData))) ) / sizeof(DWORD))+7)/8)+sizeof(uint)-1)/sizeof(uint)));
  for (int i = 0; i < updateMaskBlocks; ++i) {
    msg->Get(changeMasks[i]);
  }

  UINT objectTypeId = ID_OBJECT;
  UINT blockOffset = 0;
  UINT numBlocks = GetNumDwordBlocks(obj->GetType());
  LISTDECLEX(CMirrorHandler, callLink, handlerList);
  for (i = 0; i < numBlocks; ++i) {
    if (i >= s_objMirrorBlocks[objectTypeId]) {
      blockOffset = s_objMirrorBlocks[objectTypeId];
      objectTypeId = IncTypeId(obj, objectTypeId);
    }

    if (!forFullUpdate) {
      MirrorHandlerAdvanceBlock(&handlerList);
      LISTPTR(CMirrorHandler) classHandlers = &s_mirrorHandlers[objectTypeId][i - blockOffset];
      classHandlers = GetMirrorHandler(classHandlers, &handlerList) ? classHandlers : 0;
      LISTPTR(CMirrorHandler) objectHandlers = &foundObj->mirrorHandlers[i];
      objectHandlers = GetMirrorHandler(objectHandlers, &handlerList) ? objectHandlers : 0;
      if (classHandlers) {
        SavePreviousValue(classHandlers, obj);
      }
      if (objectHandlers) {
        SavePreviousValue(objectHandlers, obj);
      }
    }

    DWORD block;
    if (!IsMaskBitSet(changeMasks, i)) {
      if (!forFullUpdate && !zeroZeroBits) {
        continue;
      }
      block = 0;
      if (!zeroZeroBits) {
        continue;
      }
    } else {
      msg->Get(block);
    }
    BOOL success = SetObjectBlock(obj, i, block);
    FATALASSERT(success);
  }
}

static void CallMirrorHandlers(CDataStore *msg, bool forFullUpdate, DWORDLONG guid) {
  UINT          changeMasks[20];
  DWORD         junk;
  UINT          numBlocks;
  C_OBJECTHASH *foundObj;
  CGObject_C   *obj;
  BYTE          updateMaskBlocks;

  FATALASSERT(msg);
  if (!forFullUpdate) {
    msg->Get(guid);
  }

  foundObj = FindActiveObj(guid);
  if (!foundObj) {
    SkipPartialObjectUpdate(msg);
    return;
  }

  obj = (CGObject_C *)ObjectPtr(foundObj->memHandle);
  FATALASSERT(obj);

  msg->Get(updateMaskBlocks);
  FATALASSERT(updateMaskBlocks <= (((((( (((sizeof(CGObjectData) + sizeof(CGItemData) + sizeof(CGContainerData)) > (sizeof(CGObjectData) + sizeof(CGUnitData) + sizeof(CGPlayerData))) ? (sizeof(CGObjectData) + sizeof(CGItemData) + sizeof(CGContainerData)) : (sizeof(CGObjectData) + sizeof(CGUnitData) + sizeof(CGPlayerData))) ) / sizeof(DWORD))+7)/8)+sizeof(uint)-1)/sizeof(uint)));
  for (int i = 0; i < updateMaskBlocks; ++i) {
    msg->Get(changeMasks[i]);
  }

  numBlocks = GetNumDwordBlocks(obj->GetType());
  UINT objectTypeId = ID_OBJECT;
  UINT blockOffset = 0;
  LISTDECLEX(CMirrorHandler, callLink, handlerList);
  for (i = 0; i < numBlocks; ++i) {
    if (i >= s_objMirrorBlocks[objectTypeId]) {
      blockOffset = s_objMirrorBlocks[objectTypeId];
      objectTypeId = IncTypeId(obj, objectTypeId);
    }

    MirrorHandlerAdvanceBlock(&handlerList);
    GetMirrorHandler(&s_mirrorHandlers[objectTypeId][i - blockOffset], &handlerList);
    GetMirrorHandler(&foundObj->mirrorHandlers[i], &handlerList);

    if (IsMaskBitSet(changeMasks, i)) {
      if (forFullUpdate) {
        CallBlockMirrorHandlersIfChanged(&handlerList, guid, obj, (OBJECT_TYPE_ID)objectTypeId);
      } else {
        CallBlockMirrorHandlers(&handlerList, guid, obj, (OBJECT_TYPE_ID)objectTypeId);
      }
      msg->Get(junk);
    }
  }
}

static OBJECT_TYPE_ID GetOffsetSectionId(OBJECT_TYPE hierType, UINT offset) {
  if (offset < 24) {
    return ID_OBJECT;
  }
  if ((bool)(((UINT)hierType >> ID_ITEM) & 1) && offset < 144) {
    return ID_ITEM;
  }
  if ((bool)(((UINT)hierType >> ID_CONTAINER) & 1) && offset < 312) {
    return ID_CONTAINER;
  }
  if ((bool)(((UINT)hierType >> ID_UNIT) & 1) && offset < 736) {
    return ID_UNIT;
  }
  if ((bool)(((UINT)hierType >> ID_PLAYER) & 1) && offset < 2536) {
    return ID_PLAYER;
  }
  if ((bool)(((UINT)hierType >> ID_GAMEOBJECT) & 1) && offset < 80) {
    return ID_GAMEOBJECT;
  }
  if ((bool)(((UINT)hierType >> ID_DYNAMICOBJECT) & 1) && offset < 64) {
    return ID_DYNAMICOBJECT;
  }
  if ((bool)(((UINT)hierType >> ID_CORPSE) & 1) && offset < 144) {
    return ID_CORPSE;
  }
  return ID_AIGROUP;
}

static OBJECT_TYPE_ID GetSectionId(OBJECT_TYPE hierType) {
  switch (hierType) {
    case HIER_TYPE_OBJECT:
      return ID_OBJECT;
    case HIER_TYPE_ITEM:
      return ID_ITEM;
    case HIER_TYPE_CONTAINER:
      return ID_CONTAINER;
    case HIER_TYPE_UNIT:
      return ID_UNIT;
    case HIER_TYPE_PLAYER:
      return ID_PLAYER;
    case HIER_TYPE_GAMEOBJECT:
      return ID_GAMEOBJECT;
    case HIER_TYPE_DYNAMICOBJECT:
      return ID_DYNAMICOBJECT;
    case HIER_TYPE_CORPSE:
      return ID_CORPSE;
    default:
      FATALASSERT(0);
      return ID_OBJECT;
  }
}

static void InitObject(DWORD eventTime, OBJECT_TYPE_ID type, UINT memHandle, CClientObjCreate *init) {
  CGObject_C *obj = (CGObject_C *)ObjectPtr(memHandle);
  FATALASSERT(obj);

  if (init->flags & 1) {
    s_curMgr->m_activePlayer = obj->GetGUID();
    CGPlayer_C::SetActive((CGPlayer_C *)obj);
    CGPlayer_C::SetRealActivePlayer(obj->GetGUID());
  }

  switch (type) {
    case ID_ITEM:
      new (obj) CGItem_C((DWORD *)((CGItem_C *)obj + 1), eventTime, init);
      break;
    case ID_CONTAINER:
      new (obj) CGContainer_C((DWORD *)((CGContainer_C *)obj + 1), eventTime, init);
      break;
    case ID_UNIT:
      new (obj) CGUnit_C((DWORD *)((CGUnit_C *)obj + 1), eventTime, init);
      break;
    case ID_PLAYER:
      new (obj) CGPlayer_C((DWORD *)((CGPlayer_C *)obj + 1), eventTime, init);
      break;
    case ID_GAMEOBJECT:
      new (obj) CGGameObject_C((DWORD *)((CGGameObject_C *)obj + 1), eventTime, init);
      break;
    case ID_DYNAMICOBJECT:
      new (obj) CGDynamicObject_C((DWORD *)((CGDynamicObject_C *)obj + 1), eventTime, init);
      break;
    case ID_CORPSE:
      new (obj) CGCorpse_C((DWORD *)((CGCorpse_C *)obj + 1), eventTime, init);
      break;
    default:
      FATALASSERT(0);
      break;
  }
}

static inline void FreeObject(UINT memHandle) {
  CGObject_C *obj = (CGObject_C *)ObjectPtr(memHandle);

  FATALASSERT(obj);
  switch (obj->GetType()) {
    case HIER_TYPE_OBJECT:
      obj->~CGObject_C();
      break;
    case HIER_TYPE_ITEM:
      ((CGItem_C *)obj)->~CGItem_C();
      break;
    case HIER_TYPE_CONTAINER:
      ((CGContainer_C *)obj)->~CGContainer_C();
      break;
    case HIER_TYPE_UNIT:
      ((CGUnit_C *)obj)->~CGUnit_C();
      break;
    case HIER_TYPE_PLAYER:
      ((CGPlayer_C *)obj)->~CGPlayer_C();
      break;
    case HIER_TYPE_GAMEOBJECT:
      ((CGGameObject_C *)obj)->~CGGameObject_C();
      break;
    case HIER_TYPE_DYNAMICOBJECT:
      ((CGDynamicObject_C *)obj)->~CGDynamicObject_C();
      break;
    case HIER_TYPE_CORPSE:
      ((CGCorpse_C *)obj)->~CGCorpse_C();
      break;
    default:
      FATALASSERT(0);
      break;
  }
  ObjectFree(memHandle);
}

static void SkipCreateObject(CDataStore *msg) {
  CClientObjCreate::Skip(msg);
  SkipPartialObjectUpdate(msg);
}

static CGObject_C *GetObjectPtr(DWORDLONG guid) {
  C_OBJECTHASH *foundObj;

  if (!guid) {
    return 0;
  }

  foundObj = FindActiveObj(guid);
  if (!foundObj) {
    return 0;
  }

  return (CGObject_C *)ObjectPtr(foundObj->memHandle);
}

static void PostInitObject(CDataStore *msg) {
  DWORDLONG        guid;
  OBJECT_TYPE_ID   type;
  BYTE             btype;

  msg->Get(guid);
  msg->Get(btype);
  type = (OBJECT_TYPE_ID)btype;
  CGObject_C *obj = GetObjectPtr(guid);
  FATALASSERT(obj);
  CClientObjCreate init;
  init.Get(msg);

  if (obj->IsPostInited()) {
    if (type >= ID_UNIT && type <= ID_PLAYER) {
      ((CGUnit_C *)obj)->PostSetClientInitData(init.move);
    }
    CallMirrorHandlers(msg, true, guid);
    return;
  }

  switch (type) {
    case ID_PLAYER:
      ((CGPlayer_C *)obj)->PostInit(init);
      break;
    case ID_UNIT:
      ((CGUnit_C *)obj)->PostInit(init);
      break;
    case ID_CONTAINER:
      ((CGContainer_C *)obj)->PostInit(init);
      break;
    case ID_ITEM:
      ((CGItem_C *)obj)->PostInit(init);
      break;
    case ID_OBJECT:
      obj->PostInit(init);
      break;
    case ID_GAMEOBJECT:
      ((CGGameObject_C *)obj)->PostInit(init);
      break;
    case ID_DYNAMICOBJECT:
      ((CGDynamicObject_C *)obj)->PostInit(init);
      break;
    case ID_CORPSE:
      ((CGCorpse_C *)obj)->PostInit(init);
      break;
    default:
      break;
  }
  SkipPartialObjectUpdate(msg);
}

static void CreateMessage(OBJECT_TYPE_ID id) {
  switch (id) {
    case ID_OBJECT:
      SysMsgAdd("Creating object", SYSMSG_INFO, 0x20);
      break;
    case ID_ITEM:
      SysMsgAdd("Creating item", SYSMSG_INFO, 0x20);
      break;
    case ID_CONTAINER:
      SysMsgAdd("Creating container", SYSMSG_INFO, 0x20);
      break;
    case ID_UNIT:
      SysMsgAdd("Creating unit", SYSMSG_INFO, 0x20);
      break;
    case ID_PLAYER:
      SysMsgAdd("Creating player", SYSMSG_INFO, 0x20);
      break;
    case ID_GAMEOBJECT:
      SysMsgAdd("Creating game object", SYSMSG_INFO, 0x20);
      break;
    case ID_DYNAMICOBJECT:
      SysMsgAdd("Creating dynamic object", SYSMSG_INFO, 0x20);
      break;
    case ID_CORPSE:
      SysMsgAdd("Creating corpse", SYSMSG_INFO, 0x20);
      break;
  }
}

static C_OBJECTHASH *GetUpdateObject(DWORDLONG guid) {
  CHashKeyGUID  hashKey(guid);
  C_OBJECTHASH *foundObj = s_curMgr->m_objects.Ptr(guid, hashKey);
  if (foundObj) {
    return foundObj;
  }

  foundObj = s_curMgr->m_lazyCleanupObjects.Ptr(guid, hashKey);
  if (!foundObj) {
    return 0;
  }

  s_curMgr->m_lazyCleanupObjects.Unlink(foundObj);
  s_curMgr->m_lazyCleanupFifo.UnlinkNode(foundObj);
  s_curMgr->m_objects.Insert(foundObj, guid, hashKey);
  CGObject_C *object = (CGObject_C *)ObjectPtr(foundObj->memHandle);
  if (!s_curMgr->m_visibleObjects.IsLinked(foundObj)) {
    s_curMgr->m_visibleObjects.LinkNode(foundObj, LIST_TAIL, 0);
    object->Reenable();
    s_curMgr->m_reenabledObjects.LinkNode(foundObj, LIST_TAIL, 0);
  }
  return foundObj;
}

static void SetupObjectStorage(OBJECT_TYPE_ID type, UINT memHandle) {
  LPVOID storage = ObjectPtr(memHandle);
  DWORD *data;
  UINT   size;
  switch (type) {
    case ID_OBJECT:
      data = (DWORD *)((CGObject_C *)storage + 1);
      size = CGObject::TotalFields() * sizeof(DWORD);
      ((CGObject_C *)storage)->SetStorage(data);
      break;
    case ID_ITEM:
      data = (DWORD *)((CGItem_C *)storage + 1);
      size = CGItem::TotalFields() * sizeof(DWORD);
      ((CGItem_C *)storage)->SetStorage(data);
      break;
    case ID_CONTAINER:
      data = (DWORD *)((CGContainer_C *)storage + 1);
      size = CGContainer::TotalFields() * sizeof(DWORD);
      ((CGContainer_C *)storage)->SetStorage(data);
      break;
    case ID_UNIT:
      data = (DWORD *)((CGUnit_C *)storage + 1);
      size = CGUnit::TotalFields() * sizeof(DWORD);
      ((CGUnit_C *)storage)->SetStorage(data);
      break;
    case ID_PLAYER:
      data = (DWORD *)((CGPlayer_C *)storage + 1);
      size = CGPlayer::TotalFields() * sizeof(DWORD);
      ((CGPlayer_C *)storage)->SetStorage(data);
      break;
    case ID_GAMEOBJECT:
      data = (DWORD *)((CGGameObject_C *)storage + 1);
      size = CGGameObject::TotalFields() * sizeof(DWORD);
      ((CGGameObject_C *)storage)->SetStorage(data);
      break;
    case ID_DYNAMICOBJECT:
      data = (DWORD *)((CGDynamicObject_C *)storage + 1);
      size = CGDynamicObject::TotalFields() * sizeof(DWORD);
      ((CGDynamicObject_C *)storage)->SetStorage(data);
      break;
    case ID_CORPSE:
      data = (DWORD *)((CGCorpse_C *)storage + 1);
      size = CGCorpse::TotalFields() * sizeof(DWORD);
      ((CGCorpse_C *)storage)->SetStorage(data);
      break;
    default:
      FATALASSERT(0);
      return;
  }
  memset(data, 0, size);
}

static C_OBJECTHASH *AllocNewObj() {
  UINT memHandle;

  if (!ObjectAlloc(s_hashMemBlock, &memHandle)) {
    return 0;
  }

  C_OBJECTHASH *hash = (C_OBJECTHASH *)ObjectPtr(memHandle);
  if (!hash) {
    return 0;
  }

  new (hash) C_OBJECTHASH;
  hash->thisMemHandle = memHandle;
  return hash;
}

static BOOL CreateObject(DWORD eventTime, CDataStore *msg) {
  DWORDLONG        guid;
  UINT             memHandle;
  OBJECT_TYPE_ID   type;
  BYTE             btype;

  FATALASSERT(msg);
  msg->Get(guid);
  s_curMgr->m_legalGuidDeref = guid;
  msg->Get(btype);
  type = (OBJECT_TYPE_ID)btype;

  C_OBJECTHASH *foundObj = GetUpdateObject(guid);
  if (foundObj) {
    PartialUpdateFromFullUpdate(eventTime, foundObj, msg);
    return 1;
  }

  foundObj = s_curMgr->m_freeObjects.Head();
  if (foundObj) {
    s_curMgr->m_freeObjects.UnlinkNode(foundObj);
  } else {
    foundObj = s_curMgr->m_lazyCleanupFifo.Head();
    if (!foundObj) {
      foundObj = AllocNewObj();
      if (!foundObj) {
        return 0;
      }
      SysMsgAdd("NOFREEOBJECTSALLOCATING", SYSMSG_WARNING, 0x20);
    } else {
      s_curMgr->m_lazyCleanupObjects.Unlink(foundObj);
      s_curMgr->m_lazyCleanupFifo.UnlinkNode(foundObj);

      FreeObject(foundObj->memHandle);
    }
  }

  if (!ObjectAlloc(s_objHeapId[type - 1], &memHandle)) {
    s_curMgr->m_freeObjects.LinkNode(foundObj, LIST_TAIL, 0);
    SkipCreateObject(msg);
    return 0;
  }

  s_curMgr->m_objects.Insert(foundObj, guid, CHashKeyGUID(guid));
  foundObj->memHandle = memHandle;
  CClientObjCreate init;
  SetupObjectStorage(type, memHandle);
  FillInObjectData(foundObj, msg, &init, type);
  s_curMgr->m_visibleObjects.LinkNode(foundObj, LIST_TAIL, 0);
  InitObject(eventTime, type, memHandle, &init);
  CreateMessage(type);
  return 1;
}

static BOOL UpdateObjectMovement(DWORD eventTime, CDataStore *msg) {
  DWORDLONG guid;

  msg->Get(guid);
  s_curMgr->m_legalGuidDeref = guid;
  CClientMoveUpdate update;
  *msg >> update;
  if (guid == CGUnit_C::GetActiveMover()) {
    return 1;
  }

  C_OBJECTHASH *foundObj = GetUpdateObject(guid);
  if (!foundObj) {
    return 0;
  }
  CGUnit_C *unit = (CGUnit_C *)ObjectPtr(foundObj->memHandle);
  FATALASSERT(unit);
  unit->UpdateMoveInfo(eventTime, update);
  return 1;
}

static BOOL UpdateObject(CDataStore *msg) {
  FATALASSERT(msg);
  DWORDLONG guid;
  msg->Get(guid);
  if (guid == ClntObjMgrGetActivePlayer()) {
    ++s_localPlayerUpdates;
  }
  s_curMgr->m_legalGuidDeref = guid;

  C_OBJECTHASH *foundObj = GetUpdateObject(guid);
  if (!foundObj) {
    SkipPartialObjectUpdate(msg);
    return 0;
  }
  FillInPartialObjectData(foundObj, msg, 0, 0);
  return 1;
}

static void PostMovementUpdate(CDataStore *msg) {
  FATALASSERT(msg);
  DWORDLONG guid;
  msg->Get(guid);
  CClientMoveUpdate update;
  *msg >> update;
  CGObject_C *object = GetObjectPtr(guid);
  if (!object) {
    return;
  }

  switch (object->GetType()) {
    case HIER_TYPE_PLAYER:
      ((CGPlayer_C *)object)->PostMovementUpdate(update);
      break;
    case HIER_TYPE_UNIT:
      ((CGUnit_C *)object)->PostMovementUpdate(update);
      break;
    case HIER_TYPE_CONTAINER:
      ((CGContainer_C *)object)->PostMovementUpdate();
      break;
    case HIER_TYPE_ITEM:
      ((CGItem_C *)object)->PostMovementUpdate();
      break;
    case HIER_TYPE_OBJECT:
      object->PostMovementUpdate();
      break;
    case HIER_TYPE_GAMEOBJECT:
      ((CGGameObject_C *)object)->PostMovementUpdate();
      break;
    case HIER_TYPE_DYNAMICOBJECT:
      ((CGDynamicObject_C *)object)->PostMovementUpdate();
      break;
    case HIER_TYPE_CORPSE:
      ((CGCorpse_C *)object)->PostMovementUpdate();
      break;
  }
}

static void OutOfRangeMessage(DWORDLONG guid) {
  CGObject_C *object = GetObjectPtr(guid);
  if (object) {
    switch (object->GetType()) {
      case HIER_TYPE_OBJECT:
        SysMsgAdd("Forgetting object", SYSMSG_INFO, 0x20);
        break;
      case HIER_TYPE_ITEM:
        SysMsgAdd("Forgetting item", SYSMSG_INFO, 0x20);
        break;
      case HIER_TYPE_CONTAINER:
        SysMsgAdd("Forgetting container", SYSMSG_INFO, 0x20);
        break;
      case HIER_TYPE_UNIT:
        SysMsgAdd("Forgetting unit", SYSMSG_INFO, 0x20);
        break;
      case HIER_TYPE_PLAYER:
        SysMsgAdd("Forgetting player", SYSMSG_INFO, 0x20);
        break;
      case HIER_TYPE_GAMEOBJECT:
        SysMsgAdd("Forgetting game object", SYSMSG_INFO, 0x20);
        break;
      case HIER_TYPE_DYNAMICOBJECT:
        SysMsgAdd("Forgetting dynamic object", SYSMSG_INFO, 0x20);
        break;
      case HIER_TYPE_CORPSE:
        SysMsgAdd("Forgetting corpse", SYSMSG_INFO, 0x20);
        break;
    }
  }
}

static void InRangeMessage(DWORDLONG guid) {
  CGObject_C *object = GetObjectPtr(guid);
  if (object) {
    switch (object->GetType()) {
      case HIER_TYPE_OBJECT:
        SysMsgAdd("Remembering object", SYSMSG_INFO, 0x20);
        break;
      case HIER_TYPE_ITEM:
        SysMsgAdd("Remembering item", SYSMSG_INFO, 0x20);
        break;
      case HIER_TYPE_CONTAINER:
        SysMsgAdd("Remembering container", SYSMSG_INFO, 0x20);
        break;
      case HIER_TYPE_UNIT:
        SysMsgAdd("Remembering unit", SYSMSG_INFO, 0x20);
        break;
      case HIER_TYPE_PLAYER:
        SysMsgAdd("Remembering player", SYSMSG_INFO, 0x20);
        break;
      case HIER_TYPE_GAMEOBJECT:
        SysMsgAdd("Remembering game object", SYSMSG_INFO, 0x20);
        break;
      case HIER_TYPE_DYNAMICOBJECT:
        SysMsgAdd("Remembering dynamic object", SYSMSG_INFO, 0x20);
        break;
      case HIER_TYPE_CORPSE:
        SysMsgAdd("Remembering corpse", SYSMSG_INFO, 0x20);
        break;
    }
  }
}

void ClntObjMgrObjectInRange(DWORDLONG guid) {
  C_OBJECTHASH *foundObj = GetUpdateObject(guid);
  if (foundObj) {
    CGObject_C *object = (CGObject_C *)ObjectPtr(foundObj->memHandle);
    FATALASSERT(object);
    InRangeMessage(guid);
  }
}

static void UpdateInRangeObjects(CDataStore *msg) {
  DWORDLONG guid;
  UINT      count;
  msg->Get(count);
  FATALASSERT(count > 0);
  while (count) {
    --count;
    msg->Get(guid);
    if (guid != ClntObjMgrGetActivePlayer()) {
      ClntObjMgrObjectInRange(guid);
    }
  }
}

static void UpdateOutOfRangeObjects(CDataStore *msg) {
  DWORDLONG guid;
  UINT      count;
  msg->Get(count);
  FATALASSERT(count > 0);
  while (count) {
    --count;
    msg->Get(guid);
    if (guid != ClntObjMgrGetActivePlayer()) {
      ClntObjMgrObjectOutOfRange(guid, 0);
    }
  }
}

static void ClearObjectMirrorHandlers(C_OBJECTHASH *foundObj) {
  for (UINT i = 0; i < 634; ++i) {
    LISTPTR(CMirrorHandler) handlerList = &foundObj->mirrorHandlers[i];
    handlerList->Clear();
  }
}

void ClntObjMgrObjectOutOfRange(DWORDLONG guid, int shutdown) {
  C_OBJECTHASH *foundObj = FindActiveObj(guid);
  if (!foundObj) {
    return;
  }

  OutOfRangeMessage(guid);
  CGObject_C *object = (CGObject_C *)ObjectPtr(foundObj->memHandle);
  FATALASSERT(object);
  object->Disable(shutdown);
  ClearObjectMirrorHandlers(foundObj);
  s_curMgr->m_objects.Unlink(foundObj);
  UINT         hashVal = foundObj->GetHashValue();
  CHashKeyGUID hashKey = foundObj->GetKey();
  if (s_curMgr->m_visibleObjects.IsLinked(foundObj)) {
    s_curMgr->m_visibleObjects.UnlinkNode(foundObj);
  }
  s_curMgr->m_lazyCleanupObjects.Insert(foundObj, hashVal, hashKey);
  s_curMgr->m_lazyCleanupFifo.LinkNode(foundObj, LIST_TAIL, 0);
}

static void SkipSetOfObjects(CDataStore *msg) {
  LPVOID junkData;
  UINT   count;
  msg->Get(count);
  msg->GetDataInSitu(junkData, count * sizeof(DWORDLONG));
}

static BOOL ObjectUpdateHandler(LPVOID, NETMESSAGE, DWORD eventTime, CDataStore *msg) {
  DWORDLONG oldActive;
  int       success;
  UINT      numObjUpdates;
  BYTE      updateType;

  msg->Get(numObjUpdates);
  UINT marker1 = msg->Tell();
  UINT i = 0;
  msg->Get(updateType);
  if (updateType == UPDATE_OUT_OF_RANGE) {
    UpdateOutOfRangeObjects(msg);
    i = 1;
  } else {
    msg->Seek(marker1);
  }

  s_curMgr->m_allowGuidDeref = 0;
  s_localPlayerUpdates = 0;
  oldActive = ClntObjMgrGetActivePlayer();
  success = 1;
  for (; i < numObjUpdates; ++i) {
    s_curMgr->m_legalGuidDeref = 0;
    msg->Get(updateType);
    switch (updateType) {
      case UPDATE_FULL:
        if (CreateObject(eventTime, msg)) {
          continue;
        }
        SysMsgAdd("OBJECTCREATIONFAILURE", SYSMSG_FATAL, 0x20);
        break;
      case UPDATE_MOVEMENT:
        if (UpdateObjectMovement(eventTime, msg)) {
          SysMsgAdd("Updating unit movement", SYSMSG_INFO, 0x20);
          continue;
        }
        break;
      case UPDATE_PARTIAL:
        if (UpdateObject(msg)) {
          SysMsgAdd("Updating unit data", SYSMSG_INFO, 0x20);
          continue;
        }
        break;
      case UPDATE_IN_RANGE:
        UpdateInRangeObjects(msg);
        continue;
    }
    FATALASSERT(0);
    success = 0;
  }
  s_curMgr->m_allowGuidDeref = 1;
  FATALASSERT(s_localPlayerUpdates <= 1);

  msg->Seek(marker1);
  for (i = 0; i < numObjUpdates; ++i) {
    BYTE updateType;
    msg->Get(updateType);
    switch (updateType) {
      case UPDATE_FULL:
        PostInitObject(msg);
        break;
      case UPDATE_MOVEMENT:
        PostMovementUpdate(msg);
        break;
      case UPDATE_PARTIAL:
        CallMirrorHandlers(msg, false, 0);
        break;
      case UPDATE_OUT_OF_RANGE:
      case UPDATE_IN_RANGE:
        SkipSetOfObjects(msg);
        break;
      default:
        FATALASSERT(0);
        break;
    }
  }

  while (!s_curMgr->m_reenabledObjects.IsEmpty()) {
    C_OBJECTHASH *foundObj = s_curMgr->m_reenabledObjects.Head();
    s_curMgr->m_reenabledObjects.UnlinkNode(foundObj);
    CGObject_C *object = (CGObject_C *)ObjectPtr(foundObj->memHandle);
    if (object) {
      object->PostReenable();
    }
  }

  if (oldActive != ClntObjMgrGetActivePlayer()) {
    CGPartyInfo::RemoveActivePlayer(ClntObjMgrGetActivePlayer());
  }
  return success;
}

static BOOL ObjectCompressedUpdateHandler(LPVOID, NETMESSAGE, DWORD eventTime, CDataStore *msg) {
  LPVOID     data;
  UINT       origSize;
  DWORD      destSize;

  msg->Get(origSize);
  UINT compressedSize = msg->Size() - msg->Tell();
  msg->GetDataInSitu(data, compressedSize);
  LPVOID dest = _alloca(origSize);
  destSize = origSize;
  zlib_uncompress((BYTE *)dest, &destSize, (const BYTE *)data, compressedSize);
  FATALASSERT(destSize == origSize);
  WDataStore realmsg;
  realmsg.PutData(dest, destSize);
  realmsg.Finalize();
  return ObjectUpdateHandler(0, SMSG_UPDATE_OBJECT, eventTime, &realmsg);
}

static void AssignMirrorHandler(
    LISTPTR(CMirrorHandler) handlerList,
    UINT offset,
    UINT bytes,
    int (*handler)(DWORDLONG, UINT, UINT, LPCVOID, LPVOID),
    LPVOID           param,
    HANDLER_PRIORITY priority
) {
  CMirrorHandler *mirror = handlerList->NewNode(LIST_TAIL, 0, 0);
  mirror->previous.SetCount(bytes);
  mirror->offset = offset;
  mirror->handler = handler;
  mirror->param = param;
  mirror->priority = priority;
}

static void UnassignMirrorHandler(LISTPTR(CMirrorHandler) handlerList, int (*handler)(DWORDLONG, UINT, UINT, LPCVOID, LPVOID), LPVOID param) {
  ITERATELISTPTR(CMirrorHandler, handlerList, mirror) {
    if (mirror->handler == handler && mirror->param == param) {
      ITERATE_DELETEANDBREAK
    }
  }
}

static BOOL OnObjectDestroy(LPVOID, NETMESSAGE, DWORD, CDataStore *msg) {
  DWORDLONG guid;
  msg->Get(guid);
  ClntObjMgrFreeObject(guid);
  return 1;
}

static BOOL CCommand_ObjUsage(LPCSTR command, LPCSTR arguments) {
  UINT          numVisible = 0;
  UINT          numActive = 0;
  UINT          numWaiting = 0;
  UINT          numFree = 0;
  C_OBJECTHASH *object;

  for (object = s_curMgr->m_visibleObjects.Head(); (int)object > 0; object = s_curMgr->m_visibleObjects.RawNext(object)) {
    ++numVisible;
  }

  for (object = s_curMgr->m_objects.Head(); (int)object > 0; object = s_curMgr->m_objects.RawNext(object)) {
    ++numActive;
  }

  for (object = s_curMgr->m_lazyCleanupObjects.Head(); (int)object > 0; object = s_curMgr->m_lazyCleanupObjects.RawNext(object)) {
    ++numWaiting;
  }

  for (object = s_curMgr->m_freeObjects.Head(); (int)object > 0; object = s_curMgr->m_freeObjects.RawNext(object)) {
    ++numFree;
  }

  ConsoleWrite("Object manager list status:", HIGHLIGHT_COLOR);
  ConsoleWriteA("    Active objects:              %u objects (%u visible)", HIGHLIGHT_COLOR, numActive, numVisible);
  ConsoleWriteA("    Objects waiting to be freed: %u objects", HIGHLIGHT_COLOR, numWaiting);
  ConsoleWriteA("    Free objects:                %u objects", HIGHLIGHT_COLOR, numFree);
  return 1;
}

ClntObjMgr *ClntObjMgrCreate(PLAYER_TYPE type, LPVOID clientPtr) {
  return NEW(ClntObjMgr)(type, clientPtr);
}

void ClntObjMgrDestruct(ClntObjMgr *mgr) {
  if (s_curMgr == mgr) {
    s_curMgr = 0;
  }

  if (mgr) {
    if (mgr->m_net) {
      mgr->m_net->SetObjMgr(0);
    }
    DEL(mgr);
  }
}

void ClntObjMgrSetCurrent(ClntObjMgr *mgr) {
  s_curMgr = mgr;
  if (mgr) {
    ClientServices_SetCurrent(mgr->m_net);
  } else {
    ClientServices_SetCurrent(0);
  }
}

ClntObjMgr *ClntObjMgrGetCurrent() {
  return s_curMgr;
}

BOOL ClntObjMgrIsValid(int forWriting) {
  if (!s_curMgr) {
    return 0;
  }

  return SMemIsValidPointer(s_curMgr, sizeof(ClntObjMgr), forWriting) != 0;
}

void ClntObjMgrInitializeShared() {
  if (!s_heapsAllocated) {
    UINT objectType;

    for (objectType = 1; objectType < 8; ++objectType) {
      s_objHeapId[objectType - 1] = ObjectAllocAddHeap(s_objTotalSize[objectType], s_heapSizes[objectType], s_objNames[objectType]);
    }

    s_heapsAllocated = true;
    s_hashMemBlock = ObjectAllocAddHeap(sizeof(C_OBJECTHASH), 0x200, "C_OBJECTHASH");
  }

  MirrorInitialize();
  ConsoleCommandRegister("ObjUsage", CCommand_ObjUsage, GAME, 0);
}

void ClntObjMgrInitialize() {
  for (UINT i = 0; i < 64; ++i) {
    C_OBJECTHASH *hash = AllocNewObj();
    ASSERT(hash);
    s_curMgr->m_freeObjects.LinkNode(hash, LIST_TAIL, 0);
  }

  s_curMgr->m_net->SetMessageHandler(SMSG_UPDATE_OBJECT, ObjectUpdateHandler, 0);
  s_curMgr->m_net->SetMessageHandler(SMSG_COMPRESSED_UPDATE_OBJECT, ObjectCompressedUpdateHandler, 0);
  s_curMgr->m_net->SetMessageHandler(SMSG_DESTROY_OBJECT, OnObjectDestroy, 0);
}

void ClntObjMgrSetObjMirrorHandler(
    DWORDLONG guid,
    UINT      offset,
    UINT      bytes,
    int (*handler)(DWORDLONG, UINT, UINT, LPCVOID, LPVOID),
    LPVOID           param,
    HANDLER_PRIORITY priority
) {
  if (s_curMgr->m_callingMirrorHandlers) {
    OBJHANDLERREQUEST *request = s_curMgr->m_pendingObjHandlerRequests.NewNode(LIST_TAIL, 0, 0);
    request->set = 1;
    request->guid = guid;
    request->offset = offset;
    request->bytes = bytes;
    request->handler = handler;
    request->param = param;
    request->priority = priority;
    return;
  }

  C_OBJECTHASH *foundObj = FindActiveObj(guid);
  if (foundObj) {
    ActivityBegin(ACTIVITY_OBJMGR);
    CGObject_C *objectptr = (CGObject_C *)ObjectPtr(foundObj->memHandle);
    FATALASSERT(objectptr);
    OBJECT_TYPE_ID section = GetOffsetSectionId(objectptr->GetType(), offset);
    FATALASSERT(section < NUM_CLIENT_OBJECT_TYPES);
    AssignMirrorHandler(&foundObj->mirrorHandlers[offset >> 2], offset, bytes, handler, param, priority);
    ActivityEnd(ACTIVITY_OBJMGR);
  }
}

static BOOL MirrorHandlerRemoveQueued(DWORDLONG guid, CMirrorHandler *mirror) {
  ITERATELIST(OBJHANDLERREQUEST, s_curMgr->m_pendingObjHandlerRequests, request) {
    if (!request->set && request->handler == mirror->handler && request->guid == guid && request->offset == mirror->offset &&
        request->param == mirror->param)
    {
      return 1;
    }
  }
  return 0;
}

static void ProcessObjHandlersQueue() {
  FATALASSERT(!s_curMgr->m_callingMirrorHandlers);

  ITERATELIST(OBJHANDLERREQUEST, s_curMgr->m_pendingObjHandlerRequests, request) {
    if (request->set) {
      ClntObjMgrSetObjMirrorHandler(request->guid, request->offset, request->bytes, request->handler, request->param, request->priority);
    } else {
      ClntObjMgrUnsetObjMirrorHandler(request->guid, request->offset, request->handler, request->param);
    }
  }

  s_curMgr->m_pendingObjHandlerRequests.Clear();
}

void ClntObjMgrUnsetObjMirrorHandler(DWORDLONG guid, UINT offset, int (*handler)(DWORDLONG, UINT, UINT, LPCVOID, LPVOID), LPVOID param) {
  if (s_curMgr->m_callingMirrorHandlers) {
    OBJHANDLERREQUEST *request = s_curMgr->m_pendingObjHandlerRequests.NewNode(LIST_TAIL, 0, 0);
    request->set = 0;
    request->guid = guid;
    request->offset = offset;
    request->handler = handler;
    request->param = param;
    return;
  }

  C_OBJECTHASH *foundObj = FindActiveObj(guid);
  if (foundObj) {
    ActivityBegin(ACTIVITY_OBJMGR);
    CGObject_C *objectptr = (CGObject_C *)ObjectPtr(foundObj->memHandle);
    FATALASSERT(objectptr);
    OBJECT_TYPE_ID section = GetOffsetSectionId(objectptr->GetType(), offset);
    FATALASSERT(section < NUM_CLIENT_OBJECT_TYPES);
    UnassignMirrorHandler(&foundObj->mirrorHandlers[offset >> 2], handler, param);
    ActivityEnd(ACTIVITY_OBJMGR);
  }
}

void ClntObjMgrSetTypeMirrorHandler(
    OBJECT_TYPE hierType,
    UINT        offset,
    UINT        bytes,
    int (*handler)(DWORDLONG, UINT, UINT, LPCVOID, LPVOID),
    LPVOID           param,
    HANDLER_PRIORITY priority
) {
  ActivityBegin(ACTIVITY_OBJMGR);
  OBJECT_TYPE_ID section = GetSectionId(hierType);
  FATALASSERT(section < NUM_CLIENT_OBJECT_TYPES);
  AssignMirrorHandler(&s_mirrorHandlers[section][offset >> 2], offset, bytes, handler, param, priority);
  ActivityEnd(ACTIVITY_OBJMGR);
}

void ClntObjMgrUnsetTypeMirrorHandler(OBJECT_TYPE hierType, UINT offset, int (*handler)(DWORDLONG, UINT, UINT, LPCVOID, LPVOID)) {
  ActivityBegin(ACTIVITY_OBJMGR);
  OBJECT_TYPE_ID section = GetSectionId(hierType);
  FATALASSERT(section < NUM_CLIENT_OBJECT_TYPES);
  UnassignMirrorHandler(&s_mirrorHandlers[section][offset >> 2], handler, 0);
  ActivityEnd(ACTIVITY_OBJMGR);
}

void ClntObjMgrHideObject(DWORDLONG guid) {
  C_OBJECTHASH *foundObj = FindActiveObj(guid);
  if (foundObj) {
    ActivityBegin(ACTIVITY_OBJMGR);
    if (s_curMgr->m_visibleObjects.IsLinked(foundObj)) {
      s_curMgr->m_visibleObjects.UnlinkNode(foundObj);
    }
    ActivityEnd(ACTIVITY_OBJMGR);
  }
}

void ClntObjMgrShowObject(DWORDLONG guid) {
  C_OBJECTHASH *foundObj = FindActiveObj(guid);
  if (foundObj) {
    ActivityBegin(ACTIVITY_OBJMGR);
    if (!s_curMgr->m_visibleObjects.IsLinked(foundObj)) {
      s_curMgr->m_visibleObjects.LinkNode(foundObj, LIST_TAIL, 0);
    }
    ActivityEnd(ACTIVITY_OBJMGR);
  }
}

BOOL ClntObjMgrEnumVisibleObjects(BOOL (*handler)(DWORDLONG, LPVOID), LPVOID param) {
  ActivityBegin(ACTIVITY_OBJMGR);

  int success = 1;
  ITERATELIST(C_OBJECTHASH, s_curMgr->m_visibleObjects, object) {
    CHashKeyGUID hashKey = object->GetKey();
    if (!handler(hashKey.GetGUID(), param)) {
      success = 0;
      break;
    }
  }

  ActivityEnd(ACTIVITY_OBJMGR);
  return success;
}

void ClntObjMgrFreeObject(DWORDLONG guid) {
  ActivityBegin(ACTIVITY_OBJMGR);
  ClntObjMgrObjectOutOfRange(guid, 0);

  C_OBJECTHASH *foundObj = s_curMgr->m_lazyCleanupObjects.Ptr(guid, CHashKeyGUID(guid));
  if (foundObj) {
    s_curMgr->m_lazyCleanupObjects.Unlink(foundObj);
    s_curMgr->m_lazyCleanupFifo.UnlinkNode(foundObj);

    FreeObject(foundObj->memHandle);
    s_curMgr->m_freeObjects.LinkNode(foundObj, LIST_TAIL, 0);
  }
  ActivityEnd(ACTIVITY_OBJMGR);
}

CGObject_C *ClntObjMgrObjectPtr(DWORDLONG guid, LPCSTR fileName, UINT lineNumber) {
  CGObject_C *object;

  if (!s_curMgr) {
    return 0;
  }

  if (!s_curMgr->m_allowGuidDeref && guid != s_curMgr->m_legalGuidDeref) {
    FATALERROR(("Illegal client GUID derefence at %s:%d!", fileName, lineNumber));
  }

  ActivityBegin(ACTIVITY_OBJMGR);
  object = GetObjectPtr(guid);
  ActivityEnd(ACTIVITY_OBJMGR);
  return object;
}

void ClntObjMgrDestroyShared() {
  ConsoleCommandUnregister("ObjUsage");
}

void ClntObjMgrDestroy() {
  while (C_OBJECTHASH *hash = s_curMgr->m_objects.Head()) {
    CGObject_C *object = (CGObject_C *)ObjectPtr(hash->memHandle);
    FATALASSERT(object);
    ClntObjMgrObjectOutOfRange(object->GetGUID(), 1);
  }

  while (C_OBJECTHASH *hash = s_curMgr->m_lazyCleanupObjects.Head()) {
    s_curMgr->m_lazyCleanupObjects.Unlink(hash);
    s_curMgr->m_lazyCleanupFifo.UnlinkNode(hash);

    FreeObject(hash->memHandle);
    ClearObjectMirrorHandlers(hash);
  }

  while (!s_curMgr->m_freeObjects.IsEmpty()) {
    C_OBJECTHASH *hash = s_curMgr->m_freeObjects.Head();
    s_curMgr->m_freeObjects.UnlinkNode(hash);
    ClearObjectMirrorHandlers(hash);
    ObjectFree(hash->thisMemHandle);
  }

  for (UINT objectType = 0; objectType < 8; ++objectType) {
    for (UINT block = 0; block < 634; ++block) {
      s_mirrorHandlers[objectType][block].Clear();
    }
  }

  s_curMgr->m_net->ClearMessageHandler(SMSG_UPDATE_OBJECT);
  s_curMgr->m_net->ClearMessageHandler(SMSG_COMPRESSED_UPDATE_OBJECT);
  s_curMgr->m_net->ClearMessageHandler(SMSG_DESTROY_OBJECT);
}

DWORDLONG ClntObjMgrGetActivePlayer() {
  if (!s_curMgr) {
    return 0;
  }

  return s_curMgr->m_activePlayer;
}

void ClntObjMgrSetActivePlayer(DWORDLONG guid) {
  if (s_curMgr) {
    s_curMgr->m_activePlayer = guid;
  }
}

PLAYER_TYPE ClntObjMgrGetPlayerType() {
  return s_curMgr->m_type;
}

UINT ClntObjMgrGetMapID() {
  return s_curMgr->m_mapID;
}

void ClntObjMgrSetMapID(UINT mapID) {
  s_curMgr->m_mapID = mapID;
}

void ClntObjMgrSetNet(ClientConnection *net) {
  s_curMgr->m_net = net;
  ClientServices_SetCurrent(net);
}

ClientConnection *ClntObjMgrGetNet() {
  if (!s_curMgr) {
    return 0;
  }

  return s_curMgr->m_net;
}

LPVOID ClntObjMgrGetMovementGlobals() {
  if (!s_curMgr) {
    return 0;
  }

  return s_curMgr->m_movement;
}

void ClntObjMgrSetMovementGlobals(LPVOID ptr) {
  if (s_curMgr) {
    s_curMgr->m_movement = ptr;
  }
}

LPVOID ClntObjMgrGetClientPtr() {
  if (!s_curMgr) {
    return 0;
  }

  return s_curMgr->m_clientPtr;
}
