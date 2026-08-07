#include <Base/Base.h>
#include <WowConst.h>
#include <MapDefs.h>

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
#include "Object/mirror.h"
#include "ObjectAlloc/ObjectAllocTemplate.h"
#include "Ui/PartyFrame.h"
#include "WowServices/WDataStore.h"
#include "WowSvcs/WowSvcsClient/ClientConnection.h"
#include "WowSvcs/WowSvcsClient/ClientServices.h"

extern "C" int __stdcall zlib_uncompress(BYTE *dest, DWORD *destLen, const BYTE *source, DWORD sourceLen);

static ClntObjMgr *s_curMgr;
static UINT        s_hashMemBlock;
static const UINT  s_objTotalSize[8] = {
    0x48, 0xEC, 0x1B0, 0xCC0, 0x2248, 0xEC, 0x84, 0x10C,
};
static LPCSTR s_objNames[8] = {
    "CGObject_C", "CGItem_C", "CGContainer_C", "CGUnit_C", "CGPlayer_C", "CGGameObject_C", "CGDynamicObject_C", "CGCorpse_C",
};
static int s_heapSizes[8] = {
    0, 0x100, 0x20, 0x40, 0x40, 0x40, 0x20, 0x20,
};
static const UINT s_objMirrorBlocks[8] = {
    6, 36, 78, 184, 634, 20, 16, 36,
};
static BYTE s_heapsAllocated;
static UINT s_objHeapId[7];
static int  s_localPlayerUpdates;
static LISTDECL(CMirrorHandler, s_mirrorHandlers[8][634]);

static BOOL          MirrorHandlerRemoveQueued(DWORDLONG guid, CMirrorHandler *mirror);
static void          ProcessObjHandlersQueue();
static C_OBJECTHASH *AllocNewObj();
void                 SkipCreateObject(CDataStore *msg);

static BOOL IsMaskBitSet(const UINT *changeMask, UINT dwordNum) {
  return changeMask[dwordNum >> 5] & (1 << (dwordNum & 0x1F));
}

static void FillInPartialObjectData(C_OBJECTHASH *foundObj, CDataStore *msg, bool forFullUpdate, bool zeroZeroBits);

static void FillInObjectData(C_OBJECTHASH *objhash, CDataStore *msg, CClientObjCreate *init, OBJECT_TYPE_ID objTypeID) {
  FATALASSERT(msg);
  CGObject_C *obj = static_cast<CGObject_C *>(ObjectPtr(objhash->memHandle));
  FATALASSERT(obj);
  obj->SetTypeID(objTypeID);
  *msg >> init->move;
  msg->Get(init->flags);
  msg->Get(init->attackCycle);
  msg->Get(init->timerID);
  msg->Get(init->victim);
  FillInPartialObjectData(objhash, msg, 1, 1);
}

static UINT GetDataBaseOffset(OBJECT_TYPE_ID section) {
  switch (section) {
    case ID_ITEM:
    case ID_UNIT:
    case ID_GAMEOBJECT:
    case ID_DYNAMICOBJECT:
    case ID_CORPSE:
      return 24;
    case ID_CONTAINER:
      return 144;
    case ID_PLAYER:
      return 736;
    default:
      return 0;
  }
}

static void CallBlockMirrorHandlersIfChanged(LISTPTREX(CMirrorHandler) handlerList, DWORDLONG guid, CGObject_C *obj, OBJECT_TYPE_ID objectTypeId) {
  s_curMgr->m_callingMirrorHandlers = 1;

  ITERATELISTPTR(CMirrorHandler, handlerList, mirrorHandler) {
    mirrorHandler->blocksLeft = 1;
    const BYTE *data = reinterpret_cast<const BYTE *>(obj->GetData(0)) + mirrorHandler->offset;
    if (memcmp(data, mirrorHandler->previous.Ptr(), mirrorHandler->previous.Count()) && !MirrorHandlerRemoveQueued(guid, mirrorHandler)) {
      FATALASSERT(mirrorHandler->handler);
      mirrorHandler->handler(
          guid, mirrorHandler->offset - GetDataBaseOffset(objectTypeId), mirrorHandler->previous.Count(), mirrorHandler->previous.Ptr(),
          mirrorHandler->param
      );
    }
  }

  s_curMgr->m_callingMirrorHandlers = 0;
  ProcessObjHandlersQueue();
}

static void CallBlockMirrorHandlers(LISTPTREX(CMirrorHandler) handlerList, DWORDLONG guid, CGObject_C *obj, OBJECT_TYPE_ID objectTypeId) {
  s_curMgr->m_callingMirrorHandlers = 1;

  ITERATELISTPTR(CMirrorHandler, handlerList, mirrorHandler) {
    if (!MirrorHandlerRemoveQueued(guid, mirrorHandler)) {
      FATALASSERT(mirrorHandler->handler);
      const BYTE *data = reinterpret_cast<const BYTE *>(obj->GetData(0)) + mirrorHandler->offset;
      if (mirrorHandler->previous.Count() >= sizeof(DWORD) || memcmp(mirrorHandler->previous.Ptr(), data, mirrorHandler->previous.Count())) {
        mirrorHandler->handler(
            guid, mirrorHandler->offset - GetDataBaseOffset(objectTypeId), mirrorHandler->previous.Count(), mirrorHandler->previous.Ptr(),
            mirrorHandler->param
        );
      }
      mirrorHandler->blocksLeft = 1;
    }
  }

  s_curMgr->m_callingMirrorHandlers = 0;
  ProcessObjHandlersQueue();
}

