#pragma once

#include "Base/CDataStore.h"
#include "Tempest/c3spline.h"

struct CMovementStatus {
  CMovementStatus() : transport(0), transRelPosition(0.0f), transRelFacing(0.0f), worldPosition(0.0f), worldFacing(0.0f), pitch(0.0f), moveFlags(0) {
  }

  unsigned __int64   transport;
  NTempest::C3Vector transRelPosition;
  float              transRelFacing;
  NTempest::C3Vector worldPosition;
  float              worldFacing;
  float              pitch;
  unsigned int       moveFlags;
};

struct CMoveSpline {
  struct SplineFaceData {
    NTempest::C3Vector spot;
    unsigned __int64   guid;
    float              facing;
  };

  unsigned int                  flags;
  SplineFaceData                face;
  unsigned long                 start;
  unsigned long                 time;
  NTempest::C3Spline_CatmullRom spline;
};

struct CClientMoveUpdate {
  static void Skip(CDataStore *packet);

  CMovementStatus status;
  unsigned int    timeFallen;
  float           walkSpeed;
  float           runSpeed;
  float           swimSpeed;
  float           turnRate;
  CMoveSpline     spline;
};

CDataStore &operator<<(CDataStore &packet, const CClientMoveUpdate &update);
CDataStore &operator>>(CDataStore &packet, CClientMoveUpdate &update);
float CalculateFacingTo(NTempest::C3Vector &position, NTempest::C3Vector &destination);

struct CClientObjCreate {
  CClientObjCreate() : flags(0) {
  }

  void Put(CDataStore *packet) {
    *packet << move;
    packet->Put(flags);
    packet->Put(attackCycle);
    packet->Put(timerID);
    packet->Put(victim);
  }

  void Get(CDataStore *packet) {
    *packet >> move;
    packet->Get(flags);
    packet->Get(attackCycle);
    packet->Get(timerID);
    packet->Get(victim);
  }

  static void Skip(CDataStore *packet) {
    void *unused;
    CClientMoveUpdate::Skip(packet);
    packet->GetDataInSitu(unused, 20);
  }

  CClientMoveUpdate move;
  unsigned int      flags;
  unsigned int      attackCycle;
  unsigned int      timerID;
  unsigned __int64  victim;
};

enum OBJECT_TYPE_ID {
  ID_OBJECT = 0,
  ID_ITEM = 1,
  ID_CONTAINER = 2,
  ID_UNIT = 3,
  ID_PLAYER = 4,
  ID_GAMEOBJECT = 5,
  ID_DYNAMICOBJECT = 6,
  ID_CORPSE = 7,
  ID_AIGROUP = 8,
  ID_AREATRIGGER = 9
};

enum OBJECT_TYPE {
  TYPE_OBJECT = 0x001,
  TYPE_ITEM = 0x002,
  TYPE_CONTAINER = 0x004,
  TYPE_UNIT = 0x008,
  TYPE_PLAYER = 0x010,
  TYPE_GAMEOBJECT = 0x020,
  TYPE_DYNAMICOBJECT = 0x040,
  TYPE_CORPSE = 0x080,
  TYPE_AIGROUP = 0x100,
  TYPE_AREATRIGGER = 0x200,

  HIER_TYPE_OBJECT = TYPE_OBJECT,
  HIER_TYPE_ITEM = TYPE_OBJECT | TYPE_ITEM,
  HIER_TYPE_CONTAINER = TYPE_OBJECT | TYPE_ITEM | TYPE_CONTAINER,
  HIER_TYPE_UNIT = TYPE_OBJECT | TYPE_UNIT,
  HIER_TYPE_PLAYER = TYPE_OBJECT | TYPE_UNIT | TYPE_PLAYER,
  HIER_TYPE_GAMEOBJECT = TYPE_OBJECT | TYPE_GAMEOBJECT,
  HIER_TYPE_DYNAMICOBJECT = TYPE_OBJECT | TYPE_DYNAMICOBJECT,
  HIER_TYPE_CORPSE = TYPE_OBJECT | TYPE_CORPSE,
  HIER_TYPE_AIGROUP = TYPE_OBJECT | TYPE_AIGROUP,
  HIER_TYPE_AREATRIGGER = TYPE_OBJECT | TYPE_AREATRIGGER
};

struct VirtualItemInfo {
  unsigned char m_classID;
  unsigned char m_subclassID;
  unsigned char m_material;
  unsigned char m_inventoryType;
  unsigned char m_sheatheType;
  unsigned char m_padding0;
  unsigned char m_padding1;
  unsigned char m_padding2;
};

struct CGObjectData {
  unsigned __int64 m_guid;
  unsigned int     m_type;
  int              m_entryID;
  float            m_scale;
  unsigned int     pad;
};

class CGObject {
 public:
  unsigned __int64 GetGUID() const {
    return *reinterpret_cast<const unsigned __int64 *>(m_obj);
  }

  OBJECT_TYPE GetType() const {
    return static_cast<OBJECT_TYPE>(m_obj->m_type);
  }

  int IsA(OBJECT_TYPE type) const {
    return GetType() & type;
  }

  int GetEntryID() const {
    return m_obj->m_entryID;
  }

  unsigned int *GetData(unsigned int index) {
    return reinterpret_cast<unsigned int *>(m_data + index);
  }

  const unsigned int *GetData(unsigned int index) const {
    return reinterpret_cast<const unsigned int *>(m_data + index);
  }

  unsigned long *GetStorage() {
    return m_data;
  }

  const unsigned long *GetStorage() const {
    return m_data;
  }

 protected:
  unsigned long *m_data;
  CGObjectData  *m_obj;
};
