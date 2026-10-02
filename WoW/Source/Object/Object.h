#pragma once

#include "Base/CDataStore.h"
#include "Os/OsTime.h"
#include "Tempest/c3spline.h"

namespace NTempest {

  inline CDataStore &operator<<(CDataStore &s_, const C3Vector &d_) {
    s_ << d_.x;
    s_ << d_.y;
    s_ << d_.z;
    return s_;
  }

  inline CDataStore &operator>>(CDataStore &s_, C3Vector &d_) {
    s_ >> d_.x;
    s_ >> d_.y;
    s_ >> d_.z;
    return s_;
  }

  inline CDataStore &operator<<(CDataStore &s_, const C2Vector &d_) {
    s_ << d_.x;
    s_ << d_.y;
    return s_;
  }

  inline CDataStore &operator>>(CDataStore &s_, C2Vector &d_) {
    s_ >> d_.x;
    s_ >> d_.y;
    return s_;
  }

}

struct CMovementStatus {
  CMovementStatus() : transport(0), transRelPosition(0.0f), transRelFacing(0.0f), worldPosition(0.0f), worldFacing(0.0f), pitch(0.0f), moveFlags(0) {
  }

  static UINT Skip(CDataStore *packet);

  DWORDLONG          transport;
  NTempest::C3Vector transRelPosition;
  float              transRelFacing;
  NTempest::C3Vector worldPosition;
  float              worldFacing;
  float              pitch;
  UINT               moveFlags;
};

struct CMoveSpline {
  UINT flags;
  struct SplineFaceData {
    NTempest::C3Vector spot;
    DWORDLONG          guid;
    float              facing;
  };
  SplineFaceData                face;
  DWORD                         start;
  DWORD                         time;
  NTempest::C3Spline_CatmullRom spline;

  static void Skip(CDataStore *packet) {
    UINT flags = 0;
    *packet >> flags;
    UINT bytes = 0;
    if (flags & 0x00010000) {
      bytes = 12;
    }
    if (flags & 0x00020000) {
      bytes += 8;
    }
    if (flags & 0x00040000) {
      bytes += 4;
    }
    LPVOID unused;
    packet->GetDataInSitu(unused, bytes + 8);
    UINT pointCount = 0;
    *packet >> pointCount;
    packet->GetDataInSitu(unused, 12 * pointCount);
  }
};

inline CDataStore &operator<<(CDataStore &s_, const CMoveSpline &d_) {
  s_ << d_.flags;
  if (d_.flags & 0x00010000) {
    s_ << d_.face.spot;
  }
  if (d_.flags & 0x00020000) {
    s_ << d_.face.guid;
  }
  if (d_.flags & 0x00040000) {
    s_ << d_.face.facing;
  }
  s_ << int(OsGetAsyncTimeMs() - d_.start);
  s_ << d_.time;
  UINT pointCount = d_.spline.NumPoints();
  s_ << pointCount;
  for (UINT i = 0; i < pointCount; ++i) {
    s_ << d_.spline.Point(i);
  }
  return s_;
}

inline CDataStore &operator>>(CDataStore &s_, CMoveSpline &d_) {
  s_ >> d_.flags;
  if (d_.flags & 0x00010000) {
    s_ >> d_.face.spot;
  }
  if (d_.flags & 0x00020000) {
    s_ >> d_.face.guid;
  }
  if (d_.flags & 0x00040000) {
    s_ >> d_.face.facing;
  }
  DWORD timeNow = OsGetAsyncTimeMs();
  int   elapsed = s_.GetInt();
  d_.start = timeNow - elapsed;
  s_ >> d_.time;
  UINT pointCount = s_.GetUint();
  if (pointCount) {
    LPVOID points;
    s_.GetDataInSitu(points, 12 * pointCount);
    d_.spline.SetPoints(static_cast<const NTempest::C3Vector *>(points), pointCount);
  }
  return s_;
}

