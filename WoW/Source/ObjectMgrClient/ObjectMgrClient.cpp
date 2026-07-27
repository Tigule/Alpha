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

extern "C" int __stdcall
zlib_uncompress(unsigned char *dest, unsigned long *destLen, const unsigned char *source, unsigned long sourceLen);

static ClntObjMgr        *s_curMgr;
static unsigned int       s_hashMemBlock;
static const unsigned int s_objTotalSize[8] = {
    0x48, 0xEC, 0x1B0, 0xCC0, 0x2248, 0xEC, 0x84, 0x10C,
};
static const char *s_objNames[8] = {
    "CGObject_C", "CGItem_C", "CGContainer_C", "CGUnit_C", "CGPlayer_C", "CGGameObject_C", "CGDynamicObject_C", "CGCorpse_C",
};
static int s_heapSizes[8] = {
    0, 0x100, 0x20, 0x40, 0x40, 0x40, 0x20, 0x20,
};
static const unsigned int s_objMirrorBlocks[8] = {
    6, 36, 78, 184, 634, 20, 16, 36,
};
static unsigned int                                       s_heapsAllocated;
static unsigned int                                       s_objHeapId[7];
static int                                                s_localPlayerUpdates;
static TSList<CMirrorHandler, TSGetLink<CMirrorHandler> > s_mirrorHandlers[8][634];

static int MirrorHandlerRemoveQueued(unsigned __int64 guid, CMirrorHandler *mirror);
static void ProcessObjHandlersQueue();
static C_OBJECTHASH *AllocNewObj();
void SkipCreateObject(CDataStore *msg);