static void SavePreviousValue(LISTPTR(CMirrorHandler) handlerList, CGObject_C *obj) {
  FATALASSERT(obj);
  ITERATELISTPTR(CMirrorHandler, handlerList, mirrorHandler) {
    const BYTE *data = reinterpret_cast<const BYTE *>(obj->GetData(0)) + mirrorHandler->offset;
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
    case HIER_TYPE_ITEM:
    case HIER_TYPE_CONTAINER:
      if (currTypeId == ID_OBJECT) {
        return ID_ITEM;
      }
      if (currTypeId == ID_ITEM) {
        return ID_CONTAINER;
      }
      break;
    case HIER_TYPE_UNIT:
    case HIER_TYPE_PLAYER:
      if (currTypeId == ID_OBJECT) {
        return ID_UNIT;
      }
      if (currTypeId == ID_UNIT) {
        return ID_PLAYER;
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
  CMirrorHandler *handler = handlerList->Head();
  while (handler) {
    CMirrorHandler *next = handlerList->Next(handler);
    if (handler->blocksLeft-- == 1) {
      handlerList->UnlinkNode(handler);
    }
    handler = next;
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
  UINT  changeMasks[20];
  DWORD junk;
  BYTE  updateMaskBlocks = 0;

  msg->Get(updateMaskBlocks);
  FATALASSERT(updateMaskBlocks <= 20);

  UINT block;
  for (block = 0; block < updateMaskBlocks; ++block) {
    msg->Get(changeMasks[block]);
  }

  for (block = 0; block < 32 * updateMaskBlocks; ++block) {
    if (IsMaskBitSet(changeMasks, block)) {
      msg->Get(junk);
    }
  }
}

static void PartialUpdateFromFullUpdate(DWORD eventTime, C_OBJECTHASH *foundObj, CDataStore *msg) {
  CClientObjCreate createData;
  CGObject_C      *object = static_cast<CGObject_C *>(ObjectPtr(foundObj->memHandle));
  FATALASSERT(object);

  if (object->IsA(TYPE_UNIT)) {
    createData.Get(msg);
    static_cast<CGUnit_C *>(object)->SetClientInitData(eventTime, createData, object->GetGUID() == ClntObjMgrGetActivePlayer());
  } else {
    CClientObjCreate::Skip(msg);
  }
  FillInPartialObjectData(foundObj, msg, 0, 1);
}

static void FillInPartialObjectData(C_OBJECTHASH *foundObj, CDataStore *msg, bool forFullUpdate, bool zeroZeroBits) {
  UINT changeMasks[20];
  LISTDECLEX(CMirrorHandler, callLink, handlerList);
  UINT        numBlocks;
  DWORD       block;
  UINT        blockOffset;
  UINT        objectTypeId;
  CGObject_C *obj;
  BYTE        updateMaskBlocks = 0;

  FATALASSERT(foundObj);
  FATALASSERT(msg);
  obj = static_cast<CGObject_C *>(ObjectPtr(foundObj->memHandle));
  FATALASSERT(obj);

  msg->Get(updateMaskBlocks);
  FATALASSERT(updateMaskBlocks <= 20);
  for (block = 0; block < updateMaskBlocks; ++block) {
    msg->Get(changeMasks[block]);
  }

  numBlocks = GetNumDwordBlocks(obj->GetType());
  blockOffset = 0;
  objectTypeId = ID_OBJECT;
  for (block = 0; block < numBlocks; ++block) {
    if (block >= s_objMirrorBlocks[objectTypeId]) {
      blockOffset = s_objMirrorBlocks[objectTypeId];
      objectTypeId = IncTypeId(obj, objectTypeId);
    }

    if (!forFullUpdate) {
      MirrorHandlerAdvanceBlock(&handlerList);
      if (GetMirrorHandler(&s_mirrorHandlers[objectTypeId][block - blockOffset], &handlerList)) {
        SavePreviousValue(&s_mirrorHandlers[objectTypeId][block - blockOffset], obj);
      }
      if (GetMirrorHandler(&foundObj->mirrorHandlers[block], &handlerList)) {
        SavePreviousValue(&foundObj->mirrorHandlers[block], obj);
      }
    }

    DWORD data = 0;
    if (IsMaskBitSet(changeMasks, block)) {
      msg->Get(data);
    } else if (!zeroZeroBits) {
      continue;
    }
    FATALASSERT(SetObjectBlock(obj, block, data));
  }
}

static void CallMirrorHandlers(CDataStore *msg, bool forFullUpdate, DWORDLONG guid) {
  UINT changeMasks[20];
  LISTDECLEX(CMirrorHandler, callLink, handlerList);
  DWORD         junk;
  UINT          numBlocks;
  C_OBJECTHASH *foundObj;
  CGObject_C   *obj;
  BYTE          updateMaskBlocks = 0;

  FATALASSERT(msg);
  if (!forFullUpdate) {
    msg->Get(guid);
  }

  foundObj = FindActiveObj(guid);
  if (!foundObj) {
    SkipPartialObjectUpdate(msg);
    return;
  }

  obj = static_cast<CGObject_C *>(ObjectPtr(foundObj->memHandle));
  FATALASSERT(obj);

  msg->Get(updateMaskBlocks);
  FATALASSERT(updateMaskBlocks <= 20);
  UINT block;
  for (block = 0; block < updateMaskBlocks; ++block) {
    msg->Get(changeMasks[block]);
  }

  numBlocks = GetNumDwordBlocks(obj->GetType());
  UINT blockOffset = 0;
  UINT objectTypeId = ID_OBJECT;
  for (block = 0; block < numBlocks; ++block) {
    if (block >= s_objMirrorBlocks[objectTypeId]) {
      blockOffset = s_objMirrorBlocks[objectTypeId];
      objectTypeId = IncTypeId(obj, objectTypeId);
    }

    MirrorHandlerAdvanceBlock(&handlerList);
    GetMirrorHandler(&s_mirrorHandlers[objectTypeId][block - blockOffset], &handlerList);
    GetMirrorHandler(&foundObj->mirrorHandlers[block], &handlerList);

    if (IsMaskBitSet(changeMasks, block)) {
      if (forFullUpdate) {
        CallBlockMirrorHandlersIfChanged(&handlerList, guid, obj, static_cast<OBJECT_TYPE_ID>(objectTypeId));
      } else {
        CallBlockMirrorHandlers(&handlerList, guid, obj, static_cast<OBJECT_TYPE_ID>(objectTypeId));
      }
      msg->Get(junk);
    }
  }
}

static OBJECT_TYPE_ID GetOffsetSectionId(OBJECT_TYPE hierType, UINT offset) {
  if (offset < 24) {
    return ID_OBJECT;
  }
  if ((hierType & TYPE_ITEM) && offset < 144) {
    return ID_ITEM;
  }
  if ((hierType & TYPE_CONTAINER) && offset < 312) {
    return ID_CONTAINER;
  }
  if ((hierType & TYPE_UNIT) && offset < 736) {
    return ID_UNIT;
  }
  if ((hierType & TYPE_PLAYER) && offset < 2536) {
    return ID_PLAYER;
  }
  if ((hierType & TYPE_GAMEOBJECT) && offset < 80) {
    return ID_GAMEOBJECT;
  }
  if ((hierType & TYPE_DYNAMICOBJECT) && offset < 64) {
    return ID_DYNAMICOBJECT;
  }
  if ((hierType & TYPE_CORPSE) && offset < 144) {
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
  LPVOID storage = ObjectPtr(memHandle);
  FATALASSERT(storage);

  CGObject_C *object = static_cast<CGObject_C *>(storage);
  if (init->flags & 1) {
    ClntObjMgrSetActivePlayer(object->GetGUID());
    CGPlayer_C::SetActive(static_cast<CGPlayer_C *>(object));
    CGPlayer_C::SetRealActivePlayer(object->GetGUID());
  }

  switch (type) {
    case ID_ITEM:
      new (storage) CGItem_C(reinterpret_cast<DWORD *>(static_cast<CGItem_C *>(storage) + 1), eventTime, init);
      break;
    case ID_CONTAINER:
      new (storage) CGContainer_C(reinterpret_cast<DWORD *>(static_cast<CGContainer_C *>(storage) + 1), eventTime, init);
      break;
    case ID_UNIT:
      new (storage) CGUnit_C(reinterpret_cast<DWORD *>(static_cast<CGUnit_C *>(storage) + 1), eventTime, init);
      break;
    case ID_PLAYER:
      new (storage) CGPlayer_C(reinterpret_cast<DWORD *>(static_cast<CGPlayer_C *>(storage) + 1), eventTime, init);
      break;
    case ID_GAMEOBJECT:
      new (storage) CGGameObject_C(reinterpret_cast<DWORD *>(static_cast<CGGameObject_C *>(storage) + 1), eventTime, init);
      break;
    case ID_DYNAMICOBJECT:
      new (storage) CGDynamicObject_C(reinterpret_cast<DWORD *>(static_cast<CGDynamicObject_C *>(storage) + 1), eventTime, init);
      break;
    case ID_CORPSE:
      new (storage) CGCorpse_C(reinterpret_cast<DWORD *>(static_cast<CGCorpse_C *>(storage) + 1), eventTime, init);
      break;
    default:
      FATALASSERT(0);
      break;
  }
}

void SkipCreateObject(CDataStore *msg) {
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

  return static_cast<CGObject_C *>(ObjectPtr(foundObj->memHandle));
}

static void PostInitObject(CDataStore *msg) {
  CClientObjCreate init;
  DWORDLONG        guid;
  OBJECT_TYPE_ID   type;
  BYTE             btype = 0;

  msg->Get(guid);
  msg->Get(btype);
  type = static_cast<OBJECT_TYPE_ID>(btype);
  CGObject_C *object = GetObjectPtr(guid);
  FATALASSERT(object);
  init.Get(msg);

  if (object->IsPostInited()) {
    if (type == ID_UNIT || type == ID_PLAYER) {
      static_cast<CGUnit_C *>(object)->PostSetClientInitData(init.move);
    }
    CallMirrorHandlers(msg, true, guid);
    return;
  }

  switch (type) {
    case ID_OBJECT:
      object->PostInit(init);
      break;
    case ID_ITEM:
    case ID_CONTAINER:
      static_cast<CGItem_C *>(object)->PostInit(init);
      break;
    case ID_UNIT:
      static_cast<CGUnit_C *>(object)->PostInit(init);
      break;
    case ID_PLAYER:
      static_cast<CGPlayer_C *>(object)->PostInit(init);
      break;
    case ID_GAMEOBJECT:
      static_cast<CGGameObject_C *>(object)->PostInit(init);
      break;
    case ID_DYNAMICOBJECT:
      static_cast<CGDynamicObject_C *>(object)->PostInit(init);
      break;
    case ID_CORPSE:
      static_cast<CGCorpse_C *>(object)->PostInit(init);
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
  C_OBJECTHASH *foundObj = FindActiveObj(guid);
  if (foundObj) {
    return foundObj;
  }

  CHashKeyGUID hashKey(guid);
  foundObj = s_curMgr->m_lazyCleanupObjects.Ptr(guid, hashKey);
  if (!foundObj) {
    return 0;
  }

  s_curMgr->m_lazyCleanupObjects.Unlink(foundObj);
  s_curMgr->m_lazyCleanupFifo.UnlinkNode(foundObj);
  s_curMgr->m_objects.Insert(foundObj, guid, hashKey);
  CGObject_C *object = static_cast<CGObject_C *>(ObjectPtr(foundObj->memHandle));
  if (!s_curMgr->m_visibleObjects.IsLinked(foundObj)) {
    s_curMgr->m_visibleObjects.LinkNode(foundObj, LIST_TAIL, 0);
    object->Reenable();
    s_curMgr->m_reenabledObjects.LinkNode(foundObj, LIST_TAIL, 0);
  }
  return foundObj;
}

static void SetupObjectStorage(OBJECT_TYPE_ID type, UINT memHandle) {
  BYTE *storage = static_cast<BYTE *>(ObjectPtr(memHandle));

  static const UINT objectSizes[8] = {
      sizeof(CGObject_C), sizeof(CGItem_C),       sizeof(CGContainer_C),     sizeof(CGUnit_C),
      sizeof(CGPlayer_C), sizeof(CGGameObject_C), sizeof(CGDynamicObject_C), sizeof(CGCorpse_C),
  };
  static const UINT totalFields[8] = {
      CGObject::TotalFields(), CGItem::TotalFields(),       CGContainer::TotalFields(),     CGUnit::TotalFields(),
      CGPlayer::TotalFields(), CGGameObject::TotalFields(), CGDynamicObject::TotalFields(), CGCorpse::TotalFields(),
  };

  DWORD *data;
  switch (type) {
    case ID_OBJECT:
      data = reinterpret_cast<DWORD *>(storage + objectSizes[ID_OBJECT]);
      static_cast<CGObject_C *>(static_cast<LPVOID>(storage))->SetStorage(data);
      break;
    case ID_ITEM:
      data = reinterpret_cast<DWORD *>(storage + objectSizes[ID_ITEM]);
      static_cast<CGItem_C *>(static_cast<LPVOID>(storage))->SetStorage(data);
      break;
    case ID_CONTAINER:
      data = reinterpret_cast<DWORD *>(storage + objectSizes[ID_CONTAINER]);
      static_cast<CGContainer_C *>(static_cast<LPVOID>(storage))->SetStorage(data);
      break;
    case ID_UNIT:
      data = reinterpret_cast<DWORD *>(storage + objectSizes[ID_UNIT]);
      static_cast<CGUnit_C *>(static_cast<LPVOID>(storage))->SetStorage(data);
      break;
    case ID_PLAYER:
      data = reinterpret_cast<DWORD *>(storage + objectSizes[ID_PLAYER]);
      static_cast<CGPlayer_C *>(static_cast<LPVOID>(storage))->SetStorage(data);
      break;
    case ID_GAMEOBJECT:
      data = reinterpret_cast<DWORD *>(storage + objectSizes[ID_GAMEOBJECT]);
      static_cast<CGGameObject_C *>(static_cast<LPVOID>(storage))->SetStorage(data);
      break;
    case ID_DYNAMICOBJECT:
      data = reinterpret_cast<DWORD *>(storage + objectSizes[ID_DYNAMICOBJECT]);
      static_cast<CGDynamicObject_C *>(static_cast<LPVOID>(storage))->SetStorage(data);
      break;
    case ID_CORPSE:
      data = reinterpret_cast<DWORD *>(storage + objectSizes[ID_CORPSE]);
      static_cast<CGCorpse_C *>(static_cast<LPVOID>(storage))->SetStorage(data);
      break;
    default:
      FATALASSERT(0);
      return;
  }
  memset(storage + objectSizes[type], 0, totalFields[type] * sizeof(DWORD));
}

static C_OBJECTHASH *AllocNewObj() {
  UINT memHandle;

  if (!ObjectAlloc(s_hashMemBlock, &memHandle)) {
    return 0;
  }

  C_OBJECTHASH *hash = static_cast<C_OBJECTHASH *>(ObjectPtr(memHandle));
  if (!hash) {
    return 0;
  }

  new (hash) C_OBJECTHASH;
  hash->thisMemHandle = memHandle;
  return hash;
}

static BOOL CreateObject(DWORD eventTime, CDataStore *msg) {
  CClientObjCreate init;
  DWORDLONG        guid;
  UINT             memHandle;
  OBJECT_TYPE_ID   type;
  BYTE             btype = 0;

  FATALASSERT(msg);
  msg->Get(guid);
  s_curMgr->m_legalGuidDeref = guid;
  msg->Get(btype);
  type = static_cast<OBJECT_TYPE_ID>(btype);

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
    if (foundObj) {
      s_curMgr->m_lazyCleanupObjects.Unlink(foundObj);
      s_curMgr->m_lazyCleanupFifo.UnlinkNode(foundObj);

      CGObject_C *object = static_cast<CGObject_C *>(ObjectPtr(foundObj->memHandle));
      FATALASSERT(object);
      switch (object->GetType()) {
        case HIER_TYPE_OBJECT:
          object->~CGObject_C();
          break;
        case HIER_TYPE_ITEM:
          static_cast<CGItem_C *>(object)->~CGItem_C();
          break;
        case HIER_TYPE_CONTAINER:
          static_cast<CGContainer_C *>(object)->~CGContainer_C();
          break;
        case HIER_TYPE_UNIT:
          static_cast<CGUnit_C *>(object)->~CGUnit_C();
          break;
        case HIER_TYPE_PLAYER:
          static_cast<CGPlayer_C *>(object)->~CGPlayer_C();
          break;
        case HIER_TYPE_GAMEOBJECT:
          static_cast<CGGameObject_C *>(object)->~CGGameObject_C();
          break;
        case HIER_TYPE_DYNAMICOBJECT:
          static_cast<CGDynamicObject_C *>(object)->~CGDynamicObject_C();
          break;
        case HIER_TYPE_CORPSE:
          static_cast<CGCorpse_C *>(object)->~CGCorpse_C();
          break;
        default:
          FATALASSERT(0);
          break;
      }
      ObjectFree(foundObj->memHandle);
    } else {
      foundObj = AllocNewObj();
      if (!foundObj) {
        return 0;
      }
      SysMsgAdd("NOFREEOBJECTSALLOCATING", SYSMSG_INFO, 0x20);
    }
  }

  if (!ObjectAlloc(s_objHeapId[type - 1], &memHandle)) {
    s_curMgr->m_freeObjects.LinkNode(foundObj, LIST_TAIL, 0);
    SkipCreateObject(msg);
    return 0;
  }

  foundObj->memHandle = memHandle;
  CHashKeyGUID hashKey(guid);
  s_curMgr->m_objects.Insert(foundObj, guid, hashKey);
  SetupObjectStorage(type, memHandle);
  FillInObjectData(foundObj, msg, &init, type);
  s_curMgr->m_visibleObjects.LinkNode(foundObj, LIST_TAIL, 0);
  InitObject(eventTime, type, memHandle, &init);
  CreateMessage(type);
  return 1;
}

static BOOL UpdateObjectMovement(DWORD eventTime, CDataStore *msg) {
  CClientMoveUpdate update;
  DWORDLONG         guid;

  msg->Get(guid);
  s_curMgr->m_legalGuidDeref = guid;
  *msg >> update;
  if (guid == ClntObjMgrGetActivePlayer()) {
    return 1;
  }

  C_OBJECTHASH *foundObj = GetUpdateObject(guid);
  if (!foundObj) {
    return 0;
  }
  CGObject_C *object = static_cast<CGObject_C *>(ObjectPtr(foundObj->memHandle));
  FATALASSERT(object);
  FATALASSERT(object->IsA(TYPE_UNIT));
  static_cast<CGUnit_C *>(object)->UpdateMoveInfo(eventTime, update);
  return 1;
}

static BOOL UpdateObject(CDataStore *msg) {
  DWORDLONG guid;
  msg->Get(guid);
  s_curMgr->m_legalGuidDeref = guid;
  if (guid == ClntObjMgrGetActivePlayer()) {
    ++s_localPlayerUpdates;
  }

  C_OBJECTHASH *foundObj = GetUpdateObject(guid);
  if (!foundObj) {
    SkipPartialObjectUpdate(msg);
    return 0;
  }
  FillInPartialObjectData(foundObj, msg, 0, 0);
  return 1;
}

static void PostMovementUpdate(CDataStore *msg) {
  CClientMoveUpdate update;
  DWORDLONG         guid;
  msg->Get(guid);
  *msg >> update;
  CGObject_C *object = GetObjectPtr(guid);
  if (object && object->IsA(TYPE_UNIT)) {
    static_cast<CGUnit_C *>(object)->PostMovementUpdate(update);
  }
}

static void OutOfRangeMessage(DWORDLONG guid) {
  static LPCSTR messages[8] = {
      "Forgetting object", "Forgetting item",        "Forgetting container",      "Forgetting unit",
      "Forgetting player", "Forgetting game object", "Forgetting dynamic object", "Forgetting corpse",
  };
  CGObject_C *object = GetObjectPtr(guid);
  if (object) {
    SysMsgAdd(messages[GetSectionId(object->GetType())], SYSMSG_INFO, 0x20);
  }
}

static void InRangeMessage(DWORDLONG guid) {
  static LPCSTR messages[8] = {
      "Remembering object", "Remembering item",        "Remembering container",      "Remembering unit",
      "Remembering player", "Remembering game object", "Remembering dynamic object", "Remembering corpse",
  };
  CGObject_C *object = GetObjectPtr(guid);
  if (object) {
    SysMsgAdd(messages[GetSectionId(object->GetType())], SYSMSG_INFO, 0x20);
  }
}

void ClntObjMgrObjectInRange(DWORDLONG guid) {
  C_OBJECTHASH *foundObj = GetUpdateObject(guid);
  if (foundObj) {
    CGObject_C *object = static_cast<CGObject_C *>(ObjectPtr(foundObj->memHandle));
    FATALASSERT(object);
    InRangeMessage(guid);
  }
}

static void UpdateInRangeObjects(CDataStore *msg) {
  DWORDLONG guid;
  UINT      count;
  msg->Get(count);
  FATALASSERT(count);
  for (UINT i = 0; i < count; ++i) {
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
  FATALASSERT(count);
  for (UINT i = 0; i < count; ++i) {
    msg->Get(guid);
    if (guid != ClntObjMgrGetActivePlayer()) {
      ClntObjMgrObjectOutOfRange(guid, 0);
    }
  }
}

static void ClearObjectMirrorHandlers(C_OBJECTHASH *foundObj) {
  for (UINT i = 0; i < 634; ++i) {
    while (CMirrorHandler *handler = foundObj->mirrorHandlers[i].Head()) {
      foundObj->mirrorHandlers[i].UnlinkNode(handler);
      DEL(handler);
    }
  }
}

void ClntObjMgrObjectOutOfRange(DWORDLONG guid, int shutdown) {
  C_OBJECTHASH *foundObj = FindActiveObj(guid);
  if (!foundObj) {
    return;
  }

  OutOfRangeMessage(guid);
  CGObject_C *object = static_cast<CGObject_C *>(ObjectPtr(foundObj->memHandle));
  FATALASSERT(object);
  object->Disable(shutdown);
  ClearObjectMirrorHandlers(foundObj);
  s_curMgr->m_objects.Unlink(foundObj);
  if (s_curMgr->m_visibleObjects.IsLinked(foundObj)) {
    s_curMgr->m_visibleObjects.UnlinkNode(foundObj);
  }
  CHashKeyGUID hashKey(guid);
  s_curMgr->m_lazyCleanupObjects.Insert(foundObj, guid, hashKey);
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
  BYTE      marker1 = 0;
  int       success;
  UINT      numObjUpdates;
  BYTE      updateType = 0;

  msg->Get(numObjUpdates);
  UINT marker = msg->Tell();
  msg->Get(marker1);
  UINT firstUpdate = 0;
  if (marker1 == 3) {
    UpdateOutOfRangeObjects(msg);
    firstUpdate = 1;
  } else {
    msg->Seek(marker);
  }

  s_curMgr->m_allowGuidDeref = 0;
  s_localPlayerUpdates = 0;
  oldActive = ClntObjMgrGetActivePlayer();
  success = 1;
  UINT i;
  for (i = firstUpdate; i < numObjUpdates; ++i) {
    s_curMgr->m_legalGuidDeref = 0;
    msg->Get(updateType);
    switch (updateType) {
      case 0:
        if (!UpdateObject(msg)) {
          FATALASSERT(0);
          success = 0;
        } else {
          SysMsgAdd("Updating unit data", SYSMSG_INFO, 0x20);
        }
        break;
      case 1:
        if (!UpdateObjectMovement(eventTime, msg)) {
          FATALASSERT(0);
          success = 0;
        } else {
          SysMsgAdd("Updating unit movement", SYSMSG_INFO, 0x20);
        }
        break;
      case 2:
        if (!CreateObject(eventTime, msg)) {
          SysMsgAdd("OBJECTCREATIONFAILURE", SYSMSG_FATAL, 0x20);
          FATALASSERT(0);
          success = 0;
        }
        break;
      case 4:
        UpdateInRangeObjects(msg);
        break;
      default:
        FATALASSERT(0);
        success = 0;
        break;
    }
  }
  s_curMgr->m_allowGuidDeref = 1;
  FATALASSERT(s_localPlayerUpdates <= 1);

  msg->Seek(marker);
  for (i = 0; i < numObjUpdates; ++i) {
    updateType = 0;
    msg->Get(updateType);
    switch (updateType) {
      case 0:
        CallMirrorHandlers(msg, false, 0);
        break;
      case 1:
        PostMovementUpdate(msg);
        break;
      case 2:
        PostInitObject(msg);
        break;
      case 3:
      case 4:
        SkipSetOfObjects(msg);
        break;
      default:
        FATALASSERT(0);
        break;
    }
  }

  while (C_OBJECTHASH *foundObj = s_curMgr->m_reenabledObjects.Head()) {
    s_curMgr->m_reenabledObjects.UnlinkNode(foundObj);
    CGObject_C *object = static_cast<CGObject_C *>(ObjectPtr(foundObj->memHandle));
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
  WDataStore realmsg;
  LPVOID     data;
  UINT       origSize;
  DWORD      destSize;

  msg->Get(origSize);
  UINT compressedSize = msg->Size() - msg->Tell();
  msg->GetDataInSitu(data, compressedSize);
  LPVOID dest = _alloca(origSize);
  destSize = origSize;
  zlib_uncompress(static_cast<BYTE *>(dest), &destSize, static_cast<const BYTE *>(data), compressedSize);
  FATALASSERT(destSize == origSize);
  realmsg.PutData(dest, destSize);
  realmsg.Finalize();
  return ObjectUpdateHandler(0, MSG_NULL_ACTION, eventTime, &realmsg);
}

static void AssignMirrorHandler(
    LISTPTR(CMirrorHandler) handlerList,
    UINT offset,
    UINT bytes,
    int (*handler)(DWORDLONG, UINT, UINT, LPCVOID, LPVOID),
    LPVOID           param,
    HANDLER_PRIORITY priority
) {
  CMirrorHandler *mirror = NEW(CMirrorHandler);
  handlerList->LinkNode(mirror, LIST_TAIL, 0);
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
  UINT numVisible = 0;
  UINT numActive = 0;
  UINT numWaiting = 0;
  UINT numFree = 0;

  {
    ITERATELIST(C_OBJECTHASH, s_curMgr->m_visibleObjects, object) {
      ++numVisible;
    }
  }

  {
    ITERATELIST(C_OBJECTHASH, s_curMgr->m_objects, object) {
      ++numActive;
    }
  }

  {
    ITERATELIST(C_OBJECTHASH, s_curMgr->m_lazyCleanupObjects, object) {
      ++numWaiting;
    }
  }

  {
    ITERATELIST(C_OBJECTHASH, s_curMgr->m_freeObjects, object) {
      ++numFree;
    }
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
    OBJHANDLERREQUEST *request = NEW(OBJHANDLERREQUEST);
    s_curMgr->m_pendingObjHandlerRequests.LinkNode(request, LIST_TAIL, 0);
    request->guid = guid;
    request->offset = offset;
    request->bytes = bytes;
    request->handler = handler;
    request->param = param;
    request->priority = priority;
    request->set = 1;
    return;
  }

  C_OBJECTHASH *foundObj = FindActiveObj(guid);
  if (foundObj) {
    ActivityBegin(ACTIVITY_OBJMGR);
    CGObject_C *object = static_cast<CGObject_C *>(ObjectPtr(foundObj->memHandle));
    FATALASSERT(object);
    FATALASSERT(GetOffsetSectionId(object->GetType(), offset) < NUM_CLIENT_OBJECT_TYPES);
    AssignMirrorHandler(&foundObj->mirrorHandlers[offset >> 2], offset, bytes, handler, param, priority);
    ActivityEnd(ACTIVITY_OBJMGR);
  }
}

static BOOL MirrorHandlerRemoveQueued(DWORDLONG guid, CMirrorHandler *mirror) {
  ITERATELIST(OBJHANDLERREQUEST, s_curMgr->m_pendingObjHandlerRequests, request) {
    if (!request->set && request->offset == mirror->offset && request->guid == guid && request->handler == mirror->handler &&
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

  while (OBJHANDLERREQUEST *request = s_curMgr->m_pendingObjHandlerRequests.Head()) {
    s_curMgr->m_pendingObjHandlerRequests.UnlinkNode(request);
    DEL(request);
  }
}

void ClntObjMgrUnsetObjMirrorHandler(DWORDLONG guid, UINT offset, int (*handler)(DWORDLONG, UINT, UINT, LPCVOID, LPVOID), LPVOID param) {
  if (s_curMgr->m_callingMirrorHandlers) {
    OBJHANDLERREQUEST *request = NEW(OBJHANDLERREQUEST);
    s_curMgr->m_pendingObjHandlerRequests.LinkNode(request, LIST_TAIL, 0);
    request->guid = guid;
    request->handler = handler;
    request->param = param;
    request->set = 0;
    request->offset = offset;
    return;
  }

  C_OBJECTHASH *foundObj = FindActiveObj(guid);
  if (foundObj) {
    ActivityBegin(ACTIVITY_OBJMGR);
    CGObject_C *object = static_cast<CGObject_C *>(ObjectPtr(foundObj->memHandle));
    FATALASSERT(object);
    FATALASSERT(GetOffsetSectionId(object->GetType(), offset) < NUM_CLIENT_OBJECT_TYPES);
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
    if (!handler(object->GetKey().GetGUID(), param)) {
      success = 0;
      break;
    }
  }

  ActivityEnd(ACTIVITY_OBJMGR);
  return success;
}

void ClntObjMgrFreeObject(DWORDLONG guid) {
  ActivityBegin(ACTIVITY_OBJMGR);
  ClntObjMgrObjectOutOfRange(guid, 1);

  CHashKeyGUID  hashKey(guid);
  C_OBJECTHASH *foundObj = s_curMgr->m_lazyCleanupObjects.Ptr(guid, hashKey);
  if (foundObj) {
    s_curMgr->m_lazyCleanupObjects.Unlink(foundObj);
    s_curMgr->m_lazyCleanupFifo.UnlinkNode(foundObj);

    CGObject_C *object = static_cast<CGObject_C *>(ObjectPtr(foundObj->memHandle));
    FATALASSERT(object);
    switch (object->GetType()) {
      case HIER_TYPE_OBJECT:
        object->~CGObject_C();
        break;
      case HIER_TYPE_ITEM:
        static_cast<CGItem_C *>(object)->~CGItem_C();
        break;
      case HIER_TYPE_CONTAINER:
        static_cast<CGContainer_C *>(object)->~CGContainer_C();
        break;
      case HIER_TYPE_UNIT:
        static_cast<CGUnit_C *>(object)->~CGUnit_C();
        break;
      case HIER_TYPE_PLAYER:
        static_cast<CGPlayer_C *>(object)->~CGPlayer_C();
        break;
      case HIER_TYPE_GAMEOBJECT:
        static_cast<CGGameObject_C *>(object)->~CGGameObject_C();
        break;
      case HIER_TYPE_DYNAMICOBJECT:
        static_cast<CGDynamicObject_C *>(object)->~CGDynamicObject_C();
        break;
      case HIER_TYPE_CORPSE:
        static_cast<CGCorpse_C *>(object)->~CGCorpse_C();
        break;
      default:
        FATALASSERT(0);
        break;
    }

    ObjectFree(foundObj->memHandle);
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
    CGObject_C *object = static_cast<CGObject_C *>(ObjectPtr(hash->memHandle));
    FATALASSERT(object);
    ClntObjMgrObjectOutOfRange(object->GetGUID(), 1);
  }

  while (C_OBJECTHASH *hash = s_curMgr->m_lazyCleanupFifo.Head()) {
    s_curMgr->m_lazyCleanupObjects.Unlink(hash);
    s_curMgr->m_lazyCleanupFifo.UnlinkNode(hash);

    CGObject_C *object = static_cast<CGObject_C *>(ObjectPtr(hash->memHandle));
    FATALASSERT(object);
    switch (object->GetType()) {
      case HIER_TYPE_OBJECT:
        object->~CGObject_C();
        break;
      case HIER_TYPE_ITEM:
        static_cast<CGItem_C *>(object)->~CGItem_C();
        break;
      case HIER_TYPE_CONTAINER:
        static_cast<CGContainer_C *>(object)->~CGContainer_C();
        break;
      case HIER_TYPE_UNIT:
        static_cast<CGUnit_C *>(object)->~CGUnit_C();
        break;
      case HIER_TYPE_PLAYER:
        static_cast<CGPlayer_C *>(object)->~CGPlayer_C();
        break;
      case HIER_TYPE_GAMEOBJECT:
        static_cast<CGGameObject_C *>(object)->~CGGameObject_C();
        break;
      case HIER_TYPE_DYNAMICOBJECT:
        static_cast<CGDynamicObject_C *>(object)->~CGDynamicObject_C();
        break;
      case HIER_TYPE_CORPSE:
        static_cast<CGCorpse_C *>(object)->~CGCorpse_C();
        break;
      default:
        FATALASSERT(0);
        break;
    }

    ObjectFree(hash->memHandle);
    ClearObjectMirrorHandlers(hash);
  }

  while (C_OBJECTHASH *hash = s_curMgr->m_freeObjects.Head()) {
    s_curMgr->m_freeObjects.UnlinkNode(hash);
    ClearObjectMirrorHandlers(hash);
    ObjectFree(hash->thisMemHandle);
  }

  for (UINT objectType = 0; objectType < 8; ++objectType) {
    for (UINT block = 0; block < 634; ++block) {
      while (CMirrorHandler *handler = s_mirrorHandlers[objectType][block].Head()) {
        s_mirrorHandlers[objectType][block].UnlinkNode(handler);
        DEL(handler);
      }
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