struct CClientMoveUpdate {
  CClientMoveUpdate() : timeFallen(0) {
  }

  CClientMoveUpdate(const CClientMoveUpdate &);

  static void Skip(CDataStore *packet);

  CMovementStatus status;
  UINT            timeFallen;
  float           walkSpeed;
  float           runSpeed;
  float           swimSpeed;
  float           turnRate;
  CMoveSpline     spline;
};

CDataStore &operator<<(CDataStore &packet, const CClientMoveUpdate &update);
CDataStore &operator>>(CDataStore &packet, CClientMoveUpdate &update);
bool        IsAngleWithinRange(float a, float b, float fieldofView);
float       CalculateFacingTo(const NTempest::C3Vector &position, const NTempest::C3Vector &destination);

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
    LPVOID unused;
    CClientMoveUpdate::Skip(packet);
    packet->GetDataInSitu(unused, 20);
  }

  CClientMoveUpdate move;
  UINT              flags;
  UINT              attackCycle;
  UINT              timerID;
  DWORDLONG         victim;
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
  NUM_CLIENT_OBJECT_TYPES = 8,

  ID_AIGROUP = 8,
  ID_AREATRIGGER = 9,
  NUM_OBJECT_TYPES = 10
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

const OBJECT_TYPE g_heirTypeFlags[NUM_OBJECT_TYPES] = {HIER_TYPE_OBJECT,  HIER_TYPE_ITEM,       HIER_TYPE_CONTAINER,     HIER_TYPE_UNIT,
                                                       HIER_TYPE_PLAYER,  HIER_TYPE_GAMEOBJECT, HIER_TYPE_DYNAMICOBJECT, HIER_TYPE_CORPSE,
                                                       HIER_TYPE_AIGROUP, HIER_TYPE_AREATRIGGER};

struct VirtualItemInfo {
  BYTE m_classID;
  BYTE m_subclassID;
  BYTE m_material;
  BYTE m_inventoryType;
  BYTE m_sheatheType;
  BYTE m_padding0;
  BYTE m_padding1;
  BYTE m_padding2;

  BYTE operator!=(const VirtualItemInfo &);
};

struct CGObjectData {
  DWORDLONG   m_guid;
  OBJECT_TYPE m_type;
  int         m_entryID;
  float       m_scale;
  UINT        pad;
};

class CGObject {
 public:
  BYTE IsA(OBJECT_TYPE type) const {
    return (GetType() & type) != 0;
  }
  BYTE IsA(OBJECT_TYPE_ID type) const {
    return (static_cast<UINT>(GetType()) >> type) & 1;
  }
  BYTE IsExactlyA(OBJECT_TYPE_ID type) const {
    return GetType() == g_heirTypeFlags[type];
  }

  DWORDLONG GetGUID() const {
    return *reinterpret_cast<const DWORDLONG *>(m_obj);
  }

  OBJECT_TYPE GetType() const {
    return m_obj->m_type;
  }

  float GetObjectScale() const {
    return m_obj->m_scale;
  }

  int GetEntryID() const {
    return m_obj->m_entryID;
  }

  BYTE *GetData(UINT index) const {
    return reinterpret_cast<BYTE *>(m_data + index);
  }

  static UINT               GetDataSize();
  static UINT               GetBaseOffset();
  static __forceinline UINT TotalFields() {
    return 6;
  }
  static UINT GetUpdateMaskBytes();
  static UINT GetUpdateMaskBlocks();

  void SetStorage(DWORD *storage) {
    m_data = storage;
    m_obj = reinterpret_cast<CGObjectData *>(storage);
  }

  DWORD *GetStorage() {
    return m_data;
  }

 protected:
  explicit CGObject(DWORD *storage) {
    SetStorage(storage);
  }

  ~CGObject() {
  }

  CGObjectData *Obj() {
    return m_obj;
  }

  const CGObjectData *Obj() const {
    return m_obj;
  }

  DWORD        *m_data;
  CGObjectData *m_obj;
};