static int IsMaskBitSet(const unsigned int *changeMask, unsigned int dwordNum) {
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

static unsigned int GetDataBaseOffset(OBJECT_TYPE_ID section) {
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

static void CallBlockMirrorHandlersIfChanged(
    TSList<CMirrorHandler, TSGetExplicitLink<CMirrorHandler> > *handlerList,
    unsigned __int64                                            guid,
    CGObject_C                                                 *obj,
    OBJECT_TYPE_ID                                              objectTypeId
) {
  s_curMgr->m_callingMirrorHandlers = 1;

  for (CMirrorHandler *mirrorHandler = handlerList->Head(); mirrorHandler; mirrorHandler = handlerList->Next(mirrorHandler)) {
    mirrorHandler->blocksLeft = 1;
    const unsigned char *data = reinterpret_cast<const unsigned char *>(obj->GetData(0)) + mirrorHandler->offset;
    if (memcmp(data, mirrorHandler->previous.Ptr(), mirrorHandler->previous.Count()) &&
        !MirrorHandlerRemoveQueued(guid, mirrorHandler))
    {
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

static void CallBlockMirrorHandlers(
    TSList<CMirrorHandler, TSGetExplicitLink<CMirrorHandler> > *handlerList,
    unsigned __int64                                            guid,
    CGObject_C                                                 *obj,
    OBJECT_TYPE_ID                                              objectTypeId
) {
  s_curMgr->m_callingMirrorHandlers = 1;

  for (CMirrorHandler *mirrorHandler = handlerList->Head(); mirrorHandler; mirrorHandler = handlerList->Next(mirrorHandler)) {
    if (!MirrorHandlerRemoveQueued(guid, mirrorHandler)) {
      FATALASSERT(mirrorHandler->handler);
      const unsigned char *data = reinterpret_cast<const unsigned char *>(obj->GetData(0)) + mirrorHandler->offset;
      if (mirrorHandler->previous.Count() >= sizeof(unsigned long) ||
          memcmp(mirrorHandler->previous.Ptr(), data, mirrorHandler->previous.Count()))
      {
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

static void SavePreviousValue(TSList<CMirrorHandler, TSGetLink<CMirrorHandler> > *handlerList, CGObject_C *obj) {
  FATALASSERT(obj);
  for (CMirrorHandler *mirrorHandler = handlerList->Head(); mirrorHandler; mirrorHandler = handlerList->Next(mirrorHandler)) {
    const unsigned char *data = reinterpret_cast<const unsigned char *>(obj->GetData(0)) + mirrorHandler->offset;
    memcpy(mirrorHandler->previous.Ptr(), data, mirrorHandler->previous.Count());
  }
}

static C_OBJECTHASH *FindActiveObj(unsigned __int64 guid) {
  return s_curMgr->m_objects.Ptr(guid, CHashKeyGUID(guid));
}

static int SetObjectBlock(CGObject_C *obj, unsigned int i, unsigned long data) {
  FATALASSERT(obj);
  return obj->SetBlock(i, data);
}

static unsigned int IncTypeId(CGObject_C *obj, unsigned int currTypeId) {
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

static void MirrorHandlerAdvanceBlock(TSList<CMirrorHandler, TSGetExplicitLink<CMirrorHandler> > *handlerList) {
  CMirrorHandler *handler = handlerList->Head();
  while (handler) {
    CMirrorHandler *next = handlerList->Next(handler);
    if (handler->blocksLeft-- == 1) {
      handlerList->UnlinkNode(handler);
    }
    handler = next;
  }
}

static int GetMirrorHandler(
    TSList<CMirrorHandler, TSGetLink<CMirrorHandler> >         *mirrorHandlers,
    TSList<CMirrorHandler, TSGetExplicitLink<CMirrorHandler> > *handlerList
) {
  unsigned int offDword;

  FATALASSERT(handlerList);
  if (!mirrorHandlers || mirrorHandlers->IsEmpty()) {
    return 0;
  }

  for (CMirrorHandler *handler = mirrorHandlers->Head(); handler; handler = mirrorHandlers->Next(handler)) {
    offDword = handler->offset & 3;
    handlerList->LinkNode(handler, handler->priority == HANDLER_PRIORITY_HIGH ? LIST_HEAD : LIST_TAIL, 0);
    handler->blocksLeft = (handler->previous.Count() + offDword + 3) >> 2;
  }

  return 1;
}

static unsigned int GetNumDwordBlocks(OBJECT_TYPE objType) {
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
  unsigned int  changeMasks[20];
  unsigned long junk;
  unsigned int  updateMaskBlocks = 0;

  msg->Get(*reinterpret_cast<unsigned char *>(&updateMaskBlocks));
  FATALASSERT(updateMaskBlocks <= 20);

  unsigned int block;
  for (block = 0; block < updateMaskBlocks; ++block) {
    msg->Get(changeMasks[block]);
  }

  for (block = 0; block < 32 * updateMaskBlocks; ++block) {
    if (IsMaskBitSet(changeMasks, block)) {
      msg->Get(junk);
    }
  }
}

static void PartialUpdateFromFullUpdate(unsigned long eventTime, C_OBJECTHASH *foundObj, CDataStore *msg) {
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
  unsigned int                      changeMasks[20];
  TSExplicitList<CMirrorHandler, 8> handlerList;
  unsigned int                      numBlocks;
  unsigned long                     block;
  unsigned int                      blockOffset;
  unsigned int                      objectTypeId;
  CGObject_C                       *obj;
  unsigned int                      updateMaskBlocks = 0;

  FATALASSERT(foundObj);
  FATALASSERT(msg);
  obj = static_cast<CGObject_C *>(ObjectPtr(foundObj->memHandle));
  FATALASSERT(obj);

  msg->Get(*reinterpret_cast<unsigned char *>(&updateMaskBlocks));
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

    unsigned long data = 0;
    if (IsMaskBitSet(changeMasks, block)) {
      msg->Get(data);
    } else if (!zeroZeroBits) {
      continue;
    }
    FATALASSERT(SetObjectBlock(obj, block, data));
  }
}

static void CallMirrorHandlers(CDataStore *msg, bool forFullUpdate, unsigned __int64 guid) {
  unsigned int                      changeMasks[20];
  TSExplicitList<CMirrorHandler, 8> handlerList;
  unsigned long                     junk;
  unsigned int                      numBlocks;
  C_OBJECTHASH                     *foundObj;
  CGObject_C                       *obj;
  unsigned int                      updateMaskBlocks = 0;

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

  msg->Get(*reinterpret_cast<unsigned char *>(&updateMaskBlocks));
  FATALASSERT(updateMaskBlocks <= 20);
  unsigned int block;
  for (block = 0; block < updateMaskBlocks; ++block) {
    msg->Get(changeMasks[block]);
  }

  numBlocks = GetNumDwordBlocks(obj->GetType());
  unsigned int blockOffset = 0;
  unsigned int objectTypeId = ID_OBJECT;
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

static OBJECT_TYPE_ID GetOffsetSectionId(OBJECT_TYPE hierType, unsigned int offset) {
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

static void InitObject(unsigned long eventTime, OBJECT_TYPE_ID type, unsigned int memHandle, CClientObjCreate *init) {
  void *storage = ObjectPtr(memHandle);
  FATALASSERT(storage);

  CGObject_C *object = static_cast<CGObject_C *>(storage);
  if (init->flags & 1) {
    ClntObjMgrSetActivePlayer(object->GetGUID());
    CGPlayer_C::SetActive(static_cast<CGPlayer_C *>(object));
    CGPlayer_C::SetRealActivePlayer(object->GetGUID());
  }

  switch (type) {
    case ID_ITEM:
      new (storage) CGItem_C(reinterpret_cast<unsigned long *>(static_cast<CGItem_C *>(storage) + 1), eventTime, init);
      break;
    case ID_CONTAINER:
      new (storage) CGContainer_C(reinterpret_cast<unsigned long *>(static_cast<CGContainer_C *>(storage) + 1), eventTime, init);
      break;
    case ID_UNIT:
      new (storage) CGUnit_C(reinterpret_cast<unsigned long *>(static_cast<CGUnit_C *>(storage) + 1), eventTime, init);
      break;
    case ID_PLAYER:
      new (storage) CGPlayer_C(reinterpret_cast<unsigned long *>(static_cast<CGPlayer_C *>(storage) + 1), eventTime, init);
      break;
    case ID_GAMEOBJECT:
      new (storage) CGGameObject_C(reinterpret_cast<unsigned long *>(static_cast<CGGameObject_C *>(storage) + 1), eventTime, init);
      break;
    case ID_DYNAMICOBJECT:
      new (storage) CGDynamicObject_C(reinterpret_cast<unsigned long *>(static_cast<CGDynamicObject_C *>(storage) + 1), eventTime, init);
      break;
    case ID_CORPSE:
      new (storage) CGCorpse_C(reinterpret_cast<unsigned long *>(static_cast<CGCorpse_C *>(storage) + 1), eventTime, init);
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

static CGObject_C *GetObjectPtr(unsigned __int64 guid) {
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
  unsigned __int64 guid;
  OBJECT_TYPE_ID   type;
  unsigned int     btype = 0;

  msg->Get(guid);
  msg->Get(*reinterpret_cast<unsigned char *>(&btype));
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
  static const char *messages[8] = {
      "Creating object", "Creating item",        "Creating container",      "Creating unit",
      "Creating player", "Creating game object", "Creating dynamic object", "Creating corpse",
  };
  FATALASSERT(id < ID_AIGROUP);
  SysMsgAdd(messages[id], SYSMSG_INFO, 0x20);
}

static C_OBJECTHASH *GetUpdateObject(unsigned __int64 guid) {
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

static void SetupObjectStorage(OBJECT_TYPE_ID type, unsigned int memHandle) {
  unsigned char *storage = static_cast<unsigned char *>(ObjectPtr(memHandle));

  static const unsigned int offsets[8] = {
      48, 92, 120, 2528, 6240, 156, 68, 124,
  };
  static const unsigned int sizes[8] = {
      24, 144, 312, 736, 2536, 80, 64, 144,
  };

  unsigned long *data;
  switch (type) {
    case ID_OBJECT:
      data = reinterpret_cast<unsigned long *>(storage + offsets[ID_OBJECT]);
      static_cast<CGObject_C *>(static_cast<void *>(storage))->SetStorage(data);
      break;
    case ID_ITEM:
      data = reinterpret_cast<unsigned long *>(storage + offsets[ID_ITEM]);
      static_cast<CGItem_C *>(static_cast<void *>(storage))->SetStorage(data);
      break;
    case ID_CONTAINER:
      data = reinterpret_cast<unsigned long *>(storage + offsets[ID_CONTAINER]);
      static_cast<CGContainer_C *>(static_cast<void *>(storage))->SetStorage(data);
      break;
    case ID_UNIT:
      data = reinterpret_cast<unsigned long *>(storage + offsets[ID_UNIT]);
      static_cast<CGUnit_C *>(static_cast<void *>(storage))->SetStorage(data);
      break;
    case ID_PLAYER:
      data = reinterpret_cast<unsigned long *>(storage + offsets[ID_PLAYER]);
      static_cast<CGPlayer_C *>(static_cast<void *>(storage))->SetStorage(data);
      break;
    case ID_GAMEOBJECT:
      data = reinterpret_cast<unsigned long *>(storage + offsets[ID_GAMEOBJECT]);
      static_cast<CGGameObject_C *>(static_cast<void *>(storage))->SetStorage(data);
      break;
    case ID_DYNAMICOBJECT:
      data = reinterpret_cast<unsigned long *>(storage + offsets[ID_DYNAMICOBJECT]);
      static_cast<CGDynamicObject_C *>(static_cast<void *>(storage))->SetStorage(data);
      break;
    case ID_CORPSE:
      data = reinterpret_cast<unsigned long *>(storage + offsets[ID_CORPSE]);
      static_cast<CGCorpse_C *>(static_cast<void *>(storage))->SetStorage(data);
      break;
    default:
      FATALASSERT(0);
      return;
  }
  memset(storage + offsets[type], 0, sizes[type]);
}

static C_OBJECTHASH *AllocNewObj() {
  unsigned int memHandle;

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

static int CreateObject(unsigned long eventTime, CDataStore *msg) {
  CClientObjCreate init;
  unsigned __int64 guid;
  unsigned int     memHandle;
  OBJECT_TYPE_ID   type;
  unsigned int     btype = 0;

  FATALASSERT(msg);
  msg->Get(guid);
  s_curMgr->m_legalGuidDeref = guid;
  msg->Get(*reinterpret_cast<unsigned char *>(&btype));
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

static int UpdateObjectMovement(unsigned long eventTime, CDataStore *msg) {
  CClientMoveUpdate update;
  unsigned __int64  guid;

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

static int UpdateObject(CDataStore *msg) {
  unsigned __int64 guid;
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
  unsigned __int64  guid;
  msg->Get(guid);
  *msg >> update;
  CGObject_C *object = GetObjectPtr(guid);
  if (object && object->IsA(TYPE_UNIT)) {
    static_cast<CGUnit_C *>(object)->PostMovementUpdate(update);
  }
}

static void OutOfRangeMessage(unsigned __int64 guid) {
  static const char *messages[8] = {
      "Forgetting object", "Forgetting item",        "Forgetting container",      "Forgetting unit",
      "Forgetting player", "Forgetting game object", "Forgetting dynamic object", "Forgetting corpse",
  };
  CGObject_C *object = GetObjectPtr(guid);
  if (object) {
    SysMsgAdd(messages[GetSectionId(object->GetType())], SYSMSG_INFO, 0x20);
  }
}

static void InRangeMessage(unsigned __int64 guid) {
  static const char *messages[8] = {
      "Remembering object", "Remembering item",        "Remembering container",      "Remembering unit",
      "Remembering player", "Remembering game object", "Remembering dynamic object", "Remembering corpse",
  };
  CGObject_C *object = GetObjectPtr(guid);
  if (object) {
    SysMsgAdd(messages[GetSectionId(object->GetType())], SYSMSG_INFO, 0x20);
  }
}

void ClntObjMgrObjectInRange(unsigned __int64 guid) {
  C_OBJECTHASH *foundObj = GetUpdateObject(guid);
  if (foundObj) {
    CGObject_C *object = static_cast<CGObject_C *>(ObjectPtr(foundObj->memHandle));
    FATALASSERT(object);
    InRangeMessage(guid);
  }
}

static void UpdateInRangeObjects(CDataStore *msg) {
  unsigned __int64 guid;
  unsigned int     count;
  msg->Get(count);
  FATALASSERT(count);
  for (unsigned int i = 0; i < count; ++i) {
    msg->Get(guid);
    if (guid != ClntObjMgrGetActivePlayer()) {
      ClntObjMgrObjectInRange(guid);
    }
  }
}

static void UpdateOutOfRangeObjects(CDataStore *msg) {
  unsigned __int64 guid;
  unsigned int     count;
  msg->Get(count);
  FATALASSERT(count);
  for (unsigned int i = 0; i < count; ++i) {
    msg->Get(guid);
    if (guid != ClntObjMgrGetActivePlayer()) {
      ClntObjMgrObjectOutOfRange(guid, 0);
    }
  }
}

static void ClearObjectMirrorHandlers(C_OBJECTHASH *foundObj) {
  for (unsigned int i = 0; i < 634; ++i) {
    while (CMirrorHandler *handler = foundObj->mirrorHandlers[i].Head()) {
      foundObj->mirrorHandlers[i].UnlinkNode(handler);
      DEL(handler);
    }
  }
}

void ClntObjMgrObjectOutOfRange(unsigned __int64 guid, int shutdown) {
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
  void        *junkData;
  unsigned int count;
  msg->Get(count);
  msg->GetDataInSitu(junkData, count * sizeof(unsigned __int64));
}

static int ObjectUpdateHandler(void *, NETMESSAGE, unsigned long eventTime, CDataStore *msg) {
  unsigned __int64 oldActive;
  unsigned int     marker1 = 0;
  int              success;
  unsigned int     numObjUpdates;
  unsigned int     updateType = 0;

  msg->Get(numObjUpdates);
  unsigned int marker = msg->Tell();
  msg->Get(*reinterpret_cast<unsigned char *>(&marker1));
  unsigned int firstUpdate = 0;
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
  unsigned int i;
  for (i = firstUpdate; i < numObjUpdates; ++i) {
    s_curMgr->m_legalGuidDeref = 0;
    msg->Get(*reinterpret_cast<unsigned char *>(&updateType));
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
          SysMsgAdd("OBJECTCREATIONFAILURE", SYSMSG_INFO, 0x20);
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
    msg->Get(*reinterpret_cast<unsigned char *>(&updateType));
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

static int ObjectCompressedUpdateHandler(void *, NETMESSAGE, unsigned long eventTime, CDataStore *msg) {
  WDataStore    realmsg;
  void         *data;
  unsigned int  origSize;
  unsigned long destSize;

  msg->Get(origSize);
  unsigned int compressedSize = msg->Size() - msg->Tell();
  msg->GetDataInSitu(data, compressedSize);
  void *dest = _alloca(origSize);
  destSize = origSize;
  zlib_uncompress(
      static_cast<unsigned char *>(dest),
      &destSize,
      static_cast<const unsigned char *>(data),
      compressedSize
  );
  FATALASSERT(destSize == origSize);
  realmsg.PutData(dest, destSize);
  realmsg.Finalize();
  return ObjectUpdateHandler(0, MSG_NULL_ACTION, eventTime, &realmsg);
}

static void AssignMirrorHandler(
    TSList<CMirrorHandler, TSGetLink<CMirrorHandler> > *handlerList,
    unsigned int                                        offset,
    unsigned int                                        bytes,
    int(*handler)(unsigned __int64, unsigned int, unsigned int, const void *, void *),
    void            *param,
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

static void UnassignMirrorHandler(
    TSList<CMirrorHandler, TSGetLink<CMirrorHandler> > *handlerList,
    int(*handler)(unsigned __int64, unsigned int, unsigned int, const void *, void *),
    void *param
) {
  for (CMirrorHandler *mirror = handlerList->Head(); mirror; mirror = handlerList->Next(mirror)) {
    if (mirror->handler == handler && mirror->param == param) {
      DEL(mirror);
      return;
    }
  }
}

static int OnObjectDestroy(void *, NETMESSAGE, unsigned long, CDataStore *msg) {
  unsigned __int64 guid;
  msg->Get(guid);
  ClntObjMgrFreeObject(guid);
  return 1;
}

static int CCommand_ObjUsage(const char *command, const char *arguments) {
  C_OBJECTHASH *object;
  unsigned int  numVisible = 0;
  unsigned int  numActive = 0;
  unsigned int  numWaiting = 0;
  unsigned int  numFree = 0;

  for (object = s_curMgr->m_visibleObjects.Head(); object; object = s_curMgr->m_visibleObjects.Next(object)) {
    ++numVisible;
  }

  for (object = s_curMgr->m_objects.Head(); object; object = s_curMgr->m_objects.Next(object)) {
    ++numActive;
  }

  for (object = s_curMgr->m_lazyCleanupObjects.Head(); object; object = s_curMgr->m_lazyCleanupObjects.Next(object)) {
    ++numWaiting;
  }

  for (object = s_curMgr->m_freeObjects.Head(); object; object = s_curMgr->m_freeObjects.Next(object)) {
    ++numFree;
  }

  ConsoleWrite("Object manager list status:", HIGHLIGHT_COLOR);
  ConsoleWriteA("    Active objects:              %u objects (%u visible)", HIGHLIGHT_COLOR, numActive, numVisible);
  ConsoleWriteA("    Objects waiting to be freed: %u objects", HIGHLIGHT_COLOR, numWaiting);
  ConsoleWriteA("    Free objects:                %u objects", HIGHLIGHT_COLOR, numFree);
  return 1;
}

ClntObjMgr *ClntObjMgrCreate(PLAYER_TYPE type, void *clientPtr) {
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

int ClntObjMgrIsValid(int forWriting) {
  if (!s_curMgr) {
    return 0;
  }

  return SMemIsValidPointer(s_curMgr, sizeof(ClntObjMgr), forWriting) != 0;
}

void ClntObjMgrInitializeShared() {
  if (!s_heapsAllocated) {
    unsigned int objectType;

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
  for (unsigned int i = 0; i < 64; ++i) {
    C_OBJECTHASH *hash = AllocNewObj();
    ASSERT(hash);
    s_curMgr->m_freeObjects.LinkNode(hash, LIST_LINK_BEFORE, 0);
  }

  s_curMgr->m_net->SetMessageHandler(SMSG_UPDATE_OBJECT, ObjectUpdateHandler, 0);
  s_curMgr->m_net->SetMessageHandler(SMSG_COMPRESSED_UPDATE_OBJECT, ObjectCompressedUpdateHandler, 0);
  s_curMgr->m_net->SetMessageHandler(SMSG_DESTROY_OBJECT, OnObjectDestroy, 0);
}

void ClntObjMgrSetObjMirrorHandler(
    unsigned __int64 guid,
    unsigned int     offset,
    unsigned int     bytes,
    int(*handler)(unsigned __int64, unsigned int, unsigned int, const void *, void *),
    void            *param,
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
    FATALASSERT(GetOffsetSectionId(object->GetType(), offset) < ID_AIGROUP);
    AssignMirrorHandler(&foundObj->mirrorHandlers[offset >> 2], offset, bytes, handler, param, priority);
    ActivityEnd(ACTIVITY_OBJMGR);
  }
}

static int MirrorHandlerRemoveQueued(unsigned __int64 guid, CMirrorHandler *mirror) {
  for (OBJHANDLERREQUEST *request = s_curMgr->m_pendingObjHandlerRequests.Head(); request;
       request = s_curMgr->m_pendingObjHandlerRequests.Next(request))
  {
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

  for (OBJHANDLERREQUEST *request = s_curMgr->m_pendingObjHandlerRequests.Head(); request;
       request = s_curMgr->m_pendingObjHandlerRequests.Next(request))
  {
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

void ClntObjMgrUnsetObjMirrorHandler(
    unsigned __int64 guid,
    unsigned int     offset,
    int(*handler)(unsigned __int64, unsigned int, unsigned int, const void *, void *),
    void *param
) {
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
    FATALASSERT(GetOffsetSectionId(object->GetType(), offset) < ID_AIGROUP);
    UnassignMirrorHandler(&foundObj->mirrorHandlers[offset >> 2], handler, param);
    ActivityEnd(ACTIVITY_OBJMGR);
  }
}

void ClntObjMgrSetTypeMirrorHandler(
    OBJECT_TYPE hierType,
    unsigned int offset,
    unsigned int bytes,
    int(*handler)(unsigned __int64, unsigned int, unsigned int, const void *, void *),
    void *param,
    HANDLER_PRIORITY priority
) {
  ActivityBegin(ACTIVITY_OBJMGR);
  OBJECT_TYPE_ID section = GetSectionId(hierType);
  FATALASSERT(section < ID_AIGROUP);
  AssignMirrorHandler(&s_mirrorHandlers[section][offset >> 2], offset, bytes, handler, param, priority);
  ActivityEnd(ACTIVITY_OBJMGR);
}

void ClntObjMgrUnsetTypeMirrorHandler(
    OBJECT_TYPE hierType,
    unsigned int offset,
    int(*handler)(unsigned __int64, unsigned int, unsigned int, const void *, void *)
) {
  ActivityBegin(ACTIVITY_OBJMGR);
  OBJECT_TYPE_ID section = GetSectionId(hierType);
  FATALASSERT(section < ID_AIGROUP);
  UnassignMirrorHandler(&s_mirrorHandlers[section][offset >> 2], handler, 0);
  ActivityEnd(ACTIVITY_OBJMGR);
}

void ClntObjMgrHideObject(unsigned __int64 guid) {
  C_OBJECTHASH *foundObj = FindActiveObj(guid);
  if (foundObj) {
    ActivityBegin(ACTIVITY_OBJMGR);
    if (s_curMgr->m_visibleObjects.IsLinked(foundObj)) {
      s_curMgr->m_visibleObjects.UnlinkNode(foundObj);
    }
    ActivityEnd(ACTIVITY_OBJMGR);
  }
}

void ClntObjMgrShowObject(unsigned __int64 guid) {
  C_OBJECTHASH *foundObj = FindActiveObj(guid);
  if (foundObj) {
    ActivityBegin(ACTIVITY_OBJMGR);
    if (!s_curMgr->m_visibleObjects.IsLinked(foundObj)) {
      s_curMgr->m_visibleObjects.LinkNode(foundObj, LIST_TAIL, 0);
    }
    ActivityEnd(ACTIVITY_OBJMGR);
  }
}

int ClntObjMgrEnumVisibleObjects(int(*handler)(unsigned __int64, void *), void *param) {
  ActivityBegin(ACTIVITY_OBJMGR);

  int success = 1;
  for (C_OBJECTHASH *object = s_curMgr->m_visibleObjects.Head(); object; object = s_curMgr->m_visibleObjects.Next(object)) {
    if (!handler(object->m_key.GetGUID(), param)) {
      success = 0;
      break;
    }
  }

  ActivityEnd(ACTIVITY_OBJMGR);
  return success;
}

void ClntObjMgrFreeObject(unsigned __int64 guid) {
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

CGObject_C *ClntObjMgrObjectPtr(unsigned __int64 guid, const char *fileName, unsigned int lineNumber) {
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

  for (unsigned int objectType = 0; objectType < 8; ++objectType) {
    for (unsigned int block = 0; block < 634; ++block) {
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

unsigned __int64 ClntObjMgrGetActivePlayer() {
  if (!s_curMgr) {
    return 0;
  }

  return s_curMgr->m_activePlayer;
}

void ClntObjMgrSetActivePlayer(unsigned __int64 guid) {
  if (s_curMgr) {
    s_curMgr->m_activePlayer = guid;
  }
}

PLAYER_TYPE ClntObjMgrGetPlayerType() {
  return s_curMgr->m_type;
}

unsigned int ClntObjMgrGetMapID() {
  return s_curMgr->m_mapID;
}

void ClntObjMgrSetMapID(unsigned int mapID) {
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

void *ClntObjMgrGetMovementGlobals() {
  if (!s_curMgr) {
    return 0;
  }

  return s_curMgr->m_movement;
}

void ClntObjMgrSetMovementGlobals(void *ptr) {
  if (s_curMgr) {
    s_curMgr->m_movement = ptr;
  }
}

void *ClntObjMgrGetClientPtr() {
  if (!s_curMgr) {
    return 0;
  }

  return s_curMgr->m_clientPtr;
}
